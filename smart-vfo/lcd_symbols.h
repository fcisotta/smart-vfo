/*
 * lcd_symbols.ino - Smart VFO
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

byte vfoAChr[8] = {
	0b10100,
	0b10100,
	0b01000,
	0b00110,
	0b01001,
	0b01001,
	0b01111,
	0b01001
};

byte vfoBChr[8] = {
	0b10100,
	0b10100,
	0b01000,
	0b01110,
	0b01001,
	0b01110,
	0b01001,
	0b01110
};

byte rgtArrChr[8] = {
	0b00000,
	0b00000,
	0b00100,
	0b00010,
	0b11111,
	0b00010,
	0b00100,
	0b00000
};

byte txChr[8] = {
	0b00000,
	0b11111,
	0b00100,
	0b00100,
	0b00100,
	0b00101,
	0b00010,
	0b00101
};

byte lockChr[8] = {
  0b01110,
  0b10001,
  0b11111,
  0b10001,
  0b10101,
  0b10001,
  0b11111,
  0b00000
};

byte ritChr[8] = {
	0b01000,
	0b11111,
	0b00000,
	0b11111,
	0b00000,
	0b11101,
	0b10110,
	0b11111
};

byte setChr[8] = {
	0b00000,
	0b01110,
	0b11111,
	0b11111,
	0b11111,
	0b01110,
	0b00000,
	0b00000
};

// double digit: 14 (for 2m band)
byte dDigitChr[] = {
  0b10001,
  0b10011,
  0b10101,
  0b10101,
  0b10111,
  0b10001,
  0b10001,
  0b00000
};