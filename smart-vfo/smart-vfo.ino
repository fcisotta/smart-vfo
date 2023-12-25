/*
 * smart-vfo.ino - Smart VFO
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

#include "Rotary.h"
#include "si5351.h"
#include <Wire.h>
#include "LiquidCrystal_I2C.h"
#include <EEPROM.h>
#include "lcd_symbols.h"

// leave uncommented until Si5351 is connected to Arduino
//#define MOCK_Si5351

#define FW_VERSION           PSTR("v0.13 Dec 2023")

// Pinout configuration

#define DIAL_CLK              2
#define DIAL_DT               3
#define STEP_BTN_PIN          4

// Keypad buttons
#define KEYPAD_ROW1_PIN      12
#define KEYPAD_ROW2_PIN      11
#define KEYPAD_ROW3_PIN      10
#define KEYPAD_ROW4_PIN       9
#define KEYPAD_COL1_PIN       8
#define KEYPAD_COL2_PIN       7
#define KEYPAD_COL3_PIN       6
#define KEYPAD_COL4_PIN       5

#define KEYPAD_PLACEHOLDER_PIN  255

#define BAND_UP_BTN_PIN      A0
#define BAND_DN_BTN_PIN      A1
#define MODE_BTN_PIN         A2

#define OP_BTN_PIN           A3

#define SDA                  A4
#define SCL                  A5



// peripherals configuration
#define PCF8574_1_ADDR 32
#define PCF8574_2_ADDR 33


// operating states
#define STATE_VFO_DIAL     0
#define STATE_MEM_DIAL     1
#define STATE_OP_MENU      8
#define STATE_CAL_MENU     9

// operating substates
#define SUBSTATE_OP          0
#define SUBSTATE_RIT_OP      1
#define SUBSTATE_ITEM_SEL    2
#define SUBSTATE_VALUE_EDIT  3
#define SUBSTATE_PICKMEM     5
#define SUBSTATE_DGE         9

// button behaviour codes
#define BTN_NOT_PRESSED    0
#define BTN_SHORT_PRESSED  1
#define BTN_LONG_PRESSED   2
#define PLSBTN_UNCHANGED   0
#define PLSBTN_PRESSED     1
#define PLSBTN_RELEASED    2

// button long press logic
#define BTN_LOGIC_LONGPRESS_REPEAT   1
#define BTN_LONG_PRESS      1000
#define BTN_REPEAT_INTERVAL  300


// general purpose values
#define NONE    0

// RX/TX comm
#define OP_RX   0
#define OP_TX   1

// Power levels
#define DDS_POWER_2MA     0
#define DDS_POWER_4MA     1
#define DDS_POWER_6MA     2
#define DDS_POWER_8MA     3

// custom symbols
#define VFOA_SYMBOL    0
#define VFOB_SYMBOL    1
#define RGT_ARR_SYMBOL 2
#define OP_TX_SYMBOL   3
#define LOCK_SYMBOL    4
#define RIT_SYMBOL     5
#define SET_SYMBOL     6
#define DDIGIT_SYMBOL  7

// menu item types
#define MENU_CAL            0
#define MENU_OP             1
#define MENUITEM_TYPE_PICK  0
#define MENUITEM_TYPE_INT16 1
#define MENUITEM_TYPE_INT32 2
#define MENUITEM_TYPE_FREQ  3
#define MENUITEM_TYPE_BW    4

// handled error conditions
#define ERR_DDS_FREQ_RANGE   0
#define ERR_BAND_FREQ_RANGE  1
#define ERR_NO_MEM_CHS       2
#define ERR_MEM_CH_UNSET     3


// Functions state domains
#define VFO_A    0
#define VFO_B    1

#define SPLIT_OFF   0
#define SPLIT_ON    1

#define MODE_LSB 0
#define MODE_USB 1
#define MODE_AM  2
#define MODE_CW  3

const char *mode_labels[] = {"LSB", "USB", "AM ", "CW "}; // this takes 14 bytes more in SRAM


// Memory channels status
#define CH_RECORD_STATUS_UNSET  0
#define CH_RECORD_STATUS_SET    1



// *****************************************************************************************
// ********************************** working variables ************************************
// *****************************************************************************************

// calibration params
byte __design, __conv;
uint32_t __if;
uint16_t __ssb_offset, __cw_offset;
int32_t __dds_cal;
int8_t __dds_pwr0;
int8_t __dds_pwr2;
int32_t __tuning_dds_cal;

#define DESIGN_SINGLE_CONV   0
#define DESIGN_DIRECT_CONV   1
#define DESIGN_FREQX4        2

#define CONV_UPCONV          0
#define CONV_DOWNCONV        1

#define DDS_TUNING_FREQ   (1000000000ULL / SI5351_FREQ_MULT)
#define MAX_DDS_CAL_VALUE 9000

// operating params
byte cur_vfo;
byte cur_band[2];
uint16_t filt_reg;
byte split;
uint32_t step;
int32_t rit_step;
/*volatile*/ uint8_t cur_mem;
/*volatile*/ uint8_t cur_mem_pick;
int32_t _dge_buffer;
byte _dge_pos;
byte _sound;

