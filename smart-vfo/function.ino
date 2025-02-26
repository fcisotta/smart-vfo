/*
 * function.ino - Smart VFO
 *
 * Copyright (C) 2023 Giovanni Caracuta (I7IWN) <g.caracuta@libero.it>
 *                    Francesco Cisotta (IZ2QPV) <francesco@cisotta.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**************************************/
/* State management                   */
/**************************************/
void push_state() {
  prev_state = state;
}

void pop_state() {
  state = prev_state;
  if (state == STATE_VFO_DIAL)
    infer_vfo_op_substate();
  else if (state == STATE_MEM_DIAL)
    substate = SUBSTATE_OP;
} 


/**************************************/
/* Change operating state (Rx/Tx)     */
/**************************************/
void change_op() {
  cur_op = -cur_op + 1;
  display_op();
  set_freq();
}


/**************************************/
/* Change step                        */
/**************************************/
void rotate_step() {
  if (state == STATE_VFO_DIAL && substate == SUBSTATE_OP) {
    step *= 10;
    if (step > 1000000)
      step = 1;
    ee_store_step();  
  }
  else if (state == STATE_VFO_DIAL && substate == SUBSTATE_RIT_OP) {
    rit_step *= 10;
    if (rit_step > MAX_RIT_VALUE)
      rit_step = 1;
    ee_store_rit_step();  
  }
  else if (state == STATE_CAL_MENU || state == STATE_OP_MENU) {
    menu_step *= 10;
    if (menu_step > menu_value_max) // TODO: min e max value
      menu_step = 1;
  }
  display_step();
}


/**************************************/
/* Set frequency                      */
/**************************************/
void rotate_freq(unsigned char direction) {
  // capped inside band limits
  if (direction == DIR_CW && op_rec.rx + step <= bands[cur_band[cur_vfo]].upper_lim ||
    direction == DIR_CCW && op_rec.rx - step >= bands[cur_band[cur_vfo]].lower_lim) {
    op_rec.rx += (direction == DIR_CW) ? step : -step;
    book_freq_update();
  } 
}

void set_freq() {

  // Determine vfo freq to be set, based on skid and operation state (Rx/Tx)
  uint32_t vfo = op_rec.rx;
  byte mode = op_rec.mode;
  // RX frequency
  if (cur_op == OP_RX) {
  }
  // TX frequency
  else if (state == STATE_VFO_DIAL && split == SPLIT_ON) {
    byte oth_vfo = -cur_vfo+1;
    op_record other_vfo_rec = ee_find_vfo_rec(oth_vfo, cur_band[oth_vfo]);
    vfo = other_vfo_rec.rx;
    mode = other_vfo_rec.mode;
  }

  display_freq(vfo);
  display_mode(mode);
  display_rit();
  display_step();
  update_filters_control();

  // drive Si5351
  if (op_rec.rit != 0)
    vfo += op_rec.rit;

  #ifndef MOCK_Si5351
    if (__design == DESIGN_SINGLE_CONV) {

      uint32_t bfo = __if;
      if (mode == MODE_LSB)
        bfo = bfo - __ssb_offset;
      else if (mode == MODE_USB)
        bfo = bfo + __ssb_offset;
      else if (mode == MODE_CW)
        bfo = bfo + __cw_offset;

      #ifdef DEBUG
        Serial.print("vfo=");
        Serial.println(vfo);
      #endif

      uint32_t clk0; // = (bfo + (_conv ? vfo : -vfo));
      if (__conv == CONV_UPCONV)
        clk0 = bfo + vfo;
      else
        clk0 = bfo - vfo;

      if (clk0 != cur_clk0_f) {
        cur_clk0_f = clk0;
  
        // check whether frequency is reachable
        if (clk0 < SI5351_CLKOUT_MIN_FREQ ||
          clk0 > SI5351_CLKOUT_MAX_FREQ) {
          error(ERR_DDS_FREQ_RANGE);
          return;
        }

        #ifdef DEBUG
          Serial.print("CLK0=");
          Serial.println(clk0);
        #endif
        si5351.set_freq(clk0 * SI5351_FREQ_MULT, SI5351_CLK0);

      }

      uint32_t clk2 = bfo;
      if (clk2 != cur_clk2_f) {

        #ifdef DEBUG
          Serial.print("CLK2=");
          Serial.println(bfo);
        #endif

        si5351.set_freq(clk2 * SI5351_FREQ_MULT, SI5351_CLK2);

        cur_clk2_f = clk2;
      }
    }

    else if (__design == DESIGN_DIRECT_CONV) {
      si5351.set_freq((vfo * SI5351_FREQ_MULT), SI5351_CLK0);
    }

    else if (__design == DESIGN_FREQX4) {
      si5351.set_freq(vfo * 4 * SI5351_FREQ_MULT,  SI5351_CLK0);
    }

  #endif

}


