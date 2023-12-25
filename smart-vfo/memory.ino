/*
 * memory.ino - Smart VFO
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
/* Record-oriented store routines     */
/**************************************/

void ee_update_cur_op_rec_rx() {
  if (state == STATE_VFO_DIAL)
    eeprom_uint32update(MEM_VFO_ADDR+cur_vfo*VFO_BANK_SIZE+cur_band[cur_vfo]*OP_RECORD_SIZE, op_rec.rx);
  else if (state == STATE_MEM_DIAL)
    eeprom_uint32update(MEM_CH_ADDR+cur_mem*CH_RECORD_SIZE+1, op_rec.rx);
}

void ee_update_cur_op_rec_mode() {
  if (state == STATE_VFO_DIAL)
    eeprom_uint32update(MEM_VFO_ADDR+cur_vfo*VFO_BANK_SIZE+cur_band[cur_vfo]*OP_RECORD_SIZE+4, op_rec.mode);
  else if (state == STATE_MEM_DIAL)
    eeprom_uint32update(MEM_CH_ADDR+cur_mem*CH_RECORD_SIZE+5, op_rec.mode);
}

void ee_update_cur_op_rec_rit() {
  if (state == STATE_VFO_DIAL)
    eeprom_uint32update(MEM_VFO_ADDR+cur_vfo*VFO_BANK_SIZE+cur_band[cur_vfo]*OP_RECORD_SIZE+5, op_rec.rit);
  else if (state == STATE_MEM_DIAL)
    eeprom_uint32update(MEM_CH_ADDR+cur_mem*CH_RECORD_SIZE+6, op_rec.rit);
}

void ee_store_cur_op_rec_to_cur_vfo() {
  eeprom_uint32update(MEM_VFO_ADDR+cur_vfo*VFO_BANK_SIZE+cur_band[cur_vfo]*OP_RECORD_SIZE, op_rec.rx);
  eeprom_uint32update(MEM_VFO_ADDR+cur_vfo*VFO_BANK_SIZE+cur_band[cur_vfo]*OP_RECORD_SIZE+4, op_rec.mode);
  eeprom_uint32update(MEM_VFO_ADDR+cur_vfo*VFO_BANK_SIZE+cur_band[cur_vfo]*OP_RECORD_SIZE+5, op_rec.rit);
}

void ee_store_op_rec_to_vfo(op_record *ptr, byte vfo, byte band) {
  eeprom_uint32update(MEM_VFO_ADDR+vfo*VFO_BANK_SIZE+band*OP_RECORD_SIZE, ptr->rx);
  eeprom_uint32update(MEM_VFO_ADDR+vfo*VFO_BANK_SIZE+band*OP_RECORD_SIZE+4, ptr->mode);
  eeprom_uint32update(MEM_VFO_ADDR+vfo*VFO_BANK_SIZE+band*OP_RECORD_SIZE+5, ptr->rit);
}

void ee_store_cur_op_rec_to_ch(byte no) {
  eeprom_uint32update(MEM_CH_ADDR+no*CH_RECORD_SIZE, CH_RECORD_STATUS_SET);
  eeprom_uint32update(MEM_CH_ADDR+no*CH_RECORD_SIZE+1, op_rec.rx);
  eeprom_uint32update(MEM_CH_ADDR+no*CH_RECORD_SIZE+5, op_rec.mode);
  eeprom_uint32update(MEM_CH_ADDR+no*CH_RECORD_SIZE+6, op_rec.rit);
}

void ee_unset_ch(byte no) {
  eeprom_uint32update(MEM_CH_ADDR+no*CH_RECORD_SIZE, CH_RECORD_STATUS_UNSET);
}


/**************************************/
/* Record-oriented load routines      */
/**************************************/

void ee_load_vfo_rec(byte vfo, byte band) {
  EEPROM.get(MEM_VFO_ADDR+vfo*VFO_BANK_SIZE+band*OP_RECORD_SIZE, op_rec.rx);
  EEPROM.get(MEM_VFO_ADDR+vfo*VFO_BANK_SIZE+band*OP_RECORD_SIZE+4, op_rec.mode);
  EEPROM.get(MEM_VFO_ADDR+vfo*VFO_BANK_SIZE+band*OP_RECORD_SIZE+5, op_rec.rit);
}

op_record ee_find_vfo_rec(byte vfo, byte band) {
  op_record buffer;
  EEPROM.get(MEM_VFO_ADDR+vfo*VFO_BANK_SIZE+band*OP_RECORD_SIZE, buffer.rx);
  EEPROM.get(MEM_VFO_ADDR+vfo*VFO_BANK_SIZE+band*OP_RECORD_SIZE+4, buffer.mode);
  // only used for TX record in SPLIT mode. No use for RIT value
  return buffer;
}

