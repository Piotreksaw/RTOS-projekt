#include "mbed.h"
#include "DHT11/DHT11.h"

I2C i2c(PB_9, PB_8);   // SDA, SCL
#define SSD1306_ADDR 0x78
#define SSD1306_LCDWIDTH 128
#define SSD1306_LCDHEIGHT 64

DHT11 d(D8);           // DHT11
Ticker tick;
EventQueue queue;

// ---------- SSD1306 low-level ----------
void oled_cmd(uint8_t cmd) {
    char data[2];
    data[0] = 0x00; // control: command
    data[1] = cmd;
    i2c.write(SSD1306_ADDR, data, 2);
}
void oled_data(uint8_t d) {
    char data[2];
    data[0] = 0x40; // control: data
    data[1] = d;
    i2c.write(SSD1306_ADDR, data, 2);
}

void oled_init() {
    oled_cmd(0xAE);
    oled_cmd(0xD5); oled_cmd(0x80);
    oled_cmd(0xA8); oled_cmd(0x3F);
    oled_cmd(0xD3); oled_cmd(0x00);
    oled_cmd(0x40);
    oled_cmd(0x8D); oled_cmd(0x14);
    oled_cmd(0x20); oled_cmd(0x00);
    oled_cmd(0xA1); oled_cmd(0xC8);
    oled_cmd(0xDA); oled_cmd(0x12);
    oled_cmd(0x81); oled_cmd(0xCF);
    oled_cmd(0xD9); oled_cmd(0xF1);
    oled_cmd(0xDB); oled_cmd(0x40);
    oled_cmd(0xA4); oled_cmd(0xA6);
    oled_cmd(0xAF);
}

void oled_clear() {
    oled_cmd(0x21); oled_cmd(0x00); oled_cmd(0x7F);
    oled_cmd(0x22); oled_cmd(0x00); oled_cmd(0x07);

    // czyszczenie ekranu
    char buffer[17];
    buffer[0] = 0x40;
    for (int i = 1; i < 17; i++) buffer[i] = 0x00;
    for (int i = 0; i < 64; i++) {
        i2c.write(SSD1306_ADDR, buffer, 17);
    }
}

// Set start page and column for subsequent DATA writes
void oled_set_pos(int page, int column) {
    if (column < 0) column = 0;
    if (column > 127) column = 127;
    if (page < 0) page = 0;
    if (page > 7) page = 7;
    oled_cmd(0x21); oled_cmd(column); oled_cmd(127); // column address from column..127
    oled_cmd(0x22); oled_cmd(page); oled_cmd(7);    // page address from page..7
}

struct Glyph { char ch; uint8_t b[5]; };

const Glyph font_map[] = {
    {' ', {0x00,0x00,0x00,0x00,0x00}},
    {'0', {0x3E,0x51,0x49,0x45,0x3E}},
    {'1', {0x00,0x42,0x7F,0x40,0x00}},
    {'2', {0x42,0x61,0x51,0x49,0x46}},
    {'3', {0x21,0x41,0x45,0x4B,0x31}},
    {'4', {0x18,0x14,0x12,0x7F,0x10}},
    {'5', {0x27,0x45,0x45,0x45,0x39}},
    {'6', {0x3C,0x4A,0x49,0x49,0x30}},
    {'7', {0x01,0x71,0x09,0x05,0x03}},
    {'8', {0x36,0x49,0x49,0x49,0x36}},
    {'9', {0x06,0x49,0x49,0x29,0x1E}},

    {'T', {0x01,0x01,0x7F,0x01,0x01}},
    {'E', {0x7F,0x49,0x49,0x49,0x41}},
    {'M', {0x7F,0x06,0x0C,0x06,0x7F}},
    {'P', {0x7F,0x09,0x09,0x09,0x06}},
    {'H', {0x7F,0x08,0x08,0x08,0x7F}},
    {'U', {0x7F,0x40,0x40,0x40,0x7F}},
    {'C', {0x3E,0x41,0x41,0x41,0x22}},
    {'O', {0x3E,0x41,0x41,0x41,0x3E}},
    {'R', {0x7F,0x09,0x19,0x29,0x46}},
    {'D', {0x7F,0x41,0x41,0x22,0x1C}},
    {':', {0x00,0x36,0x36,0x00,0x00}},
    {'%', {0x62,0x64,0x08,0x13,0x23}},
    {'A', {0x7E,0x11,0x11,0x11,0x7E}},
    {'I', {0x00,0x41,0x7F,0x41,0x00}},

    {'Y', {0x03,0x04,0x78,0x04,0x03}},
    {'S', {0x46,0x49,0x49,0x49,0x31}},
};

