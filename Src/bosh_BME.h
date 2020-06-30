#ifndef BOSH_BME_GUARD
#define BOSH_BME_GUARD
#include "bosh_BME280/bme280.h"
#include "Driver_I2C.h"

typedef struct applicaiton_state {
  double humidity;
  double pressure;
  double temp;
} application_state;

void init_BME(ARM_DRIVER_I2C* i2c);
void run_BME(application_state* state);
void BME_i2c_event_register(uint32_t event);
void BME_set_enable();

#endif // BOSH_BME_GUARD