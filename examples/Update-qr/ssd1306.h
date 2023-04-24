#include <U8g2lib.h> // https://github.com/olikraus/u8g2/
#include <qrcodegen.h> // https://github.com/lbernstone/qrcodegen
#include "global_vars.h"
#define OLED_PINS 5,7,6 // RST,CLK,DATA
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, OLED_PINS);

void show_qrcode(uint8_t qr_scale = 2) {
  char str_qrcode[40];
  snprintf(str_qrcode, 40, "WIFI:S:%12s;T:WPA;P:%10s", WiFi.softAPSSID().c_str(), passwd);
  uint16_t max_pixels = min(u8g2.getDisplayWidth(),u8g2.getDisplayHeight());
  uint8_t qr_version = (((max_pixels - 2) / qr_scale) - 17) / 4;
  uint8_t qrcode[qrcodegen_BUFFER_LEN_FOR_VERSION(qr_version)];
  uint8_t tempBuffer[qrcodegen_BUFFER_LEN_FOR_VERSION(qr_version)];
  if (!qrcode || !tempBuffer) {
    log_e("Insufficient memory: %d", 2 * qrcodegen_BUFFER_LEN_FOR_VERSION(qr_version));
    return;
  }
  bool qr_err = !qrcodegen_encodeText(str_qrcode, tempBuffer, qrcode, qrcodegen_Ecc_LOW,
                                      qrcodegen_VERSION_MIN, qr_version, qrcodegen_Mask_AUTO, true);

  u8g2.firstPage(); // I write this in pages to support devices without full framebuffer
  do {
    if (qr_err) {
      log_e("QR string is too long: %s", str_qrcode);
      u8g2.setFont(u8g2_font_5x7_mf);
      u8g2.drawStr(0, 10, "QR string is too long");
      u8g2.drawStr(0, 20, str_qrcode);
    } else {
      u8g2.drawBox(0, 0, max_pixels, max_pixels); // frame the whole thing
      uint16_t qr_size = qrcodegen_getSize(qrcode);
      uint8_t x0 = (max_pixels - qr_size * qr_scale) / 2; // offset inside frame
      uint8_t y0 = (max_pixels - qr_size * qr_scale) / 2;
      // loop through the 1 module bitmap
      for (uint8_t y = 0; y < qr_size; y++) {
        for (uint8_t x = 0; x < qr_size; x++) {
          u8g2.setColorIndex(!qrcodegen_getModule(qrcode, x, y));
          u8g2.drawBox(x0 + x * qr_scale, y0 + y * qr_scale, qr_scale, qr_scale);
        }
      }
      if (otaDone) {
        if (otaDone == 1) {
          u8g2.setFont(u8g2_font_open_iconic_www_4x_t);
          u8g2.drawStr(80, 48, "H");
        } else {
          u8g2.setFont(u8g2_font_inr21_mr);
          char pct[4];
          snprintf(pct, 4, "%*d%%", 2, otaDone);
          u8g2.drawStr(70, 42, pct);
        }
      }
    }
  } while (u8g2.nextPage());
}

void init1306(const char* passwd) {
  u8g2.initDisplay();
  u8g2.setPowerSave(0);
  u8g2.setContrast(1);
  show_qrcode();
}
