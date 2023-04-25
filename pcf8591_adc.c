#include <stdio.h>
#include "pico/stdlib.h"

#include "pcf8591_adc.h"

// I2C address
static const uint8_t PCF8591_ADDR = 0x48;

/**
 * Setup the i2c for PCF8591
 * 
 * @param i2c the i2c instance
 * @param sda_pin the pin connected to sda of pcf8591
 * @param scl_pin the pin connected to scl of pcf8591
*/
bool setup_pcf8591(i2c_inst_t *i2c, uint sda_pin, uint scl_pin)
{
  printf("Attempting to find pcf8591\r\n");

  //Initialize I2C port at 100 kHz
  i2c_init(i2c, 100 * 1000);
  printf("i2c init to 100 kHz for pcf8591\r\n");

  // Initialize I2C pins
  gpio_set_function(sda_pin, GPIO_FUNC_I2C);
  gpio_set_function(scl_pin, GPIO_FUNC_I2C);
  gpio_pull_up(sda_pin);
  gpio_pull_up(scl_pin);
  printf("gpio pins set for I2C - SDA: %d  SCL: %d\r\n", sda_pin, scl_pin);

  // Read from I2C to make sure that we can communicate with the PCF8591
  int res;
  uint8_t data[1] = {0};

  res = i2c_read_blocking(i2c, PCF8591_ADDR, data, 1, false);
  if (res < 0) {
    printf("ERROR: Could not communicate with PCF8591\r\n %hhx", data[0]);
    return false;
  }
  else {
    printf("Found PCF8591 %hhx\r\n", data[0]);
    return true;
  }
}

/**
 * Read a single value from the PCF8591 adc
 * 
 * @param i2c the pico i2c instance
 * @param adc index of the adc from the PCF8591 - being 0 to 3
*/
uint8_t read_adc(i2c_inst_t *i2c, uint8_t adc)
{
  // Make sure adc is 0 to 3
  adc = MIN(adc, 3);

  // Prepare array to accept data
  uint8_t data[2] = {0, 0};

  // printf("about to write request to read adc %hhx\r\n", adc);
  i2c_write_blocking(i2c, PCF8591_ADDR, &adc, 1, true);
  // printf("reading adc\r\n");
  int num_bytes_read = i2c_read_blocking(i2c, PCF8591_ADDR, data, 2, false);
  
  printf("read %d -> %hhx\r\n", num_bytes_read, data[1]);

  // data is 2nd byte read - first is previous value in adc
  return data[1];
}

/**
 * Use adc to read all bytes in one go
 * 
 * @param i2c the i2c instance from the pico
 * @param out the array to which the data is read (must be at least 4 bytes in size)
*/
int read_all_adc( i2c_inst_t *i2c, uint8_t *out)
{
  uint8_t data[5] = {0,0,0,0,0};
  uint8_t reg = 0x04; // auto increment - channel 0

  // printf("about to write request to read adc %hhx\r\n", adc);
  i2c_write_blocking(i2c, PCF8591_ADDR, &reg, 1, true);
  // printf("reading adc\r\n");

  // first byte is ignored, as will contain old adc value
  int num_bytes_read = i2c_read_blocking(i2c, PCF8591_ADDR, data, 5, false);
  
  // printf("read %d -> %hhx,%hhx,%hhx,%hhx\r\n", num_bytes_read, 
  //   data[1], data[2],
  //   data[3], data[4]);

  // write data to output - bytes 1 to 4 read are copied to out bytes 0 to 3
  for(int i = 0; i < MIN(4, num_bytes_read - 1); i++) out[i] = data[i+1];

  // return the number of bytes we set - should always be 4
  return num_bytes_read - 1;
}

void update_value(joystick_adc_map_t *joy, uint8_t data[])
{
  joy->raw_adc = data[joy->read_from_adc];
  joy->value = (joy->invert ? (255 - joy->raw_adc) : joy->raw_adc) - 128;

  // expand center - ensure upright sticks report mid value (0)
  if (joy->value > -8 && joy->value < 8)
  {
    joy->value = 0;
  }
}

/**
 * Read adc and output. This will read the values from the adc, map each to the required outputs
 * using the values.
 * 
 * @param idx_map an array of the indexes for the adc to read to output - 4 values required.
*/
void read_joysticks(i2c_inst_t *i2c, joystick_adc_map_t *in_out)
{
  uint8_t data[4] = {127,127,127,127};

  int bytes_read = read_all_adc(i2c, data);

  
  for(int i = 0; i < 4; i++)
  {
    // Convert to negative left/up value
    update_value(&in_out[i], data);
  }
}
