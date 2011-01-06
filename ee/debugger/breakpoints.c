#include <tamtypes.h>
#include <kernel.h>
#include <iopheap.h>
#include <iopcontrol.h>
#include <loadfile.h>
#include <sifdma.h>
#include <sifrpc.h>
#include <string.h>
#include <syscallnr.h>
#include <io_common.h>
#include <fileio.h>
#include <sbv_patches.h>
#include <debug.h>
#include "breakpoints.h"

/* 
 * haltExecutionAndWait: this function waits for a resume instruction before continueing execution
 */
static void haltExecutionAndWait(void)
{
	//Wait for a command from client before continuing
	haltState = 1;
	waitForClientInput = 1;
	do
	{
		execRemoteCmd();
	}while(haltState);
	//We have received a command and the command wants to resume execution
}

/*
 * conditionalBranchParse: this tests for BEQ BEQL BGTZ BGTZL BLEZ BLEZL BNE BNEL branches.
 */
static u32 conditionalBranchParse(u32 instr, u32* target u32* conditionMet)
{
	u32 rs, rt;
	rs = _RS(instr);
	rt = _RT(instr);
	
	//64 bit comparison
	//condition <- (GPR[rs] 63..0 == GPR[rt] 63..0)
	
	switch(_Funct(instr))
	{
		case BEQ_OPCODE:
		case BEQL_OPCODE:
			*conditionMat = (cpuRegs.GPR.r[rs].SD[0] == cpuRegs.GPR.r[rt].SD[0]);
			break;
		case BGTZ_OPCODE:
		case BGTZL_OPCODE:
			*conditionMet = (cpuRegs.GPR.r[rs].SD[0] > 0);
			break;
		case BLEZ_OPCODE:
		case BLEZL_OPCODE:
			*conditionMet = (cpuRegs.GPR.r[rs].SD[0] <= 0);
			break;
		case BNE_OPCODE:
		case BNEL_OPCODE:
			*conditionMet =  (cpuRegs.GPR.r[rs].SD[0] != rt);
			break;
		default:
			return 0;
	}
	
	short ofs;
	ofs = (short) (instr & 0xffff);
	*target += ofs * 4;
	
	return 1;
}

/*
 * conditionalBranchREGIMMParse: this tests for BGEZ BGEZL BGEZAL BGEZALL BLTZ BLTZL BLTZAL BLTZALL branches.
 */
static u32 conditionalBranchREGIMMParse(u32 instr, u32* link, u32* target u32* conditionMet)
{
	u32 rs, rt;
	rs = _RS(instr);
	rt = _RT(instr);
	
	//64 bit comparison
	//condition <- (GPR[rs] 63..0 == GPR[rt] 63..0)
	
	switch(_Funct(instr))
	{
		case REGIMM_OPCODE:
			{
				switch((instr >> 16) & 0x1f)
				{
					case BGEZ_OPCODE:
					case BGEZL_OPCODE:
						*conditionMat  = (cpuRegs.GPR.r[rs].SD[0] >= 0);
						break;
					case BGEZAL_OPCODE:
					case BGEZALL_OPCODE:
						*conditionMet  = (cpuRegs.GPR.r[rs].SD[0] >= 0);
						*link = 1;
						break;
					case BLTZ_OPCODE:
					case BLTZL_OPCODE:
						*conditionMet  = (cpuRegs.GPR.r[rs].SD[0] < 0);
						break;
					case BLTZAL_OPCODE:
					case BLTZALL_OPCODE:
						*conditionMet  = (cpuRegs.GPR.r[rs].SD[0] < 0);
						*link = 1;
						break;
					default:
						return 0;
				}
			}
			break;
		default:
			return 0;
	}
	
	short ofs;
	ofs = (short) (instr & 0xffff);
	*target += ofs * 4;
	
	return 1;
}

/*
 * jJalParse: this tests for J JAL jumps.
 */
