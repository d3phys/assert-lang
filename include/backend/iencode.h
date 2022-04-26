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



/* Some X86-64 Instruction Encodings 
   Note the reverse order due to little endian */

/*
struct ie64_call {
        ubyte opcode = 0xe8;
        imm32 imm    = 0;      
} __attribute__((packed));

struct ie64_ret {
        ubyte opcode = 0xc3;     
} __attribute__((packed));

struct ie64_push {
        ubyte opcode = 0x50;     
} __attribute__((packed));

struct ie64_pop {
        ubyte opcode = 0x58;     
} __attribute__((packed));
*/

/* mov asm instructions' encodings 
   movxy - move x to y (att syntax) */

/*
struct ie64_movabs {
        const ubyte rex = 0x48;
        ubyte opcode    = 0xb8;  
        imm64 imm       = 0;   
} __attribute__((packed));

struct ie64_movrr {
        const ubyte rex    = 0x48;
        const ubyte opcode = 0x89;
        ie64_modrm modrm   = { .mod = 0b11, .reg = 0b000, .rm = 0b000 };
} __attribute__((packed));

struct ie64_movrm {
        const ubyte rex    = 0x48;
        const ubyte opcode = 0x89;
        
        ie64_modrm modrm = { .byte = 0x00 };
        ie64_sib sib     = { .byte = 0x00 };  
        imm32 imm        = 0;   
} __attribute__((packed));

struct ie64_movmr {
        const ubyte rex    = 0x48;
        const ubyte opcode = 0x89;
        
        ie64_modrm modrm = { .byte = 0x32 };
        ie64_sib sib     = { .byte = 0x12 };  
        imm32 imm        = 0;   
} __attribute__((packed));

struct ie64_movmr {
        const ubyte rex    = 0x48;
        const ubyte opcode = 0x89;
        
        ie64_modrm modrm = { .byte = 0x32 };
        ie64_sib sib     = { .byte = 0x12 };  
        imm32 imm        = 0;   
} __attribute__((packed));



ie64_mov_rr rsp2rbp = {};
rsp2rbp.modrm.reg = IE64_RSP;
rsp2rbp.modrm.rm  = IE64_RBP;
encode(vm, &rsp2rbp, sizeof(ie64_mov_rr));

*/

#endif /* IENCODE_H */
