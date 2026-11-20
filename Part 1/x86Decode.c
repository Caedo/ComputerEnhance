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

#if defined(_MSC_VER)
# define Trap() __debugbreak()
#elif defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
# define Trap() __builtin_trap()
#else
# error Unknown trap intrinsic for this compiler.
#endif

#define AssertAlways(x) do{if(!(x)) {Trap();}}while(0)
#if BUILD_DEBUG
# define Assert(x) AssertAlways(x)
#else
# define Assert(x) (void)(x)
#endif
#define InvalidPath        AssertAlways(!"Invalid Path!")
#define NotImplemented     AssertAlways(!"Not Implemented!")

char* RegisterNames[] = { 
    "INVALID",
    "al", "cl", "dl", "bl",
    "ah", "ch", "dh", "bh", 
    "ax", "cx", "dx", "bx",
    "sp", "bp", "si", "di"
};

typedef enum Reg_t {
    Reg_Invalid, 
    Reg_AL, Reg_CL, Reg_DL, Reg_BL,
    Reg_AH, Reg_CH, Reg_DH, Reg_BH,
    Reg_AX, Reg_CX, Reg_DX, Reg_BX,
    Reg_SP, Reg_BP, Reg_SI, Reg_DI,
    Reg_COUNT 
} Reg;
typedef enum RegSide_t { Side_Lower, Side_Higher } RegSide;

Reg RegisterAccessTable[2][8] = {
    {Reg_AL, Reg_CL, Reg_DL, Reg_BL, Reg_AH, Reg_CH, Reg_DH, Reg_BH },
    {Reg_AX, Reg_CX, Reg_DX, Reg_BX, Reg_SP, Reg_BP, Reg_SI, Reg_DI }
};

typedef struct RegisterAccess_T {
    Reg reg;
} RegisterAccess;

char* RegisterName(RegisterAccess reg) {
    return RegisterNames[reg.reg];
}

char* EffectiveAddressNames[] = {
    "INVALID",
    "bx + si", "bx + di", "bp + si", "bp + di",
    "si", "di", "bp", "bx"
};

typedef enum EffectiveAddressType_t {
    EA_Invalid,
    EA_BXSI, EA_BXDI, EA_BPSI, EA_BPDI, EA_SI, EA_DI, EA_BP, EA_BX, EA_COUNT
} EffectiveAddressType;

typedef struct Address_t {
    EffectiveAddressType type;
    u16 displacement;
} Address;

void PrintAdress(Address ad) {
    if(ad.displacement == 0) {
        printf("[%s]", EffectiveAddressNames[ad.type]);
    }
    else {
        printf("[%s + %hu]", EffectiveAddressNames[ad.type], ad.displacement);
    }
}

typedef struct Immedate_t {
    u8 explicit;
    u8 wide;
    u16 value;
} ImmediateValue;

typedef enum OperandType_t {
    OpT_Invalid,
    OpT_Immediate,
    OpT_Register,
    OpT_EffectiveAddress,
    OpT_DirectAddress,
} OperandType;

typedef struct Operand_t {
    OperandType type;
    union {
        u16 direct;
        ImmediateValue immediate;
        RegisterAccess reg;
        Address address;
    };
} Operand;

typedef enum InstructionMnem_t {
    Inst_Invalid,
    Inst_Mov,

    Inst_Add,
    Inst_Sub,
    Inst_Cmp,

    Inst_Jnz,
    Inst_Je,
    Inst_Jl,
    Inst_Jle,
    Inst_Jb,
    Inst_Jbe,
    Inst_Jp,
    Inst_Jo,
    Inst_Js,
    Inst_Jne,
    Inst_Jnl,
    Inst_Jg,
    Inst_Jnb,
    Inst_Ja,
    Inst_Jnp,
    Inst_Jno,
    Inst_Jns,
    Inst_Loop,
    Inst_Loopz,
    Inst_Loopnz,
    Inst_Jcxz,


    Inst_COUNT
} InstructionMnem;

char* InstructionName[] = {
    "INVALID",
    "mov",
    "add",
    "sub",
    "cmp",

    "jnz",
    "je",
    "jl",
    "jle",
    "jb",
    "jbe",
    "jp",
    "jo",
    "js",
    "jne",
    "jnl",
    "jg",
    "jnb",
    "jg",
    "jnp",
    "jno",
    "jns",
    "loop",
    "loopz",
    "loopnz",
    "jcxz",
};

