//
// Nuvoton LDROM (bootloader) init
// (for N76E003 or similar)
//

#include "nuvoicp.h"
#include "ldrom.h"

// erase locked chip
static constexpr bool may_unlock = true;

// interface pins
enum {
  DAT = 4,
  CLK = 5,
  RST = 6,
  LED = 13,
};

#define LOG(text)   Serial.print(text" ")
#define LOG_L(text) Serial.println(text)
#define LOG_C(ch)   Serial.write(ch)
#define LOG_N(num)  Serial.print(num, HEX)

NuvotonICP icp(DAT, CLK, RST);

bool enter_icp();
bool update_config(const uint8_t config[5]);
bool update_ldrom(const uint8_t rom[] PROGMEM, size_t length);

void loop() { /* nothing */ }

void setup()
{
  pinMode(LED, OUTPUT);         // LED on
  digitalWrite(LED, HIGH);

  Serial.begin(9600);
  LOG("Nuvoton Device ID");
  if (enter_icp()) {
    LOG_N(icp.device_id());     // Device ID
    LOG_L();

    LOG("LDROM");               // LDROM update
    if (update_ldrom(ldrom, sizeof(ldrom))) {
      LOG_L("OK");

      static const uint8_t config[5] = {
        icp.config0(false),
        icp.config1(sizeof(ldrom)),
        0xff, 0xff, 0xff,
      };

      LOG("CONFIG");            // CONFIG update
      if (update_config(config)) {
        LOG_L("OK");
        digitalWrite(LED, LOW); // LED off
        pinMode(LED, INPUT);
      } else
        LOG_L("Fail");
    } else
      LOG_L("Fail");
  } else
    LOG_L("None");

  icp.exit();
  Serial.end();
}

bool enter_icp()
{
  uint8_t company_id = icp.enter();

  if (company_id == 0xff && icp.locked()) {
    LOG("locked");
    if (!may_unlock)
      return false;

    // try unlock
    icp.mass_erase();
    LOG("erased");
    icp.exit();
    company_id = icp.enter();
  }

  return (company_id == NuvotonICP::NUVOTON);
}

bool update_config(const uint8_t config[5])
{
  uint8_t result = icp.config_compare(config);

  if (!icp.is_equal(result)) {
    // erase
    if (!icp.is_empty_flash(result)) {
      icp.config_erase();
      LOG_C('x');
    }
    // write
    icp.config_write(config);
    LOG_C('.');
    // verify
    if (!icp.is_equal(icp.config_compare(config)))
      return false;
  }

  return true;
}

bool update_ldrom(const uint8_t rom[] PROGMEM, size_t length)
{
  // loader size in KiB
  const uint8_t ld_kb = (length / 1024) + !!(length % 1024);
  if (ld_kb < 1 || ld_kb > 4)
    return false;

  const uint16_t ldsize = ld_kb * 1024;
  const uint16_t psize = icp.psize();
  const uint8_t incomplete_page = length / psize;
  const uint8_t first_empty_page = incomplete_page + !!(length % psize);
  uint8_t data[psize];

  uint8_t page = 0;
  for (uint16_t offset = 0; offset < ldsize; offset += psize, ++page) {
    if (page < incomplete_page) {
      // complete page
      memcpy_P(data, &rom[offset], psize);
    } else if (page < first_empty_page) {
      // incomplete non-empty page
      uint16_t leftover = length - offset;
      memcpy_P(data, &rom[offset], leftover);
      memset(&data[leftover], 0xff, psize - leftover);
    } else if (page == first_empty_page) {
      // empty page (reset data[] once)
      memset(data, 0xff, psize);
    }

    uint16_t address = icp.size() - ldsize + offset;
    uint8_t result = icp.flash_compare(address, data, psize);

    if (!icp.is_equal(result)) {
      // erase
      if (!icp.is_empty_flash(result)) {
        icp.flash_erase(address);
        LOG_C('x');
      }
      // write
      if (!icp.is_empty_buffer(result)) {
        icp.flash_write(address, data, psize);
        LOG_C('.');
      }
      // verify
      if (!icp.is_equal(icp.flash_compare(address, data, psize)))
        return false;
    }
  }

  return true;
}