// Projekt z przedmiotu "Systemy czasu rzeczywistego"
// Piotrkowski Michał 319001
// Sawicki Piotr 319003


#include "mbed.h"
#include "i2c_our.h"

I2C i2c(I2C_SDA, I2C_SCL);

const int LPS25HB_ADDR = 0x5D << 1;   // adres 8-bitowy

// Rejestry LPS25HB
#define WHO_AM_I_REG     0x0F
#define CTRL_REG1        0x20
#define CTRL_REG2        0x21
#define PRESS_OUT_XL     0x28
#define TEMP_OUT_L       0x2B

int main() {

    i2c.frequency(400000);    // przyspieszenie do 400kHz

    // --- Odczyt WHO_AM_I ---
    char reg = WHO_AM_I_REG;
    char whoami = 0;
    i2c.write(LPS25HB_ADDR, &reg, 1, true);  // wysyłamy adres rejestru (repeated start)
    i2c.read(LPS25HB_ADDR, &whoami, 1);

    printf("WHO_AM_I = 0x%X\r\n", whoami);   // powinno zwrócić 0xBD

    // --- Włączenie czujnika ---
    // PD=1 (uruchomienie), ODR=1Hz (010)

    // Zapis do CTRL_REG1
    char cfg1[2] = { CTRL_REG1, 0x90 };
    i2c.write(LPS25HB_ADDR, cfg1, 2);
    ThisThread::sleep_for(200ms);

    // Odczyt CTRL_REG1
    reg = CTRL_REG1;
    char val = 0;
    i2c.write(LPS25HB_ADDR, &reg, 1, true);
    i2c.read(LPS25HB_ADDR, &val, 1);
    printf("CTRL_REG1 = 0x%X\r\n", val);

    // Zapis do CTRL_REG2 (auto-increment)
    char cfg2[2] = { CTRL_REG2, 0x10 };
    i2c.write(LPS25HB_ADDR, cfg2, 2);
    ThisThread::sleep_for(50ms);
    


    // Odczyt CTRL_REG2
    reg = CTRL_REG2;
    i2c.write(LPS25HB_ADDR, &reg, 1, false);  // nie repeated start
    i2c.read(LPS25HB_ADDR, &val, 1);
    printf("CTRL_REG2 = 0x%X\r\n", val);

    while (true) {

        char status_reg = 0x27;
        char status = 0;

        // Odczyt STATUS_REG
        i2c.write(LPS25HB_ADDR, &status_reg, 1, true);
        i2c.read(LPS25HB_ADDR, &status, 1);

        // Sprawdzamy, czy dostępne są zarówno dane ciśnienia (P_DA) jak i temperatury (T_DA)
        if ((status & 0x04) && (status & 0x01)) {

            char addr = PRESS_OUT_XL | 0x80; // 0x28 | 0x80 = 0xA8, auto-increment
            char data[5];  // 3 bajty ciśnienia + 2 bajty temperatury

            // Odczyt danych
            i2c.write(LPS25HB_ADDR, &addr, 1, true);
            i2c.read(LPS25HB_ADDR, data, 5);

            // Składanie wartości ciśnienia i temperatury
            int32_t press_raw = ((uint32_t)data[2] << 16) |
                                ((uint32_t)data[1] << 8) |
                                (uint32_t)data[0];

            int16_t temp_raw = (int16_t)((uint16_t)data[4] << 8 | data[3]);

            // Konwersja do hPa i °C
            float pressure_hPa = press_raw / 4096.0f;
            float temperature_C = 42.5f + (float)temp_raw / 480.0f;

            // Wyświetlanie RAW i przeliczonych wartości
            printf("RAW P: %d %d %d | RAW T: %d %d\r\n",
                data[0], data[1], data[2],
                data[3], data[4]);

            printf("Pressure: %.2f hPa | Temp: %.2f C\r\n",
                pressure_hPa, temperature_C);
        }

ThisThread::sleep_for(500ms);

        
    }

}
