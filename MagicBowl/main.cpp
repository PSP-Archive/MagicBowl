/*
 * PSP Magic Bowl....
 *
 *  Copyright (C) 2009 André Borrmann
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

extern "C" {
#include <pspdebug.h>
#include <pspkernel.h>
}

#include "MagicBowlApp.h"

PSP_MODULE_INFO("PSP Magic Bowl", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-2048);

int main(int argc, char* argv[]){
	ClMagicBowlApp* myHomebrew;

	myHomebrew = ClMagicBowlApp::get_instance();

	if (myHomebrew->init()){
		myHomebrew->run();
		myHomebrew->exit();
	}

	ClMagicBowlApp::clean_instance();
	//once finished exit the application
	sceKernelExitGame();
	return 0;
}
