#include <stdint.h>
#include "pulp.h"

#define LIS3DHTR_ADDR 0x1D   // config for SD0 connected to VCC (00011101)

#define REG_WHO_AM_I   0x0F
#define REG_CTRL_REG4  0x20
#define REG_CTRL_REG5  0x24
#define REG_CTRL_REG6  0x25
#define REG_STATUS     0x27
#define REG_OUT_X_L    0x28

#define WHO_AM_I_VALUE 0x3F

// CTRL_REG4: ODR=100Hz (0110), BDU=1, ZEN=1, YEN=1, XEN=1 → 0x67
#define CTRL_REG4_VAL 0x67 
// CTRL_REG5: BW=800Hz (00), FS=±2g (000), ST=off (00), SPI=4wire (0) → 0x00
#define CTRL_REG5_VAL 0x00
// CTRL_REG6: ADD_INC=1 → 0x10, else 0x0
#define CTRL_REG6_VAL 0x10

#define SENSITIVITY_2G 0.06f

typedef struct {
    float x_g;
    float y_g;
    float z_g;
} accel_data_t;

static int lis3dhtr_write_reg(i2c_t *i2c, uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = { reg, value };
    return i2c_write(i2c, buf, 2, 1);
}

static int lis3dhtr_read_reg(i2c_t *i2c, uint8_t reg, uint8_t *out, int len)
{
    int ret = i2c_write(i2c, &reg, 1, 0);
    if (ret != 0) return ret;

    int bytes = i2c_read(i2c, out, len, 0);
    return (bytes == len) ? 0 : -1;
}

int lis3dhtr_init(i2c_t *i2c)
{
    uint8_t who_am_i = 0;

    if (lis3dhtr_read_reg(i2c, REG_WHO_AM_I, &who_am_i, 1) != 0) {
        return -1; // comm failed
    }
    if (who_am_i != WHO_AM_I_VALUE) {
        return -2; // unknown device
    }

    if (lis3dhtr_write_reg(i2c, REG_CTRL_REG6, CTRL_REG6_VAL) != 0) return -3;
    if (lis3dhtr_write_reg(i2c, REG_CTRL_REG5, CTRL_REG5_VAL) != 0) return -4;
    if (lis3dhtr_write_reg(i2c, REG_CTRL_REG4, CTRL_REG4_VAL) != 0) return -5;

    return 0;
}

int lis3dhtr_read_accel(i2c_t *i2c, accel_data_t *out)
{
    uint8_t status = 0;
    
    do {
        if (lis3dhtr_read_reg(i2c, REG_STATUS, &status, 1) != 0) {
            return -1;
        }
    } while (!(status & 0x08)); // wait for data availability

    uint8_t raw[6] = {0};
    if (lis3dhtr_read_reg(i2c, REG_OUT_X_L, raw, 6) != 0) {
        return -2;
    }

    // combine bytes with 2s complement
    int16_t x_raw = (int16_t)((raw[1] << 8) | raw[0]);
    int16_t y_raw = (int16_t)((raw[3] << 8) | raw[2]);
    int16_t z_raw = (int16_t)((raw[5] << 8) | raw[4]);

    // convert to g's
    out->x_g = x_raw * SENSITIVITY_2G / 1000.0f;
    out->y_g = y_raw * SENSITIVITY_2G / 1000.0f;
    out->z_g = z_raw * SENSITIVITY_2G / 1000.0f;

    return 0;
}

int main(void)
{
    // Setup device
    i2c_dev_t dev;
    i2c_dev_init(&dev);
    dev.id           = 0;                    // I2C peripheral ke-0
    dev.cs           = LIS3DHTR_ADDR << 1;   // Alamat 7-bit di-shift untuk UDMA
    dev.max_baudrate = 400000;               // 400 kHz (fast mode)

    // open I2C peripheral
    i2c_t *i2c = i2c_open(&dev);
    if (i2c == NULL) {
        return -1;
    }

    i2c_settimeout(10000, true);

    // sensor init
    int ret = lis3dhtr_init(i2c);
    if (ret != 0) {
        i2c_close(i2c);
        return ret;
    }

    // read accelerometer
    accel_data_t accel;
    while (1) {
        ret = lis3dhtr_read_accel(i2c, &accel);
        if (ret == 0) {
            printf("X=%.3fg Y=%.3fg Z=%.3fg\n", accel.x_g, accel.y_g, accel.z_g);
        }
    }

    i2c_close(i2c);
    return 0;
}
void pe_start(void)
	{
	}