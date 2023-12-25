/*
 * flow.ino - Smart VFO
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

void done() {
  done_beep();
}

void error(byte error) {

  // Error conditions:
  // 0 = mixer output frequency not reachable
  // 1 = no memory stored
  // 2 = 
  if (error == ERR_DDS_FREQ_RANGE)
    display_err(F("f out DDS range"));
  else if (error == ERR_BAND_FREQ_RANGE)
    display_err(F("f out bnd range"));
  else if (error == ERR_NO_MEM_CHS)
    display_err(F("no mem ch saved"));
  else if (error == ERR_MEM_CH_UNSET)
    display_err(F("mem ch not set"));

  err_beep();
  delay(1000);
  display_init(); // it also draws frequency, steps, etc. according to initial base state
  set_freq();
}