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
  
        printf("p:%.2f\t mbar\ta:%.2f m\tt:%.2f deg C\r\n",pressure,altitude,temperature);
        // printf("pressure raw: %f\r\n", pressure);
        // printf("altitude raw: %f\r\n", altitude);
        // printf("temperature raw: %f\r\n", temperature);

        // printf("test");

        ThisThread::sleep_for(5000ms);
    }
    
}