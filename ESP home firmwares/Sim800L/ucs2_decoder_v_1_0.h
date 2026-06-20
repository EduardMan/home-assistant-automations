#pragma once

/**
 * @file    ucs2_decoder.h
 * @brief   Декодирование SMS-сообщений из UCS2 (шестнадцатеричный
 *          формат) в читаемый текст UTF-8.
 *
 * Поддерживаются:
 *   - Чистый UCS2 (hex-строка, длина кратна 4)
 *   - PDU-формат MTS (заголовок 14 символов, затем UCS2-текст)
 *
 * Использование в ESPHome:
 *   #include "ucs2_decoder.h"
 *   ...
 *   std::string readable = decode_text(id(sms_sensor).state);
 *   id(decoded_sensor).publish_state(readable);
 *
 * Зависимости: <string>, <algorithm>, <cctype>.
 */

#include <string>
#include <algorithm>

//----------------------------------------------------------
/**
 * @brief Декодирует SMS-сообщение из UCS2 (hex) в читаемый
 *        текст UTF-8.
 *
 * Протестировано на:
 * - МТС
 * - Мегафон
 * @param raw Строка, полученная от сенсора SMS (может быть
 *            чистым UCS2 или PDU с заголовком).
 * @return    Декодированная строка UTF-8, либо исходная,
 *            если вход пуст/некорректен.
 */
static std::string decode_text(std::string raw) {

  // Обработка пустого или неизвестного значения
  if (raw.empty() || raw == "unknown")
    return raw;

  std::string hex;

  // Определяем, является ли сообщение PDU (MTS)
  bool is_pdu = false;

  if (raw.length() > 50) {
    std::string prefix = raw.substr(0, 2);

    if (prefix == "06" || prefix == "07" ||
        prefix == "08" || prefix == "01") {
      is_pdu = true;
    }
  }

  // Если это PDU и длина достаточна для заголовка,
  // берём текст начиная с 15-го символа (индекс 14)
  if (is_pdu && raw.length() > 14) {
    hex = raw.substr(14); // извлекаем текст из PDU
  } else {
    hex = raw; // иначе считаем, что пришёл чистый UCS2
  }

  // Проверяем, является ли строка шестнадцатеричной и её длина кратна 4
  bool is_hex = true;
  for (char c : hex) {
    if (!isxdigit(c)) {
      is_hex = false;
      break;
    }
  }

  if (!is_hex || hex.length() % 4 != 0)
    return hex;

  // Декодирование UCS2 в UTF-8
  // Каждые 4 hex-символа = один UCS2 код
  std::string text;
  for (size_t i = 0; i < hex.length(); i += 4) {
    // Преобразуем 4 hex-символа в целое число
    uint16_t code =
      strtol(hex.substr(i, 4).c_str(), nullptr, 16);

    // Преобразуем UCS2 код в UTF-8 последовательность
    if (code < 0x80) {
      // 1 байт (ASCII)
      text += (char)code;
    } else if (code < 0x800) {
      // 2 байта
      text += (char)(0xC0 | (code >> 6));
      text += (char)(0x80 | (code & 0x3F));
    } else {
      // 3 байта
      text += (char)(0xE0 | (code >> 12));
      text += (char)(0x80 | ((code >> 6) & 0x3F));
      text += (char)(0x80 | (code & 0x3F));
    }
  }

  return text;
}
