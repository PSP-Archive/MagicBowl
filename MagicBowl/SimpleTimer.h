/*
 * SimpleTimer.h Provides a simple Timer class
 *
 * Copyright (C) 2009 André Borrmann
 *
 * This program is free software;
 * you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation;
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program;
 * if not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef CLSIMPLETIMER_H_
#define CLSIMPLETIMER_H_

extern "C"{
#include <pspkernel.h>
#include <pspdebug.h>
#include <psprtc.h>
}

class ClSimpleTimer {
public:
	ClSimpleTimer();
	virtual ~ClSimpleTimer();
	/*
	 * reset delta counter
	 */
	void reset();
	/*
	 * returns delta seconds since last reset
	 */
	float getDeltaSeconds();
	/*
	 * was the timer reseted ?
	 * return true if the timer was resetted once
	 */
	bool isReset();

protected:
	u64 lastTick;
	float tickFrequency; //reciproke off tickFrequ
	bool reseted;
};

#endif /* CLSIMPLETIMER_H_ */