boolean ee_load_ch_rec(byte no) {
  byte ch_status_buff;
  EEPROM.get(MEM_CH_ADDR+no*CH_RECORD_SIZE, ch_status_buff);
  if (ch_status_buff == CH_RECORD_STATUS_UNSET)
    return false;
  EEPROM.get(MEM_CH_ADDR+no*CH_RECORD_SIZE+1, op_rec.rx);
  EEPROM.get(MEM_CH_ADDR+no*CH_RECORD_SIZE+5, op_rec.mode);
  EEPROM.get(MEM_CH_ADDR+no*CH_RECORD_SIZE+6, op_rec.rit);
  if (no != cur_mem) {
    cur_mem = no;
    ee_store_cur_mem();            
  }
  return true;
}


/****************************************/
/* Operating parameters store routines  */
/****************************************/

void ee_store_cur_vfo()
{
  EEPROM.update(MEM_PARAMS_ADDR, cur_vfo);
}

void ee_store_cur_band()
{
  EEPROM.update(MEM_PARAMS_ADDR+cur_vfo+1, cur_band[cur_vfo]);
}

void ee_store_split()
{
  EEPROM.update(MEM_PARAMS_ADDR+3, split);
}

void ee_store_step()
{
  eeprom_uint32update(MEM_PARAMS_ADDR+4, step);
}

void ee_store_rit_step()
{
  eeprom_uint32update(MEM_PARAMS_ADDR+8, rit_step);
}

void ee_store_cur_mem()
{
  EEPROM.update(MEM_PARAMS_ADDR+12, cur_mem);
}

void ee_store_state()
{
  EEPROM.update(MEM_PARAMS_ADDR+13, state);
}

void ee_store_sound()
{
  EEPROM.update(MEM_PARAMS_ADDR+14, _sound);
}


/******************************************/
/* Calibration parameters store routines  */
/******************************************/

void ee_store_design()
{
  EEPROM.update(MEM_CAL_PARAMS_ADDR, __design);
}

void ee_store_if()
{
  eeprom_uint32update(MEM_CAL_PARAMS_ADDR+1, __if);
}

void ee_store_conv()
{
  EEPROM.update(MEM_CAL_PARAMS_ADDR+5, __conv);
}

void ee_store_ssb_offset()
{
  eeprom_uint16update(MEM_CAL_PARAMS_ADDR+6, __ssb_offset);
}

void ee_store_cw_offset()
{
  eeprom_uint16update(MEM_CAL_PARAMS_ADDR+8, __cw_offset);
}

void ee_store_dds_cal()
{
  eeprom_int32update(MEM_CAL_PARAMS_ADDR+10, __dds_cal);
}

void ee_store_dds_pwr()
{
  EEPROM.update(MEM_CAL_PARAMS_ADDR+14, __dds_pwr0);
  EEPROM.update(MEM_CAL_PARAMS_ADDR+16, __dds_pwr2);
}


/**************************************/
/* Boot time current params load      */
/**************************************/
void ee_boot_load()
{
  // load operating params
  EEPROM.get(MEM_PARAMS_ADDR, cur_vfo);
  EEPROM.get(MEM_PARAMS_ADDR+1, cur_band[0]);
  EEPROM.get(MEM_PARAMS_ADDR+2, cur_band[1]);
  EEPROM.get(MEM_PARAMS_ADDR+3, split);
  EEPROM.get(MEM_PARAMS_ADDR+4, step);
  EEPROM.get(MEM_PARAMS_ADDR+8, rit_step);
  EEPROM.get(MEM_PARAMS_ADDR+12, cur_mem);
  EEPROM.get(MEM_PARAMS_ADDR+13, state);
  EEPROM.get(MEM_PARAMS_ADDR+14, _sound);

  // load calibration params
  EEPROM.get(MEM_CAL_PARAMS_ADDR, __design);
  EEPROM.get(MEM_CAL_PARAMS_ADDR+1, __if);
  EEPROM.get(MEM_CAL_PARAMS_ADDR+5, __conv);
  EEPROM.get(MEM_CAL_PARAMS_ADDR+6, __ssb_offset);
  EEPROM.get(MEM_CAL_PARAMS_ADDR+8, __cw_offset);
  EEPROM.get(MEM_CAL_PARAMS_ADDR+10, __dds_cal);
  EEPROM.get(MEM_CAL_PARAMS_ADDR+14, __dds_pwr0);
  EEPROM.get(MEM_CAL_PARAMS_ADDR+16, __dds_pwr2);

}


