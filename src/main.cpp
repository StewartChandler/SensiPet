#include <atomic>

#include "Adafruit_SSD1306.h"
#include "mbed.h"

DigitalOut myled(LED1);

// an I2C sub-class that provides a constructed default
class I2CPreInit : public I2C
{
  public:
    I2CPreInit(PinName sda, PinName scl) : I2C(sda, scl)
    {
        frequency(400000);
        start();
    };
};

I2CPreInit gI2C(PB_9, PB_8);
Adafruit_SSD1306_I2c gOled(gI2C, LED1, 0x78, 64, 128);

constexpr static std::array<uint8_t, 128 * 64 / 8> circle_image{[]() constexpr {
    std::array<uint8_t, 128 * 64 / 8> buf{0};
    const std::size_t r = 20;
    const std::size_t r_sqrd = r * r;

    for (std::size_t y = 0; y < 64; y++) {
        for (std::size_t x = 0; x < 128; x++) {
            const std::size_t bit_idx = y * 128 + x;
            const std::size_t sub_idx = bit_idx % 8;
            const std::size_t buf_idx = bit_idx / 8;

            const int32_t dx = int32_t(x) - 64;
            const int32_t dy = int32_t(y) - 32;

            buf[buf_idx] |= uint8_t(dx * dx + dy * dy < int32_t(r_sqrd)) << sub_idx;
        }
    }

    return std::move(buf);
}};

EventQueue eq;

enum SCREEN : uint8_t {
    SCREEN_SECONDS = 0,
    SCREEN_CIRCLE = 1,
};

static std::atomic<uint8_t> screen_type{SCREEN_SECONDS};

static std::atomic<uint16_t> seconds_elapsed{0};

void update_time() {
    seconds_elapsed.fetch_add(1, std::memory_order::memory_order_release);
}

void handle_button() {
    screen_type.fetch_xor(1, std::memory_order::memory_order_release);
}

void screen_run() {
    SCREEN st = static_cast<SCREEN>(screen_type.load(std::memory_order::memory_order_acquire));
    uint16_t s = seconds_elapsed.load(std::memory_order::memory_order_acquire);

    switch (st) {
        case SCREEN_SECONDS:
            gOled.clearDisplay();
            myled = !myled;
            gOled.printf("%u\r", s);
            gOled.display();
            break;
        case SCREEN_CIRCLE:
            gOled.clearDisplay();
            for (std::size_t y = 0; y < 64; y++) {
                for (std::size_t x = 0; x < 128; x++) {
                    const std::size_t bit_idx = y * 128 + x;
                    const size_t sub_idx = bit_idx % 8;
                    const size_t buf_idx = bit_idx / 8;

                    gOled.drawPixel(x, y, (circle_image[buf_idx] >> sub_idx) & 1);
                }
            }
            gOled.display();
            break;
    }
}

InterruptIn button{BUTTON1};

// main() runs in its own thread in the OS
int main()
{
    gOled.printf("%ux%u OLED Display\r\n", gOled.width(), gOled.height());

    // ok to just call directly a it is a single atomic function
    button.fall(handle_button);

    eq.call_every(chrono::milliseconds(1000), update_time);
    eq.call_every(chrono::milliseconds(100), screen_run);

    eq.dispatch_forever();
}