// operating states
byte state;
byte prev_state;
byte substate; 
bool ui_lock = false; 
byte cur_op = OP_RX;

// DDS state
byte cur_dds_pwr = 255; // null value
uint32_t cur_clk0_f = 0;
uint32_t cur_clk2_f = 0;

// display content cache
byte _d_cur_mode;
byte _d_cur_rit;
byte _d_cur_op;
int32_t _d_cur_rit_v;


/****** Buttons ******/
// Button states
typedef struct {
                 byte pin; // port if stand-alone, 255 if in keypad
                 byte keypad_pos[2];
                 long downTs;
                 byte state;
                 byte repeatIter;
                 byte longPressServed;
               } btn_grid;
btn_grid btns[] = {
  {STEP_BTN_PIN,       {0, 0}, 0, 1, 0, 0}, // dial button
  {BAND_UP_BTN_PIN,    {0, 0}, 0, 1, 0, 0}, // band+ button
  {BAND_DN_BTN_PIN,    {0, 0}, 0, 1, 0, 0}, // band- button
  {MODE_BTN_PIN,       {0, 0}, 0, 1, 0, 0}, // mode button
  {OP_BTN_PIN,         {0, 0}, 0, 1, 0, 0}, // rx/tx comm line
  {KEYPAD_PLACEHOLDER_PIN, {KEYPAD_ROW1_PIN, KEYPAD_COL1_PIN}, 0, 1, 0, 0}, // keypad buttons
  {KEYPAD_PLACEHOLDER_PIN, {KEYPAD_ROW1_PIN, KEYPAD_COL2_PIN}, 0, 1, 0, 0}, //        "
  {KEYPAD_PLACEHOLDER_PIN, {KEYPAD_ROW1_PIN, KEYPAD_COL3_PIN}, 0, 1, 0, 0}, //        "
  {KEYPAD_PLACEHOLDER_PIN, {KEYPAD_ROW1_PIN, KEYPAD_COL4_PIN}, 0, 1, 0, 0}, //        "
  {KEYPAD_PLACEHOLDER_PIN, {KEYPAD_ROW2_PIN, KEYPAD_COL1_PIN}, 0, 1, 0, 0}, //        "
  {KEYPAD_PLACEHOLDER_PIN, {KEYPAD_ROW2_PIN, KEYPAD_COL2_PIN}, 0, 1, 0, 0}, //        "
  {KEYPAD_PLACEHOLDER_PIN, {KEYPAD_ROW2_PIN, KEYPAD_COL3_PIN}, 0, 1, 0, 0}, //        "
  {KEYPAD_PLACEHOLDER_PIN, {KEYPAD_ROW2_PIN, KEYPAD_COL4_PIN}, 0, 1, 0, 0}, //        "
  {KEYPAD_PLACEHOLDER_PIN, {KEYPAD_ROW3_PIN, KEYPAD_COL1_PIN}, 0, 1, 0, 0}, //        "
  {KEYPAD_PLACEHOLDER_PIN, {KEYPAD_ROW3_PIN, KEYPAD_COL2_PIN}, 0, 1, 0, 0}, //        "
  {KEYPAD_PLACEHOLDER_PIN, {KEYPAD_ROW3_PIN, KEYPAD_COL3_PIN}, 0, 1, 0, 0}, //        "
  {KEYPAD_PLACEHOLDER_PIN, {KEYPAD_ROW3_PIN, KEYPAD_COL4_PIN}, 0, 1, 0, 0}, //        "
  {KEYPAD_PLACEHOLDER_PIN, {KEYPAD_ROW4_PIN, KEYPAD_COL1_PIN}, 0, 1, 0, 0}, //        "
  {KEYPAD_PLACEHOLDER_PIN, {KEYPAD_ROW4_PIN, KEYPAD_COL2_PIN}, 0, 1, 0, 0}, //        "
  {KEYPAD_PLACEHOLDER_PIN, {KEYPAD_ROW4_PIN, KEYPAD_COL3_PIN}, 0, 1, 0, 0}, //        "
  {KEYPAD_PLACEHOLDER_PIN, {KEYPAD_ROW4_PIN, KEYPAD_COL4_PIN}, 0, 1, 0, 0}, //        "
  {KEYPAD_PLACEHOLDER_PIN, {KEYPAD_ROW2_PIN, KEYPAD_COL4_PIN}, 0, 1, 0, 0}  //        "
};

