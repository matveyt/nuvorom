#if !defined(NUVOICP_H)
#define NUVOICP_H

class NuvotonICP {
  public:
    static constexpr uint8_t NUVOTON = 0xda;    // company ID
    static constexpr uint16_t N76E616 = 0x2f50; // device ID
    static constexpr uint8_t COMPARE_EQUAL = 1, // compare() flags
      COMPARE_EMPTY_FLASH = 2, COMPARE_EMPTY_BUFFER = 4;
  protected:
    static constexpr uint32_t MAGIC_RESET = 0x9e1cb6;
    static constexpr uint32_t MAGIC_ENTER = 0x5aa503, MAGIC_EXIT = 0x0f78f0;
    static constexpr uint16_t DELAY_RESET1 = 10000, DELAY_RESET2 = 1000;
    static constexpr uint16_t DELAY_MASS1 = 65000, DELAY_MASS2 = 1000;
    static constexpr uint16_t DELAY_PAGE1 = 6000, DELAY_PAGE2 = 100;
    static constexpr uint16_t DELAY_PROG1 = 25, DELAY_PROG2 = 5;
    static constexpr uint16_t DELAY_READ1 = 1, DELAY_READ2 = 1;

  public:
    NuvotonICP(uint8_t dataPin, uint8_t clockPin, uint8_t resetPin)
      : m_dataPin(dataPin), m_clockPin(clockPin), m_resetPin(resetPin), m_id(-1) {}

    uint8_t enter();
    void exit();
    bool locked();

    static constexpr uint8_t config0(bool cbs) { return cbs ? 0xff : 0x7f; }
    static constexpr uint8_t config1(uint16_t ldsize) { return (0xff - ldsize / 1024); }
    static constexpr bool cbs(uint8_t config0) { return !!(config0 & 0x80); }
    static constexpr int16_t ldsize(uint8_t config1)
    { return ((7 - (config1 & 7)) * 1024); }
    static constexpr uint16_t id2pgsize(uint16_t id)
    { return (id != N76E616) ? 128 : 256; }
    static constexpr uint32_t id2size(uint16_t id)
    { return (((uint8_t)id >> 4) <= 4) ? (4096L << ((uint8_t)id >> 4)) : (18 * 1024); }
    static constexpr bool is_equal(uint8_t result)
    { return !!(result & COMPARE_EQUAL); }
    static constexpr bool is_empty_flash(uint8_t result)
    { return !!(result & COMPARE_EMPTY_FLASH); }
    static constexpr bool is_empty_buffer(uint8_t result)
    { return !!(result & COMPARE_EMPTY_BUFFER); }

    uint16_t update_device_id();
    uint16_t device_id() const { return m_id; }
    uint16_t pgsize() const { return id2pgsize(device_id()); }
    uint32_t size() const { return id2size(device_id()); }

    // CONFIG
    void config_erase() { erase(0x30000L); }
    void config_read(uint8_t buffer[5]) { read(0x30000L, buffer, 5); }
    void config_write(uint8_t buffer[5]) { write(0x30000L, buffer, 5); }
    uint8_t config_compare(uint8_t buffer[5]) { return compare(0x30000L, buffer, 5); }
    // FLASH
    void flash_erase(uint16_t address) { erase(0L + address); }
    void flash_read(uint16_t address, uint8_t buffer[], size_t bufsize)
    {
      read(0L + address, buffer, bufsize);
    }
    void flash_write(uint16_t address, uint8_t buffer[], size_t bufsize)
    {
      write(0L + address, buffer, bufsize);
    }
    uint8_t flash_compare(uint16_t address, uint8_t buffer[], size_t bufsize)
    {
      return compare(0L + address, buffer, bufsize);
    }
    // MASS
    void mass_erase();

  protected:
    void pin_float(uint8_t pin)
    { pinMode(pin, INPUT); digitalWrite(pin, LOW); }
    void pin_init(uint8_t pin, uint8_t data=LOW)
    { digitalWrite(pin, data); pinMode(pin, OUTPUT); }
    uint8_t read8() { return shiftIn(m_dataPin, m_clockPin, MSBFIRST); }
    void write8(uint8_t data=0xff) { shiftOut(m_dataPin, m_clockPin, MSBFIRST, data); }
    void write24(uint8_t code, uint32_t param) { write24(param << 6 | code); }
    void write24(uint32_t data);

    void tiktok(uint16_t us1, uint16_t us2);
    uint8_t read9(bool last=false);

    void erase(uint32_t address);
    void read(uint32_t address, uint8_t buffer[], size_t bufsize);
    void write(uint32_t address, uint8_t buffer[], size_t bufsize);
    uint8_t compare(uint32_t address, uint8_t buffer[], size_t bufsize);

  private:
    uint8_t m_dataPin, m_clockPin, m_resetPin;
    uint16_t m_id;
};

#endif // NUVOICP_H