/**************************************/
/* Set RIT                            */
/**************************************/
void rotate_rit(unsigned char direction) {
  if (direction == DIR_CW && op_rec.rit + rit_step <= MAX_RIT_VALUE ||
    direction == DIR_CCW && op_rec.rit - rit_step >= (MAX_RIT_VALUE * -1)) {
    if (direction == DIR_CW && op_rec.rit < 0 && op_rec.rit + rit_step > 0 ||
      direction == DIR_CCW && op_rec.rit > 0 && op_rec.rit - rit_step < 0)
      op_rec.rit = -op_rec.rit;
    else
      op_rec.rit += (direction == DIR_CW) ? rit_step : -rit_step;
    book_rit_update();
  }
}

void toggle_rit_editing() {
  flush_store_updates();
  if (substate == SUBSTATE_OP) {
    substate = SUBSTATE_RIT_OP;
    op_rec.rit = _o_rit[cur_vfo][cur_band[cur_vfo]];
  }
  else if (substate == SUBSTATE_RIT_OP) {
    reset_vfo_rit();
    substate = SUBSTATE_OP;
  }
  set_freq();
}

void reset_vfo_rit() {
  _o_rit[cur_vfo][cur_band[cur_vfo]] = op_rec.rit;
  op_rec.rit = 0;
  ee_update_cur_op_rec_rit();
}

void infer_vfo_op_substate() {
  if (op_rec.rit != 0)
    substate = SUBSTATE_RIT_OP;
  else
    substate = SUBSTATE_OP;
}


/**************************************/
/* Set SPLIT                          */
/**************************************/
void toggle_split() {
  split = -split + 1;
  ee_store_split();
  display_vfo_mem();
  set_freq();
}


/**************************************/
/* Set DDS power                      */
/**************************************/
void set_pwr() {

  si5351_drive drive;

  if (__dds_pwr0 == DDS_POWER_2MA)
    drive = SI5351_DRIVE_2MA;
  else if (__dds_pwr0 == DDS_POWER_4MA)
    drive = SI5351_DRIVE_4MA;
  else if (__dds_pwr0 == DDS_POWER_6MA)
    drive = SI5351_DRIVE_6MA;
  else
    drive = SI5351_DRIVE_8MA;

  si5351.drive_strength(SI5351_CLK0, drive);

  if (__design == DESIGN_SINGLE_CONV) {
    if (__dds_pwr2 == DDS_POWER_2MA)
      drive = SI5351_DRIVE_2MA;
    else if (__dds_pwr2 == DDS_POWER_4MA)
      drive = SI5351_DRIVE_4MA;
    else if (__dds_pwr2 == DDS_POWER_6MA)
      drive = SI5351_DRIVE_6MA;
    else
      drive = SI5351_DRIVE_8MA;

    si5351.drive_strength(SI5351_CLK2, drive);
  }
}


/**************************************/
/* Change band                        */
/**************************************/
void rotate_band(unsigned char direction) {
  flush_store_updates();
  if (direction == DIR_CW)
    cur_band[cur_vfo] = cur_band[cur_vfo] == BANDS_NO - 1 ? 0 : cur_band[cur_vfo] + 1;
  else
    cur_band[cur_vfo] = cur_band[cur_vfo] > 0 ? cur_band[cur_vfo] - 1 : BANDS_NO - 1;
  ee_store_cur_band();
  ee_load_vfo_rec(cur_vfo, cur_band[cur_vfo]);
  #ifdef DEBUG
    Serial.print("Band: ");
    Serial.println(cur_band[cur_vfo]);
  #endif
  infer_vfo_op_substate();
  set_freq();
}


/**************************************/
/* Filters control                    */
/**************************************/
void update_filters_control() {

  filt_reg = (1 << cur_band[cur_vfo]) + (1 << (op_rec.mode + 12));

  // first 8 band filters control lines
  Wire.beginTransmission(PCF8574_1_ADDR);
  Wire.write(filt_reg & 0xFF);
  Wire.endTransmission();

  // last 3 band filters lines + 4 mode filters control lines
  Wire.beginTransmission(PCF8574_2_ADDR);
  Wire.write(filt_reg >> 8);
  Wire.endTransmission();

}