const int FONT_MAP_SIZE = sizeof(font_map) / sizeof(Glyph);

const uint8_t* get_glyph(char c) {
    for (int i = 0; i < FONT_MAP_SIZE; i++) {
        if (font_map[i].ch == c) return font_map[i].b;
    }
    return font_map[0].b; // space
}


void print_char(char c) {
    const uint8_t* g = get_glyph(c);
    for (int i = 0; i < 5; i++) oled_data(g[i]);
    oled_data(0x00); // spacer
}

void print_string(const char *s) {
    while (*s) {
        print_char(*s++);
    }
}

void print_string_at(int page, int column, const char *s) {
    oled_set_pos(page, column);
    print_string(s);
}

// Funkcja tworzy dwie warstwy (dolną i górną) i wypisuje je na top_page i top_page+1.
void print_string_big_vert_at(int top_page, int column, const char *s) {
    uint8_t low[128];
    uint8_t high[128];
    int idx = 0;

    while (*s && idx < 128) {
        const uint8_t* g = get_glyph(*s++);
        for (int col = 0; col < 5 && idx < 128; col++) {
            uint8_t src = g[col];
            uint16_t expanded = 0;
            for (int b = 0; b < 7; b++) {
                if (src & (1 << b)) {
                    expanded |= (1 << (b * 2));
                    expanded |= (1 << (b * 2 + 1));
                }
            }
            low[idx] = expanded & 0xFF;
            high[idx] = (expanded >> 8) & 0xFF;
            idx++;
        }

        if (idx < 128) {
            low[idx] = 0x00;
            high[idx] = 0x00;
            idx++;
        }
    }

    oled_set_pos(top_page, column);
    for (int i = 0; i < idx; i++) oled_data(low[i]);

    int upper_page = top_page + 1;
    if (upper_page <= 7) {
        oled_set_pos(upper_page, column);
        for (int i = 0; i < idx; i++) oled_data(high[i]);
    }
}

void update_display_with_readings(int temp, int hum) {
    char tval[16];
    char hval[16];

    snprintf(tval, sizeof(tval), "%d C", temp);
    snprintf(hval, sizeof(hval), "%d %%", hum);

    oled_clear();

    print_string_big_vert_at(0, 0, "TEMPERATURE:");
    print_string_at(2, 0, tval);

    print_string_big_vert_at(4, 0, "HUMIDITY:");

    print_string_at(6, 0, hval);

}

void dht_handler() {
    int s = d.readData();
    if (s != DHT11::OK) {
        print_string_at(2, 0, "Err");
        printf("DHT read error: %d\r\n", s);
    } else {
        int temp = d.readTemperature();
        int hum  = d.readHumidity();
        // printf("T:%d, H:%d\r\n", temp, hum);
        update_display_with_readings(temp, hum);
    }
}

void tick_cb() {
    queue.call(dht_handler);
}

int main() {
    i2c.frequency(400000);
    oled_init();
    oled_clear();

    print_string_at(3, 0, "Initializing...");
    ThisThread::sleep_for(600ms);
    oled_clear();

    tick.attach(&tick_cb, 5s);
    queue.call(dht_handler);
    queue.dispatch_forever();
}
