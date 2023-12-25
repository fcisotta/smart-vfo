/*
 * input.ino - Smart VFO
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

/**********************************************************/
/* Read single or keypad function button, with debouncing */
/**********************************************************/
// if long pressed (>= 0,5 sec.) returns BTN_LONG_PRESSED
// if short pressed (< 0,5 sec.) returns BTN_SHORT_PRESSED
// if not pressed (or pressed < 20 ms) returns BTN_NOT_PRESSED 
byte get_button(byte btn, byte btn_logic_modifier) {
  byte rc = BTN_NOT_PRESSED;

  byte btnReading;
  if (btns[btn].pin != KEYPAD_PLACEHOLDER_PIN) {
    btnReading = digitalRead(btns[btn].pin);
  }
  else {
    // query keypad

    digitalWrite(btns[btn].keypad_pos[1], 0);
    btnReading = digitalRead(btns[btn].keypad_pos[0]);   
    digitalWrite(btns[btn].keypad_pos[1], 1);
  }

  long btnPressDuration;

  // evaluate button pressure duration
  if (btnReading == 0 && btns[btn].state == 1)  // fronte di discesa pulsante
     btns[btn].downTs = millis();

  else if (btnReading == 1 && btns[btn].state == 0)  // fronte di risalita pulsante
  {
    btnPressDuration = millis() - btns[btn].downTs;
    if (btnPressDuration > 20 && btnPressDuration < BTN_LONG_PRESS && btns[btn].repeatIter == 0) // rilevamento pressione breve del pulsante, al rilascio
      rc = BTN_SHORT_PRESSED;
    btns[btn].repeatIter = 0;
    btns[btn].longPressServed = 0;
  }

  else if (btnReading == 0 && btns[btn].state == 0)  // pulsante che rimane nello stato premuto dai cicli precedenti
  {
    btnPressDuration = millis() - btns[btn].downTs;
    if ((btn_logic_modifier & BTN_LOGIC_LONGPRESS_REPEAT == BTN_LOGIC_LONGPRESS_REPEAT || btns[btn].longPressServed == 0) && (
    (btn_logic_modifier & BTN_LOGIC_LONGPRESS_REPEAT == BTN_LOGIC_LONGPRESS_REPEAT && btns[btn].repeatIter > 1 && btnPressDuration >= BTN_REPEAT_INTERVAL) || btnPressDuration >= BTN_LONG_PRESS) ) // rilevamento pressione lunga oppure ripetizione pressione lunga successiva
    {
      rc = BTN_LONG_PRESSED;
      btns[btn].longPressServed = 1;

      if (btn_logic_modifier & BTN_LOGIC_LONGPRESS_REPEAT == BTN_LOGIC_LONGPRESS_REPEAT) {
        btns[btn].repeatIter++;
        btns[btn].downTs = millis();
      }
    }
  }
  
  btns[btn].state = btnReading; 
  return rc;

}


/*********************************/
/* Listen to keypad digit button */
/*********************************/
byte which_digit_button(byte btn_logic_modifier) {
  byte rc = 255;

  for (int i = 0; i < 10; i++) {
    if (get_button(digit_btns[i], 0) > BTN_NOT_PRESSED) {
      rc = (i+1) % 10;
    }
  }
  return rc;
}


/**************************************/
/* Read pulse button                  */
/**************************************/
byte get_pulsebutton(byte btn) {
  byte rc = PLSBTN_UNCHANGED;

  byte btnReading = digitalRead(btns[btn].pin);

  if (btnReading != btns[btn].state) {
    rc = btnReading == 1 ? PLSBTN_RELEASED : PLSBTN_PRESSED;
    btns[btn].state = btnReading;
  }

  return rc;
}


/**************************************/
/* Read momentary button              */
/**************************************/
byte get_mombutton(byte btn) {
  return digitalRead(btns[btn].pin);
}
