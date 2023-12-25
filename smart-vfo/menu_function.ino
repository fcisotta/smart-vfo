/*
 * menu_function.ino - Smart VFO
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
/* Toggle MENU                        */
/**************************************/
void toggle_menu(byte menu_id) {
  // leave menu
  if (state == STATE_CAL_MENU)
    resetFunc();
  else if (state == STATE_OP_MENU) {
    pop_state();
    unalloc_menu();
    display_init();
    set_freq();
  }
  // enter menu
  else {
    push_state();
    // enter MENU state
    state = (menu_id == MENU_CAL) ? STATE_CAL_MENU : STATE_OP_MENU;
    display_init();
    menu_init();
    display_menu();
  }
}


/**************************************/
/* Init MENU                          */
/**************************************/
void menu_init() {

  if (state == STATE_CAL_MENU) {

    #define CAL_MENU_ITEMS_NO  8
    menu_items_no = CAL_MENU_ITEMS_NO;
    menu_items_valid = (byte *) malloc(menu_items_no);
    menu_labels = (char **)malloc(menu_items_no * sizeof(char *));

    menu_labels[0] = (char *) malloc(7);
    strcpy_P(menu_labels[0], PSTR("Design"));

    menu_labels[1] = (char *) malloc(3);
    strcpy_P(menu_labels[1], PSTR("IF"));

    menu_labels[2] = (char *) malloc(11);
    strcpy_P(menu_labels[2], PSTR("Conversion"));

    menu_labels[3] = (char *) malloc(11);
    strcpy_P(menu_labels[3], PSTR("SSB offset"));

    menu_labels[4] = (char *) malloc(10);
    strcpy_P(menu_labels[4], PSTR("CW offset"));
    
    menu_labels[5] = (char *) malloc(8);
    strcpy_P(menu_labels[5], PSTR("DDS cal"));

    menu_labels[6] = (char *) malloc(9);
    strcpy_P(menu_labels[6], PSTR("CLK0 pwr"));
    
    menu_labels[7] = (char *) malloc(9);
    strcpy_P(menu_labels[7], PSTR("CLK2 pwr"));

  }
  else if (state == STATE_OP_MENU) {
    #define MENU_ITEMS_NO  2
    menu_items_no = MENU_ITEMS_NO;
    menu_items_valid = (byte *) malloc(menu_items_no);
    menu_labels = (char **)malloc(menu_items_no * sizeof(char *));

    //menu[0].label = (char *) malloc(6);
    /*menu_labels[0] = (char *) malloc(6);
    strcpy_P(menu_labels[0], PSTR("Sound"));*/

    menu_labels[0] = (char *) malloc(14);
    strcpy_P(menu_labels[0], PSTR("Si5351 status"));

    menu_labels[1] = (char *) malloc(14);
    strcpy_P(menu_labels[1], PSTR("Firmware info"));

  }

  // init menu items validity
  for (byte i=0; i < menu_items_no; i++)
    menu_items_valid[i] = 1;

  substate = SUBSTATE_ITEM_SEL;
}

void unalloc_menu() {
  for (byte i = 0; i < menu_items_no; i++)
    free(menu_labels[i]);
  free(menu_labels);
  free(menu_items_valid);
}


void update_menu_valid() {
  // re-evaluate menu items validity
  if (state == STATE_CAL_MENU) {
    menu_items_valid[1] = __design == DESIGN_SINGLE_CONV;
    menu_items_valid[2] = __design == DESIGN_SINGLE_CONV;
    menu_items_valid[3] = __design == DESIGN_SINGLE_CONV;
    menu_items_valid[4] = __design == DESIGN_SINGLE_CONV;
    menu_items_valid[7] = __design == DESIGN_SINGLE_CONV;
  }
}


