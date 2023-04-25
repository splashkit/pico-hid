#include "hardware/i2c.h"

typedef struct joystick_adc_map_st
{
  uint8_t read_from_adc; // 0..3
  bool invert; // default is left/up negative. So low will map to left. Invert will flip adc value, so high is left.
  uint8_t raw_adc;
  int8_t value;
} joystick_adc_map_t;

/**
 * Setup the i2c for PCF8591
 * 
 * @param i2c the i2c instance
 * @param sda_pin the pin connected to sda of pcf8591
 * @param scl_pin the pin connected to scl of pcf8591
*/
bool setup_pcf8591(i2c_inst_t *i2c, uint sda_pin, uint scl_pin);

/**
 * Read a single value from the PCF8591 adc
 * 
 * @param i2c the pico i2c instance
 * @param adc index of the adc from the PCF8591 - being 0 to 3
*/
uint8_t read_adc(i2c_inst_t *i2c, uint8_t adc);

/**
 * Use adc to read all bytes in one go
 * 
 * @param i2c the i2c instance from the pico
 * @param out the array to which the data is read (must be at least 4 bytes in size)
*/
int read_all_adc( i2c_inst_t *i2c, uint8_t *out);


/**
 * Read adc and output. This will read the values from the adc, map each to the required outputs
 * using the values.
 * 
 * @param idx_map an array of the indexes for the adc to read to output - 4 values required.
*/
void read_joysticks(i2c_inst_t *i2c, joystick_adc_map_t *in_out);