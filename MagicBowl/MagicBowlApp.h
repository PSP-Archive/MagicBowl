/*
 * ClMagicBowlApp.h
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


#ifndef CLMAGICBOWLAPP_H_
#define CLMAGICBOWLAPP_H_
extern "C"{
#include <stdio.h>
#include <pspkernel.h>
#include <psputility.h>
}
#include <3dHomebrew.h>
#include "TextHelper.h"
#include "SimpleTimer.h"
#include "Level.h"
#include "ClTextureMgr.h"
#include "SimpleMenu.h"
#include "ClHBLogging.h"

using namespace monzoom;

//empty forward declaration to prevent cyclic issues
//as the application and level header includes each other
class ClLevel;
typedef struct MDLFileData;

/*
 * structure defining the current user state
 * this is subject to get saved as save game at a
 * later stage
 */
typedef struct UserState{
	short maxLevel; //the last level the player has succeeded
	unsigned int points;
	unsigned int mana;
	short red_cyan;
	bool triggerFlipped;
}UserState;

class ClMagicBowlApp : public Cl3dHomebrew {
	friend class ClLevel;
public:
	/*
	 * Definition of the different states the application
	 * can be in
	 */
	typedef enum ApplicationStates {
		MBA_LOAD_PART1 = 0,
		MBA_LOAD_PART2,
		MBA_INITIALIZING,
		MBA_WAIT_MAIN_MENU,
		MBA_PREPARE_MAIN_MENU,
		MBA_MAIN_MENU,

		MBA_LOAD_TRAINING,
		MBA_LOAD_LEVEL,
		MBA_RUN_LEVEL,
		MBA_LEVEL_FAILED,
		MBA_LEVEL_SUCCESS,
		MBA_LEVEL_MENU,
		MBA_LEVELFAILED_MENU,

		MBA_LOAD_GAME,
		MBA_SAVE_GAME,

		MBA_EXIT_LEVEL,

		MBA_SHOW_CONTROL,
		MBA_SHOW_CREDITS,
		MBA_SHOW_OPTIONS,
		MBA_SHOW_RENDER_OPTIONS,
		MBA_OPT_ENABLE_3D,
		MBA_OPT_ENABLE_3D_2,
		MBA_OPT_DISABLE_3D,
		MBA_OPT_TRIGGER_NORMAL,
		MBA_OPT_TRIGGER_FLIPPED
	}ApplicationStates;

	enum ButtonTexture {
		BTN_CROSS = 0,
		BTN_TRIANGLE,
		BTN_RTRIGGER,
		BTN_LTRIGGER,
		BTN_CIRCLE,
		BTN_STICK
	};
	/*
	 * singleton instantiation
	 */
	static ClMagicBowlApp* get_instance();

	static void clean_instance();
	/*
	 * redefine the init method to run additional initializations
	 */
	bool init();
	/*
	 * redefine the exit method to do additional clean up
	 */
	void exit();
	/*
	 * the main rendering method of this application
	 * it will be called as part of the main loop.
	 * there for the application will display/render stuff
	 * dependent on the application state
	 */
	void render();

	/*
	 * overload the initRenderTarget method to flip the FOV for
	 * simulation of antialiasing
	 */
	static void initRenderTarget();

protected:
	/*
	 * instantiation of the application class should not be possible
	 * from the outside (singleton) therefore the constructor is protected
	 */
	ClMagicBowlApp();
	virtual ~ClMagicBowlApp();

	/** process the main menu
	 *
	 */
	void doMainMenu();

	/** process Options menu
	 *
	 */
	void doOptions();

	/** process Control settings menu
	 *
	 */
	void doControls();

	/** process Credits menu
	 *
	 */
	void doCredits();

	/** process the render options submenu
	 *
	 */
	void doRenderOptions();

	void blendTextureToScreen(monzoom::Texture* texture, short alpha);
	void blendTextureToScreen(monzoom::Texture* texture, short x, short y, short width, short height, short alpha);

	void blendFrameToScreen(short x, short y, short width, short height, int color);

	void displayProgressBar(short x, short y, short width, short progress);

	SceUtilitySavedataParam dialog;
	void showSaveDialogue(PspUtilitySavedataMode mode);
	void initSaveDialogue(SceUtilitySavedataParam* saveData, PspUtilitySavedataMode mode);
	/*
	 * handle MainMenu inputs and return application state
	 * if it need to be changed
	 */
	//ApplicationStates handleMainMenu();
	/*
	 * member attributes
	 */
	clTextHelper* textHelper;
	ClTextureMgr* textureMgr;
	ClSimpleMenu* mainMenu;
	ClSimpleMenu* optionMenu;
	ClSimpleMenu* levelMenu;
	ClSimpleMenu* levelInGameMenu;
	monzoom::Texture* introTex;
	monzoom::Texture* logoTex;
	monzoom::Texture* mainMenuTex, *mainMenuLogo;
	monzoom::Texture* loadingTex;
	monzoom::Texture* opt3Doff, *opt3Dred, *opt3Dgreen;
	monzoom::Texture* buttons[6];
	monzoom::Texture* arrowLeft, *arrowRight;
	ApplicationStates currentState;
	ClSimpleTimer* timer;

	ClLevel* currentLevel;
	short currLevelId;
	short maxLevelId;
	MDLFileData* levelData;
	MDLFileData* trainingLevel;
	bool render3D;

	int introImageId;
	int introMusicId;

	UserState playerInfo;
	typedef struct SaveGameData{
		int level;
		unsigned int points;
		unsigned int mana;
	}SaveGameData;

	SaveGameData saveGame;
	PspUtilitySavedataListSaveNewData gameData;

private:
	static ClMagicBowlApp* _instance;
	float curr_ms;
	u64 last_tick;
	u32 tick_frequency;
	int frame_count;
	static bool fovFlip;
};

#endif /* CLMAGICBOWLAPP_H_ */
