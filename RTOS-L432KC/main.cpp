// Projekt z przedmiotu "Systemy czasu rzeczywistego"
// Piotrkowski Michał 319001
// Sawicki Piotr 319003
#include "mbed.h"

// inicjalizacja komunikacji I2C do komunikacji z czujnikiem temperatuy i ciśnienia
I2C i2c(I2C_SDA, I2C_SCL);

// adres czujnika LPS25HB: 1011101b
const int LPS25HB_ADDR  = 0x5D << 1;      // adres w hexie

// ostatni bit w adresie odpowiada za odczytywanie lub zapisywanie


int main()
{
    i2c.frequency(100000);

    while (true) {

    }
}