/**************************************/
/* Change mode                        */
/**************************************/
void rotate_mode() {
  op_rec.mode = (op_rec.mode + 1) % 4;
  ee_update_cur_op_rec_mode();
  set_freq();
}


/**************************************/
/* Change current vfo                 */
/**************************************/
void change_curvfo() {
  flush_store_updates();
  cur_vfo = -cur_vfo + 1;
  ee_store_cur_vfo();
  display_cur_vfo();
  ee_load_vfo_rec(cur_vfo, cur_band[cur_vfo]);
  infer_vfo_op_substate();
  set_freq();
}


/**************************************/
/* Set UI lock                        */
/**************************************/
void toggle_uilock() {
  ui_lock = ! ui_lock;
  display_uilock();
  if (!ui_lock)
    set_freq();
}


/**************************************/
/* Toggle MEMory channels             */
/**************************************/
void step_into_vfo_dial() {
  flush_store_updates();
  state = STATE_VFO_DIAL;
  ee_store_state();
  infer_vfo_op_substate();
  ee_load_vfo_rec(cur_vfo, cur_band[cur_vfo]);
  display_vfo_mem();
  set_freq();
}

void step_into_mem_dial() {
  flush_store_updates();
  if (ee_seek_ch_rec(CH_SEEK_CUR)) {
    state = STATE_MEM_DIAL;
    substate = SUBSTATE_OP;
    ee_store_state();
    display_vfo_mem();
    set_freq();
  }
  else
    error(ERR_NO_MEM_CHS);
}


/**************************************/
/* Dump VFO to MEM operations         */
/**************************************/
void initiate_dump_vfo_to_mem() {
  flush_store_updates();
  substate = SUBSTATE_PICKMEM;
  cur_mem_pick = cur_mem;
  display_vfo_mem();
  display_step();
}

void rotate_pick_mem(unsigned char direction) {
  cur_mem_pick = (cur_mem_pick + ((direction == DIR_CW) ? +1 : -1) + CH_NO) % CH_NO;
  book_pick_mem();
}

void finalize_dump_vfo_to_mem(boolean go) {
  if (go) {
    ee_store_cur_op_rec_to_ch(cur_mem_pick);
    cur_mem = cur_mem_pick;
    done();
  }
  infer_vfo_op_substate();
  display_vfo_mem();
  display_step();
}


/**************************************/
/* Dump MEM to VFO operations         */
/**************************************/
void dump_mem_to_vfo() {
  flush_store_updates();
  // fallback to VFO_DIAL state
  state = STATE_VFO_DIAL;
  substate = SUBSTATE_OP;
  ee_store_state();
  // switch to maching band
  accept_vfo_freq(_dge_buffer);
  // overwrite rec on current vfo
  ee_store_cur_op_rec_to_cur_vfo();
  done();
  // refresh display and tune
  display_vfo_mem();
  set_freq();
}


/**************************************/
/* Direct frequency entry             */
/**************************************/
void initiate_vfo_freq_entry() {
  substate = SUBSTATE_DGE;
  _dge_buffer = 0;
  _dge_pos = 0;
  display_init_vfo_dge();
}

void process_vfo_freq_entry_digit(byte digit) {
  _dge_buffer *= 10;
  _dge_buffer += digit;
  _dge_pos++;
  display_process_freq_dge_digit(digit);
  if (_dge_pos == 9)
    finalize_vfo_freq_entry(true);
}

void finalize_vfo_freq_entry(bool go) {
  if (go) {
    if (accept_vfo_freq(_dge_buffer)) {
      if (op_rec.rit != 0)
        reset_vfo_rit();
      op_rec.rx = _dge_buffer;
      ee_update_cur_op_rec_rx();
      display_clear_dge_prompt();
      substate = SUBSTATE_OP;
      set_freq();
    }
    else {
      infer_vfo_op_substate();
      error(ERR_BAND_FREQ_RANGE);
    }
  }
  else {
    display_clear_dge_prompt();
    infer_vfo_op_substate();
    set_freq();
  }
}


/**************************************/
/* Direct channel entry             */
/**************************************/
void initiate_mem_ch_entry() {
  substate = SUBSTATE_DGE;
  _dge_buffer = 0;
  _dge_pos = 0;
  display_init_ch_dge();
}

void process_mem_ch_entry_digit(byte digit) {
  _dge_buffer *= 10;
  _dge_buffer += digit;
  _dge_pos++;
  display_process_ch_dge_digit(digit);
  if (_dge_pos == 2)
    finalize_mem_ch_entry(true);
}

