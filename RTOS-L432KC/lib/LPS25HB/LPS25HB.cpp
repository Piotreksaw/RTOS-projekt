#include "LPS25HB.h"
#include "mbed.h"

/*  SAMPLE CODE

#include "mbed.h"
#include "LPS25HB.H"

I2C i2c(I2C_SDA, I2C_SCL);
LPS25HB ps(i2c);

int main(){
    ThisThread::sleep_for(100ms);
    if (!ps.init()){
        printf("Failed to autodetect pressure sensor!\r\n");
        while (1);
    }
    ps.enableDefault();
    
    while(1){
        float pressure = ps.readPressureMillibars();
        float altitude = ps.pressureToAltitudeMeters(pressure);
        float temperature = ps.readTemperatureC();
  
        // printf("p:%.2f\t mbar\ta:%.2f m\tt:%.2f deg C\r\n",pressure,altitude,temperature);
        printf("pressure raw: %f\r\n", pressure);
        printf("altitude raw: %f\r\n", altitude);
        printf("temperature raw: %f\r\n", temperature);

        printf("test");

        ThisThread::sleep_for(5000ms);
    }
    
}

*/


// Defines ///////////////////////////////////////////////////////////

// The Arduino two-wire interface uses a 7-bit number for the address,
#define SA0_LOW_ADDRESS  0b1011100
#define SA0_HIGH_ADDRESS 0b1011101

#define TEST_REG_NACK -1

#define LPS331AP_WHO_ID 0xBB
#define LPS25H_WHO_ID   0xBD

// Constructors //////////////////////////////////////////////////////

LPS25HB::LPS25HB(I2C& p_i2c):_i2c(p_i2c)
{
    _device = device_auto;
  
    // Pololu board pulls SA0 high, so default assumption is that it is
    // high
    address = SA0_HIGH_ADDRESS;
}

// Public Methods ////////////////////////////////////////////////////

// sets or detects device type and slave address; returns bool indicating success
bool LPS25HB::init(deviceType device, uint8_t sa0)
{
    if (!detectDeviceAndAddress(device, (sa0State)sa0))
        return false;
    
    switch (_device)
    {
        case device_25H:
        translated_regs[-INTERRUPT_CFG] = LPS25H_INTERRUPT_CFG;
        translated_regs[-INT_SOURCE]    = LPS25H_INT_SOURCE;
        translated_regs[-THS_P_L]       = LPS25H_THS_P_L;
        translated_regs[-THS_P_H]       = LPS25H_THS_P_H;
        return true;
        break;
      
        case device_331AP:
        translated_regs[-INTERRUPT_CFG] = LPS331AP_INTERRUPT_CFG;
        translated_regs[-INT_SOURCE]    = LPS331AP_INT_SOURCE;
        translated_regs[-THS_P_L]       = LPS331AP_THS_P_L;
        translated_regs[-THS_P_H]       = LPS331AP_THS_P_H;
        return true;
        break;
    }
    return true;
}

// turns on sensor and enables continuous output
void LPS25HB::enableDefault(void)
{
    if (_device == device_25H)
    {
        // 0xB0 = 0b10110000
        // PD = 1 (active mode);  ODR = 011 (12.5 Hz pressure & temperature output data rate)
        writeReg(CTRL_REG1, 0xB0);
    }
    else if (_device == device_331AP)
    {
        // 0xE0 = 0b11100000
        // PD = 1 (active mode);  ODR = 110 (12.5 Hz pressure & temperature output data rate)
        writeReg(CTRL_REG1, 0xE0);
    }
}

// writes register
void LPS25HB::writeReg(char reg, char value)
{
    // if dummy register address, look up actual translated address (based on device type)
    if (reg < 0)
    {
        reg = translated_regs[-reg];
    }
    char command[] = {reg,value};
    _i2c.write(address << 1,command,2);
}

