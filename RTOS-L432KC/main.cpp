#include "mbed.h"
#include "LPS25HB.h"
#include "NTC.h"
#include "ntc_10k_44031.h"
#include <cstdio>
#include <math.h>

#define NTC_10K_44031_RESISTANCE_AT_25C		10000	// 10K
#define NTC_10K_44031_ROOM_TEMP_IN_KELVIN	298.15
#define NTC_10K_44031_BETA_VALUE			3950

AnalogIn NTC_adc(PA_6);
I2C i2c(I2C_SDA, I2C_SCL);
LPS25HB ps(i2c);
ntc_10k_44031rc ntc_test();



int main(){

    uint16_t buffer;

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

        float Vcc = 3.3f;
        float Vadc = NTC_adc.read() * Vcc;
        float R_fixed = 10000.0f;
        float R_ntc;



        
        R_ntc = R_fixed * Vadc / (Vcc - Vadc);

        // printf("ADC: %.2f, R_NTC: %.2f ohm\r\n", Vadc, R_ntc);


        float temperature_NTC = (1 / ((log(R_ntc / NTC_10K_44031_RESISTANCE_AT_25C) /
			     NTC_10K_44031_BETA_VALUE) + (1 / NTC_10K_44031_ROOM_TEMP_IN_KELVIN))) - 273.15;

        
        

        
  
        // printf("p:%.2f\t mbar\ta:%.2f m\tt:%.2f deg C\r\n", pressure, altitude, temperature);

        printf("Odczyty z LPS26HB: \r\n");
        printf("Temperatura: %.2f\r\n", temperature);
        printf("Ciśnienie: %.2f\r\n \n", pressure);
        // printf("altitude raw: %f\r\n", altitude);

        printf("Temperatura z NTC: \r\n");
        printf("%.2f \r\n\n", temperature_NTC);

        // printf("test");

        ThisThread::sleep_for(5000ms);
    }
    
}