#include "pico/stdlib.h"

#include "pcf8591_adc.h"
#include "pico_hid.h"

typedef enum {
  DPAD_UP_ID = 0,
  DPAD_RIGHT_ID,
  DPAD_DOWN_ID,
  DPAD_LEFT_ID
} dpad_index;

typedef enum {
  ADC_LEFT_JOY_X = 0,
  ADC_LEFT_JOY_Y,
  ADC_RIGHT_JOY_X,
  ADC_RIGHT_JOY_Y
} joy_direction;

typedef enum {
  SRC_DPAD = 0,
  SRC_BUTTON,
  SRC_ADC
} button_source_kind;

typedef struct button_source {
  uint32_t action;
  uint8_t gpio_pin;
} button_source;

typedef struct dpad_source {
  uint8_t gpio_pin[4]; // Up, Right, Down, Left
} dpad_source;

typedef struct adc_source {
  joy_direction direction;
  uint8_t adc_channel;
} adc_source;

typedef union input_source {
  adc_source adc_src;
  button_source button_src;
  dpad_source dpad_src;
} input_source;

typedef struct button_data {
  button_source_kind source;
  input_source data;
} button_data;

/**
 * The config for the buttons.
*/
const button_data _button_config[] = {
  {
    SRC_DPAD, {
      .dpad_src = {
        {
          18, // Up
          19, // Right
          17, // Down
          16 // Left
        }
      }
    }
  },
  { SRC_BUTTON, { .button_src = { GAMEPAD_BUTTON_SOUTH, 7 } } },
  { SRC_BUTTON, { .button_src = { GAMEPAD_BUTTON_EAST, 8 } } },
  { SRC_BUTTON, { .button_src = { GAMEPAD_BUTTON_NORTH, 5 } } },
  { SRC_BUTTON, { .button_src = { GAMEPAD_BUTTON_WEST, 6 } } },
  { SRC_BUTTON, { .button_src = { GAMEPAD_BUTTON_MODE, 9 } } },
  { SRC_BUTTON, { .button_src = { GAMEPAD_BUTTON_SELECT, 22 } } },
  { SRC_BUTTON, { .button_src = { GAMEPAD_BUTTON_START, 26 } } }
};

/**
 * Mapping for ADC values - lx, ly, rx, ry
*/
static joystick_adc_map_t joy_map[] = {
  {
    .read_from_adc = 2, // lx
    .invert = false,
    .raw_adc = 0,
    .value = 0
  },
  {
    .read_from_adc = 3, // ly
    .invert = true,
    .raw_adc = 0,
    .value = 0
  },
  {
    .read_from_adc = 1, // rx
    .invert = false,
    .raw_adc = 0,
    .value = 0
  },
  {
    .read_from_adc = 0, // ry
    .invert = false,
    .raw_adc = 0,
    .value = 0
  }
};

const int _button_config_count = 8;

// Pins
const uint sda_pin = 20;
const uint scl_pin = 21;

void setup_controller_buttons(void)
{
  printf("Finding pcf8591\r\n");
  // Find pcf8591
  bool found = setup_pcf8591(i2c0, sda_pin, scl_pin);
  int tries = 0;

  while (!found && tries++ < 10)
  {
    printf("Retry\r\n");
    sleep_ms(100);
    found = setup_pcf8591(i2c0, sda_pin, scl_pin);
  }
  printf("Found: %d\r\n", found);
  
  for (int i = 0; i < _button_config_count; i++ )
  {
    if ( _button_config[i].source == SRC_DPAD )
    {
      for( int j = 0; j < 4; j++ )
      {
        gpio_init(_button_config[i].data.dpad_src.gpio_pin[j]);
        gpio_set_dir(_button_config[i].data.dpad_src.gpio_pin[j], GPIO_IN);
        gpio_pull_up(_button_config[i].data.dpad_src.gpio_pin[j]);
      }
    }
    else if (_button_config[i].source == SRC_BUTTON)
    {
      gpio_init(_button_config[i].data.button_src.gpio_pin);
      gpio_set_dir(_button_config[i].data.button_src.gpio_pin, GPIO_IN);
      gpio_pull_up(_button_config[i].data.button_src.gpio_pin);
    }
  }
}

bool is_empty(const hid_gamepad_report_t *report)
{
  return 0 == report->buttons + report->hat + report->x + report->y + report->z + report->rx + report->ry + report->rz;
}

void update_dpad(hid_gamepad_report_t *report, const dpad_source *data)
{
  uint8_t result = GAMEPAD_HAT_CENTERED;

  if ( !gpio_get(data->gpio_pin[DPAD_UP_ID]) )
  {
    if ( !gpio_get(data->gpio_pin[DPAD_LEFT_ID])) result = GAMEPAD_HAT_UP_LEFT;
    else if ( !gpio_get(data->gpio_pin[DPAD_RIGHT_ID])) result = GAMEPAD_HAT_UP_RIGHT;
    else result = GAMEPAD_HAT_UP;
  }
  else if ( !gpio_get(data->gpio_pin[DPAD_DOWN_ID]) )
  {
    if ( !gpio_get(data->gpio_pin[DPAD_LEFT_ID])) result = GAMEPAD_HAT_DOWN_LEFT;
    else if ( !gpio_get(data->gpio_pin[DPAD_RIGHT_ID])) result = GAMEPAD_HAT_DOWN_RIGHT;
    else result = GAMEPAD_HAT_DOWN;
  }
  else if ( !gpio_get(data->gpio_pin[DPAD_LEFT_ID])) result = GAMEPAD_HAT_LEFT;
  else if ( !gpio_get(data->gpio_pin[DPAD_RIGHT_ID])) result = GAMEPAD_HAT_RIGHT;

  report->hat = result;
}

void update_button(hid_gamepad_report_t *report, const button_source *data)
{
  if (!gpio_get(data->gpio_pin)) // pulled down
  {
    report->buttons |= data->action;
  }
}

/**
 * Setup the list of buttons to be read, and what they map to.
*/
void update_hid_report_controller(hid_gamepad_report_t *report)
{
  for (int i = 0; i < _button_config_count; i++ )
  {
    if (_button_config[i].source == SRC_DPAD)
    {
      update_dpad(report, &_button_config[i].data.dpad_src);
    }
    else if (_button_config[i].source == SRC_BUTTON)
    {
      update_button(report, &_button_config[i].data.button_src);
    }
  }

  // Read all ADC values into joy_map
  read_joysticks(i2c0, joy_map);

  report->x = joy_map[0].value;
  report->y = joy_map[1].value;
  report->rx = joy_map[2].value;
  report->ry = joy_map[3].value;
}