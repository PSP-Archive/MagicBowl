/*
 * Level.h This represent a MagicBowl level
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

#ifndef CLLEVEL_H_
#define CLLEVEL_H_

typedef struct MDLFileData;

#include "MagicBowlApp.h"
#include <ClSceneMgr.h>
#include "ClBowlPlayer.h"
#include "SimpleTimer.h"
#include <ClTextureMgr.h>
#include "ClEventManager.h"

using namespace monzoom;

class ClMagicBowlApp;

typedef enum LevelStates {
	MBL_INITIALIZING = 0,
	MBL_INIT_SUCCESS = 1,
	MBL_INGAME_MENU = 2,
	MBL_INIT_FAILED = 99,
	MBL_TASK_FAILED = 10,
	MBL_TASK_FINISHED = 11,
}LevelStates;


typedef struct MDLActions{
	//short strLenSource;
	char* sourceObject;
	//short strLenTarget;
	char* targetObject;
	unsigned short action;
}MDLActions;

typedef struct MDLFileData{
	char* objectFile;
	char* loadImage;
	char* shortInfo;
	float gravity;
	float bowlPosX, bowlPosY, bowlPosZ;
	float lightPosX, lightPosY, lightPosZ;
	unsigned int lightColor;
	float maxTime;
	unsigned short actions;
	MDLActions* actionList;
}MDLFileData;

typedef struct ActionData{
	void* trigger;
	EventId event;
}ActionData;

#define MAX_MANA 5000

class ClLevel {
public:
	/*
	 * Default Constructor of the level
	 */
	ClLevel(MDLFileData levelData, ClMagicBowlApp* app);
	/*
	 * will start the asynch initialization
	 */
	void initLevel(bool triggerFlipped);

	/*
	 * render the current level
	 */
	void render(short red_cyan = 0);

	virtual ~ClLevel();

	/*
	 * returns initializing progress from 0 to 100
	 */
	int getInitProgress();

	/*
	 * returns the current Level State
	 */
	LevelStates getLevelState();

	/* set the current state
	 *
	 */
	void setLevelState(LevelStates newState);

protected:
	/*
	 * Member attributes
	 */
	ClMagicBowlApp* app; //my parent App...
	MDLFileData lvlFileData;
	struct ThreadParams {
		ClLevel* level;
	};
	LevelStates currentState;
	int initProgress;
	bool firstRender; //indicates the first time the level will be rendered
	Texture* timerTex;
	Texture* manaTex;

	monzoom::ClSceneMgr* levelScene; //the 3D scene of this level
	ClBowlPlayer* sBowl;
	ClSceneObject* touchObject;

	unsigned int planeCount;

	ClSimpleTimer* timer;
	ClSimpleTimer* runTime;
	float maxTime;
	bool switchSoundPlaying;
	float viewAngle,r3dAngle;
	float viewDistance;
	float viewAngleChange;
	ScePspFVector3 camPos;

	struct LevelPhysics {
		float gravity;
	} physics;

	int sosInactive;
	int sosVisible;

	void handlePad();
	const void* offscreenTarget;

	/*
	 * if the scene was rendered to offscreen, this method will bring it onto screen
	 * with a color filter for red/cyan 3D rendering
	 */
	void drawOffscreen2Screen(unsigned int color, const void* offScreenPtr);
	/*
	 * the end state view on the level will be rendered through
	 * several passes. Each Method represents one pass
	 */
	void renderPassObjectsNormal();
	void renderPassBowlShadowCast(ScePspFVector4* bowlPos, ScePspFMatrix4* lightProj, ScePspFMatrix4* lightView);
	void renderPassObjectsShadowReciever(ScePspFMatrix4* lightProj,ScePspFMatrix4* lightView);
	void renderPassBowlNormal();
	/*
	 * Static initialization Method to allow asynch
	 * initializing of Level Data
	 */
	static int initThread(SceSize args, void *argp);

	static bool SceneSortPredicate(monzoom::ClSceneObject* o1, monzoom::ClSceneObject* o2);
};

#endif /* CLLEVEL_H_ */