typedef struct Instruction_t {
    InstructionMnem inst;
    Operand operands[2];
} Instruction;

void PrintInstruction(Instruction inst) {
    printf("%s ", InstructionName[inst.inst]);
    for(int i = 0; i < 2; i++) {
        Operand op = inst.operands[i];

        if(op.type == OpT_Invalid) {
            break;
        }

        if(i != 0) {
            printf(", ");
        }

        switch(op.type) {
        case OpT_Immediate:
            char* explicitSize = "";
            if(op.immediate.explicit) {
                explicitSize = op.immediate.wide ? "word " : "byte ";
            }
            printf("%s%hu", explicitSize, op.immediate.value);
        break;

        case OpT_Register:         printf("%s", RegisterName(op.reg)); break;
        case OpT_EffectiveAddress: PrintAdress(op.address);            break;
        case OpT_DirectAddress:    printf("[%hu]", op.direct);          break;
        
        default: Assert(!"Not supported operand type");
        }
    }
    printf("\n");
}

typedef struct Reader_T {
    u8* bytes;
    long size;
    long pos;
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
    if(r->pos + 1 > r->size) {
        return 0;
    }

    return r->bytes[r->pos++];
}

u16 ReaderNextWord(Reader* r, u8 wide) {
    int size = wide == 1 ? 2 : 1;
    if(r->pos + size > r->size) {
        return 0;
    }

    u16 ret = r->bytes[r->pos++];
    if(wide) {
        u16 high = r->bytes[r->pos++];
        ret = ret | (high << 8);
    }

    return ret;
}

int ReaderIsEmpty(Reader* r) {
    return r->pos >= r->size;
}

void PrintBinary(u16 num) {
    for (int i = sizeof(u16) * 8 - 1; i >= 0; i--) {
        printf("%d", (num >> i) & 1);
    }
}

Operand ParseEffectiveAddress(Reader* reader, u8 mod, u8 rm, u8 w) {
    Operand ret = {0};
    if(mod == 0b00) {
        // Direct Address
        if(rm == 0b110) {
            u16 address = ReaderNextWord(reader, 1);

            ret.type = OpT_DirectAddress;
            ret.direct = address;
        }
        else {
            ret.type = OpT_EffectiveAddress;
            ret.address.type = rm + 1;
        }
    }
    else if(mod == 0b01 || mod == 0b10) {
        u16 disp = ReaderNextWord(reader, mod == 0b10);

        ret.type = OpT_EffectiveAddress;
        ret.address.type = rm + 1;
        ret.address.displacement = disp;
    }
    else if(mod == 0b11) {
        ret.type = OpT_Register;
        ret.reg.reg = RegisterAccessTable[w][rm];
    }

    return ret;
}

typedef union InstMods_t {
    u8 byte;
    struct {
        u8 rm: 3;
        u8 reg: 3;
        u8 mod: 2;
    };
} InstMods;

