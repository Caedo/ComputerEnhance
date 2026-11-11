#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

char* WordRegisterNames[] = {
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di"
};

char* ByteRegisterNames[] = {
    "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"
};

char* EffectiveAddressNames[] {
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

char ReaderNextByte(Reader* r) {
    if(r->pos >= r->size) {
        return 0;
    }

    return r->bytes[r->pos++];
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

    char byte;
    while(byte = ReaderNextByte(&reader)) {
        if((byte & 0b11111100) == 0b10001000) {
            char w = (byte & 0b1);
            char d = ((byte >> 1) & 0b1);

            byte = ReaderNextByte(&reader);

            char rm  = (byte & 0b111);
            char reg = ((byte >> 3) & 0b111);
            char mod = ((byte >> 6) & 0b11);

            // printf("mov\n");
            // printf("d = %i\n", (int) d);
            // printf("w = %i\n", (int) w);
            // printf("mod = %i\n", (int) mod);
            // printf("reg = %i\n", (int) reg);
            // printf("rm = %i\n\n", (int) rm);

            char** regiterNames = w == 1 ? WordRegisterNames : ByteRegisterNames;

            switch(mod) {
            // Memory mode, no displacement
            case 0b00: {
                printf("Mod 00 not supported\n");
            }
            break;
            
            // Memory mode, 8 - bit displacement
            case 0b01: {
                printf("Mod 01 not supported\n");
            }
            break;

            // Memory mode, 16 - bit displacement
            case 0b10: {
                printf("Mod 10 not supported\n");
            }
            break;

            // Register mode
            case 0b011: {
                char src  = d == 0 ? reg : rm;
                char dest = d == 0 ? rm  : reg;

                printf("mov %s, %s\n", regiterNames[dest], regiterNames[src]);
            }
            break;
            }
        }

    }

    return 0;
}