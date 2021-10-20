#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdio.h>

/* enable backwards compatibility so we can use _delay_us function pointer */
#define __DELAY_BACKWARD_COMPATIBLE__
#include <util/delay.h>

#include <i2cmaster.h>
#include <bme280.h>

#define BAUD 57600
#include <util/setbaud.h>

void uart_init(void) {
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;

    UCSR0A |= _BV(U2X0); /* single speed */

    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); /* 8-bit data */
    UCSR0B = _BV(RXEN0) | _BV(TXEN0);   /* Enable RX and TX */
}

int uart_putchar(char c, FILE *stream)
{
    if (c == '\n')
        uart_putchar('\r', stream);
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = c;

    return 0;
}

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

static struct bme280_dev bme;
static uint8_t bme280_i2c_addr;

void bme280_setup(void)
{
    int8_t rslt = BME280_OK;

    bme280_i2c_addr = BME280_I2C_ADDR_SEC;
    bme.intf = BME280_I2C_INTF;
    bme.intf_ptr = &bme280_i2c_addr;
    bme.read = user_i2c_read;
    bme.write = user_i2c_write;
    bme.delay_us = user_delay_us;

    if (bme280_init(&bme) != BME280_OK) {
        printf("bme: no sensor found\n");
        return;
    }

    bme.settings.osr_h = BME280_OVERSAMPLING_1X;
    bme.settings.osr_p = BME280_OVERSAMPLING_1X;
    bme.settings.osr_t = BME280_OVERSAMPLING_1X;
    bme.settings.filter = BME280_FILTER_COEFF_OFF;

    int settings_sel = BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL |
        BME280_OSR_HUM_SEL | BME280_FILTER_SEL;

    rslt = bme280_set_sensor_settings(settings_sel, &bme);
    if (rslt != BME280_OK) {
        printf("bme: set sensor settings failed, (code %d).\n", rslt);
        return;
    }

    rslt = bme280_set_sensor_mode(BME280_FORCED_MODE, &bme);
    if (rslt != BME280_OK){
        printf("bme: set forced mode failed, (code %d).\n", rslt);
        return;
    }
}

int main (void)
{
    uart_init();
    FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_RW);
    stdout = stdin = &uart_output;

    i2c_init();
    bme280_setup();

    for (;;) {
        uint32_t req_delay;
        int8_t rslt = BME280_OK;

        rslt = bme280_set_sensor_mode(BME280_FORCED_MODE, &bme);
        if (rslt != BME280_OK){
            printf("bme: set forced mode failed, (code %d).\n", rslt);
            continue;
        }

        req_delay = bme280_cal_meas_delay(&bme.settings);
        bme.delay_us(req_delay, bme.intf_ptr);

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
