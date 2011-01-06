#ifndef _BREAKPOINTS_H_
#define _BREAKPOINTS_H_

/* Represents a memory address and the data at that address */
typedef struct {
	u32 addr;
	u32 data;
} memory_location;

/* Stores poisition of breakpoint and if it is active */
typedef struct {
	memory_location location;
	u32 active;
} sw_breakpoint;

/* Create our break op code */
u32 SW_BREAK_OP;

/* sw breakpoint list */
#define MAX_SW_BREAKPOINTS 8
sw_breakpoint sw_breakpoints[MAX_SW_BREAKPOINTS];

/* 128 bit GPR */
typedef union {   
	u64 UD[2];
	s64 SD[2];
	u32 UL[4];
	s32 SL[4];
	u16 US[8];
	s16 SS[8];
	u8  UC[16];
	s8  SC[16];
} GPR_reg __attribute((packed));

/* All 32 GPRs */
typedef union {
	struct {
		GPR_reg r0, at, v0, v1, a0, a1, a2, a3,
				t0, t1, t2, t3, t4, t5, t6, t7,
				s0, s1, s2, s3, s4, s5, s6, s7,
				t8, t9, k0, k1, gp, sp, fp, ra;
	} n;
	GPR_reg r[32];
} GPRregs __attribute((packed));

/* All COP0 Registers */
typedef union {
	struct {
		u32	Index,    Random,    EntryLo0,  EntryLo1,
			Context,  PageMask,  Wired,     Reserved0,
			BadVAddr, Count,     EntryHi,   Compare;
		union {
			struct {
				u32 IE:1;
				u32 EXL:1;
				u32 ERL:1;
				u32 KSU:2;
				u32 unused0:3;
				u32 IM:8;
				u32 EIE:1;
				u32 _EDI:1;
				u32 CH:1;
				u32 unused1:3;
				u32 BEV:1;
				u32 DEV:1;
				u32 unused2:2;
				u32 FR:1;
				u32 unused3:1;
				u32 CU:4;
			} bits;
			u32 val;
		} Status;
		
		union {
			struct {
				u32 unused0:2;
				u32 ExcCode:5;
				u32 unused1:3;
				u32 IP1:2;
				u32 SIOP:1;
				u32 unused2:2;
				u32 IP2:1;
				u32 EXC2:3;
				u32 unused4:9;
				u32 CE:2;
				u32 BD2:1;
				u32 BD:1;
			} bits;
			u32 val;
		} Cause;
		u32 EPC,       PRid,
			Config,   LLAddr,    WatchLO,   WatchHI,
			XContext, Reserved1, Reserved2, Debug,
			DEPC,     PerfCnt,   ErrCtl,    CacheErr,
			TagLo,    TagHi,     ErrorEPC,  DESAVE;
	} n;
	u32 r[32];
} CP0regs __attribute((packed));

/* 32bit Float register */
typedef union {
	float f;
	u32 UL;
} FPRreg;

/* All 32/64bit COP1 registers */
typedef struct {
	FPRreg fpr[32];		// 32bit floating point registers
	u32 fprc[2];		// 32bit floating point control registers
	FPRreg ACC;			// 32 bit accumulator 
} fpuRegisters;

/* All preserved registers in exception handler */
typedef struct {
    GPRregs GPR;
	GPR_reg HI;
	GPR_reg LO;	// hi & lo 128bit wide
	u32 SA;		// Shift amount
	CP0regs CP0;
} cpuRegisters __attribute((packed));

/* Storage area for all register in an exception handler */
cpuRegisters cpuRegs;
/* Can be used to send COP1 registers to client */
fpuRegisters fpuRegs;



/* Step into the next instruction */
#define DBG_STEP 0
/* Step over the next instruction */
#define DBG_STEP_OVER 1
/* Break on return from the current function */
#define DBG_RUN_TIL_RETURN 2


