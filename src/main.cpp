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

// main() runs in its own thread in the OS
int main()
{
    uint16_t x = 0;
    gOled.printf("%ux%u OLED Display\r\n", gOled.width(), gOled.height());

    while (1)
    {
        gOled.clearDisplay();
        myled = !myled;
        gOled.printf("%u\r", x);
        gOled.display();
        x++;
        ThisThread::sleep_for(1s);
    }
}