Instruction ParseNextInstruction(Reader* reader) {
    Instruction ret = {0};

    u8 byte = ReaderNextByte(reader);
    // register/memory move
    if((byte & 0b11111100) == 0b10001000) {
        ret.inst = Inst_Mov;

        u8 w = (byte & 0b1);
        u8 d = ((byte >> 1) & 0b1);

        InstMods mods = {0};
        mods.byte = ReaderNextByte(reader);

        ret.operands[0] = ParseEffectiveAddress(reader, mods.mod, mods.rm, w);
        ret.operands[1].type = OpT_Register;
        ret.operands[1].reg.reg = RegisterAccessTable[w][mods.reg];

        if(d == 1) {
            Operand tmp = ret.operands[0];
            ret.operands[0] = ret.operands[1];
            ret.operands[1] = tmp;
        }
    }
    // immdiate to register/memory
    else if((byte & 0b11111110) == 0b11000110) {
        ret.inst = Inst_Mov;

        u8 w = (byte & 0b1);
        byte = ReaderNextByte(reader);
        u8 rm  = (byte & 0b111);
        u8 mod = ((byte >> 6) & 0b11);

        ret.operands[0] = ParseEffectiveAddress(reader, mod, rm, w);

        u16 data = ReaderNextWord(reader, w == 1);
        ret.operands[1].type = OpT_Immediate;
        ret.operands[1].immediate.explicit = 1;
        ret.operands[1].immediate.wide = w == 1;
        ret.operands[1].immediate.value = data;
    }
    // immediate to register
    else if((byte & 0b11110000) == 0b10110000) {
        ret.inst = Inst_Mov;

        u8 reg = (byte & 0b111);
        u8 w = ((byte >> 3) & 0b1);

        u16 data = ReaderNextWord(reader, w == 1);

        ret.operands[0].type = OpT_Register;
        ret.operands[0].reg.reg = RegisterAccessTable[w][reg];

        ret.operands[1].type = OpT_Immediate;
        ret.operands[1].immediate.value = data;
    }
    // Mov Direct address to accumulator
    else if((byte & 0b11111100) == 0b10100000) {
        ret.inst = Inst_Mov;

        u8 w = (byte & 0b1);
        u8 toMemory = ((byte >> 1) & 0b1);

        u16 address = ReaderNextWord(reader, w == 1);

        ret.operands[0].type = OpT_Register;
        ret.operands[0].reg.reg = Reg_AX;

        ret.operands[1].type = OpT_DirectAddress;
        ret.operands[1].direct = address;

        if(toMemory) {
            Operand tmp = ret.operands[0];
            ret.operands[0] = ret.operands[1];
            ret.operands[1] = tmp;
        }
    }
    // Add reg/mem with register to either
    else if((byte & 0b11111100) == 0b00000000 ||
            (byte & 0b11111100) == 0b00101000 ||
            (byte & 0b11111100) == 0b00111000 )
    {
        if((byte & 0b00111000) == 0b00000000) {
            ret.inst = Inst_Add;
        }
        else if((byte & 0b00111000) == 0b00101000) {
            ret.inst = Inst_Sub;
        }
        else if((byte & 0b00111000) == 0b00111000) {
            ret.inst = Inst_Cmp;
        }

        u8 w = (byte & 0b1);
        u8 d = ((byte >> 1) & 0b1);

        InstMods mods = {0};
        mods.byte = ReaderNextByte(reader);

        ret.operands[0] = ParseEffectiveAddress(reader, mods.mod, mods.rm, w);
        ret.operands[1].type = OpT_Register;
        ret.operands[1].reg.reg = RegisterAccessTable[w][mods.reg];

        if(d == 1) {
            Operand tmp = ret.operands[0];
            ret.operands[0] = ret.operands[1];
            ret.operands[1] = tmp;
        }
    }
    // Add Immediate to register/memory
    else if((byte & 0b11111100) == 0b10000000) {
        u8 w = (byte & 0b1);
        u8 s = ((byte >> 1) & 0b1);

        byte = ReaderNextByte(reader);
        u8 rm  = (byte & 0b111);
        u8 inst  = ((byte >> 3) & 0b111);
        u8 mod = ((byte >> 6) & 0b11);

        if(inst == 0b000) {
            ret.inst = Inst_Add;
        }
        else if(inst == 0b101) {
            ret.inst = Inst_Sub;
        }
        else if(inst == 0b111) {
            ret.inst = Inst_Cmp;
        }

        ret.operands[0] = ParseEffectiveAddress(reader, mod, rm, w);

        u16 data = ReaderNextWord(reader, s == 0 && w == 1);
        ret.operands[1].type = OpT_Immediate;
        ret.operands[1].immediate.explicit = mod != 0b11;
        ret.operands[1].immediate.wide     = s == 1; // s indicates sign extension
        ret.operands[1].immediate.value    = data;
    }
    // Add immediate to accumulator
    else if((byte & 0b11111110) == 0b00000100 ||
            (byte & 0b11111110) == 0b00101100 ||
            (byte & 0b11111110) == 0b00111100 )
    {
        if((byte & 0b00111000) == 0b00000000) {
            ret.inst = Inst_Add;
        }
        else if((byte & 0b00111000) == 0b00101000) {
            ret.inst = Inst_Sub;
        }
        else if((byte & 0b00111000) == 0b00111000) {
            ret.inst = Inst_Cmp;
        }

        u8 w = (byte & 0b1);
        u16 value = ReaderNextWord(reader, w == 1);

        ret.operands[0].type = OpT_Register;
        ret.operands[0].reg.reg = Reg_AX;

        ret.operands[1].type = OpT_Immediate;
        ret.operands[1].immediate.value = value;
    }
    // jumps...
    else if(byte == 0b01110100) {
        ret.inst = Inst_Je;
        ret.operands[0].type = OpT_Immediate;
        ret.operands[0].immediate.value = ReaderNextByte(reader);
    }
    else if(byte == 0b01111100) {
        ret.inst = Inst_Jl;
        ret.operands[0].type = OpT_Immediate;
        ret.operands[0].immediate.value = ReaderNextByte(reader);
    }
    else if(byte == 0b01111110) {
        ret.inst = Inst_Jle;
        ret.operands[0].type = OpT_Immediate;
        ret.operands[0].immediate.value = ReaderNextByte(reader);
    }

    else if(byte == 0b01110010) {
        ret.inst = Inst_Jb;
        ret.operands[0].type = OpT_Immediate;
        ret.operands[0].immediate.value = ReaderNextByte(reader);
    }

    else if(byte == 0b01110110) {
        ret.inst = Inst_Jbe;
        ret.operands[0].type = OpT_Immediate;
        ret.operands[0].immediate.value = ReaderNextByte(reader);
    }

    else if(byte == 0b01111010) {
        ret.inst = Inst_Jp;
        ret.operands[0].type = OpT_Immediate;
        ret.operands[0].immediate.value = ReaderNextByte(reader);
    }
    else if(byte == 0b01110000) {
        ret.inst = Inst_Jo;
        ret.operands[0].type = OpT_Immediate;
        ret.operands[0].immediate.value = ReaderNextByte(reader);
    }
    else if(byte == 0b01111000) {
        ret.inst = Inst_Js;
        ret.operands[0].type = OpT_Immediate;
        ret.operands[0].immediate.value = ReaderNextByte(reader);
    }
    else if(byte == 0b01110101) {
        ret.inst = Inst_Jne;
        ret.operands[0].type = OpT_Immediate;
        ret.operands[0].immediate.value = ReaderNextByte(reader);
    }
    else if(byte == 0b01111101) {
        ret.inst = Inst_Jnl;
        ret.operands[0].type = OpT_Immediate;
        ret.operands[0].immediate.value = ReaderNextByte(reader);
    }
    else if(byte == 0b01111111) {
        ret.inst = Inst_Jg;
        ret.operands[0].type = OpT_Immediate;
        ret.operands[0].immediate.value = ReaderNextByte(reader);
    }
    else if(byte == 0b01110011) {
        ret.inst = Inst_Jnb;
        ret.operands[0].type = OpT_Immediate;
        ret.operands[0].immediate.value = ReaderNextByte(reader);
    }
    else if(byte == 0b01110111) {
        ret.inst = Inst_Ja;
        ret.operands[0].type = OpT_Immediate;
        ret.operands[0].immediate.value = ReaderNextByte(reader);
    }
    else if(byte == 0b01111011) {
        ret.inst = Inst_Jnp;
        ret.operands[0].type = OpT_Immediate;
        ret.operands[0].immediate.value = ReaderNextByte(reader);
    }
    else if(byte == 0b01110001) {
        ret.inst = Inst_Jno;
        ret.operands[0].type = OpT_Immediate;
        ret.operands[0].immediate.value = ReaderNextByte(reader);
    }
    else if(byte == 0b01111001) {
        ret.inst = Inst_Jns;
        ret.operands[0].type = OpT_Immediate;
        ret.operands[0].immediate.value = ReaderNextByte(reader);
    }
    else if(byte == 0b11100010) {
        ret.inst = Inst_Loop;
        ret.operands[0].type = OpT_Immediate;
        ret.operands[0].immediate.value = ReaderNextByte(reader);
    }
    else if(byte == 0b11100001) {
        ret.inst = Inst_Loopz;
        ret.operands[0].type = OpT_Immediate;
        ret.operands[0].immediate.value = ReaderNextByte(reader);
    }
    else if(byte == 0b11100000) {
        ret.inst = Inst_Loopnz;
        ret.operands[0].type = OpT_Immediate;
        ret.operands[0].immediate.value = ReaderNextByte(reader);
    }
    else if(byte == 0b11100011) {
        ret.inst = Inst_Jcxz;
        ret.operands[0].type = OpT_Immediate;
        ret.operands[0].immediate.value = ReaderNextByte(reader);
    }

    return ret;
}

int main(int argc, char* argv[]) {
    if(argc != 2) {
        return 1;
    }

    FILE* file = fopen(argv[1], "rb");
    if(file == NULL) {
        printf("Can't find the file");
        return 1;
    }

    Reader reader = CreateReader(file);
    fclose(file);

    Instruction inst;
    do {
        inst = ParseNextInstruction(&reader);
        PrintInstruction(inst);
    } while(ReaderIsEmpty(&reader) == 0 && inst.inst != OpT_Invalid);

    return 0;
}
