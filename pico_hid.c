#include "pico/stdlib.h"

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
          3, // Up
          2, // Right
          16, // Down
          17 // Left
        }
      }
    }
  },
  { SRC_BUTTON, { .button_src = { GAMEPAD_BUTTON_SOUTH, 7 } } },
  { SRC_BUTTON, { .button_src = { GAMEPAD_BUTTON_EAST, 4 } } },
  { SRC_BUTTON, { .button_src = { GAMEPAD_BUTTON_NORTH, 5 } } },
  { SRC_BUTTON, { .button_src = { GAMEPAD_BUTTON_WEST, 6 } } },
  { SRC_BUTTON, { .button_src = { GAMEPAD_BUTTON_SELECT, 8 } } },
  { SRC_BUTTON, { .button_src = { GAMEPAD_BUTTON_START, 9 } } },
  { SRC_BUTTON, { .button_src = { GAMEPAD_BUTTON_MODE, 10 } } },
  { SRC_ADC, { .adc_src = { ADC_LEFT_JOY_Y, 0 } } },
  { SRC_ADC, { .adc_src = { ADC_LEFT_JOY_X, 1 } } },
  { SRC_ADC, { .adc_src = { ADC_RIGHT_JOY_Y, 2 } } },
  { SRC_ADC, { .adc_src = { ADC_RIGHT_JOY_X, 3 } } }
};

const int _button_config_count = 8;

void setup_controller_buttons(void)
{
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
    else
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

// I2C address
// static const uint8_t PCF8591_ADDR = (0x90 >> 1);


void update_adc_input(hid_gamepad_report_t *report, const adc_source *data)
{
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
}