/********************************************/
/* Rotate menu items/values                 */
/********************************************/
void rotate_menu(unsigned char direction) {

  // rotate menu items
  if (substate == SUBSTATE_ITEM_SEL) {

    // evaluate menu items validity
    update_menu_valid();

    do {
      menu_curitem += (direction == DIR_CW) ? +1 : -1;
      if (menu_curitem < 0)
        menu_curitem += menu_items_no;
      menu_curitem %= menu_items_no;
    }
    while (menu_items_valid[menu_curitem] == 0);

  }
  // rotate or edit menu values
  else if (substate == SUBSTATE_VALUE_EDIT) {

    if (menu_value_type == MENUITEM_TYPE_PICK) {
      menu_curvalue += (direction == DIR_CW) ? +1 : -1;
      if (menu_curvalue > menu_values_no - 1)
        menu_curvalue = 0;
      else if (menu_curvalue == -1)
        menu_curvalue = menu_values_no - 1;
    }
    else if (menu_value_type == MENUITEM_TYPE_FREQ || menu_value_type == MENUITEM_TYPE_BW ||
     menu_value_type == MENUITEM_TYPE_INT32) {

      if ((direction == DIR_CW && menu_value + menu_step <= menu_value_max) ||
        (direction == DIR_CCW && menu_value - menu_step >= menu_value_min)) {
        if (direction == DIR_CW && menu_value < 0 && menu_value + menu_step > 0 ||
          direction == DIR_CCW && menu_value > 0 && menu_value - menu_step < 0)
          menu_value = -menu_value;
        else
          menu_value += (direction == DIR_CW) ? menu_step : -menu_step;
      }
      menu_sel_feedback();
    }
  }

  book_display_menu();
}

void menu_sel_init() {
  if (state == STATE_CAL_MENU && menu_curitem == 4)
    init_cal_freq();
}

void menu_sel_feedback() {
  if (state == STATE_CAL_MENU && menu_curitem == 4)
    adj_cal_freq(menu_value);
}

void menu_sel_end() {
  if (state == STATE_CAL_MENU && menu_curitem == 4)
    stop_cal_freq();
}