static u32 jJalParse(u32 instr, u32* target, u32* link)
{
	switch(_Funct(instr))
	{
		case JAL_OPCODE:
			*link = 1;
			break;
		case J_OPCODE:
			break;
		default:
			return 0;
	}
	
	u32 ofs;

	ofs = instr & 0x3ffffff;
	*target = (ofs * 2) | (*target & 0xf0000000);
	
	return 1;
}

/*
 * jrJalrParse: this tests for JR JALR jumps.
 */
static u32 jrJalrParse(u32 instr, u32* target, u32* link)
{
	switch(_Funct(instr))
	{
		case SPECIAL_OPCODE:
			{
				switch(instr & 0x3f)
				{
					case JALR_OPCODE: 
						*link = 1;
						break;
					case JR_OPCODE:
						break;
					default:
						return 0;
				};
			}
			break;
		default:
			return 0;
	}
	
	u32 rs;
	rs = _RS(instr);
	// GPR[rs] 31..0
	*target = cpuRegs.GPR.r[rs].UL[0];
	
	return 1;
}

/*
 * copParse: this tests for BEQ BEQL BGTZ BGTZL BLEZ BLEZL BNE BNEL branches.
 */
static u32 copParse(u32 instr, u32* target u32* conditionMet)
{
	u32 rs, rt;
	rs = _RS(instr);
	rt = _RT(instr);
	
	switch(_Funct(instr))
	{
		case COP0_OPCODE:
		case COP1_OPCODE:
		case COP2_OPCODE:
			{
				switch((instr >> 16) & 0x3ff)
				{
					case BCXF_OPCODE:
					case BCXFL_OPCODE:
					case BCXT_OPCODE:
					case BCXTL_OPCODE:
						{
							short ofs;

							ofs = (short) (instr & 0xffff);
							*conditional = 1;
							branch = 1;
							*target += ofs * 4;
						}
						break;
					default:
						return 0;
				};
			}
			break;
		default:
			return 0;
	}
	
	short ofs;

	ofs = (short) (instr & 0xffff);
	*target += ofs * 4;
	
	return 1;
}


static u32 isBranchOrJump(u32 addr, u32 instr, u32* conditional, u32* link, u32* target, u32* conditionMet)
{
	u32 branch = 0;
	u32 jump = 0;

	//reset output arguments
	*conditional = *link = *conditionMat = 0;
	//Point target to next instruction to calculate offsets easier
	*target = addr + 0x4;
	
	if(conditionalBranchParse(instr, target, conditionMet) || conditionalBranchREGIMMParse(instr, link, target, conditionMet))
	{
		//we had a conditional branch; target has been calculated and conditionMet has been calculated
		*conditional = 1;
		branch = 1;
	}else if(jJalParse(instr, target, link))
	{
		//J or JAL; target has been calculated and link has been set
		jump = 1;
	}else if(jrJalrParse(instr, target, link))
	{
		//JR or JALR; target has been calculated and link has been set
		jump = 1;
	}else if(copParse(instr, target, conditionMet))
	{
		//we had a conditional branch; target has been calculated and conditionMet has been calculated
		*conditional = 1;
		branch = 1;
		//TODO Parse conditions of COP0, COP1 and COP2 processor instructions
	}
	return (branch || jump);
	/*
	switch(_Funct(instr))
	{
		case BEQ_OPCODE:
		case BEQL_OPCODE:
			conditionResult = (rs == tr);
		case BGTZ_OPCODE:
		case BGTZL_OPCODE:
			conditionResult = (rs > 0);
		case BLEZ_OPCODE:
		case BLEZL_OPCODE:
			conditionResult = (rs <= 0);
		case BNE_OPCODE:
		case BNEL_OPCODE:
			conditionResult = (rs != tr);
			{
				short ofs;
				ofs = (short) (instr & 0xffff);
				*conditional = 1;
				branch = 1;
				*target += ofs * 4;
			}
			break;
		case REGIMM_OPCODE:
			{
				switch((instr >> 16) & 0x1f)
				{
					case BGEZ_OPCODE:
					case BGEZAL_OPCODE:
					case BGEZALL_OPCODE:
					case BGEZL_OPCODE:
					case BLTZ_OPCODE:
					case BLTZAL_OPCODE:
					case BLTZALL_OPCODE:
					case BLTZL_OPCODE:
						{
							short ofs;

							ofs = (short) (instr & 0xffff);
							*conditional = 1;
							branch = 1;
							*target += ofs * 4;
						}
						break;
				}
			}
			break;
		case JAL_OPCODE: *link = 1;
		case J_OPCODE:
			{
				u32 ofs;

				ofs = instr & 0x3ffffff;
				*target = (ofs * 2) | (*target & 0xf0000000);
				branch = 1;
				*conditional = 0;
			}
			break;
		case SPECIAL_OPCODE:
			{
				switch(instr & 0x3f)
				{
					case JALR_OPCODE: *link = 1;
					case JR_OPCODE:
						{
							u32 rs;

							rs = (instr >> 21) & 0x1f;
							*target = regs->r[rs].r32[0];
							branch = 1;
							*conditional = 1;
						}
						break;
				};
			}
			break;
		case COP0_OPCODE:
		case COP1_OPCODE:
		case COP2_OPCODE:
			{
				switch((instr >> 16) & 0x3ff)
				{
					case BCXF_OPCODE:
					case BCXFL_OPCODE:
					case BCXT_OPCODE:
					case BCXTL_OPCODE:
						{
							short ofs;

							ofs = (short) (instr & 0xffff);
							*conditional = 1;
							branch = 1;
							*target += ofs * 4;
						}
						break;
				};
			}
			break;
	};
	return branch;*/
}
/*
 * exceptionDebugger: this function sends required information to client depending on
 * 					the breakpoint event and waits for an instruction to resume
 */
