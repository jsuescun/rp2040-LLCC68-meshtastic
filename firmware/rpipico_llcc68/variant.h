// variant.h personalizado para Raspberry Pi Pico + DX-LR20 (LLCC68)
// Basado en el variant oficial rpipico, adaptado para control explícito
// de RXEN y TXEN del switch RF del DX-LR20.
//
// Cableado:
//   SCK   -> GP10 (pin físico 14)
//   MOSI  -> GP11 (pin físico 15)
//   MISO  -> GP12 (pin físico 16)
//   NSS   -> GP3  (pin físico 5)
//   NRST  -> GP15 (pin físico 20)
//   DIO1  -> GP20 (pin físico 26)
//   BUSY  -> GP2  (pin físico 4)
//   RXEN  -> GP21 (pin físico 27)
//   TXEN  -> GP22 (pin físico 29)

// #define RADIOLIB_CUSTOM_ARDUINO 1
// #define RADIOLIB_TONE_UNSUPPORTED 1
// #define RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED 1

#define ARDUINO_ARCH_AVR

// default I2C pins:
// SDA = 4
// SCL = 5

// Recommended pins for SerialModule:
// txd = 8
// rxd = 9

#define EXT_NOTIFY_OUT 16
#define BUTTON_PIN 17

#define LED_POWER PIN_LED

#define BATTERY_PIN -1
// ratio of voltage divider = 3.0 (R17=200k, R18=100k)
#define ADC_MULTIPLIER 3.1 // 3.0 + a bit for being optimistic
#define BATTERY_SENSE_RESOLUTION_BITS ADC_RESOLUTION

// Usar LLCC68 en lugar de SX1262
#define USE_LLCC68

#undef LORA_SCK
#undef LORA_MISO
#undef LORA_MOSI
#undef LORA_CS

#define LORA_SCK  10
#define LORA_MISO 12
#define LORA_MOSI 11
#define LORA_CS   3

#define LORA_DIO0  RADIOLIB_NC
#define LORA_RESET 15
#define LORA_DIO1  20
#define LORA_DIO2  RADIOLIB_NC
#define LORA_DIO3  RADIOLIB_NC

#ifdef USE_LLCC68
#define SX126X_CS    LORA_CS
#define SX126X_DIO1  LORA_DIO1
#define SX126X_BUSY  2           // GP2 -> BUSY del módulo
#define SX126X_RESET LORA_RESET
// Control explícito del switch RF del DX-LR20 (PE4259)
// TXEN: HIGH en TX, LOW en RX  (controlado por DIO2 interno del LLCC68)
// RXEN: HIGH en RX, LOW en TX  (señal complementaria via GPIO)
#define SX126X_TXEN  22          // GP22 -> TXEN del módulo
#define SX126X_RXEN  21          // GP21 -> RXEN del módulo
// TCXO del DX-LR20 a 1.8V via DIO3
#define SX126X_DIO3_TCXO_VOLTAGE 1.8
#endif
