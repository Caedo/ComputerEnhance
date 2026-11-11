#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>


typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

char* WordRegisterNames[] = {
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di"
};

char* ByteRegisterNames[] = {
    "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"
};

char* EffectiveAddressNames[] = {
    "bx + si",
    "bx + di",
    "bp + si",
    "bp + di",
    "si",
    "di",
    "bp",
    "bx"
};

typedef struct Reader_T {
    char* bytes;
    long size;
    int pos;
} Reader;

Reader CreateReader(FILE* file) {
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* bytes = (char*) malloc(size);
    fread(bytes, size, 1, file);

    Reader ret = {
        .bytes = bytes,
        .size = size,
        .pos = 0
    };

    return ret;
}

u8 ReaderNextByte(Reader* r) {
    if(r->pos >= r->size) {
        return 0;
    }

    return r->bytes[r->pos++];
}

int ReaderIsEmpty(Reader* r) {
    return r->pos >= r->size;
}

void PrintBinary(u8 num) {
    for (int i = sizeof(u8) * 8 - 1; i >= 0; i--) {
        printf("%d", (num >> i) & 1);
    }
}

int main(int argc, char* argv[]) {
    if(argc != 2) {
        // print usage
        return 1;
    }

    FILE* file = fopen(argv[1], "rb");
    if(file == NULL) {
        printf("Can't find the file");
        return 1;
    }

    Reader reader = CreateReader(file);
    fclose(file);

    while(ReaderIsEmpty(&reader) == 0) {
        u8 byte = ReaderNextByte(&reader);

        // register/memory move
        if((byte & 0b11111100) == 0b10001000) {
            u8 w = (byte & 0b1);
            u8 d = ((byte >> 1) & 0b1);

            byte = ReaderNextByte(&reader);

            u8 rm  = (byte & 0b111);
            u8 reg = ((byte >> 3) & 0b111);
            u8 mod = ((byte >> 6) & 0b11);

            char** registerNames = w == 1 ? WordRegisterNames : ByteRegisterNames;

            switch(mod) {
            // Memory mode, no displacement
            case 0b00: {
                // Direct Address
                if(rm == 0b110) {
                    u32 disp = ReaderNextByte(&reader);
                    u8 dispHigh = ReaderNextByte(&reader);
                    disp = disp | (((u32)dispHigh) << 8);

                    printf("mov %s, [%u]\n", registerNames[reg], disp);
                }
                else {
                    printf("mov %s, [%s]\n",registerNames[reg], EffectiveAddressNames[rm]);
                }
            }
            break;

            // Memory mode, 8/16 - bit displacement
            case 0b01:
            case 0b10: {
                u32 disp = ReaderNextByte(&reader);
                if(mod == 0b10) {
                    u8 dispHigh = ReaderNextByte(&reader);
                    disp = disp | (((u32)dispHigh) << 8);
                }

                if(d == 1) {
                    if(disp == 0) {
                        printf("mov %s, [%s]\n", registerNames[reg], EffectiveAddressNames[rm]);
                    }
                    else {
                        printf("mov %s, [%s + %u]\n", registerNames[reg], EffectiveAddressNames[rm], disp);
                    }
                }
                else {
                    if(disp == 0) {
                        printf("mov [%s], %s\n", EffectiveAddressNames[rm], registerNames[reg]);
                    }
                    else {
                        printf("mov [%s + %u], %s\n", EffectiveAddressNames[rm], disp, registerNames[reg]);
                    }
                }
            }
            break;

            // Register mode
            case 0b011: {
                u8 src  = d == 0 ? reg : rm;
                u8 dest = d == 0 ? rm  : reg;

                printf("mov %s, %s\n", registerNames[dest], registerNames[src]);
            }
            break;
            }
        }
        // immediate to register/memory mov
        // else if((byte & 0b11111110) == 0b11000110) {
        //     u8 w = (byte & 0b1);

        //     byte = ReaderNextByte(&reader);

        //     u8 rm  = (byte & 0b111);
        //     u8 mod = ((byte >> 6) & 0b11);

        // }
        // immediate to register
        else if((byte & 0b11110000) == 0b10110000) {
            u8 reg = (byte & 0b111);
            u8 w = ((byte >> 3) & 0b1);

            u32 data = (u32) ReaderNextByte(&reader);
            if(w == 1) {
                u32 dataHigh = (u32) ReaderNextByte(&reader);
                data = data | (dataHigh << 8);
            }

            char** registerNames = w == 1 ? WordRegisterNames : ByteRegisterNames;
            printf("mov, %s, %u\n", registerNames[reg], data);
        }
        else {
            printf("Opcode: ");
            PrintBinary(byte);
            printf(" Not supported\n");
        }
    }

    return 0;
}