/* Break op code - Used to set a software breakpoint */
#define CREATE_BREAK_OP(code) ((code && 0x7FFFF) << 6) | 0x0000000d
/* Max Trap code = 0x7FF */
/* TEG - Trap on rs == rt */
#define CREATE_TEQ(code, rs, rt) (rs << 21) | (rt << 16) | (code << 6) | 0x34
/* TEQI - Trap on rs == imm */
#define CREATE_TEQI(rs, imm) (1 << 26) |  (rs << 21) | (0xC << 16) | imm
/* TGE - Trap on rs >= rt */
#define CREATE_TGE(code, rs, rt) (rs << 21) | (rt << 16) | (code << 6) | 0x30
/* TGEI - Trap on rs >= imm */
#define CREATE_TGEI(rs, imm) (1 << 26) |  (rs << 21) | (0x8 << 16) | imm
/* TGEIU - Trap on rs >= unsigned imm */
#define CREATE_TGEI(rs, imm) (1 << 26) |  (rs << 21) | (0x9 << 16) | imm
/* TGEU - Trap on rs >= rt rs and rt as unsigned ints */
#define CREATE_TGEU(code, rs, rt) (rs << 21) | (rt << 16) | (code << 6) | 0x31

/* TLT - Trap on rs < rt */
#define CREATE_TLT(code, rs, rt) (rs << 21) | (rt << 16) | (code << 6) | 0x32
/* TLTI - Trap on rs < imm */
#define CREATE_TLTI(rs, imm) (1 << 26) |  (rs << 21) | (0xA << 16) | imm
/* TLTIU - Trap on rs < imm unsigned */
#define CREATE_TLTI(rs, imm) (1 << 26) |  (rs << 21) | (0xB << 16) | imm
/* TLTU - Trap on rs < rt unsigned */
#define CREATE_TLTU(code, rs, rt) (rs << 21) | (rt << 16) | (code << 6) | 0x33

/* TNE - Trap on rs != rt */
#define CREATE_TLT(code, rs, rt) (rs << 21) | (rt << 16) | (code << 6) | 0x36
/* TNEI - Trap on rs != imm */
#define CREATE_TLTI(rs, imm) (1 << 26) |  (rs << 21) | (0xE << 16) | imm

/* NOP */
#define NOP_OP		0x00000000;

#define _Funct(instr)  ((instr      ) & 0x3F)  // The funct part of the instruction register 
#define _Rd(instr)     ((instr >> 11) & 0x1F)  // The rd part of the instruction register 
#define _Rt(instr)     ((instr >> 16) & 0x1F)  // The rt part of the instruction register 
#define _Rs(instr)     ((instr >> 21) & 0x1F)  // The rs part of the instruction register 
#define _Sa(instr)     ((instr >>  6) & 0x1F)  // The sa part of the instruction register
#define _Im(instr)     ((u16)instr)			// The immediate part of the instruction register
#define _Target(instr) (instr & 0x03ffffff)    // The target part of the instruction register

#define _Imm(instr)	((s16)instr) // sign-extended immediate
#define _ImmU(instr)	(instr&0xffff) // zero-extended immediate

/* From SIO */
/* Define branch and jump function codes*/
#define BEQ_OPCODE              0x4
#define BEQL_OPCODE             0x14
#define BGTZ_OPCODE     		0x7
#define BGTZL_OPCODE    		0x17
#define BLEZ_OPCODE             0x6
#define BLEZL_OPCODE    		0x16
#define BNE_OPCODE              0x5
#define BNEL_OPCODE             0x15

/* Reg Imm */
#define REGIMM_OPCODE   		0x1
#define BGEZ_OPCODE             0x1
#define BGEZAL_OPCODE   		0x11
#define BGEZALL_OPCODE  		0x13
#define BGEZL_OPCODE    		0x3
#define BLTZ_OPCODE             0
#define BLTZAL_OPCODE   		0x10
#define BLTZALL_OPCODE  		0x12
#define BLTZL_OPCODE    		0x2

#define J_OPCODE                0x2
#define JAL_OPCODE              0x3

/* Special opcode */
#define SPECIAL_OPCODE  		0
#define JALR_OPCODE             0x9
#define JR_OPCODE               0x8

/* Cop Branches (all the same) */
#define COP0_OPCODE             0x10
#define COP1_OPCODE             0x11
#define COP2_OPCODE             0x12
#define BCXF_OPCODE             0x100
#define BCXFL_OPCODE    		0x102
#define BCXT_OPCODE             0x101
#define BCXTL_OPCODE    		0x103

#endif