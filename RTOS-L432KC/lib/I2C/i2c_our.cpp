#include "i2c_our.h"
#include "mbed.h"


    

void i2c_init_our()
{
    I2C i2c(I2C_SDA, I2C_SCL);
    i2c.frequency(400000);
}