// Buttons ref position along the matrix
#define STEP_BTN          0
#define BAND_UP_BTN       1
#define BAND_DN_BTN       2
#define MODE_BTN          3
#define OP_BTN            4
#define VFO_AB_BTN        8
#define VFO_MV_BTN       12
#define MR_MIN_BTN       16
#define ESC_LOCK_BTN     17
#define ENT_MENU_BTN     19
#define RIT_SPLIT_BTN    20

byte digit_btns[] = {5, 6, 7, 9, 10, 11, 13, 14, 15, 18};


// Operations data
typedef struct {
                uint32_t rx;
                byte mode;
                int32_t rit;
              } op_record;
#define OP_RECORD_SIZE     9

typedef struct {
                byte status;
                op_record data;
              } ch_record;
#define CH_RECORD_SIZE    10

#define BANDS_NO          12
//#define VFO_BANK_SIZE    121 // = BANDS_NO * OP_RECORD_SIZE
#define VFO_BANK_SIZE     (BANDS_NO * OP_RECORD_SIZE)

#define CH_NO             40

#define CH_SEEK_CUR        0
#define CH_SEEK_PREV      -1
#define CH_SEEK_NEXT      +1


op_record op_rec;

#define MAX_RIT_VALUE      5000


// Bands hardcoded data
typedef struct {
                 //char label[5];
                 uint32_t lower_lim, upper_lim;
               } band;
band bands[] = {
  { /*"160m",*/  180000000UL / SI5351_FREQ_MULT,  186000000UL / SI5351_FREQ_MULT},
  { /*"80m", */  350000000UL / SI5351_FREQ_MULT,  380000000UL / SI5351_FREQ_MULT},
  { /*"60m", */  530000000UL / SI5351_FREQ_MULT,  538000000UL / SI5351_FREQ_MULT},
  { /*"40m", */  699500000UL / SI5351_FREQ_MULT,  725000000UL / SI5351_FREQ_MULT},
  { /*"30m", */ 1010000000UL / SI5351_FREQ_MULT, 1015000000UL / SI5351_FREQ_MULT},
  { /*"20m", */ 1399500000UL / SI5351_FREQ_MULT, 1450000000UL / SI5351_FREQ_MULT},
  { /*"17m", */ 1805000000UL / SI5351_FREQ_MULT, 1820000000UL / SI5351_FREQ_MULT},
  { /*"15m", */ 2099500000UL / SI5351_FREQ_MULT, 2150000000UL / SI5351_FREQ_MULT},
  { /*"12m", */ 2485000000UL / SI5351_FREQ_MULT, 2500000000UL / SI5351_FREQ_MULT},
  { /*"10m", */ 2800000000UL / SI5351_FREQ_MULT, 3000000000UL / SI5351_FREQ_MULT},
  { /*" 6m", */ 5000000000UL / SI5351_FREQ_MULT, 5200000000UL / SI5351_FREQ_MULT},
  { /*" 2m", */ 14400000000UL / SI5351_FREQ_MULT, 14600000000UL / SI5351_FREQ_MULT}
};


// operating param cache
int32_t _o_rit[BANDS_NO][2] = {0};

/****** eeprom management ******/
#define EEPROM_DATAMODEL_VERSION   10  // always greater than 0, to stay apart from blank eeprom

#define MEM_BOOT_ADDR          0
#define MEM_PARAMS_ADDR       10
#define MEM_CAL_PARAMS_ADDR   30
#define MEM_VFO_ADDR          60
#define MEM_CH_ADDR          280

// async eeprom persistence management
#define EEPROM_STORE_DELAY  5000

static long freq_store_delay_start_tm;
static long rit_store_delay_start_tm;
static boolean trigger_store_freq;
static boolean trigger_store_rit;


// async operations booking variables
static boolean trigger_set_freq;
static boolean trigger_display_vfo_mem;
static unsigned char trigger_seek_mem;
static boolean trigger_pick_mem;
/*volatile*/ boolean trigger_display_menu;
static boolean trigger_tune_cal;



