#include "Display.h"
#include <assert.h>
#include <SPI.h>
using namespace MAX7219;

// opcodes for the MAX7221 and MAX7219
static constexpr uint8_t OP_NOOP        = 0;
static constexpr uint8_t OP_DIGIT0      = 1;
static constexpr uint8_t OP_DIGIT1      = 2;
static constexpr uint8_t OP_DIGIT2      = 3;
static constexpr uint8_t OP_DIGIT3      = 4;
static constexpr uint8_t OP_DIGIT4      = 5;
static constexpr uint8_t OP_DIGIT5      = 6;
static constexpr uint8_t OP_DIGIT6      = 7;
static constexpr uint8_t OP_DIGIT7      = 8;
static constexpr uint8_t OP_DECODEMODE  = 9;
static constexpr uint8_t OP_INTENSITY   = 10;
static constexpr uint8_t OP_SCANLIMIT   = 11;
static constexpr uint8_t OP_SHUTDOWN    = 12;
static constexpr uint8_t OP_DISPLAYTEST = 15;

#define CS_LOW()           digitalWrite(_pin_cs, LOW)

#define CS_HIGH()                \
  do {                           \
    digitalWrite(_pin_cs, HIGH); \
    delayMicroseconds(5);        \
  } while (0)

static void inline transfer(uint8_t address, uint8_t data) {
  uint8_t d[2] = {address, data};
  SPI.transferBytes(d, d, 2 * sizeof(uint8_t));
}

Display::Display(const int pin_cs, const IBuffer &buffer, const std::vector<setting_t> &devices)
    : _pin_cs(pin_cs)
    , _buffer(&buffer)
    , _devices(devices.cbegin(), devices.cend()) {}

void Display::broadcast(uint8_t address, uint8_t data) const {
  size_t  size = _devices.size() * 2;
  uint8_t spiData[size];

  spiData[0] = address;
  spiData[1] = data;

  for (size_t i = 1; i < _devices.size(); i++) {
    void *src  = spiData;
    void *dest = spiData + (i * 2);
    memcpy(dest, src, 2 * sizeof(uint8_t));
  }

  CS_LOW();
  SPI.transferBytes(spiData, spiData, size * sizeof(uint8_t));
  CS_HIGH();
}

void Display::init() const {
  pinMode(_pin_cs, OUTPUT);
  CS_HIGH();

  shutdownMode(true); // すべてのモジュールが shutdown 状態であることを保証する
  testMode(false);    // すべてのモジュールが test mode でないことを保証する
  setIntensity(15);
  broadcast(OP_SCANLIMIT, 7);
  broadcast(OP_DECODEMODE, 0);
}

void Display::testMode(bool value) const {
  broadcast(OP_DISPLAYTEST, value ? 1 : 0);
}

void Display::shutdownMode(bool value) const {
  broadcast(OP_SHUTDOWN, value ? 0 : 1);
}

void Display::setIntensity(uint8_t intensity) const {
  broadcast(OP_INTENSITY, intensity);
}

void Display::setIntensity(std::vector<uint8_t> intensities) const {

  assert(intensities.size() == _devices.size());

  size_t  size = _devices.size() * 2;
  uint8_t spiData[size];

  for (size_t i = 0; i < _devices.size(); i++) {
    size_t addr       = i * 2;
    spiData[addr]     = OP_INTENSITY;
    spiData[addr + 1] = intensities.at(i);
  }

  CS_LOW();
  SPI.transferBytes(spiData, spiData, size * sizeof(uint8_t));
  CS_HIGH();
}

void Display::clearAll() const {
  for (uint8_t opcode = OP_DIGIT0; opcode <= OP_DIGIT7; opcode++)
    broadcast(opcode, 0);
}

void Display::send() const {

#ifdef DEBUG_MAX7219DISPLAY_BUFFER
  _buffer->printToSerial();
#endif

  size_t dev_size = _devices.size();
  if (dev_size == 0)
    return;

  // 送信するデータの長さ
  size_t size = dev_size * 2U;

  for (size_t row = 0; row < 8; row++) {
    
    // MAX7219 モジュールでは、LED 上端が OP_DIGIT7、下端が OP_DIGIT0 に対応する。
    uint8_t opcode = OP_DIGIT7 - row;

    uint8_t spiData[size];

    for (size_t dev_i = 0; dev_i < dev_size; dev_i++) {
      auto    dev       = _devices.at(dev_i);
      uint8_t data;

      if (dev.rotation == Rotate::Clockwise)
        data = _buffer->getVerticalFrom(dev.topleft_x + row, dev.topleft_y, !dev.reverse);
      else if (dev.rotation == Rotate::_180)
        data = _buffer->getHorizontialFrom(dev.topleft_x, dev.topleft_y + 7 - row, dev.reverse);
      else if (dev.rotation == Rotate::Counterclockwise)
        data = _buffer->getVerticalFrom(dev.topleft_x + 7 - row, dev.topleft_y, dev.reverse);
      else /* Rotate::_0 */
        data = _buffer->getHorizontialFrom(dev.topleft_x, dev.topleft_y + row, !dev.reverse);

      // 遠いデバイスから順に送る
      size_t addr       = dev_i * 2;
      spiData[addr]     = opcode;
      spiData[addr + 1] = data;
    }

    CS_LOW();
    SPI.transferBytes(spiData, spiData, size * sizeof(uint8_t));
    CS_HIGH();
  }
}

#undef CS_LOW
#undef CS_HIGH
