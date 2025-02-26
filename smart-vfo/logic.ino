/*
 * logic.ino - Smart VFO
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

int8_t enc_buff = 0;
byte enc_lastop = DIR_CW;

/**************************************/
/* Interrupt service routine for      */
/* encoder frequency change           */
/**************************************/
ISR(PCINT2_vect) {
  unsigned char result = r.process();
  /**************************************/
  /* dir = DIR_CW    Increment          */
  /* dir = DIR_CCW   Decrement          */
  /**************************************/
  if (!ui_lock && cur_op != OP_TX && result > 0) {

    // encoder run scaler
    if (result == enc_lastop)
      enc_buff++;
    else {
      enc_lastop = result;
      enc_buff = 1;
    }
    if (enc_buff < __enc_scale)
      return;
    else
      enc_buff = 0;

    if (state == STATE_VFO_DIAL && substate == SUBSTATE_OP) {
      rotate_freq(result);
    }
    else if (state == STATE_VFO_DIAL && substate == SUBSTATE_RIT_OP) {
      rotate_rit(result);
    }
    else if (state == STATE_VFO_DIAL && substate == SUBSTATE_PICKMEM) {
      rotate_pick_mem(result);
    }
    else if (state == STATE_MEM_DIAL) {
      rotate_mem(result);
    }
    else if (state == STATE_OP_MENU || state == STATE_CAL_MENU) {
      rotate_menu(result);
    }

  }
}


void loop()
{

  // Control OP button
  if (get_pulsebutton(OP_BTN) > PLSBTN_UNCHANGED)
    change_op();

  // Control STEP button
  switch (get_button(STEP_BTN, 0)) {
    case BTN_SHORT_PRESSED:
      rotate_step();
      break;
    case BTN_LONG_PRESSED:
      break;
  }

  // Control digit entry for VFO state
  if (state == STATE_VFO_DIAL && substate == SUBSTATE_DGE) {

    if (get_button(ESC_LOCK_BTN, 0) == BTN_SHORT_PRESSED)
      finalize_vfo_freq_entry(false);
    else {      
      byte digit = which_digit_button(0);
      if (digit < 255)
        process_vfo_freq_entry_digit(digit);
    }
  }

  // Control digit entry for MEM state
  else if (state == STATE_MEM_DIAL && substate == SUBSTATE_DGE) {

    if (get_button(ESC_LOCK_BTN, 0) == BTN_SHORT_PRESSED)
      finalize_mem_ch_entry(false);
    else {      
      byte digit = which_digit_button(0);
      if (digit < 255)
        process_mem_ch_entry_digit(digit);
    }
  }

  // Control channel select for VFO state
  else if (state == STATE_VFO_DIAL && substate == SUBSTATE_PICKMEM) {

    if (get_button(ESC_LOCK_BTN, 0) == BTN_SHORT_PRESSED)
      finalize_dump_vfo_to_mem(false);

    else if (get_button(ENT_MENU_BTN, 0) == BTN_SHORT_PRESSED)
      finalize_dump_vfo_to_mem(true);
  }

  // Control menu navigation
  else if (state == STATE_OP_MENU || state == STATE_CAL_MENU) {

    if (get_button(ESC_LOCK_BTN, 0) == BTN_SHORT_PRESSED) {
        if (substate == SUBSTATE_ITEM_SEL)
          toggle_menu(NONE);
        else if (substate == SUBSTATE_VALUE_EDIT)
          leave_menu_item();
    }
    else if (get_button(ENT_MENU_BTN, 0) == BTN_SHORT_PRESSED) {
        if (substate == SUBSTATE_ITEM_SEL)
          enter_menu_item();
        else if (substate == SUBSTATE_VALUE_EDIT)
          process_menu_item();      
    }

  }

  // Control VFO operations
  else if (state == STATE_VFO_DIAL) {

    switch (get_button(BAND_UP_BTN, 0)) {
      case BTN_SHORT_PRESSED:
        rotate_band(DIR_CW);
        break;
      case BTN_LONG_PRESSED:
        break;
    }

    switch (get_button(BAND_DN_BTN, 0)) {
      case BTN_SHORT_PRESSED:
        rotate_band(DIR_CCW);
        break;
      case BTN_LONG_PRESSED:
        break;
    }

    switch (get_button(MODE_BTN, 0)) {
      case BTN_SHORT_PRESSED:
        rotate_mode();
        break;
      case BTN_LONG_PRESSED:
        break;
    }

    switch (get_button(VFO_AB_BTN, 0)) {
      case BTN_SHORT_PRESSED:
        change_curvfo();
        break;
      case BTN_LONG_PRESSED:
        vfo_even(); // VFO A=B
        break;
    }

    switch (get_button(MR_MIN_BTN, 0)) {
      case BTN_SHORT_PRESSED:
        step_into_mem_dial();
        break;
      case BTN_LONG_PRESSED:
        initiate_dump_vfo_to_mem();
        break;
    }

    switch (get_button(RIT_SPLIT_BTN, 0)) {
      case BTN_SHORT_PRESSED:
        toggle_rit_editing();
        break;
      case BTN_LONG_PRESSED:
        toggle_split();
        break;
    }

    switch (get_button(ESC_LOCK_BTN, 0)) {
      case BTN_SHORT_PRESSED:
        break;
      case BTN_LONG_PRESSED:
        toggle_uilock();
        break;
    }

    switch (get_button(ENT_MENU_BTN, 0)) {
      case BTN_SHORT_PRESSED:
        initiate_vfo_freq_entry();
        break;
      case BTN_LONG_PRESSED:
        toggle_menu(MENU_OP);
        break;
    }
  }

  // Control MEM operations
  else if (state == STATE_MEM_DIAL) {

    switch (get_button(VFO_MV_BTN, 0)) {
      case BTN_SHORT_PRESSED:
        step_into_vfo_dial();
        break;
      case BTN_LONG_PRESSED:
        dump_mem_to_vfo();
        break;
    }

    switch (get_button(MR_MIN_BTN, 0)) {
      case BTN_SHORT_PRESSED:
        break;
      case BTN_LONG_PRESSED:
        unset_cur_mem();
        break;
    }

    switch (get_button(ESC_LOCK_BTN, 0)) {
      case BTN_SHORT_PRESSED:
        break;
      case BTN_LONG_PRESSED:
        toggle_uilock();
        break;
    }

    switch (get_button(ENT_MENU_BTN, 0)) {
      case BTN_SHORT_PRESSED:
        initiate_mem_ch_entry();
        break;
      case BTN_LONG_PRESSED:
        toggle_menu(MENU_OP);
        break;
    }
  }

  // allow booked actions
  serve_tune_cal();
 
  serve_set_freq();
  serve_display_vfo_mem();
  serve_seek_mem();
  serve_pick_mem();
  serve_display_menu();

  serve_store_freq(false);
  serve_store_rit(false);
}
