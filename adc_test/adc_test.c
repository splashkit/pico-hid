#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "../pcf8591_adc.h"

// // I2C address
// static const uint8_t PCF8591_ADDR = 0x48;

// uint8_t read_adc( i2c_inst_t *i2c,
//               uint8_t adc)
// {
//   // adc = min(adc, 3);
//   uint8_t data[2] = {0, 0};

// //   printf("about to write request to read adc %hhx\r\n", adc);
//   i2c_write_blocking(i2c, PCF8591_ADDR, &adc, 1, true);
// //   printf("reading adc\r\n");
//   int num_bytes_read = i2c_read_blocking(i2c, PCF8591_ADDR, data, 2, false);

//   printf("read %d -> %hhx\r\n", num_bytes_read, data[1]);

// //   int read = reg_read(i2c, PCF8591_ADDR, adc, &data, false);

//   return data[1];
// }

// int read_all_adc( i2c_inst_t *i2c,
//               uint8_t *out)
// {
//   // adc = min(adc, 3);
//   uint8_t data[5] = {0,0,0,0,0};
//   uint8_t reg = 0x04; // auto increment - channel 0

// //   printf("about to write request to read adc %hhx\r\n", adc);
//   i2c_write_blocking(i2c, PCF8591_ADDR, &reg, 1, true);
// //   printf("reading adc\r\n");

//   int num_bytes_read = i2c_read_blocking(i2c, PCF8591_ADDR, data, 5, false);

//   printf("read %d -> %hhx,%hhx,%hhx,%hhx\r\n", num_bytes_read,
//     data[1], data[2],
//     data[3], data[4]);

//     for(int i = 0; i < 4; i++) out[i] = data[i+1];

// //   int read = reg_read(i2c, PCF8591_ADDR, adc, &data, false);

//   return num_bytes_read - 1;
// }

/*******************************************************************************
 * Main
 */
int main()
{
  // Initialize chosen serial port
  stdio_init_all();

  printf("Started - PCF8591 test\r\n");

  for(int i = 0; i < 50; i++)
  {
    printf(".");
    sleep_ms(100);
  }
  printf("\r\n");

  // Pins
  const uint sda_pin = 20;
  const uint scl_pin = 21;

  // I2C
  i2c_inst_t *i2c = i2c0;

  // Find pcf8591
  bool found = setup_pcf8591(i2c, sda_pin, scl_pin);

  while (!found)
  {
    sleep_ms(1000);
    found = setup_pcf8591(i2c, sda_pin, scl_pin);
  }

  // Buffer to store raw reads
  uint8_t data[4];

  joystick_adc_map_t joy_map[] = {
    {
      .read_from_adc = 2,
      .invert = false,
      .raw_adc = 0,
      .value = 0
    },
    {
      .read_from_adc = 3,
      .invert = true,
      .raw_adc = 0,
      .value = 0
    },
    {
      .read_from_adc = 1,
      .invert = false,
      .raw_adc = 0,
      .value = 0
    },
    {
      .read_from_adc = 0,
      .invert = false,
      .raw_adc = 0,
      .value = 0
    }
  };

  // Loop forever
  while (true)
  {
    printf("Reading...\r\n");

    // Read 4 adc values individually
    uint8_t a0 = read_adc(i2c, 0);
    uint8_t a1 = read_adc(i2c, 1);
    uint8_t a2 = read_adc(i2c, 2);
    uint8_t a3 = read_adc(i2c, 3);

    // Read all at once
    uint8_t num_read = read_all_adc(i2c, data);

    // Read joysticks
    read_joysticks(i2c, joy_map);

    // Print results
    // wiring = LX = a2, LY = a3, Rx = a1, RY = a0
    printf("LX: %hhx | LY: %hhx | RX: %hhx | RY: %hhx\r\n", a2, a3, a1, a0);
    printf("LX: %hhx | LY: %hhx | RX: %hhx | RY: %hhx\r\n", data[2], data[3], data[1], data[0]);
    printf("LX: %d | LY: %d | RX: %d | RY: %d\r\n", joy_map[0].value, joy_map[1].value, joy_map[2].value, joy_map[3].value);

    sleep_ms(1000);
  }
}