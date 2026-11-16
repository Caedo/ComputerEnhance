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
        u16 value;
        RegisterAccess reg;
        Address address;
    };
} Operand;

typedef enum InstructionMnem_t {
    Inst_Invalid,
    Inst_Mov,

    Inst_COUNT
} InstructionMnem;

char* InstructionName[] = {
    "INVALID",
    "mov"
};

typedef struct Instruction_t {
    InstructionMnem inst;
    Operand operands[2];
} Instruction;

void PrintInstruction(Instruction inst) {
    printf("%s ", InstructionName[inst.inst]);
    for(int i = 0; i < 2; i++) {
        Operand op = inst.operands[i];
        if(i != 0 && op.type != OpT_Invalid) {
            printf(", ");
        }

        switch(op.type) {
        case OpT_Invalid:          printf("{INVALID OPERAND}");        break;
        case OpT_Immediate:        printf("%hu", op.value);            break;
        case OpT_Register:         printf("%s", RegisterName(op.reg)); break;
        case OpT_EffectiveAddress: PrintAdress(op.address);            break;
        case OpT_DirectAddress:    printf("[%hu]", op.value);          break;
        default: Assert(!"Not supported operand type");
        }
    }
    printf("\n");
}

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

Instruction ParseNextInstruction(Reader* reader) {
    Instruction ret = {0};

    u8 byte = ReaderNextByte(reader);
    // register/memory move
    if((byte & 0b11111100) == 0b10001000) {
        ret.inst = Inst_Mov;

        u8 w = (byte & 0b1);
        u8 d = ((byte >> 1) & 0b1);

        byte = ReaderNextByte(reader);

        u8 rm  = (byte & 0b111);
        u8 reg = ((byte >> 3) & 0b111);
        u8 mod = ((byte >> 6) & 0b11);

        Operand dest;
        if(mod == 0b00) {
            // Direct Address
            if(rm == 0b110) {
                u32 disp = ReaderNextByte(reader);
                u8 dispHigh = ReaderNextByte(reader);
                disp = disp | (((u32)dispHigh) << 8);

                ret.operands[0].type = OpT_DirectAddress;
                ret.operands[0].value = disp;
            }
            else {
                ret.operands[0].type = OpT_EffectiveAddress;
                ret.operands[0].address.type = rm + 1;
            }
        }
        else if(mod == 0b01 || mod == 0b10) {
            u32 disp = ReaderNextByte(reader);
            if(mod == 0b10) {
                u8 dispHigh = ReaderNextByte(reader);
                disp = disp | (((u32)dispHigh) << 8);
            }

            ret.operands[0].type = OpT_EffectiveAddress;
            ret.operands[0].address.type = rm + 1;
            ret.operands[0].address.displacement = disp;
        }
        else if(mod == 0b11) {
            ret.operands[0].type = OpT_Register;
            ret.operands[0].reg.reg = RegisterAccessTable[w][rm];
        }

        ret.operands[1].type = OpT_Register;
        ret.operands[1].reg.reg = RegisterAccessTable[w][reg];

        if(d == 1) {
            Operand tmp = ret.operands[0];
            ret.operands[0] = ret.operands[1];
            ret.operands[1] = tmp;
        }
    }
    // immediate to register
    else if((byte & 0b11110000) == 0b10110000) {
        u8 reg = (byte & 0b111);
        u8 w = ((byte >> 3) & 0b1);

        u16 data = (u16) ReaderNextByte(reader);
        if(w == 1) {
            u16 dataHigh = (u16) ReaderNextByte(reader);
            data = data | (dataHigh << 8);
        }

        ret.inst = Inst_Mov;
        ret.operands[0].type = OpT_Register;
        ret.operands[0].reg.reg = RegisterAccessTable[w][reg];

        ret.operands[1].type = OpT_Immediate;
        ret.operands[1].value = data;
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
    } while(ReaderIsEmpty(&reader) == false && inst.inst != OpT_Invalid);

    return 0;
}
