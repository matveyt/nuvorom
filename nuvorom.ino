//
// Nuvoton LDROM (bootloader) init
// (for N76E003 or similar chips)
//

#include "nuvoicp.h"
#include "rom.h"

// erase locked chip
static constexpr bool may_unlock = true;

// interface pins
enum {
  DAT = 4,
  CLK = 5,
  RST = 6,
  LED = 13,
};

void led_on()
{
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
}

void led_off()
{
  digitalWrite(LED, LOW);
  pinMode(LED, INPUT);
}

void loop() {}

void setup()
{
  uint8_t data[256];  // max page size

  Serial.begin(9600);
  led_on();

  // enter programming mode
  NuvotonICP icp(DAT, CLK, RST);
  uint8_t company_id = icp.enter();
  if (company_id == 0xff && icp.locked()) {
    // locked chip
    Serial.println(F("Device locked"));
    if (may_unlock) {
      icp.mass_erase();
      icp.read_device_id();
      Serial.println(F("Device erased"));
    } else
      goto exit;
  } else if (company_id != NuvotonICP::NUVOTON) {
    Serial.println(F("No device found"));
    goto exit;
  }

  uint16_t device_id = icp.device_id();
  Serial.print(F("Device 0x"));
  Serial.println(device_id, HEX);
  if (device_id == 0 || device_id == 0xffff)
    goto exit;

  // CONFIG
  uint8_t config[5] = {
    icp.config0(false),
    icp.config1(sizeof(rom)),
    0xff, 0xff, 0xff,
  };
  uint8_t result = icp.config_compare(config);
  if (!icp.is_equal(result)) {
    // erase
    if (!icp.is_empty_flash(result)) {
      icp.config_erase();
      Serial.println(F("CONFIG erased"));
    }
    // write
    icp.config_write(config);
    Serial.println(F("CONFIG written"));
    // verify
    if (!icp.is_equal(icp.config_compare(config))) {
      Serial.println(F("Verify fail"));
      goto exit;
    }
  }

  // LDROM
  static_assert(sizeof(rom) == 1024 || sizeof(rom) == 2048
    || sizeof(rom) == 3072 || sizeof(rom) == 4096,
    "ROM size must be 1K, 2K, 3K or 4K");
  const size_t ldstart = icp.size() - sizeof(rom);
  const uint16_t pgsize = icp.pgsize();

  Serial.print(F("LDROM update "));
  for (size_t offset = 0; offset < sizeof(rom); offset += pgsize) {
    memcpy_P(data, &rom[offset], pgsize);
    result = icp.flash_compare(ldstart + offset, data, pgsize);
    if (!icp.is_equal(result)) {
      // erase
      if (!icp.is_empty_flash(result)) {
        icp.flash_erase(ldstart + offset);
        Serial.write('x');
      }
      // write
      if (!icp.is_empty_buffer(result)) {
        icp.flash_write(ldstart + offset, data, pgsize);
        Serial.write('.');
      }
      // verify
      if (!icp.is_equal(icp.flash_compare(ldstart + offset, data, pgsize))) {
        Serial.println(F("Verify fail"));
        goto exit;
      }
    }
  }
  Serial.println(F("Ok"));
  led_off();

exit:
  icp.exit();
  Serial.end();
}