/*****************************************************************/
/* Init eeprom with current datamodel version and default values */
/*****************************************************************/
void ee_init()
{
  #ifdef DEBUG
    Serial.println("Resetting EEPROM...");
  #endif
  
  // init boot record
  EEPROM.update(MEM_BOOT_ADDR, EEPROM_DATAMODEL_VERSION);


  // init scalar params
  cur_vfo = VFO_B;
  cur_band[VFO_B] = 5;
  ee_store_cur_band();

  cur_vfo = VFO_A;
  cur_band[VFO_A] = 5;
  ee_store_cur_band();
  ee_store_cur_vfo();

  split = 0;
  ee_store_split();

  step = 100;
  ee_store_step();

  rit_step = 10;
  ee_store_rit_step();

  cur_mem = 0;
  ee_store_cur_mem();

  state = STATE_VFO_DIAL;
  ee_store_state();

  _sound = 1;
  ee_store_sound();


  // init calibration params
  __design = DESIGN_SINGLE_CONV;
  ee_store_design();

  __if = 999900000UL / SI5351_FREQ_MULT;
  ee_store_if();

  __conv = CONV_UPCONV;
  ee_store_conv();

  __ssb_offset = 1500;
  ee_store_ssb_offset();

  __cw_offset = 800;
  ee_store_cw_offset();

  __dds_cal = 0;
  ee_store_dds_cal();

  __dds_pwr0 = 3;
  __dds_pwr2 = 3;
  ee_store_dds_pwr();


  // operations data
  op_record vfos[BANDS_NO] = {
    // VFO A
    { 184000000UL / SI5351_FREQ_MULT, MODE_LSB, 0},
    { 365000000UL / SI5351_FREQ_MULT, MODE_LSB, 0},
    { 536000000UL / SI5351_FREQ_MULT, MODE_LSB, 0},
    { 710000000UL / SI5351_FREQ_MULT, MODE_LSB, 0},
    {1013000000UL / SI5351_FREQ_MULT, MODE_USB, 0},
    {1420000000UL / SI5351_FREQ_MULT, MODE_USB, 0},
    {1812000000UL / SI5351_FREQ_MULT, MODE_USB, 0},
    {2120000000UL / SI5351_FREQ_MULT, MODE_USB, 0},
    {2494000000UL / SI5351_FREQ_MULT, MODE_USB, 0},
    {2895000000UL / SI5351_FREQ_MULT, MODE_USB, 0},
    {5010000000UL / SI5351_FREQ_MULT, MODE_USB, 0},
    {14450000000UL / SI5351_FREQ_MULT, MODE_USB, 0}
  };

  for (byte i = 0; i < 2; i++)
    for (byte j = 0; j < BANDS_NO; j++)
      ee_store_op_rec_to_vfo(&vfos[j], i, j);

  // init memory channels
  for (byte i = 0; i < CH_NO; i++)
    ee_unset_ch(i);

  done();
}


/************************************************************/
/* Evaluate whether EEPROM is already populated, or init it */
/************************************************************/
boolean ee_check()
{
  byte eeprom_dm_ver;
  EEPROM.get(MEM_BOOT_ADDR, eeprom_dm_ver);

  #ifdef DEBUG
    Serial.print("EEPROM datamodel =");
    Serial.println(eeprom_dm_ver);
  #endif

  if (eeprom_dm_ver != EEPROM_DATAMODEL_VERSION) {
    ee_init();
    return false;
  }

  return true;
}

void eeprom_uint32update(int addr, uint32_t val) {
  EEPROM.update(addr, val & 0xFF);
  EEPROM.update(addr + 1, (val >> 8) & 0xFF);
  EEPROM.update(addr + 2, (val >> 16) & 0xFF);
  EEPROM.update(addr + 3, (val >> 24) & 0xFF);
}

void eeprom_int32update(int addr, int32_t val) {
  EEPROM.update(addr, val & 0xFF);
  EEPROM.update(addr + 1, (val >> 8) & 0xFF);
  EEPROM.update(addr + 2, (val >> 16) & 0xFF);
  EEPROM.update(addr + 3, (val >> 24) & 0xFF);
}

void eeprom_uint16update(int addr, uint16_t val) {
  EEPROM.update(addr, val & 0xFF);
  EEPROM.update(addr + 1, (val >> 8) & 0xFF);
}


/**************************************/
/* Browsing into memory channels      */
/**************************************/
boolean ee_seek_ch_rec(int8_t offset) {
  // argument values:
  // 0 = current or first subsequent
  // +1 = next one or first subsequent
  // -1 = previous one or first preceding
  // returns:
  // one applicable channel has been found
  byte start_cur_mem = (cur_mem + offset + CH_NO) % CH_NO;
  int8_t i = start_cur_mem;
  do {
    if (ee_load_ch_rec(i))
      return true;
    i = (i + ((offset >= 0) ? 1 : -1) + CH_NO) % CH_NO;
  }
  while (i != start_cur_mem);
  return false;
}

boolean ee_check_ch_set(byte no) {
  byte ch_status_buff;
  EEPROM.get(MEM_CH_ADDR+no*CH_RECORD_SIZE, ch_status_buff);
  return ch_status_buff;
}
