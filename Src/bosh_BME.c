#include "bosh_BME.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <string.h>


#include "Driver_I2C.h"

// declaration of static methods
static int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data,
                            uint16_t len);
static int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr,
                             uint8_t *reg_data, uint16_t len);
static void user_delay_ms(uint32_t period);

static void wait_for_transmision();

// declaration of static members

static volatile uint32_t communication_event = 0;
static ARM_DRIVER_I2C *communication;
static struct bme280_dev dev;
static volatile bool BME280_enable = true;

// implementation of public interface

void init_BME(ARM_DRIVER_I2C *i2c) {


  communication = i2c;

  dev.dev_id = BME280_I2C_ADDR_PRIM;
  dev.intf = BME280_I2C_INTF;
  dev.read = user_i2c_read;
  dev.write = user_i2c_write;
  dev.delay_ms = user_delay_ms;

  /* Recommended mode of operation: Indoor navigation */
  dev.settings.osr_h = BME280_OVERSAMPLING_1X;
  dev.settings.osr_p = BME280_OVERSAMPLING_16X;
  dev.settings.osr_t = BME280_OVERSAMPLING_2X;
  dev.settings.filter = BME280_FILTER_COEFF_16;

  bme280_init(&dev);
}

void run_BME(application_state *state) {
  if (BME280_enable) {
    BME280_enable = false;

    int8_t rslt;
    uint8_t settings_sel;
    struct bme280_data comp_data;
    uint32_t req_delay;

    settings_sel = BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL |
                   BME280_OSR_HUM_SEL | BME280_FILTER_SEL;

    rslt = bme280_set_sensor_settings(settings_sel, &dev);
    rslt = bme280_set_sensor_mode(BME280_FORCED_MODE, &dev);
    req_delay = bme280_cal_meas_delay(&dev.settings);

    dev.delay_ms(req_delay);
    rslt = bme280_get_sensor_data(BME280_ALL, &comp_data, &dev);
    state->humidity = comp_data.humidity;
    state->pressure = comp_data.pressure;
    state->temp = comp_data.temperature;
    if (rslt != 0) {
      // TODO Error handling
    }
  }
}

void BME_set_enable() { BME280_enable = true; }

// implementation of private methods

static void user_delay_ms(uint32_t period) { HAL_Delay(period); }

static int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data,
                            uint16_t len) {
  int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */

  /*
   * The parameter dev_id can be used as a variable to store the I2C address of
   * the device
   */

  /*
   * Data on the bus should be like
   * |------------+---------------------|
   * | I2C action | Data                |
   * |------------+---------------------|
   * | Start      | -                   |
   * | Write      | (reg_addr)          |
   * | Stop       | -                   |
   * | Start      | -                   |
   * | Read       | (reg_data[0])       |
   * | Read       | (....)              |
   * | Read       | (reg_data[len - 1]) |
   * | Stop       | -                   |
   * |------------+---------------------|
   */
  communication->MasterTransmit(dev_id, &reg_addr, 1, false);
  wait_for_transmision();
  communication->MasterReceive(dev_id, reg_data, len, false);
  wait_for_transmision();

  return rslt;
}
static int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr,
                             uint8_t *reg_data, uint16_t len) {

  int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */

  /*
   * The parameter dev_id can be used as a variable to store the I2C address of
   * the device
   */

  /*
   * Data on the bus should be like
   * |------------+---------------------|
   * | I2C action | Data                |
   * |------------+---------------------|
   * | Start      | -                   |
   * | Write      | (reg_addr)          |
   * | Write      | (reg_data[0])       |
   * | Write      | (....)              |
   * | Write      | (reg_data[len - 1]) |
   * | Stop       | -                   |
   * |------------+---------------------|
   */

  uint8_t buff[64];
  buff[0] = reg_addr;
  memcpy(buff + 1, reg_data, len);
  communication->MasterTransmit(dev_id, buff, len + 1, false);
  wait_for_transmision();
  return rslt;
}

void BME_i2c_event_register(uint32_t event) { communication_event = event; }
static void wait_for_transmision() {
  while ((communication_event & ARM_I2C_EVENT_TRANSFER_DONE) == 0U)
    ;
  communication_event = 0;
}