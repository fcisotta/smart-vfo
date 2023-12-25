/*
 * book.ino - Smart VFO
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

/* Booking logic:
La scrittura su eeprom è sempre su prenotazione, per gestire la finestra mobile di delay. Quindi sempre invocata con book.
La visualizzazione a display è su prenotazione solo quando passa da encoder, servita da interrupt. Da qui viene invocata poi 
la funzione set_ relativa, in function.ino, che si vede anche nelle chiamate dirette run_().
*/

/**************************************/
/* Booking logic init                 */
/**************************************/
void book_store_init() {

  // async operations booking variables
  trigger_set_freq = false;
  trigger_display_vfo_mem = false;
  trigger_seek_mem = DIR_NONE;
  trigger_pick_mem = false;
  trigger_display_menu = false;

  // async eeprom write booking variables
  book_store_delay_reset();
  trigger_store_freq = false;
  trigger_store_rit = false;

  trigger_tune_cal = false;

}

void book_store_delay_reset() {
  freq_store_delay_start_tm = millis(); // frequency snapshot to eeprom timestamp
  rit_store_delay_start_tm = millis();
}


/**************************************/
/* Bundled update bookings            */
/**************************************/
void book_freq_update() {
  book_set_freq();
  book_store_freq();
}

void book_rit_update() {
  book_set_freq();
  book_store_rit();
}

void flush_store_updates() {
  serve_store_freq(true);
  serve_store_rit(true);
  // serve_store_cur_mem(true);
}


/**************************************/
/* RX frequency update logic          */
/**************************************/
void book_set_freq() {
  trigger_set_freq = true;
}

void serve_set_freq() {
  if (trigger_set_freq) {
    trigger_set_freq = false;
    set_freq();
  }
}


// Eeprom store
void book_store_freq() {
  // trigger save to eeprom   // TODO: optimize
  freq_store_delay_start_tm = millis(); // delay next eeprom write
  trigger_store_freq = true;
}

void serve_store_freq(boolean force) {
  if (trigger_store_freq && (force || millis() - freq_store_delay_start_tm > EEPROM_STORE_DELAY)) {
    ee_update_cur_op_rec_rx();
    trigger_store_freq = false;    
  }
}


// Eeprom store
void book_store_rit() {
  // trigger save to eeprom   // TODO: optimize
  rit_store_delay_start_tm = millis();
  trigger_store_rit = true;
}

void serve_store_rit(boolean force) {
  if (trigger_store_rit && (force || millis() - rit_store_delay_start_tm > EEPROM_STORE_DELAY)) {
    ee_update_cur_op_rec_rit();
    trigger_store_rit = false;    
  }
}


/**************************************/
/* VFO/MEM channel update logic       */
/**************************************/
void book_display_vfo_mem() {
  trigger_display_vfo_mem = true;
}

void serve_display_vfo_mem() {
  if (trigger_display_vfo_mem) {
    trigger_display_vfo_mem = false;
    display_vfo_mem();
  }
}


/**************************************/
/* MEM rotation logic                 */
/**************************************/
void book_seek_mem(unsigned char dir) {
  trigger_seek_mem = dir;
}

void serve_seek_mem() {
  if (trigger_seek_mem != DIR_NONE) {
    seek_mem(trigger_seek_mem);
    trigger_seek_mem = DIR_NONE;
  }
}


/**************************************/
/* PICK MEM rotation logic            */
/**************************************/
void book_pick_mem() {
  trigger_pick_mem = true;
}

void serve_pick_mem() {
  if (trigger_pick_mem) {
    display_upd_cur_mem_pick();
    display_step();
    trigger_pick_mem = false;
  }
}


/**************************************/
/* MENU rotation logic                */
/**************************************/
void book_display_menu() {
  trigger_display_menu = true;
}

void serve_display_menu() {
  if (trigger_display_menu) {
    display_menu();
    trigger_display_menu = false;
  }
}


/**************************************/
/* Calibration increments logic       */
/**************************************/
void book_tune_cal() {
  trigger_tune_cal = true;
}

void serve_tune_cal() {
  if (trigger_tune_cal) {
    tune_cal_freq();
    trigger_tune_cal = false;
  }
}
