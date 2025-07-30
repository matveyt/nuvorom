#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if !defined(_countof)
#define _countof(o) (sizeof(o) / sizeof(o[0]))
#endif // _countof

int main(int argc, char* argv[])
{
    size_t addr = 0;
    const size_t reclen = 8;
    uint8_t kb[1024];

    if (argc != 2) {
        fprintf(stderr, "File argument required\n");
        return 1;
    }
    FILE* rom = fopen(argv[1], "rb");
    if (!rom) {
        fprintf(stderr, "%s not found\n", argv[1]);
        return 1;
    }

    printf("const uint8_t rom[] PROGMEM = {\n");

    size_t count;
    while ((count = fread(kb, sizeof(kb[0]), _countof(kb), rom)) > 0) {
        if (count < _countof(kb))
            memset(&kb[count], 0xff, sizeof(kb) - count * sizeof(kb[0]));

        for (size_t i = 0; i < _countof(kb) / reclen; ++i) {
            printf("    ");
            for (size_t j = 0; j < reclen; ++j)
                printf("0x%02x, ", kb[reclen * i + j]);
            printf("    // %03x\n", addr);
            addr += reclen;
        }
    }

    printf("};");
    return 0;
}

// $ tcc -run rom2c.c bootloader-N76E003.bin >rom.h
