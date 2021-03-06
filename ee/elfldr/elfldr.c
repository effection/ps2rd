/*
 * ELF loader
 *
 * Copyright (C) 2009-2010 jimmikaelkael <jimmikaelkael@wanadoo.fr>
 * Copyright (C) 2009-2010 Mathias Lafeldt <misfire@debugon.org>
 *
 * This file is part of PS2rd, the PS2 remote debugger.
 *
 * PS2rd is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PS2rd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PS2rd.  If not, see <http://www.gnu.org/licenses/>.
 *
 * $Id$
 */

#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <string.h>
#include <stdio.h>
#include <iopcontrol.h>
#include <iopheap.h>
#include <fileio.h>
#include <io_common.h>

#define GS_BGCOLOUR	*((vu32*)0x120000E0)

/* do not link to strcpy() from libc! */
#define __strcpy(dest, src) \
	strncpy(dest, src, strlen(src))

#define ELF_MAGIC	0x464c457f
#define ELF_PT_LOAD	1

typedef struct {
	u8	ident[16];
	u16	type;
	u16	machine;
	u32	version;
	u32	entry;
	u32	phoff;
	u32	shoff;
	u32	flags;
	u16	ehsize;
	u16	phentsize;
	u16	phnum;
	u16	shentsize;
	u16	shnum;
	u16	shstrndx;
} elf_header_t;

typedef struct {
	u32	type;
	u32	offset;
	void	*vaddr;
	u32	paddr;
	u32	filesz;
	u32	memsz;
	u32	flags;
	u32	align;
} elf_pheader_t;

#define MAX_ARGS 15

static int _argc;
static char *_argv[1 + MAX_ARGS];
static char _argbuf[256];

/*
 * ELF loader function
 */
static void loadElf(void)
{
	int i, ret, fd;
	t_ExecData elf;
	elf_header_t elf_header;
	elf_pheader_t elf_pheader;

	GS_BGCOLOUR = 0x400040; /* dark purple */

	ResetEE(0x7f);

	/* wipe user memory */
	for (i = 0x00100000; i < 0x02000000; i += 64) {
		__asm__ (
			"\tsq $0, 0(%0) \n"
			"\tsq $0, 16(%0) \n"
			"\tsq $0, 32(%0) \n"
			"\tsq $0, 48(%0) \n"
			:: "r" (i)
		);
	}

	/* clear scratchpad memory */
	memset((void*)0x70000000, 0, 16 * 1024);

	/* HACK do not reset IOP when launching ELF from mass */
	if (!(_argv[0][0] == 'm' && _argv[0][1] == 'a' &&
			_argv[0][2] == 's' && _argv[0][3] == 's')) {
		/* reset IOP */
		SifInitRpc(0);
		SifResetIop();
		SifInitRpc(0);

		FlushCache(0);
		FlushCache(2);

		/* reload modules */
		SifLoadFileInit();
		SifLoadModule("rom0:SIO2MAN", 0, NULL);
		SifLoadModule("rom0:MCMAN", 0, NULL);
		SifLoadModule("rom0:MCSERV", 0, NULL);
	}

	GS_BGCOLOUR = 0x004000; /* dark green */

	/* try to load the ELF with SifLoadElf() first */
	memset(&elf, 0, sizeof(t_ExecData));
	ret = SifLoadElf(_argv[0], &elf);
	if (!ret && elf.epc) {
		/* exit services */
		fioExit();
		SifLoadFileExit();
		SifExitIopHeap();
		SifExitRpc();

		FlushCache(0);
		FlushCache(2);

		GS_BGCOLOUR = 0x000000; /* black */

		/* finally, run game ELF... */
		ExecPS2((void*)elf.epc, (void*)elf.gp, _argc, _argv);

		SifInitRpc(0);
	}

	GS_BGCOLOUR = 0x000040; /* dark maroon */

	/* if SifLoadElf() failed, load the ELF manually */
	fioInit();
 	fd = open(_argv[0], O_RDONLY);
 	if (fd < 0) {
		goto error; /* can't open file, exiting... */
 	}

	/* read ELF header */
	if (read(fd, &elf_header, sizeof(elf_header)) != sizeof(elf_header)) {
		close(fd);
		goto error; /* can't read header, exiting... */
	}

	/* check ELF magic */
	if ((*(u32*)elf_header.ident) != ELF_MAGIC) {
		close(fd);
		goto error; /* not an ELF file, exiting... */
	}

	/* copy loadable program segments to RAM */
	for (i = 0; i < elf_header.phnum; i++) {
		lseek(fd, elf_header.phoff+(i*sizeof(elf_pheader)), SEEK_SET);
		read(fd, &elf_pheader, sizeof(elf_pheader));

		if (elf_pheader.type != ELF_PT_LOAD)
			continue;

		lseek(fd, elf_pheader.offset, SEEK_SET);
		read(fd, elf_pheader.vaddr, elf_pheader.filesz);

		if (elf_pheader.memsz > elf_pheader.filesz)
			memset(elf_pheader.vaddr + elf_pheader.filesz, 0,
					elf_pheader.memsz - elf_pheader.filesz);
	}

	close(fd);

	/* exit services */
	fioExit();
	SifLoadFileExit();
	SifExitIopHeap();
	SifExitRpc();

	FlushCache(0);
	FlushCache(2);

	GS_BGCOLOUR = 0x000000; /* black */

	/* finally, run game ELF... */
	ExecPS2((void*)elf_header.entry, NULL, _argc, _argv);
error:
	GS_BGCOLOUR = 0x404040; /* dark gray screen: error */
	SleepThread();
}

/*
 * LoadExecPS2 replacement function. The real one is evil...
 * This function is called by the main ELF to start the game.
 */
void MyLoadExecPS2(const char *filename, int argc, char *argv[])
{
	char *p = _argbuf;
	int i;

	GS_BGCOLOUR = 0x400000; /* dark blue */

	/* sometimes, args are stored in kernel RAM */
	DI();
	ee_kmode_enter();

	/* copy args from main ELF */
	_argc = argc > MAX_ARGS ? MAX_ARGS : argc;

	memset(_argbuf, 0, sizeof(_argbuf));

	__strcpy(p, filename);
	_argv[0] = p;
	p += strlen(filename) + 1;
	_argc++;

	for (i = 0; i < argc; i++) {
		__strcpy(p, argv[i]);
		_argv[i + 1] = p;
		p += strlen(argv[i]) + 1;
	}

	ee_kmode_exit();
	EI();

	GS_BGCOLOUR = 0x004040; /* dark olive */

	/*
	 * ExecPS2() does the following for us:
	 * - do a soft EE peripheral reset
	 * - terminate all threads and delete all semaphores
	 * - set up ELF loader thread and run it
	 */
	ExecPS2(loadElf, NULL, 0, NULL);
}
