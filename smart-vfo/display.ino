/*
 * display.ino - Smart VFO
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

const char blank = ' ';
const char *doubleblank = "  ";
const char zero = '0';
const char dot = '.';

/**************************************/
/* Main display init                  */
/**************************************/
void display_init() {
  display_cache_reset();
  
  if (state == STATE_VFO_DIAL || state == STATE_MEM_DIAL) {
    display_vfo_mem();
    display_op();
    display_uilock();
    display_clear_res();
    lcd.cursor();
  }

  if (state == STATE_OP_MENU || state == STATE_CAL_MENU) {
    display_clear_line(1);
    lcd.noCursor();
  }
}

void display_cache_reset() {
  _d_cur_mode = 255;
  _d_cur_rit = 1;
  _d_cur_op = 255;
  _d_cur_rit_v = 0;
}


/**************************************/
/* Displays vfo / mem states          */
/**************************************/
void display_vfo_mem() {
  if (state == STATE_VFO_DIAL && substate != SUBSTATE_PICKMEM) {
    display_blanks(0, 1, 5);
    display_cur_vfo();
  }
  else if (state == STATE_VFO_DIAL && substate == SUBSTATE_PICKMEM) {
    lcd.setCursor(1, 0);
    lcd.write((uint8_t)RGT_ARR_SYMBOL);
    lcd.print("m");
    display_upd_cur_mem_pick();
  }
  else if (state == STATE_MEM_DIAL) {
    lcd.setCursor(0, 0);
    lcd.print(PSTR("  m"));
    display_upd_cur_mem();
  }
}

void display_cur_vfo() {
  lcd.setCursor(0, 0);
  lcd.write((uint8_t)cur_vfo); // it should be VFOA_SYMBOL or VFOB_SYMBOL, but... keeping it simple!
  if (split == SPLIT_ON) {
    lcd.print("/");
    lcd.write((uint8_t)(-cur_vfo+1));
  }
  else {
    lcd.print(doubleblank);
  }
}

void display_upd_cur_mem() {
  lcd.setCursor(3, 0);
  if (cur_mem+1 < 10)
    lcd.print('0');
  lcd.print(cur_mem+1);
}

void display_upd_cur_mem_pick() {
  lcd.setCursor(3, 0);
  if (cur_mem_pick+1 < 10)
    lcd.print('0');
  lcd.print(cur_mem_pick+1);
  lcd.write(ee_check_ch_set(cur_mem_pick) ? (uint8_t)SET_SYMBOL : blank);
}


/**************************************/
/* Displays the frequency             */
/**************************************/
void display_freq(uint32_t f) {  
  _display_freq(0, 6, f);
}

void _display_freq(byte row, byte col, uint32_t freq) {
  uint16_t f = freq / 1000000;
  byte bl = false;
  // display main frequency, passed in vfo argument
  lcd.setCursor(col, row);
  if (f >= 140 && f < 150) {
    lcd.write((uint8_t)DDIGIT_SYMBOL);
    f -= 140;
  }
  else if (f < 10)
    lcd.print(blank);
  if (f < 1) {
    lcd.print(doubleblank);
  } else {
    lcd.print(f);
    lcd.print(dot);
    bl = true;
  }
  f = (freq % 1000000) / 1000;
  if (f < 100)
    lcd.print(bl ? zero : blank);
  if (f < 10)
    lcd.print(bl ? zero : blank);
  if (f < 1 && !bl) {
    lcd.print(doubleblank);
  } else {
    bl = true;
    lcd.print(f);
    lcd.print(dot);
  }
  f = freq % 1000;
  if (f < 100)
    lcd.print(bl ? zero : blank);
  if (f < 10)
    lcd.print(bl ? zero : blank);
  lcd.print(f);

}


/**************************************/
/* Displays INT values                */
/**************************************/
void _display_int32_value(byte row, byte col, int32_t value) {
  // +/-100k values range
  lcd.setCursor(col, row);
  lcd.print((value >= 0) ? "+" : "-");
  value = abs(value);
  if (value < 10000) // TODO: optimize
    lcd.print(blank);
  if (value < 1000)
    lcd.print(doubleblank);
  if (value < 100)
    lcd.print(blank);
  if (value < 10)
    lcd.print(blank);
  int16_t v = value / 1000;
  if (v > 0) {
    lcd.print(v);
    lcd.print('.');
  }
  v = (value % 1000);
  if (v < 100 && value >= 1000)
    lcd.print('0');
  if (v < 10 && value >= 1000)
    lcd.print('0');
  lcd.print(v);

}


/**************************************/
/* Frequency digits entry             */
/**************************************/
void display_init_vfo_dge() {
  _display_init_freq_dge(0, 4);
}

void _display_init_freq_dge(byte row, byte col) {
  lcd.setCursor(col, row);
  lcd.print('>');
  display_blanks(-1, -1, 11);
  lcd.setCursor(col+1, row);
}

void display_process_freq_dge_digit(byte digit) {
  lcd.print(digit);
  if (_dge_pos == 3 || _dge_pos == 6)
    lcd.print(dot);
}

void display_clear_dge_prompt() {
  lcd.setCursor(4, 0);
  lcd.print(blank);
  lcd.print(blank);
}


/**************************************/
/* Channel digits entry               */
/**************************************/
void display_init_ch_dge() {
  lcd.setCursor(3, 0);
  lcd.print(doubleblank);
  lcd.setCursor(3, 0);
}

void display_process_ch_dge_digit(byte digit) {
  lcd.print(digit);
}