/***************************************************/
/* Process menu items (edit value)                 */
/***************************************************/
void enter_menu_item() {
  // setup navigation of the item values
  // CALIBRATION MENU
  if (state == STATE_CAL_MENU) {
    switch (menu_curitem) {
      case 0:
        menu_value_type = MENUITEM_TYPE_PICK;
        menu_values_no = 3;
        menu_values_len = 12;
        menu_values_domain = (char *) malloc(menu_values_no * menu_values_len);
        strcpy_P(menu_values_domain, PSTR("Single conv"));
        strcpy_P(menu_values_domain+menu_values_len, PSTR("Direct conv"));
        strcpy_P(menu_values_domain+menu_values_len*2, PSTR("Freq x 4"));
        menu_curvalue = __design;
        break;
      case 1:
        menu_value_type = MENUITEM_TYPE_FREQ;
        menu_value_min = 10; // must be power of 10
        menu_value_max = 90000000;
        menu_value = __if;
        menu_step = 100;
        break;
      case 2:
        menu_value_type = MENUITEM_TYPE_PICK;
        menu_values_no = 2;
        menu_values_len = 9;
        menu_values_domain = (char *) malloc(menu_values_no * menu_values_len);
        strcpy_P(menu_values_domain, PSTR("LO=IF+RF"));
        strcpy_P(menu_values_domain+menu_values_len, PSTR("LO=IF-RF"));
        menu_curvalue = __conv;
        break;
      case 3:
        menu_value_type = MENUITEM_TYPE_BW;
        menu_value_min = 10; // must be power of 10
        menu_value_max = 10000;
        menu_value = __ssb_offset;
        menu_step = 10;
        break;
      case 4:
        menu_value_type = MENUITEM_TYPE_BW;
        menu_value_min = 10; // must be power of 10
        menu_value_max = 10000;
        menu_value = __cw_offset;
        menu_step = 10;
        break;
      case 5:
        menu_value_type = MENUITEM_TYPE_INT32;
        menu_value_min = MAX_DDS_CAL_VALUE * -1; // must be power of 10
        menu_value_max = MAX_DDS_CAL_VALUE;
        menu_value = __dds_cal;
        menu_step = 10;
        menu_sel_init();
        break;
      case 6:
      case 7:
        menu_value_type = MENUITEM_TYPE_PICK;
        menu_values_no = 4;
        menu_values_len = 5;
        menu_values_domain = (char *) malloc(menu_values_no * menu_values_len);
        strcpy_P(menu_values_domain, PSTR("2 mA"));
        strcpy_P(menu_values_domain+menu_values_len, PSTR("4 mA"));
        strcpy_P(menu_values_domain+menu_values_len*2, PSTR("6 mA"));
        strcpy_P(menu_values_domain+menu_values_len*3, PSTR("8 mA"));
        if (menu_curitem == 5) {
          menu_curvalue = __dds_pwr0;
        }
        else {
          menu_curvalue = __dds_pwr2;
        }
        break;
    }
  }

  // RUNTIME MENU
  else if (state == STATE_OP_MENU) {
    switch (menu_curitem) {
    /*if (menu_curitem == 0) {
      menu_value_type = MENUITEM_TYPE_PICK;
      menu_values_no = 2;
      menu_values_len = 4;
      menu_values_domain = (char *) malloc(menu_values_no * menu_values_len);
      strcpy_P(menu_values_domain, PSTR("OFF"));
      strcpy_P(menu_values_domain+menu_values_len, PSTR("ON "));
      menu_curvalue = _sound;
    }*/
      case 0:
        menu_value_type = MENUITEM_TYPE_PICK;
        menu_values_no = 1;
        menu_values_len = 16;
        menu_values_domain = (char *) malloc(menu_values_no * menu_values_len);
        si5351_status(menu_values_domain);
        menu_curvalue = 0;
        break;
      case 1:
        menu_value_type = MENUITEM_TYPE_PICK;
        menu_values_no = 1;
        menu_values_len = 17;
        menu_values_domain = (char *) malloc(menu_values_no * menu_values_len);
        strcpy_P(menu_values_domain, FW_VERSION);
        menu_curvalue = 0;
        break;
    }

  }

  substate = SUBSTATE_VALUE_EDIT;

  if (menu_value_type == MENUITEM_TYPE_FREQ || menu_value_type == MENUITEM_TYPE_BW ||
     menu_value_type == MENUITEM_TYPE_INT32)
    lcd.cursor();

  display_menu();
}

void process_menu_item() {

  // save mod
  // CALIBRATION MENU
  if (state == STATE_CAL_MENU) {
    if (menu_curitem == 0) {
      __design = menu_curvalue;
      ee_store_design();
    }
    else if (menu_curitem == 1) {
      __if = menu_value;
      ee_store_if();
    }
    else if (menu_curitem == 2) {
      __conv = menu_curvalue;
      ee_store_conv();
    }
    else if (menu_curitem == 3) {
      __ssb_offset = menu_value;
      ee_store_ssb_offset();
    }
    else if (menu_curitem == 4) {
      __cw_offset = menu_value;
      ee_store_cw_offset();
    }
    else if (menu_curitem == 5) {
      __dds_cal = menu_value;
      ee_store_dds_cal();
    }
    else if (menu_curitem == 6) {
      __dds_pwr0 = menu_curvalue;
      ee_store_dds_pwr();
    }
    else if (menu_curitem == 7) {
      __dds_pwr2 = menu_curvalue;
      ee_store_dds_pwr();
    }
  }

  // OPERATING MENU
  else if (state == STATE_OP_MENU) {
    /*if (menu_curitem == 0) {
      _sound = menu_curvalue;
      ee_store_sound();
    }*/

  }

  done_beep();
  leave_menu_item();
}

void leave_menu_item() {

  menu_sel_end();

  // clear menu value line on display
  display_clear_line(1);
  if (menu_value_type == MENUITEM_TYPE_PICK)
    free(menu_values_domain);
  else if (menu_value_type == MENUITEM_TYPE_FREQ ||
    menu_value_type == MENUITEM_TYPE_BW || menu_value_type == MENUITEM_TYPE_INT32)
    lcd.noCursor();

  substate = SUBSTATE_ITEM_SEL;

  display_menu();

}