/*
 * myutil.h - Useful utility functions
 *
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
 */

#ifndef _MYUTIL_H_
#define _MYUTIL_H_

#define NUM_SYSCALLS	256

#define KSEG0(x)	((void*)(((u32)(x)) | 0x80000000))
#define MAKE_J(addr)	(u32)(0x08000000 | (0x03FFFFFF & ((u32)addr >> 2)))
#define MAKE_JAL(addr)	(u32)(0x0C000000 | (0x03FFFFFF & ((u32)addr >> 2)))
#define MAKE_ORI(rt, rs, imm) \
			(u32)((0x0D << 26) | (rs << 21) | (rt << 16) | (imm))
#define J_TARGET(j)	(u32)((0x03FFFFFF & (u32)j) << 2)

#define JR_RA		0x03E00008
#define NOP		0x00000000

/* Devices returned by get_dev() */
enum dev {
	DEV_CD,
	DEV_HOST,
	DEV_MASS,
	DEV_MC0,
	DEV_MC1,
	DEV_UNKN
};

u32 kmem_read(void *addr, void *buf, u32 size);
u32 kmem_write(void *addr, const void *buf, u32 size);
void flush_caches(void);
void install_debug_handler(const void *handler);
void reset_iop(const char *img);
int load_modules(const char **modv);
int set_dir_name(char *filename);
char *get_base_name(const char *full, char *base);
enum dev get_dev(const char *path);
int file_exists(const char *filename);
char *read_text_file(const char *filename, int maxsize);
int upload_file(const char *filename, u32 addr, int *size);

#endif /*_MYUTIL_H_*/
