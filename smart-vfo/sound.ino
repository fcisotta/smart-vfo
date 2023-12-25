/*
 * sound.ino - Smart VFO
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

void err_beep() {
  _beep(1500, 700);
}

void done_beep() {
  _beep(500, 64);
}

void _beep(uint32_t half_period_us, uint32_t duration_ms) {
  return; /* Sound features temporarily suppressed, while lacking an output port to drive buzzer
  if (!_sound)
    return;
  uint16_t cycles = duration_ms * 1000 / (2 * half_period_us);
  byte reg2 = filt_reg >> 8;
  for(uint16_t i = 0; i < cycles; i++)
  {
    reg2 |= 0b10000000;
    Wire.beginTransmission(PCF8574_2_ADDR);
    Wire.write(reg2);
    Wire.endTransmission();
    delayMicroseconds(half_period_us);
    reg2 &= 0b01111111;
    Wire.beginTransmission(PCF8574_2_ADDR);
    Wire.write(reg2);
    Wire.endTransmission();
    delayMicroseconds(half_period_us);
  }*/
}
