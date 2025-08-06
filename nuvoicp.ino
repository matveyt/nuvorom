#include "nuvoicp.h"

uint8_t NuvotonICP::enter()
{
  // pin setup
  pin_init(m_resetPin);
  pin_init(m_dataPin);
  pin_init(m_clockPin);

  // toggle reset
  for (int i = 23; i >= 0; --i) {
    digitalWrite(m_resetPin, bitRead(MAGIC_RESET, i));
    delay(DELAY_RESET1 / 1000);
  }

  // magic enter sequence
  write24(MAGIC_ENTER);
  delay(DELAY_RESET2 / 1000);

  // read company id
  write24(0x0b, 0);
  uint8_t company_id = read9(true);
  if (company_id == NUVOTON) {
    // read device id
    write24(0x0c, 0);
    uint8_t lo = read9();
    uint8_t hi = read9(true);
    m_id = makeWord(hi, lo);
  } else
    m_id = NONE;

  return company_id;
}

void NuvotonICP::exit()
{
  if (m_id != NONE) {
    // toggle reset
    digitalWrite(m_resetPin, HIGH);
    delay(DELAY_RESET1 / 1000);
    digitalWrite(m_resetPin, LOW);
    delay(DELAY_RESET1 / 1000);
    // magic exit sequence
    write24(MAGIC_EXIT);
    delay(DELAY_RESET2 / 1000);
  }

  // pin release
  pin_float(m_clockPin);
  pin_float(m_dataPin);
  pin_float(m_resetPin);
}

// read LOCK bit of CONFIG0
bool NuvotonICP::locked()
{
  write24(0, 0x30000L);
  uint8_t config0 = read9(true);
  return (!bitRead(config0, 1) && config0) ? true : false;
}

void NuvotonICP::write24(uint32_t data)
{
  write8((uint8_t)(data >> 16));
  write8((uint8_t)(data >> 8));
  write8((uint8_t)(data));
}

// toggle clock line
void NuvotonICP::tiktok(uint16_t us1, uint16_t us2)
{
  if (us1 < 1000)
    delayMicroseconds(us1);
  else
    delay(us1 / 1000);
  digitalWrite(m_clockPin, HIGH);
  if (us2 < 1000)
    delayMicroseconds(us2);
  else
    delay(us2 / 1000);
  digitalWrite(m_clockPin, LOW);
  digitalWrite(m_dataPin, LOW);

}

// eight bits of data + one stop bit
uint8_t NuvotonICP::read9(bool last)
{
  pin_float(m_dataPin);
  uint8_t data = read8();
  pin_init(m_dataPin, last ? HIGH : LOW);
  tiktok(DELAY_READ1, DELAY_READ2);
  return data;
}

// erase everything
void NuvotonICP::mass_erase()
{
  write24(0x26, 0x03a5a5);
  write8();
  digitalWrite(m_dataPin, HIGH);
  tiktok(DELAY_MASS1, DELAY_MASS2);
}

// erase single page
void NuvotonICP::erase(uint32_t address)
{
  write24(0x22, address);
  write8();
  digitalWrite(m_dataPin, HIGH);
  tiktok((device_id() != N76E616) ? DELAY_PAGE1 : 40000, DELAY_PAGE2);
}

// read flash
void NuvotonICP::read(uint32_t address, uint8_t buffer[], size_t bufsize)
{
  if (bufsize <= 0)
    return;

  write24(0, address);
  for (size_t i = 0; i < bufsize - 1; ++i)
    buffer[i] = read9();
  buffer[bufsize - 1] = read9(true);  // last read
}

// write flash
void NuvotonICP::write(uint32_t address, uint8_t buffer[], size_t bufsize)
{
  if (bufsize <= 0)
    return;

  write24(0x21, address);
  write8(buffer[0]);              // write 1st byte
  for (size_t i = 1; i < bufsize; ++i) {
    digitalWrite(m_dataPin, LOW); // data continuation
    tiktok((device_id() != N76E616) ? DELAY_PROG1 : 40, DELAY_PROG2);
    write8(buffer[i]);            // write next byte
  }
  digitalWrite(m_dataPin, HIGH);  // end of data
  tiktok((device_id() != N76E616) ? DELAY_PROG1 : 40, DELAY_PROG2);
}

// verify flash
uint8_t NuvotonICP::compare(uint32_t address, uint8_t buffer[], size_t bufsize)
{
  uint8_t result = COMPARE_EQUAL | COMPARE_EMPTY_FLASH | COMPARE_EMPTY_BUFFER;
  if (bufsize <= 0)
    return result;

  write24(0, address);
  for (size_t i = 0; i < bufsize; ++i) {
    uint8_t flsh = read9((i != bufsize - 1) ? false : true);
    uint8_t bffr = (buffer != 0) ? buffer[i] : 0xff;
    if (flsh != bffr)
      result &= ~COMPARE_EQUAL;         // flash differs from supplied buffer
    if (flsh != 0xff)
      result &= ~COMPARE_EMPTY_FLASH;   // flash has some data (must be erased first)
    if (bffr != 0xff)
      result &= ~COMPARE_EMPTY_BUFFER;  // buffer has some data (must write to flash)
  }

  return result;
}