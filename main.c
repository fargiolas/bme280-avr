/*
 * Copyright (c) 2021 Filippo Argiolas.
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdio.h>

/* enable backwards compatibility so we can use _delay_us function pointer */
#define __DELAY_BACKWARD_COMPATIBLE__
#include <util/delay.h>

#include <uart.h>
#include <i2cmaster.h>
#include <bme280.h>

/* Bosch BME280 AVR "port": it just needs a couple of
 * platform-specific functions for i2c read and write and delays and
 * it works out of the box */
int8_t user_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    uint8_t dev_id = *((uint8_t *) intf_ptr);

    i2c_start(dev_id << 1 | I2C_WRITE);
    i2c_write(reg_addr);
    i2c_stop();

    i2c_start(dev_id << 1 | I2C_READ);

    while (len--) {
        *reg_data++ = i2c_read(len != 0);
    }

    i2c_stop();
    return 0;
}

int8_t user_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    uint8_t dev_id = *((uint8_t *) intf_ptr);

    i2c_start(dev_id << 1 | I2C_WRITE);
    i2c_write(reg_addr);

    while (len--) {
        i2c_write(*reg_data++);
    }

    i2c_stop();
    return 0;
}

void user_delay_us(uint32_t period, void *intf_ptr)
{
    _delay_us(period);
}


/* that's it, now we can just use the library straight from bosch examples */
static struct bme280_dev bme;
static uint8_t bme280_i2c_addr;

void bme280_setup(void)
{
    int8_t rslt = BME280_OK;

    bme280_i2c_addr = BME280_I2C_ADDR_SEC; /* Adafruit BME280 uses secondary address 0x77 */
    bme.intf = BME280_I2C_INTF;
    bme.intf_ptr = &bme280_i2c_addr;
    /* map r/w and delay to platform functions */
    bme.read = user_i2c_read;
    bme.write = user_i2c_write;
    bme.delay_us = user_delay_us;

    rslt = bme280_init(&bme);
    if (rslt != BME280_OK) {
        printf("Failed to initialize the device (code %+d).\n", rslt);
        return;
    }

    /* no filtering or oversampling */
    bme.settings.osr_h = BME280_OVERSAMPLING_1X;
    bme.settings.osr_p = BME280_OVERSAMPLING_1X;
    bme.settings.osr_t = BME280_OVERSAMPLING_1X;
    bme.settings.filter = BME280_FILTER_COEFF_OFF;

    int settings_sel = BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL |
        BME280_OSR_HUM_SEL | BME280_FILTER_SEL;

    rslt = bme280_set_sensor_settings(settings_sel, &bme);
    if (rslt != BME280_OK) {
        printf("Set sensor settings failed, (code %d).\n", rslt);
        return;
    }

    rslt = bme280_set_sensor_mode(BME280_FORCED_MODE, &bme);
    if (rslt != BME280_OK){
        printf("Set forced mode failed, (code %d).\n", rslt);
        return;
    }
}

int main (void)
{
    uart_init();
    /* map std streams to uart so we can use printf */
    FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_RW);
    stdout = stdin = &uart_output;

    i2c_init();
    bme280_setup();

    /* calculate minimum delay from subsequent measurements given the current settings */
    uint32_t req_delay = bme280_cal_meas_delay(&bme.settings);

    for (;;) {
        int8_t rslt = BME280_OK;

        rslt = bme280_set_sensor_mode(BME280_FORCED_MODE, &bme);
        if (rslt != BME280_OK){
            printf("Set forced mode failed, (code %d).\n", rslt);
            continue;
        }


        /* wait for measurement to complete */
        bme.delay_us(req_delay, bme.intf_ptr);

        /* gather and print data */
        struct bme280_data comp_data;

        rslt = bme280_get_sensor_data(BME280_ALL, &comp_data, &bme);
        if (rslt != BME280_OK) {
            printf("Failed to get sensor data (code %d).\n", rslt);
            continue;
        }

        printf("%f,%f,%f\n", comp_data.temperature, comp_data.humidity, comp_data.pressure/100.);

        /* req_delay should in principle be enough but something is
         * not working properly and the device gets stuck returning
         * the same value if we don't give it a couple of us
         * more... probably I'm missing something */
        _delay_ms(1);

    }

    return 0;
}
