#ifndef IENCODE_H
#define IENCODE_H

#include <stdint.h>

typedef uint8_t  imm8;
typedef uint16_t imm16;
typedef uint32_t imm32;
typedef uint64_t imm64;

typedef unsigned char ubyte; 

enum ie64_register_encoding {
        IE64_RAX = 0x00,
        IE64_RCX = 0x01,
        IE64_RDX = 0x02,
        IE64_RBX = 0x03,
        IE64_RSP = 0x04,
        IE64_RBP = 0x05,
        IE64_RSI = 0x06,
        IE64_RDI = 0x07,
        IE64_R8  = 0x08,
        IE64_R9  = 0x09,
        IE64_R10 = 0x0a,
        IE64_R11 = 0x0b,
        IE64_R12 = 0x0c,
        IE64_R13 = 0x0d,
        IE64_R14 = 0x0e,
        IE64_R15 = 0x0f,      
};

/* Some X86-64 Instruction Encoding bytes 
   Note the reverse order due to the little endian */

union ie64_rex {
        struct { ubyte b:1, x:1, r:1, w:1, fixed:4; }; 
        ubyte byte;      
};

union ie64_modrm { 
        struct { ubyte rm:3, reg:3, mod:2; };       
        ubyte byte;
};

union ie64_sib {
        struct { ubyte base:3, index:3, scale:2; };
        ubyte byte;
};

#endif /* IENCODE_H */
