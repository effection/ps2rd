/*
 * Video mode patcher
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
 *
 * $Id$
 */

#include <tamtypes.h>

extern int vmode;
extern int ydiff_lores;
extern int ydiff_hires;

int get_vmode(void)
{
	return vmode;
}

void set_vmode(int m)
{
	vmode = m;
}

void get_ydiff(int *lo, int *hi)
{
	*lo = ydiff_lores;
	*hi = ydiff_hires;
}

void set_ydiff(int lo, int hi)
{
	ydiff_lores = lo;
	ydiff_hires = hi;
}