void finalize_mem_ch_entry(bool go) {
  substate = SUBSTATE_OP;
  if (go) {
    if (ee_load_ch_rec(_dge_buffer-1))
      set_freq();
    else
      error(ERR_MEM_CH_UNSET);
  }
  else {
    display_vfo_mem();
    display_step();
  }
}


/**************************************/
/* Rotate memory channels             */
/**************************************/
void rotate_mem(unsigned char direction) {
  book_seek_mem(direction);
}

void seek_mem(unsigned char direction) {
  if (ee_seek_ch_rec((direction == DIR_CW) ? +1 : -1)) {
    display_vfo_mem();
    set_freq();
  }
  else
    error(ERR_NO_MEM_CHS);
}


/**************************************/
/* Retrieve band and accepting freq   */
/**************************************/
uint8_t covering_band(uint32_t rx) {
  for (int i = 0; i < BANDS_NO; i++) {
    if (rx >= bands[i].lower_lim && rx < bands[i].upper_lim) {
      return i;
    }
  }
  return 255;
}

boolean accept_vfo_freq(uint32_t freq) {
  // find matching band 
  uint8_t target_band;
  if ((target_band = covering_band(freq)) == 255)
    return false;
  flush_store_updates();
  // band switch
  if (cur_band[cur_vfo] != target_band) {
    // switch to new band
    cur_band[cur_vfo] = target_band;
    #ifdef DEBUG
      Serial.print("New band:");
      Serial.println(cur_band[cur_vfo]);
    #endif
    ee_store_cur_band();
  }
  return true;
}


/**************************************/
/* Erase memory channel      */
/**************************************/
void unset_cur_mem() {
  // unset current mem
  ee_unset_ch(cur_mem);
  done();
  // seek next mem or error and fallback to VFO_DIAL
  if (ee_seek_ch_rec(CH_SEEK_CUR)) {
    display_vfo_mem();
    set_freq();
  }
  else {
    state = STATE_VFO_DIAL;
    ee_store_state();
    error(ERR_NO_MEM_CHS);
  }
}
  

/**************************************/
/* Even VFO A = B                     */
/**************************************/
void vfo_even() {
  byte winner = cur_vfo;
  cur_vfo = -cur_vfo + 1;
  cur_band[cur_vfo] = cur_band[winner];
  ee_store_cur_band();
  ee_store_cur_op_rec_to_cur_vfo();
  cur_vfo = winner;
  done();
}


/**************************************/
/* Si5351 init                        */
/**************************************/
void si5351_init() {
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, -__dds_cal * SI5351_FREQ_MULT); //If you're using a 27Mhz crystal, put in 27000000 instead of 0
  // 0 is the default crystal frequency of 25Mhz.
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);

}


/**************************************/
/* Si5351 monitoring                  */
/**************************************/
void si5351_status(char * buffer) {
  // poll device and wait
  si5351.update_status();
  delay(500);

  // format status message
  // I:0 A:0 O:1 R:2
  strcpy_P(buffer, PSTR("I:  A:  O:  R: "));
  buffer[2] = si5351.dev_status.SYS_INIT + 48;
  buffer[6] = si5351.dev_status.LOL_A + 48;
  buffer[10] = si5351.dev_status.LOS + 48;
  buffer[14] = si5351.dev_status.REVID + 48;

  return;
}


/**************************************/
/* Si5351 tuning support routines     */
/**************************************/
void init_cal_freq() {
  si5351.set_freq(DDS_TUNING_FREQ * SI5351_FREQ_MULT, SI5351_CLK0);
  si5351.output_enable(SI5351_CLK0, 1);
}

void adj_cal_freq(int32_t cal_factor) {
  __tuning_dds_cal = cal_factor;
  book_tune_cal();
}

void tune_cal_freq() {
  si5351.set_correction(-__tuning_dds_cal * SI5351_FREQ_MULT, SI5351_PLL_INPUT_XO);
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
  si5351.pll_reset(SI5351_PLLA);
  si5351.set_freq(DDS_TUNING_FREQ * SI5351_FREQ_MULT, SI5351_CLK0);
}

void stop_cal_freq() {
  si5351.set_correction(-__dds_cal * SI5351_FREQ_MULT, SI5351_PLL_INPUT_XO); // rollback to stable correction
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
  si5351.pll_reset(SI5351_PLLA);
  si5351.output_enable(SI5351_CLK0, 0);
}