// reads register
int8_t LPS25HB::readReg(char reg)
{
    char value;
    // if dummy register address, look up actual translated address (based on device type)
    if(reg < 0)
    {
        reg = translated_regs[-reg];
    }
    _i2c.write(address << 1,&reg,1);
    _i2c.read((address << 1)|1,&value,1);
  
    return value;
}

// reads pressure in millibars (mbar)/hectopascals (hPa)
float LPS25HB::readPressureMillibars(void)
{
    return (float)readPressureRaw() / 4096;
}

// reads pressure in inches of mercury (inHg)
float LPS25HB::readPressureInchesHg(void)
{
    return (float)readPressureRaw() / 138706.5;
}

// reads pressure and returns raw 24-bit sensor output
int32_t LPS25HB::readPressureRaw(void)
{
    // assert MSB to enable register address auto-increment
  
    char command = PRESS_OUT_XL | (1 << 7);
    char p[3];
    _i2c.write(address << 1,&command,1);
    _i2c.read((address << 1)| 1,p,3);
  
    // combine int8_ts
    return (int32_t)(int8_t)p[2] << 16 | (uint16_t)p[1] << 8 | p[0];
}

// reads temperature in degrees C
float LPS25HB::readTemperatureC(void)
{
    return 42.5 + (float)readTemperatureRaw() / 480;
}

// reads temperature in degrees F
float LPS25HB::readTemperatureF(void)
{
    return 108.5 + (float)readTemperatureRaw() / 480 * 1.8;
}

// reads temperature and returns raw 16-bit sensor output
int16_t LPS25HB::readTemperatureRaw(void)
{
    char command = TEMP_OUT_L | (1 << 7);
  
    // assert MSB to enable register address auto-increment
    char t[2];
    _i2c.write(address << 1,&command,1);
    _i2c.read((address << 1)| 1,t,2);

    // combine bytes
    return (int16_t)(t[1] << 8 | t[0]);
}

// converts pressure in mbar to altitude in meters, using 1976 US
// Standard Atmosphere model (note that this formula only applies to a
// height of 11 km, or about 36000 ft)
//  If altimeter setting (QNH, barometric pressure adjusted to sea
//  level) is given, this function returns an indicated altitude
//  compensated for actual regional pressure; otherwise, it returns
//  the pressure altitude above the standard pressure level of 1013.25
//  mbar or 29.9213 inHg
float LPS25HB::pressureToAltitudeMeters(double pressure_mbar, double altimeter_setting_mbar)
{
    return (1 - pow(pressure_mbar / altimeter_setting_mbar, 0.190263)) * 44330.8;//pressure;
}

// converts pressure in inHg to altitude in feet; see notes above
float LPS25HB::pressureToAltitudeFeet(double pressure_inHg, double altimeter_setting_inHg)
{
    return (1 - pow(pressure_inHg / altimeter_setting_inHg, 0.190263)) * 145442;//pressure;
}

// Private Methods ///////////////////////////////////////////////////

bool LPS25HB::detectDeviceAndAddress(deviceType device, sa0State sa0)
{
    if (sa0 == sa0_auto || sa0 == sa0_high)
    {
        address = SA0_HIGH_ADDRESS;
        if (detectDevice(device)) return true;
    }
    if (sa0 == sa0_auto || sa0 == sa0_low)
    {
        address = SA0_LOW_ADDRESS;
        if (detectDevice(device)) return true;
    }

    return false;
}

bool LPS25HB::detectDevice(deviceType device)
{
    uint8_t id = testWhoAmI(address);
  
    if ((device == device_auto || device == device_25H) && id == (uint8_t)LPS25H_WHO_ID)
    {
        _device = device_25H;
        return true;
    }
    if ((device == device_auto || device == device_331AP) && id == (uint8_t)LPS331AP_WHO_ID)
    {
        _device = device_331AP;
        return true;
    }

    return false;
}

int LPS25HB::testWhoAmI(uint8_t address)
{
    char command = WHO_AM_I;
    char status = 0;
    _i2c.write(address << 1,&command,1);
    _i2c.read((address << 1)| 1,&status,1);
    return status;
}