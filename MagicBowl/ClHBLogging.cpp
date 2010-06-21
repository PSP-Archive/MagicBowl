/*
 * ClHBLogging.cpp
 *
 *  Copyright (C) 2010 André Borrmann
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
 */

extern "C"{
#include <pspiofilemgr.h>
#include <string.h>
#include <stdio.h>
}
#include "ClHBLogging.h"

int ClHBLogging::logFile = -1;

void ClHBLogging::log(const char * text, ...){
	/*if(logFile = sceIoOpen("ms0:/hb.log", PSP_O_WRONLY | PSP_O_CREAT, 0777) >= 0){
		sceIoWrite(logFile, text, strlen(text));

		sceIoClose(logFile);
	}*/
	FILE* fd = fopen("ms0:/hb.log", "a");
	if (fd){
		fwrite(text, sizeof(char), strlen(text), fd);
		fclose(fd);
	}
}
