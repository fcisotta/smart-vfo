
# Version history

### 0.13 (2023-12-08)
- Consistency check among menu params (only applicable are shown)

### 0.12 (2023-12-03)
- Set RTX design as a calibration param

### 0.11.1 (2023-11-02)
- Fixed timing for Si5351 init

### 0.11.0 (2023-11-01)
- Patched Si5351 Etherkit library for PLL-A stability issue
- Runtime Si5351 calibration, in calibration menu.

### 0.10.0 (2023-10-28)
- Temporary fix for a Si5351 PLL-A stability issue on leaving the 2m band
- Si5351 correction factor editable in calibration menu (tunable through an external procedure pointed out in README)
- Si5351 status info shown into operating menu
- Memory optimization

### 0.9.6 (2023-09-23)
- Extended direct frequency entry feature to the 2m band
- Removed sound option from the operating menu (feature excluded from v0.9.5)

### 0.9.5 (2023-09-23)
- New 2m band (shifted band-switch lines on PCF8574-2, temporarily leaving out buzzer drive)

### 0.9.4 (2023-07-20)
- Persistence of RIT offset over disabling/reenabling
- Tuned startup steps values
- Fixed RIT step excursion

### 0.9.3 (2023-07-08)
- Fixed RIT excursion (previously only rotating upward on certain bands)

### 0.9.2 (2023-07-07)
- Fixed IF offsets (previously not adding into clock signals)

### 0.9.1 (2023-06-11)
*[First release of 2nd gen version (introducing keyboard and memory channels dialing)]*

### 0.1 (2021-11-23)
*[Last release of 1st gen version]*

