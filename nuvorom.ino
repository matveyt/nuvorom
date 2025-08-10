//
// Nuvoton LDROM (bootloader) init
// (for N76E003 or similar)
//

#include "nuvoicp.h"
#include "ldrom.h"

constexpr bool may_unlock = true;
constexpr uint8_t DAT = 4, CLK = 5, RST = 6, LED = 13;

inline void print_begin() { Serial.begin(9600); }
inline void print_end() { Serial.end(); }
inline void print(const char str[]) { Serial.print(str); }
inline void print(char ch) { Serial.write(ch); }
inline void print(unsigned long num, int base) { Serial.print(num, base); }
inline void println(const char str[]) { Serial.println(str); }
inline void println() { Serial.println(); }

bool enter_icp();
bool update_config(const uint8_t config[5]);
bool update_ldrom(const uint8_t rom[] PROGMEM, size_t length);
void loop() { /* nothing */ }

NuvotonICP icp(DAT, CLK, RST);

void setup()
{
  print_begin();
  if (LED)
    icp.pin_init(LED, HIGH);

  print("Nuvoton Device ");
  if (enter_icp()) {
    print(icp.device_id(), HEX);  // Device ID
    println();

    print("LDROM ");              // LDROM update
    if (update_ldrom(ldrom, sizeof(ldrom))) {
      println("OK");

      static const uint8_t config[5] = {
        icp.config0(false),
        icp.config1(sizeof(ldrom)),
        UINT8_MAX, UINT8_MAX, UINT8_MAX,
      };
      print("CONFIG ");           // CONFIG update
      if (update_config(config)) {
        println("OK");
        if (LED)
          icp.pin_float(LED);
      } else
        println("Fail");
    } else
      println("Fail");
  } else
    println("None");

  icp.exit();
  print_end();
}

bool enter_icp()
{
  uint8_t company_id = icp.enter();

  if (company_id == UINT8_MAX && icp.locked()) {
    print("locked ");
    if (!may_unlock)
      return false;

    // try unlock
    icp.mass_erase();
    print("erased ");
    icp.exit();
    delay(10);
    company_id = icp.enter();
  }

  return (company_id == NuvotonICP::NUVOTON);
}

bool update_config(const uint8_t config[5])
{
  uint8_t result = icp.config_compare(config);
  if (icp.equal(result))
    return true;  // nothing to do

  // erase
  if (icp.need_erase(result)) {
    icp.config_erase();
    print('x');
  }
  // write
  if (icp.need_write(result)) {
    icp.config_write(config);
    print('.');
  }
  // verify
  return icp.config_verify(config);
}

bool update_ldrom(const uint8_t rom[] PROGMEM, size_t length)
{
  // check ldrom size
  const uint16_t ldsize = icp.ldsize(length);
  if (ldsize == 0 || ldsize > 4096)
    return false;

  const uint16_t psize = icp.psize();
  const uint8_t incomplete_page = length / psize;
  const uint8_t empty_page = incomplete_page + !!(length % psize);

  uint8_t data[icp.psize_max()], page = 0;
  for (uint16_t offset = 0; offset < ldsize; offset += psize, ++page) {
    if (page < incomplete_page) {
      // complete page
      memcpy_P(data, &rom[offset], psize);
    } else if (page < empty_page) {
      // incomplete non-empty page
      uint16_t leftover = length - offset;
      memcpy_P(data, &rom[offset], leftover);
      memset(&data[leftover], UINT8_MAX, psize - leftover);
    }

    uint16_t address = icp.size() - ldsize + offset;
    uint8_t* p_data = (page < empty_page) ? data : nullptr;
    uint8_t result = icp.flash_compare(address, p_data, psize);
    if (icp.equal(result))
      continue; // nothing to do

    // erase
    if (icp.need_erase(result)) {
      icp.flash_erase(address);
      print('x');
    }
    // write
    if (icp.need_write(result)) {
      icp.flash_write(address, p_data, psize);
      print('.');
    }
    // verify
    if (!icp.flash_verify(address, p_data, psize))
      return false;
  }

  return true;
}
