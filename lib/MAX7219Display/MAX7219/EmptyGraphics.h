#ifndef MAX7219Display_EmptyGraphics_H_
#define MAX7219Display_EmptyGraphics_H_

#include <Arduino.h>

namespace MAX7219 {

class EmptyGraphics {
  template <uint8_t Width, uint8_t Height>
  bool tryGetGlyph(char16_t c, uint8_t *retval) const {
    return false;
  }
};

} // namespace MAX7219

#endif // MAX7219Display_EmptyGraphics_H_