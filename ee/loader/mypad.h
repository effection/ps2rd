/*
 * Pad wrapper and helper functions
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

#ifndef _MYPAD_H_
#define _MYPAD_H_

#include <tamtypes.h>
#include <libpad.h>
#include "dbgprintf.h"

static inline void padWaitReady(int port, int slot)
{
	int state, last_state;
	char str[16];

	state = padGetState(port, slot);
	last_state = -1;
	while ((state != PAD_STATE_STABLE) && (state != PAD_STATE_FINDCTP1)) {
		if (state != last_state) {
			padStateInt2String(state, str);
			D_PRINTF("Please wait, pad (%d/%d) is in state %s.\n",
				port+1, slot+1, str);
		}
		last_state = state;
		state = padGetState(port, slot);
	}

	/* Pad ever 'out of sync'? */
	if (last_state != -1)
		D_PRINTF("Pad (%d/%d) OK!\n", port+1, slot+1);
}

#endif /* _MYPAD_H_ */