static void exceptionDebugger(u32 exceptionLevel)
{
	FlushCache(0);
    FlushCache(2);

	u32 epc;
	u32 exceptionNumber;
	
	if(exceptionLevel == 1)
	{
		epc = cpuRegs.CP0.n.EPC;
		exceptionNumber = cpuRegs.CP0.n.Cause.bits.ExcCode;
	}else
	{
		epc = cpuRegs.CP0.n.ErrorEPC;
		exceptionNumber = cpuRegs.CP0.n.Cause.bits.EXC2;
	} 

	//Calculate new return address and store it back to the EPC
	//Check branch delay bit
	if(cpuRegs.CP0.n.Cause.bits.BD == 0)
	{
		//Just add 4 to the EPC
		cpuRegs.CP0.n.EPC += 0x4;
	}else
	{
		//We are in a branch delay slot.
		u32 bdSlotAddr = epc + 0x4;
		
		u32 conditionalJump, linkedJump, targetAddress, conditionMet;
		
		//This IF should always be true!
		if(isBranchOrJump(epc, *(u32*)epc, &conditionalJump, &linkedJump &targetAddress, &conditionMet))
		{
			//We now know where this branch or jump is going to go
			if(conditionalJump)
			{
				if(conditionMet)
					cpuRegs.CP0.n.EPC = targetAddress;
				else
					cpuRegs.CP0.n.EPC = bdSlotAddr + 0x4;
			}else
				cpuRegs.CP0.n.EPC = targetAddress;
		}else
		{
			//WTF! May have missed/messed up parsing of certain branch instruction
			//For now force execution to pause 
			haltExecutionAndWait();
		}
	}
	
	//Check for sw breakpoint. lv2handler will handle hardware breakpoints and Performance Counters (for single stepping)
	if(exceptionLevel == 1)
	{
		//Break and traps
		if(exceptionNumber == 9 || exceptionNumber == 13)
		{
			//Send address of hit
			//Send word from address
			//TODO send next X amount of addresses for convenience of viewing on client?
			//send registers
			haltExecutionAndWait();
		}
	}else
	{
		//Performance Counter exception
		if(exceptionNumber == 2)
		{
			
		}
		//Debug
		else if(exceptionNumber == 3)
		{
			
		}
	}
	//Restore all the registers from before the exception handler called here
	//Set PC to be the line that should have been executed after the exception handler and execute
	returnFromLv1Exception();
}