// TODO: quanto dev'essere il BFO negli altri modi? Implementare array.


/****** menus management ******/
char **menu_labels;
byte menu_items_no;
byte *menu_items_valid;
char *menu_values_domain;  // this is allocated in menu.ino
byte menu_values_no;
byte menu_values_len;

// menu service variables
int8_t menu_curitem = 0;
int8_t menu_curvalue;
byte menu_value_type;
int32_t menu_value;
int32_t menu_value_min, menu_value_max;
int32_t menu_step;


// *****************************************************************************************
// ************************ init objects with external libraries ***************************
// *****************************************************************************************

LiquidCrystal_I2C lcd(0x27, 16, 2);

Si5351 si5351;

Rotary r = Rotary(DIAL_CLK, DIAL_DT);


// *****************************************************************************************
// ********************************** macro functions **************************************
// *****************************************************************************************
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

void(* resetFunc) (void) = 0; //declare reset function at address 0


// *****************************************************************************************
// ********************************** main init routine ************************************
// *****************************************************************************************

void setup()
{
  Serial.begin(19200);
  Wire.begin();

  lcd.init();
  lcd.backlight();

  lcd.createChar(VFOA_SYMBOL, vfoAChr);
  lcd.createChar(VFOB_SYMBOL, vfoBChr);
  lcd.createChar(RGT_ARR_SYMBOL, rgtArrChr);
  lcd.createChar(OP_TX_SYMBOL, txChr);
  lcd.createChar(LOCK_SYMBOL, lockChr);
  lcd.createChar(RIT_SYMBOL, ritChr);
  lcd.createChar(SET_SYMBOL, setChr);
  lcd.createChar(DDIGIT_SYMBOL, dDigitChr);


  pinMode(STEP_BTN_PIN, INPUT_PULLUP);
  pinMode(BAND_UP_BTN_PIN, INPUT_PULLUP);
  pinMode(BAND_DN_BTN_PIN, INPUT_PULLUP);
  pinMode(MODE_BTN_PIN, INPUT_PULLUP);
  pinMode(OP_BTN_PIN, INPUT_PULLUP);

  pinMode(KEYPAD_ROW1_PIN, INPUT_PULLUP);
  pinMode(KEYPAD_ROW2_PIN, INPUT_PULLUP);
  pinMode(KEYPAD_ROW3_PIN, INPUT_PULLUP);
  pinMode(KEYPAD_ROW4_PIN, INPUT_PULLUP);
  pinMode(KEYPAD_COL1_PIN, OUTPUT);
  pinMode(KEYPAD_COL2_PIN, OUTPUT);
  pinMode(KEYPAD_COL3_PIN, OUTPUT);
  pinMode(KEYPAD_COL4_PIN, OUTPUT);

  digitalWrite(KEYPAD_COL1_PIN, 0);
  digitalWrite(KEYPAD_COL2_PIN, 0);
  digitalWrite(KEYPAD_COL3_PIN, 0);
  digitalWrite(KEYPAD_COL4_PIN, 0);


  PCICR |= (1 << PCIE2);           // Enable pin change interrupt for the encoder
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  sei();


  // init store freq to eeprom timing
  book_store_init();


  // launch memory factory reset
  if (get_mombutton(BAND_DN_BTN) == 0) {
    btns[BAND_DN_BTN].state = 0; // set button state so that it's not detected as released at next loop cycle
    btns[BAND_DN_BTN].downTs = -BTN_LONG_PRESS;
    ee_init();
  }
  // init from memory
  else {
    ee_check();
    ee_boot_load();
  }

  ee_load_vfo_rec(cur_vfo, cur_band[cur_vfo]);
  if (state == STATE_VFO_DIAL && op_rec.rit != 0) {
    display_invalidate_rit();
    substate = SUBSTATE_RIT_OP;
  }
  else
    substate = SUBSTATE_OP;


  //initialize the Si5351
  si5351_init();

  update_filters_control(); // this sets filters both for band and mode

  // set DDS output power
  set_pwr(); // set current DDS power to relevant clocks

  // launch calibration menu
  if (get_mombutton(BAND_UP_BTN) == 0) {
    btns[BAND_UP_BTN].state = 0; // set button state so that it's not detected as released at next loop cycle
    btns[BAND_UP_BTN].downTs = -BTN_LONG_PRESS;
    toggle_menu(MENU_CAL);
  }
  else {  
    display_init(); // it also draws frequency, steps, etc. according to initial base state
    set_freq();
  }

}
