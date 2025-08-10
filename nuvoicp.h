#pragma once

// Nuvoton In-Circuit Programming client
class NuvotonICP {
  public:
    static constexpr uint8_t NUVOTON = 0xda;        // company ID
    static constexpr uint16_t N76E616 = 0x2f50,     // device ID
      N76E003 = 0x3650, MS51FB9AE = 0x4b21;
    static constexpr uint8_t COMPARE_NOT_EQUAL = 1, // compare() flags
      COMPARE_GREATER = 2, COMPARE_NOT_EMPTY = 4,
      COMPARE_NEED_ERASE = COMPARE_NOT_EQUAL | COMPARE_GREATER,
      COMPARE_NEED_WRITE = COMPARE_NOT_EQUAL | COMPARE_NOT_EMPTY;
  protected:
    static constexpr uint32_t MAGIC_RESET = 0x9e1cb6;
    static constexpr uint32_t MAGIC_ENTER = 0x5aa503, MAGIC_EXIT = 0x0f78f0;
    static constexpr uint16_t DELAY_RESET_1 = 10000, DELAY_RESET_2 = 1000;
    static constexpr uint16_t DELAY_MASS_1 = 65000, DELAY_MASS_2 = 1000;
    static constexpr uint16_t DELAY_PAGE_1 = 6000, DELAY_PAGE_2 = 100,
      DELAY_PAGE_MAX = 40000;
    static constexpr uint16_t DELAY_PROG_1 = 25, DELAY_PROG_2 = 5, DELAY_PROG_MAX = 40;
    static constexpr uint16_t DELAY_READ_1 = 1, DELAY_READ_2 = 1;

  public:
    NuvotonICP(uint8_t dataPin, uint8_t clockPin, uint8_t resetPin) :
      m_dataPin(dataPin), m_clockPin(clockPin), m_resetPin(resetPin), m_id(UINT16_MAX)
    {}

    uint8_t enter();  // return company ID
    void exit();
    bool locked();

    static void pin_float(uint8_t pin)
    { pinMode(pin, INPUT); digitalWrite(pin, LOW); }
    static void pin_init(uint8_t pin, uint8_t data=LOW)
    { digitalWrite(pin, data); pinMode(pin, OUTPUT); }

    static constexpr bool equal(uint8_t result)
    { return !(result & COMPARE_NOT_EQUAL); }
    static constexpr bool need_erase(uint8_t result)
    { return ((result & COMPARE_NEED_ERASE) == COMPARE_NEED_ERASE); }
    static constexpr bool need_write(uint8_t result)
    { return ((result & COMPARE_NEED_WRITE) == COMPARE_NEED_WRITE); }

    static constexpr uint8_t config0(bool cbs)
    { return cbs ? UINT8_MAX : INT8_MAX; }
    static constexpr uint8_t config1(uint16_t ldsize)
    { return (UINT8_MAX - (ldsize + 1023) / 1024); }
    static constexpr bool cbs(uint8_t config0)
    { return (config0 > INT8_MAX); }
    static constexpr uint16_t ldsize(uint8_t config1)
    { return ((7 - (config1 & 7)) * 1024); }
    static constexpr uint16_t ldsize(uint16_t length)
    { return ((length + 1023) & ~1023); }

    static constexpr uint16_t psize_max() { return 256; }
    static constexpr uint16_t id2psize(uint16_t id)
    { return (id != N76E616) ? 128 : psize_max(); }
    static constexpr uint32_t id2size(uint16_t id)
    { return (((uint8_t)id >> 4) <= 4) ? (4096L << ((uint8_t)id >> 4)) : (18 * 1024); }

    uint16_t device_id() const { return m_id; }
    uint16_t psize() const { return id2psize(device_id()); }
    uint32_t size() const { return id2size(device_id()); }

    // CONFIG
    void config_erase()
    { erase(0x30000L); }
    void config_read(uint8_t buffer[5])
    { read(0x30000L, buffer, 5); }
    void config_write(const uint8_t buffer[5])
    { write(0x30000L, buffer, 5); }
    uint8_t config_compare(const uint8_t buffer[5])
    { return compare(0x30000L, buffer, 5); }
    bool config_verify(const uint8_t buffer[5])
    { return equal(config_compare(buffer)); }

    // FLASH
    void flash_erase(uint16_t address)
    { erase(0L + address); }
    void flash_read(uint16_t address, uint8_t buffer[], size_t bufsize)
    { read(0L + address, buffer, bufsize); }
    void flash_write(uint16_t address, const uint8_t buffer[], size_t bufsize)
    { write(0L + address, buffer, bufsize); }
    uint8_t flash_compare(uint16_t address, const uint8_t buffer[], size_t bufsize)
    { return compare(0L + address, buffer, bufsize); }
    bool flash_verify(uint16_t address, const uint8_t buffer[], size_t bufsize)
    { return equal(flash_compare(address, buffer, bufsize)); }

    // MASS
    void mass_erase();

  protected:
    uint8_t read8()
    { return shiftIn(m_dataPin, m_clockPin, MSBFIRST); }
    void write8(uint8_t data=UINT8_MAX)
    { shiftOut(m_dataPin, m_clockPin, MSBFIRST, data); }
    void write24(uint8_t code, uint32_t param)
    { write24(param << 6 | code); }
    void write24(uint32_t data);

    void tiktok(uint16_t us1, uint16_t us2);
    uint8_t read9(bool last=false);

    void erase(uint32_t address);
    void read(uint32_t address, uint8_t buffer[], size_t bufsize);
    void write(uint32_t address, const uint8_t buffer[], size_t bufsize);
    uint8_t compare(uint32_t address, const uint8_t buffer[], size_t bufsize);

  private:
    uint8_t m_dataPin, m_clockPin, m_resetPin;
    uint16_t m_id;
};