/**************************************/
/* Displays the step cursor           */
/**************************************/
void display_step() {
  if (state == STATE_VFO_DIAL && substate == SUBSTATE_OP)
    _display_step(0, 15, step);
  else if (state == STATE_VFO_DIAL && substate == SUBSTATE_RIT_OP)
    _display_step(1, 15, rit_step);
  else if ((state == STATE_VFO_DIAL && substate == SUBSTATE_PICKMEM) || state == STATE_MEM_DIAL)
    _display_step(0, 4, 1);
  else if (state == STATE_CAL_MENU || state == STATE_OP_MENU)
    _display_step(1, 9, menu_step);
}

void _display_step(byte row, byte col, uint32_t step) {
  // col = right end of label under editing
  // switch operator used for better performance
  uint8_t _col = col;
  switch (step) // TODO: optimize
  {
    case 10000000:
      _col -= 9;
      break;
    case 1000000:
      _col -= 8;
      break;
    case 100000:
      _col -= 6;
      break;
    case 10000:
      _col -= 5;
      break;
    case 1000:
      _col -= 4;
      break;
    case 100:
      _col -= 2;
      break;
    case 10:
      _col -= 1;
      break;
    case 1:
      break;
  }
  lcd.setCursor(_col, row);
}


/**************************************/
/* Displays operation state           */
/**************************************/
void display_op() {
  if (_d_cur_op != cur_op) {
    lcd.setCursor(0, 1);
    if (cur_op == OP_TX)
      lcd.write((uint8_t)OP_TX_SYMBOL);
    else
      lcd.print(blank);
    _d_cur_op = cur_op;
  }
}


/**************************************/
/* Displays UI lock state             */
/**************************************/
void display_uilock() {
  lcd.setCursor(1, 1);
  if (ui_lock) {
    lcd.write((uint8_t)LOCK_SYMBOL); // lock custom symbol
    lcd.noCursor();
  }
  else {
    lcd.print(blank);
    lcd.cursor();
  }
}


/**************************************/
/* Displays mode                      */
/**************************************/
void display_mode(byte mode) {
  if (_d_cur_mode != mode) {
    lcd.setCursor(2, 1);
    const char *label = mode_labels[mode];
    lcd.print(label);
    _d_cur_mode = mode;
  }
}


/**************************************/
/* Displays reserved chars            */
/**************************************/
void display_clear_res() {
  lcd.setCursor(5, 1);
  lcd.print(doubleblank);
}

/**************************************/
/* Displays RIT                       */
/**************************************/
void display_invalidate_rit() {
  _d_cur_rit = 1;
}

void display_rit() {
  if (substate != SUBSTATE_RIT_OP && _d_cur_rit >= 1 && op_rec.rit == 0) {
    display_blanks(1, 7, 9);
    _d_cur_rit = 0;
    _d_cur_rit_v = 0;
  }
  else if (substate == SUBSTATE_RIT_OP || op_rec.rit != 0) {
    if (_d_cur_rit <= 1) {
      lcd.setCursor(7, 1);
      lcd.write((uint8_t)RIT_SYMBOL); // RIT custom symbol
      _d_cur_rit = 2;
    }
    else {
      lcd.setCursor(8, 1);
    }

    if (_d_cur_rit_v != op_rec.rit) {

      int32_t rit = op_rec.rit;
      int16_t s;

      lcd.print((rit >= 0) ? "+" : "-");
      rit = abs(rit);
      if (rit < 100000) // TODO: optimize
        lcd.print(blank);
      if (rit < 10000)
        lcd.print(blank);
      if (rit < 1000)
        lcd.print(doubleblank);
      if (rit < 100)
        lcd.print(blank);
      if (rit < 10)
        lcd.print(blank);
      s = rit / 1000;
      if (s > 0) {
        lcd.print(s);
        lcd.print('.');
      }
      s = (rit % 1000);
      if (s < 100 && rit >= 1000)
        lcd.print('0');
      if (s < 10 && rit >= 1000)
        lcd.print('0');
      lcd.print(s);

      _d_cur_rit_v = op_rec.rit;
    }

    _d_cur_rit = 1;
  }

}


/**************************************/
/* Displays menu                      */
/**************************************/
void display_menu() {

  char line[17];

  if (substate == SUBSTATE_VALUE_EDIT) {
    lcd.setCursor(0, 1);

    if (menu_value_type == MENUITEM_TYPE_PICK) {
      sprintf(line, "%-14s", menu_values_domain + menu_curvalue * menu_values_len);
      lcd.print(line);
    }
    else if (menu_value_type == MENUITEM_TYPE_FREQ || menu_value_type == MENUITEM_TYPE_BW) {
      _display_freq(1, 0, menu_value);
      display_step();
    }
    else if (menu_value_type == MENUITEM_TYPE_INT32) {
      _display_int32_value(1, 3, menu_value);
      display_step();
    }
  }
  else {
    sprintf(line, "%d: %-14s", menu_curitem, menu_labels[menu_curitem]);
    lcd.setCursor(0, 0);
    lcd.print(line);
  }
}

void display_clear_line(byte line) {
  display_blanks(line, 0, 16);
}

void display_err(const __FlashStringHelper*errmsg) {
  lcd.noCursor();
  display_clear_line(0);
  lcd.setCursor(0, 0);
  lcd.print(PSTR("ERROR"));
  display_clear_line(1);
  lcd.setCursor(0, 1);
  lcd.print(errmsg);
}

void display_blanks(int8_t row, int8_t col, byte no) {
  // set row = -1 to bypass cursor positioning
  if (row >= 0)
    lcd.setCursor(col, row);
  for (int i = 0; i < no; i++)
    lcd.print(blank);
}