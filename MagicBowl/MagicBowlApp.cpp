/*
 * ClMagicBowlApp.cpp
 *
 *  Created on: Dec 29, 2009
 *      Author: andreborrmann
 */

//#define RENDER_STATS

extern "C"{
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <pspgum.h>
#include <psputility.h>
#include <malloc.h>
#include <string.h>
#include <psptypes3d.h>
#include <unistd.h>
#include <psppower.h>
}
#include "MagicBowlApp.h"
#include "Mp3Helper.h"
#include <Mp4Decoder.h>

using namespace monzoom;

#define DATABUFFLEN   0x20

char key[] = "QTAK319JQKJ952HA";
char *titleShow = "New Save\0";

char nameMultiple[][20] =	// End list with ""
{
 "0000",
 "0001",
 "0002",
 "0003",
 "0004",
 ""
};

PspUtilitySavedataListSaveNewData newData;

ClMagicBowlApp* ClMagicBowlApp::_instance = 0;
bool ClMagicBowlApp::fovFlip = false;

ClMagicBowlApp *ClMagicBowlApp::get_instance(){
	if (!_instance) {
		_instance = new ClMagicBowlApp();
	}

	return _instance;
}

bool ClMagicBowlApp::init(){
	//reduce the color depth to gain some performance boost
	this->initParams.gu_PSM = GU_PSM_8888;
	this->initParams.gu_WaitVblank = false;
	if (!Cl3dHomebrew::init()) return false;
	//open the Config file to get the Level Descriptions
	FILE* file;
	file = fopen("data/MBConfig.mbl", "rb");
	if (!file) return false;
	char id[3];
	//first read file identifier
	fread(id, sizeof(char), 3, file);
	if (strncmp(id, "MBL", 3) != 0) return false;
	//now read the number of levels
	fread(&maxLevelId, sizeof(short), 1, file);
	//now get memory for the level descriptions
	levelData = (MDLFileData*)malloc(sizeof(MDLFileData)*maxLevelId);
	//now read all Level data
	for (short i=0;i<maxLevelId;i++){
		//get length of stored object file name
		int length;
		fread(&length, sizeof(int), 1, file);
		//get Memory for file name
		levelData[i].objectFile = (char*)malloc((sizeof(char)*length)+1);
		fread(levelData[i].objectFile, sizeof(char), length, file);
		//terminate string
		levelData[i].objectFile[length] = 0;

		fread(&length, sizeof(int), 1, file);
		//get Memory for file name
		levelData[i].loadImage = (char*)malloc((sizeof(char)*length)+1);
		fread(levelData[i].loadImage, sizeof(char), length, file);
		//terminate string
		levelData[i].loadImage[length] = 0;


		fread(&length, sizeof(int), 1, file);
		//get Memory for file name
		levelData[i].shortInfo = (char*)malloc((sizeof(char)*length)+1);
		fread(levelData[i].shortInfo, sizeof(char), length, file);
		//terminate string
		levelData[i].shortInfo[length] = 0;

		fread(&levelData[i].gravity, sizeof(float), 1, file);
		fread(&levelData[i].bowlPosX, sizeof(float),3, file);
		fread(&levelData[i].lightPosX, sizeof(float),3, file);
		fread(&levelData[i].lightColor, sizeof(int), 1, file);
		fread(&levelData[i].maxTime, sizeof(float), 1, file);

		//read actions for this level
		fread(&levelData[i].actions, sizeof(unsigned short), 1, file);
		if (levelData[i].actions > 0){
			levelData[i].actionList = (MDLActions*)malloc(sizeof(MDLActions)*levelData[i].actions);
			for (int a=0;a<levelData[i].actions;a++){
				fread(&length,sizeof(int), 1, file);
				levelData[i].actionList[a].sourceObject = (char*)malloc(length+1);
				fread(levelData[i].actionList[a].sourceObject, sizeof(char), length, file);
				levelData[i].actionList[a].sourceObject[length] = 0;
				fread(&length,sizeof(int), 1, file);
				levelData[i].actionList[a].targetObject = (char*)malloc(length+1);
				fread(levelData[i].actionList[a].targetObject, sizeof(char), length, file);
				levelData[i].actionList[a].targetObject[length] = 0;

				fread(&levelData[i].actionList[a].action, sizeof(unsigned short), 1, file);
			}
		}
	}

	fclose(file);

	//set the start level to 1
	currLevelId = 1;

	//int test = 0;
	//setup the text helper
	textHelper = clTextHelper::getInstance();
	//setup the initial state
	currentState = MBA_LOAD_PART1;//MBA_MAIN_MENU;//
	currentLevel = 0;
	//setup the timer
	timer = new ClSimpleTimer();
	sceRtcGetCurrentTick(&last_tick);
	tick_frequency = sceRtcGetTickResolution();
	frame_count = 0;
	curr_ms = 1.0f;

	textureMgr = ClTextureMgr::getInstance();
	textureMgr->setBasePath("img");
	logoTex = 0;
	introTex = 0;
	mainMenuTex = 0;
	loadingTex = 0;
	mainMenu = 0;
	levelMenu = 0;
	levelInGameMenu = 0;

	fovFlip = false;
	ClMp3Helper::initMP3();
	introMusicId = 0;

	//set initial player state
	playerInfo.maxLevel = -1; //no level succeeded
	playerInfo.points = 0;    //no points at all
	playerInfo.mana = 0;      //no mana at all
	playerInfo.red_cyan = false; //default no 3D effect
	playerInfo.triggerFlipped = false; //default trigger rotation behavior

	//setup the training level
	trainingLevel = new MDLFileData[1];
	trainingLevel->bowlPosX = 0.0f;
	trainingLevel->bowlPosY = 5.0f;
	trainingLevel->bowlPosZ = 0.0f;
	trainingLevel->lightPosX = 0.0f;
	trainingLevel->lightPosY = 20.0f;
	trainingLevel->lightPosZ = -5.0f;
	trainingLevel->gravity = 9.81f;//0.1f;
	trainingLevel->lightColor = 0xffffffff;
	trainingLevel->maxTime = 999.0f;
	trainingLevel->objectFile = "data/Training.mon\0";
	trainingLevel->loadImage = "Training.png\0";
	trainingLevel->shortInfo = "There is an easy and a more difficult way to survive this level. Try out how it works and what you are able to do. You may need it in the compitions coming next...";
	trainingLevel->actions = 5;
	trainingLevel->actionList = (MDLActions*)malloc(sizeof(MDLActions)*trainingLevel->actions);
	trainingLevel->actionList[0].action = 1;
	trainingLevel->actionList[0].sourceObject = "NULL\0";
	trainingLevel->actionList[0].targetObject = "OutBox\0";

	trainingLevel->actionList[1].action = 3;
	trainingLevel->actionList[1].sourceObject = "Platform\0";//"NULL\0";
	trainingLevel->actionList[1].targetObject = "Platform\0";

	trainingLevel->actionList[2].action = 3;
	trainingLevel->actionList[2].sourceObject = "Fahrstuhl\0";//"NULL\0";
	trainingLevel->actionList[2].targetObject = "Fahrstuhl\0";

	trainingLevel->actionList[3].action = 1;
	trainingLevel->actionList[3].sourceObject = "NULL\0";
	trainingLevel->actionList[3].targetObject = "Fahrstuhl\0";

	trainingLevel->actionList[4].action = 1;
	trainingLevel->actionList[4].sourceObject = "NULL\0";
	trainingLevel->actionList[4].targetObject = "Platform\0";

	//make sure set the PSP does not go to
	//idle mode if the analog stick is moved
	sceCtrlSetIdleCancelThreshold(64, 100);

	int mp4Res = ClMp4Decoder::init("/mods/mpeg_vsh370.prx");
	render3D = false;

	//do init some GU stuff different from HBC
	Cl3dHomebrew::startGu();
	this->initRenderTarget();
	Cl3dHomebrew::finishGu();

	//set PSP to maximum speed, this cost lots of battery but is needed for maximum performance...
	scePowerSetClockFrequency(333, 333, 166);

	//load the button textures
	buttons[BTN_CROSS] = textureMgr->loadFromPNG("btn_cross.png\0");
	buttons[BTN_TRIANGLE] = textureMgr->loadFromPNG("btn_triangle.png\0");
	buttons[BTN_LTRIGGER] = textureMgr->loadFromPNG("btn_ltrigg.png\0");
	buttons[BTN_RTRIGGER] = textureMgr->loadFromPNG("btn_rtrigg.png\0");
	buttons[BTN_CIRCLE]   = textureMgr->loadFromPNG("btn_circle.png\0");
	buttons[BTN_STICK]    = textureMgr->loadFromPNG("btn_stick.png\0");
	return true;
}

void ClMagicBowlApp::exit(){
	//clean up Texture Manager data
	ClTextureMgr::freeInstance();
	//clean up the level data
	if (maxLevelId > 0){
		for (short i=0;i<maxLevelId;i++){
			free(levelData[i].objectFile);
			if (levelData[i].actions > 0){
				for (unsigned short a = 0; a < levelData[i].actions; a++){
					free(levelData[i].actionList[a].sourceObject);
					free(levelData[i].actionList[a].targetObject);
				}
				free(levelData[i].actionList);
			}
		}
		free(levelData);
	}
	//cleanup menu
	if (mainMenu) delete(mainMenu);
	if (trainingLevel->actions > 0){
		//for (unsigned short a = 0; a < trainingLevel->actions; a++){
			//free(trainingLevel->actionList[a].sourceObject);
			//free(trainingLevel->actionList[a].targetObject);
		//}
		free(trainingLevel->actionList);
	}
	delete(trainingLevel);
	ClMp3Helper::closeMP3();
}


ClMagicBowlApp::ClMagicBowlApp()
: mainMenuTex(0), mainMenuLogo(0), opt3Doff(0), opt3Dred(0), opt3Dgreen(0),
  arrowLeft(0), arrowRight(0){
	// TODO Auto-generated constructor stub
	buttons[0] = 0;
	buttons[1] = 0;
	buttons[2] = 0;
	buttons[3] = 0;
	buttons[4] = 0;
}

void ClMagicBowlApp::render(){
	int fade, loadBarLen;
	SceCtrlData pad;
	short menuState;
	int mp4Res;
	char* longText;
	monzoom::Texture* background;
	switch (currentState){
	/*
	 * Initial state of the application, display intro and switch
	 * to Main Menu
	 */
	case MBA_LOAD_PART1:
		//if the timer was not reseted, reset it now
		if (!timer->isReset()){
			timer->reset();
			//start playing the main sound
			introMusicId = ClMp3Helper::playMP3("snd/main_menu.mp3",0x4000);
			//ClMp3Helper::setMP3Volume(introMusicId, 0x4000);
		} else {
			//as the timer is initialized now blend the logo
			if (logoTex == 0){
				logoTex = textureMgr->loadFromPNG("Logo.png\0");
			}
			if (timer->getDeltaSeconds() < 5.0f) {
				short alpha = (float)254 * timer->getDeltaSeconds() / 5.0f;
				if (alpha > 0xff)
					alpha = 0xff;
				blendTextureToScreen(logoTex, alpha);
			}
			//between 3 and 6 sec leave the screen
			if (timer->getDeltaSeconds() > 5.0f && timer->getDeltaSeconds() < 8.0f){
				blendTextureToScreen(logoTex, 0xff);
			}
			//between 6 and 9 sec blend out...
			if (timer->getDeltaSeconds() > 8.0f && timer->getDeltaSeconds() < 13.0f){
				short alpha = (float)254 * (timer->getDeltaSeconds() - 8.0f) / 5.0f;
				if (alpha > 0xff)
					alpha = 0xff;
				blendTextureToScreen(logoTex, 0xff - alpha);
			}
			//after 9 sec we go to the next stage
			if (timer->getDeltaSeconds() > 13.0){
				currentState = MBA_LOAD_PART2;
				timer->reset();
				//free the texture for the logo..
				textureMgr->freeTexture(logoTex->id);
				logoTex = 0;
			}
		}
		break;
	case MBA_LOAD_PART2:
		//mp4Res = ClMp4Decoder::playMp4("img/Intro.mp4", PSP_CTRL_START);
		currentState = MBA_WAIT_MAIN_MENU;
		break;
	case MBA_WAIT_MAIN_MENU:
		//blend the intro image
		if (!introTex){
			introTex = textureMgr->loadFromPNG("intro.png\0");
		}
		//for 3 seconds display an intro to the game
		if (timer->getDeltaSeconds() < 5.0f) {
			short alpha = (float)254 * timer->getDeltaSeconds() / 5.0f;
			if (alpha > 0xff)
				alpha = 0xff;
			blendTextureToScreen(introTex, alpha);
		}
		//leave the intro for 3 secs
		if (timer->getDeltaSeconds() > 5.0f && timer->getDeltaSeconds() < 6.0f){
			blendTextureToScreen(introTex, 0xff);
		}
		if (timer->getDeltaSeconds() > 6.0f){
			//wait until a certain button was pressed to continue
			blendTextureToScreen(introTex, 0xff);
			textHelper->writeText(SANSERIF_BOLD, "Press 'X' to start", 300, 250);
			/*
			 * do stay in this state until the cross button was pressed
			 */
			sceCtrlPeekBufferPositive(&pad, 1);
			//button pressed ?
			if (pad.Buttons != 0){
				if (pad.Buttons & PSP_CTRL_CROSS){
					//if we have the cross we need to switch
					//to the next state from the main menu
					//this will be ....
					timer->reset();
					currentState = MBA_PREPARE_MAIN_MENU;
				}
			}
		}
		break;

	case MBA_PREPARE_MAIN_MENU:
		// than blend out again
		if (timer->getDeltaSeconds() < 5.0f){
			short alpha = (float)254 * (timer->getDeltaSeconds()) / 5.0f;
			if (alpha > 0xff)
				alpha = 0xff;
			blendTextureToScreen(introTex, 0xff - alpha);
		}
		if (timer->getDeltaSeconds() > 5.0f){
			currentState = MBA_MAIN_MENU;
			timer->reset();
			textureMgr->freeTexture(introTex->id);
			introTex = 0;
		}

		break;
	/*
	 * Main Menu. Display the same and check for the selected item
	 * dependent from the action on the main menu the next state will
	 * be set
	 */
	case MBA_MAIN_MENU:
		doMainMenu();
		break;
	/*
	 * start the training mode
	 */
	case MBA_LOAD_TRAINING:
		//while loading the level we will show some level descriptions
		//and a screen of the level may be ?
		if (!currentLevel){
			currLevelId = 0xffff;
			currentLevel = new ClLevel(currLevelId,*trainingLevel, this);//levelData[currLevelId-1], this);//
			currentLevel->initLevel(this->playerInfo.triggerFlipped);
		}
		if (loadingTex == 0){
				loadingTex = textureMgr->loadFromPNG(trainingLevel->loadImage);
		} else
			this->blendTextureToScreen(loadingTex, 0xff);

		//display the description, not part of the image to be open for multi language support ;o)
		textHelper->writeTextBlock(SANSERIF_SMALL_REGULAR, trainingLevel->shortInfo, 105, 190, 250);
		loadBarLen = (150*currentLevel->getInitProgress() / 100);
		displayProgressBar(170, 251, 150, loadBarLen);

		if (currentLevel->getLevelState() == MBL_INIT_SUCCESS) {
			//if successfully loaded start the level, or wait for a key pressed ?
			textHelper->writeText(SERIF_SMALL_REGULAR, "Press (X) to continue", 180, 262);
			sceCtrlPeekBufferPositive(&pad, 1);
			if (pad.Buttons && pad.Buttons & PSP_CTRL_CROSS){
				currentState = MBA_RUN_LEVEL;
				loadingTex = 0;
				playerInfo.mana = 0;
				//if (introMusicId)
					//	ClMp3Helper::stopMP3(introMusicId);
				//introMusicId = ClMp3Helper::playMP3("snd/Training.mp3", 0x5000);
			};
		}
		break;
	/*
	 * load the next level and display some "loading.." animation to indicate
	 * that the user need to wait some time
	 */
	case MBA_LOAD_LEVEL:
		//TODO:
		//display level specific image with something like a mission during loading
		//start the level based on button state
		if (!currentLevel) {
			currentLevel = new ClLevel(currLevelId,levelData[currLevelId-1], this);
			currentLevel->initLevel(this->playerInfo.triggerFlipped);
		}

		if (loadingTex == 0){
				loadingTex = textureMgr->loadFromPNG(levelData[currLevelId-1].loadImage);
		} else
			this->blendTextureToScreen(loadingTex, 0xff);

		//display the description, not part of the image to be open for multi language support ;o)
		textHelper->writeTextBlock(SANSERIF_SMALL_REGULAR, levelData[currLevelId-1].shortInfo, 105, 190, 250);

		//display progress bar
		loadBarLen = (150*currentLevel->getInitProgress() / 100);
		displayProgressBar(170, 251, 150, loadBarLen);

		if (currentLevel->getLevelState() == MBL_INIT_SUCCESS) {
			textHelper->writeText(SERIF_SMALL_REGULAR, "Press (X) to continue", 180, 262);
			sceCtrlPeekBufferPositive(&pad, 1);
			if (pad.Buttons && pad.Buttons & PSP_CTRL_CROSS){
				currentState = MBA_RUN_LEVEL;
				loadingTex = 0;
			}
		}
		break;
	case MBA_RUN_LEVEL:
		doLevel();
		break;

	case MBA_LEVEL_FAILED:
	/*	//for 3 seconds display that you have failed this level
		if (timer->getDeltaSeconds() < 3.0f
		    && currLevelId == (short)0xffff) {
			textHelper->writeText(SANSERIF_BOLD, "Training Failed, try again...", 100, 100);
		} else {*/
			//now set the next state
			if (currLevelId == (short)0xffff)currentState = MBA_LOAD_TRAINING;
			else currentState = MBA_LEVELFAILED_MENU;
		//}
		break;
	case MBA_LEVEL_SUCCESS:
		//if the player has finished a regular level
		//move to the next one and show some "congrat" info!
		//for 3 seconds display that you have finished this level
		if (currLevelId == 1 && timer->getDeltaSeconds() < 4.0f){
			background = textureMgr->loadFromPNG("SURF20.png\0");
			if (background)
				blendTextureToScreen(background, 0xff);
			textHelper->writeTextBlock(SANSERIF_BOLD, "Training completed...\nDo prepare for the challenge.", 50, 100, 400);
		} else
		if (currLevelId > this->maxLevelId && timer->getDeltaSeconds() < 8.0f){
			background = textureMgr->loadFromPNG("SUN02.png\0");
			if (background)
				blendTextureToScreen(background, 0xff);
			textHelper->writeTextBlock(SANSERIF_BOLD, "Congratulation.\nYou have succeded for now.\n But do carefully look forward to your future...",50,100,400);
		} else {
			//now set the next state
			if (currLevelId > maxLevelId){
				//switch to the main menu if we have reached the last level
				//or just finished the training
				currLevelId = 1;
				currentState = MBA_MAIN_MENU;
			} else {
				currentState = MBA_LEVEL_MENU;//MBA_LOAD_LEVEL;//
			}
		}
		break;

	/*
	 * render the menu between 2 levels to be able to
	 * save the current state...
	 */
	case MBA_LEVEL_MENU:
		doLevelMenu();
		break;
	case MBA_LEVELFAILED_MENU:
		doLevelFailedMenu();
		break;

	case MBA_SAVE_GAME:
		showSaveDialogue(PSP_UTILITY_SAVEDATA_LISTSAVE);
		currentState = MBA_LEVEL_MENU;
		break;
	case MBA_LOAD_GAME:
		showSaveDialogue(PSP_UTILITY_SAVEDATA_LISTLOAD);
		//if we have loaded a level then go to it, otherwise
		//just back to the main menu
		if (this->currLevelId)
			currentState = MBA_LOAD_LEVEL;
		else
			currentState = MBA_MAIN_MENU;
		break;
	case MBA_EXIT_LEVEL:
		delete(currentLevel);
		currentLevel=0;
		currentState = MBA_MAIN_MENU;
		break;

	case MBA_SHOW_CONTROL:
		doControls();
		break;
	case MBA_SHOW_CREDITS:
		doCredits();
		break;
	case MBA_SHOW_OPTIONS:
		doOptions();
		break;
	case MBA_SHOW_RENDER_OPTIONS:
		// render option menu to switch between normal, 3D-cyan and 3D-green
		this->doRenderOptions();
		break;
	default:
		break;
	}

	//display some statistics...
#ifdef RENDER_STATS
		float curr_fps = 1.0f / curr_ms;
		char text[50];
		sprintf(text, "%d.%03d",(int)curr_fps,(int)((curr_fps-(int)curr_fps) * 1000.0f));
		textHelper->writeText(SERIF_SMALL_REGULAR, text, 0, 10);

		// simple frame rate counter
		++frame_count;
		u64 curr_tick;
		sceRtcGetCurrentTick(&curr_tick);
		if ((curr_tick-last_tick) >= tick_frequency)
		{
			float time_span = ((int)(curr_tick-last_tick)) / (float)tick_frequency;
			curr_ms = time_span / frame_count;

			frame_count = 0;
			sceRtcGetCurrentTick(&last_tick);
		}
#endif
}

void ClMagicBowlApp::doMainMenu(){
	short menuState;

	if (mainMenuTex == 0){
		mainMenuTex = textureMgr->loadFromPNG("main_menu.png\0");
	}
	if (mainMenuLogo == 0)
		mainMenuLogo = textureMgr->loadFromPNG("main_menu_logo.png\0");

	if (!mainMenu) {
		//setup the main menu
		mainMenu = new ClSimpleMenu(100, 100);
		//create the items
		ClSimpleMenuItem* item;
		item = new ClSimpleMenuItem("Start new game", MBA_LOAD_TRAINING);
		mainMenu->addItem(item);
		item = new ClSimpleMenuItem("Continue game", MBA_LOAD_GAME);
		mainMenu->addItem(item);
		item = new ClSimpleMenuItem("Options", MBA_SHOW_OPTIONS);
		mainMenu->addItem(item);
		item = new ClSimpleMenuItem("Credits", MBA_SHOW_CREDITS);
		mainMenu->addItem(item);
	}
	blendTextureToScreen(mainMenuTex, 0x88);
	blendTextureToScreen(mainMenuLogo, 50, 0, 385, 83, 0xff);
	menuState = mainMenu->handleMenu();

	//if we have executed a Menu item we act on the selected one
	//this mean switching the application state
	if (menuState > 0) currentState = (ApplicationStates)menuState;
	if (currentState != MBA_MAIN_MENU){
		//if leaving the menu destroy the same
		delete(mainMenu);
		mainMenu = 0;
	}
}

void ClMagicBowlApp::doLevel(){
	//first check for level state
	short lvlState;
	short menuState;

	lvlState = currentLevel->getLevelState();
	switch (lvlState){
	case MBL_INGAME_MENU:
		//render the level, and blend in game menu over it
		currentLevel->render(playerInfo.red_cyan);
		if (!levelInGameMenu) {
			//setup the main menu
			levelInGameMenu = new ClSimpleMenu(100, 100);
			//create the items
			ClSimpleMenuItem* item;
			item = new ClSimpleMenuItem("Continue", MBL_INIT_SUCCESS);
			levelInGameMenu->addItem(item);
			item = new ClSimpleMenuItem("Restart Level", MBL_TASK_FAILED);
			levelInGameMenu->addItem(item);
			item = new ClSimpleMenuItem("Leave Level", MBA_MAIN_MENU);
			levelInGameMenu->addItem(item);
		}
		blendFrameToScreen(0, 0, 480, 270, 0x99000000);
		textHelper->writeText(SANSERIF_BOLD   ,"Level Paused", 100, 80);
		menuState = levelInGameMenu->handleMenu();
		if (menuState>=0){
			if (menuState == MBA_MAIN_MENU){
				currentState = MBA_MAIN_MENU;
				delete(currentLevel);
				currentLevel=0;
			}
			else
				currentLevel->setLevelState((LevelStates)menuState);

			delete(levelInGameMenu);
			levelInGameMenu = 0;
		}
		break;
	case MBL_INIT_SUCCESS:
		currentLevel->render(playerInfo.red_cyan);
		break;
	case MBL_TASK_FAILED:
		delete(currentLevel);
		currentLevel=0;
		currentState = MBA_LEVEL_FAILED;
		//display animation of failed level, blocks current thread...
		//which is as expected ;o)
		ClMp4Decoder::playMp4("img/lvlFail.mp4", 0);
		timer->reset();
		break;
	case MBL_TASK_FINISHED:
		delete(currentLevel);
		currentLevel=0;
		timer->reset();
		currentState = MBA_LEVEL_SUCCESS;
		//if we have done this training we set the first
		//level active
		if (currLevelId == (short)0xffff){
			currLevelId = 1;
			this->playerInfo.mana = 0; //no mana after the training...
		}
		else
			//we can increase to the next level
			currLevelId++;
		break;
	}
}

void ClMagicBowlApp::doLevelMenu(){
	short menuState;

	monzoom::Texture* background = textureMgr->loadFromPNG("PLANET17.png\0");
	blendTextureToScreen(background, 0xff);

	if (!levelMenu) {
		//setup the main menu
		levelMenu = new ClSimpleMenu(100, 100);
		//create the items
		ClSimpleMenuItem* item;
		item = new ClSimpleMenuItem("Continue", MBA_LOAD_LEVEL);
		levelMenu->addItem(item);
		item = new ClSimpleMenuItem("Save", MBA_SAVE_GAME);
		levelMenu->addItem(item);
		item = new ClSimpleMenuItem("Main Menu", MBA_MAIN_MENU);
		levelMenu->addItem(item);
	}
	char text[50];
	if (currLevelId<=maxLevelId)
		sprintf(text, "Prepare for Level %d", currLevelId);

	textHelper->writeText(SANSERIF_BOLD   ,text, 100, 80);
	menuState = levelMenu->handleMenu();
	//if we have executed a Menu item we act on the selected one
	//this mean switching the application state
	if (menuState > 0) currentState = (ApplicationStates)menuState;
	if (currentState!=MBA_LEVEL_MENU){
		delete(levelMenu);
		levelMenu = 0;
		//release some resources
		textureMgr->freeTexture(background->id);
	}
}

void ClMagicBowlApp::doLevelFailedMenu(){
	short menuState;

	monzoom::Texture* background = textureMgr->loadFromPNG("SURF22.png\0");
	blendTextureToScreen(background, 0xff);

	if (!levelMenu) {
		//setup the main menu
		levelMenu = new ClSimpleMenu(100, 100);
		//create the items
		ClSimpleMenuItem* item;
		item = new ClSimpleMenuItem("Try again", MBA_LOAD_LEVEL);
		levelMenu->addItem(item);
		item = new ClSimpleMenuItem("Main Menu", MBA_MAIN_MENU);
		levelMenu->addItem(item);
	}
	textHelper->writeText(SANSERIF_BOLD   ,"Level Failed", 100, 80);
	menuState = levelMenu->handleMenu();
	//if we have executed a Menu item we act on the selected one
	//this mean switching the application state
	if (menuState > 0) {
		currentState = (ApplicationStates)menuState;
	}
	if (currentState!=MBA_LEVELFAILED_MENU){
		delete(levelMenu);
		levelMenu = 0;
		//release texture resource
		textureMgr->freeTexture(background->id);
	}
}

void ClMagicBowlApp::doOptions(){
	short menuState;

	if (mainMenuTex == 0)
		mainMenuTex = textureMgr->loadFromPNG("main_menu.png\0");
	if (mainMenuTex != 0)
		blendTextureToScreen(mainMenuTex, 0x88);

	//display default items
	blendFrameToScreen(0, 0, 480, 20, 0x7fff0000);
	textHelper->writeText(SANSERIF_BOLD, "Options", 200, 17);

	if (!optionMenu) {
		//setup the main menu
		optionMenu = new ClSimpleMenu(100, 70);
		//create the items
		ClSimpleMenuItem* item;
		item = new ClSimpleMenuItem("Graphic options", MBA_SHOW_RENDER_OPTIONS);
		optionMenu->addItem(item);
		item = new ClSimpleMenuItem("Controls", MBA_SHOW_CONTROL);
		optionMenu->addItem(item);
		item = new ClSimpleMenuItem("Main Menu", MBA_MAIN_MENU);
		optionMenu->addItem(item);
	}
	menuState = optionMenu->handleMenu();

	switch (menuState){
	case MBA_MAIN_MENU:
		currentState = MBA_MAIN_MENU;
		//if leaving the menu destroy the same
		delete(optionMenu);
		optionMenu = 0;
		break;
	case MBA_SHOW_RENDER_OPTIONS:
		currentState = MBA_SHOW_RENDER_OPTIONS;
		break;
	case MBA_SHOW_CONTROL:
		currentState = MBA_SHOW_CONTROL;
		break;
	default:
		currentState = MBA_SHOW_OPTIONS;
		break;
	}
}

void ClMagicBowlApp::doControls(){
	SceCtrlData pad;
	static SceCtrlData lastPad;
	char* longText;

	if (mainMenuTex == 0)
		mainMenuTex = textureMgr->loadFromPNG("main_menu.png\0");
	if (mainMenuTex != 0)
		blendTextureToScreen(mainMenuTex, 0x88);

	if (arrowLeft == 0)
		arrowLeft = textureMgr->loadFromPNG("arrow_left.png\0");
	if (arrowRight == 0)
		arrowRight = textureMgr->loadFromPNG("arrow_right.png\0");

	//display default items
	blendFrameToScreen(0, 0, 480, 20, 0x7fff0000);
	textHelper->writeText(SANSERIF_BOLD, "Controls", 200, 17);

	//do render the buttons needed to play the game, image + text
	blendFrameToScreen(30, 35, 420, 210, 0x88880000);
	blendTextureToScreen(buttons[BTN_STICK], 50, 50, 19, 19, 0xff);
	textHelper->writeText(SANSERIF_SMALL_REGULAR, "move the bowl", 90, 63);
	blendTextureToScreen(buttons[BTN_RTRIGGER], 47, 75, 26, 11, 0xff);
	if (!playerInfo.triggerFlipped)
		textHelper->writeText(SANSERIF_SMALL_REGULAR, "rotate view right", 90, 85);
	else
		textHelper->writeText(SANSERIF_SMALL_REGULAR, "rotate view left", 90, 85);
	blendTextureToScreen(buttons[BTN_LTRIGGER], 47, 95, 26, 11, 0xff);
	if (!playerInfo.triggerFlipped)
		textHelper->writeText(SANSERIF_SMALL_REGULAR, "rotate view left", 90, 105);
	else
		textHelper->writeText(SANSERIF_SMALL_REGULAR, "rotate view right", 90, 105);

	blendTextureToScreen(buttons[BTN_CIRCLE], 52, 115, 13, 13, 0xff);
	textHelper->writeText(SANSERIF_SMALL_REGULAR, "use Mana to accelerate the bowl", 90, 125);

	//the small "menu" enable switching of the L/R-Trigger behavior
	blendFrameToScreen(40, 150, 150, 20, 0x7fff0000);
	textHelper->writeText(SANSERIF_SMALL_REGULAR, "Rotation behavior", 47, 165);

	blendFrameToScreen(195, 150, 240, 20, 0x7fff0000);
	if (!playerInfo.triggerFlipped){
		textHelper->writeText(SANSERIF_SMALL_REGULAR, "Standard", 260, 165);
		blendTextureToScreen(arrowRight, 415, 155, 10, 10, 0xff);
	} else {
		blendTextureToScreen(arrowLeft, 200, 155, 10, 10, 0xff);
		textHelper->writeText(SANSERIF_SMALL_REGULAR, "Flipped", 265, 165);
	}

	blendFrameToScreen(0, 257, 480, 15, 0x7fff0000);
	if (buttons[BTN_TRIANGLE] != 0)
		blendTextureToScreen(buttons[BTN_TRIANGLE], 410, 258, 13, 13, 0xff);
	textHelper->writeText(SANSERIF_SMALL_BOLD, "back", 430, 270);

	//handle the buttons
	sceCtrlPeekBufferPositive(&pad, 1);
	//button pressed ?
	if (pad.Buttons != 0 && pad.Buttons != lastPad.Buttons){
		if (pad.Buttons & PSP_CTRL_RIGHT && !playerInfo.triggerFlipped){
			ClMp3Helper::playMP3("snd//whup11.mp3");
			playerInfo.triggerFlipped = true;
		}
		if (pad.Buttons & PSP_CTRL_LEFT && playerInfo.triggerFlipped){
			ClMp3Helper::playMP3("snd//whup11.mp3");
			playerInfo.triggerFlipped = false;
		}
		if (pad.Buttons & PSP_CTRL_TRIANGLE){
			//go back to main menu
			currentState = MBA_SHOW_OPTIONS;
			ClMp3Helper::playMP3("snd//blubb1.mp3");
		}
	}
	lastPad = pad;
}

void ClMagicBowlApp::doCredits(){
	SceCtrlData pad;
	char* longText;

	if (mainMenuTex == 0)
		mainMenuTex = textureMgr->loadFromPNG("main_menu.png\0");

	if (mainMenuTex != 0)
		blendTextureToScreen(mainMenuTex, 0x88);

	//display default items
	blendFrameToScreen(0, 0, 480, 20, 0x7fff0000);
	textHelper->writeText(SANSERIF_BOLD, "Credits", 200, 17);

	blendFrameToScreen(0, 257, 480, 15, 0x7fff0000);
	if (buttons[BTN_TRIANGLE] != 0)
		blendTextureToScreen(buttons[BTN_TRIANGLE], 410, 258, 13, 13, 0xff);
	textHelper->writeText(SANSERIF_SMALL_BOLD, "back", 430, 270);

	blendFrameToScreen(30, 35, 420, 210, 0x88880000);
	//now write the text
	longText = "(c) AnMaBaGiMa 2010\nMusic/Soundtracks:                                       =================\nMaintheme: LESIEM Fundamentum (Koellner Lichter 2004)\n Special thanks goes to:                                       ===================\ncooleyes - for mp4 movie playing code                              tekkno_fan  - inspiring the 3D mode                                              pspking.de  - german psp forum                                                   ps2dev.org  - english psp developer forum";
	textHelper->writeTextBlock(SANSERIF_SMALL_REGULAR, longText, 40, 60, 410);

	//handle the buttons
	sceCtrlPeekBufferPositive(&pad, 1);
	//button pressed ?
	if (pad.Buttons != 0){
		if (pad.Buttons & PSP_CTRL_TRIANGLE){
			//go back to main menu
			currentState = MBA_MAIN_MENU;
			ClMp3Helper::playMP3("snd//blubb1.mp3");
		}
	}
}

void ClMagicBowlApp::doRenderOptions(){
	SceCtrlData pad;
	static SceCtrlData lastPad;

	if (mainMenuTex == 0)
		mainMenuTex = textureMgr->loadFromPNG("main_menu.png\0");
	if (arrowLeft == 0)
		arrowLeft = textureMgr->loadFromPNG("arrow_left.png\0");
	if (arrowRight == 0)
		arrowRight = textureMgr->loadFromPNG("arrow_right.png\0");


	if (mainMenuTex != 0)
		blendTextureToScreen(mainMenuTex, 0x88);

	//display default items
	blendFrameToScreen(0, 0, 480, 20, 0x7fff0000);
	textHelper->writeText(SANSERIF_BOLD, "Graphic options", 180, 17);
	blendFrameToScreen(30, 35, 420, 210, 0x88880000);

	//the small "menu" enable switching of the L/R-Trigger behavior
	blendFrameToScreen(45, 200, 150, 20, 0x7fff0000);
	textHelper->writeText(SANSERIF_SMALL_REGULAR, "Render mode", 50, 215);

	blendFrameToScreen(197, 200, 241, 20, 0x7fff0000);
	//dependent on the current mode we display the right image
	switch (playerInfo.red_cyan){
	case 0:
		if (opt3Doff == 0)
			opt3Doff = textureMgr->loadFromPNG("option_3D_off.png\0");
		if (opt3Doff != 0)
			blendTextureToScreen(opt3Doff, 45, 40, 393, 157, 0xff);
		textHelper->writeText(SANSERIF_SMALL_REGULAR, "Standard", 260, 215);
		break;
	case 1:
		if (opt3Dred == 0)
			opt3Dred = textureMgr->loadFromPNG("option_3D_rc.png\0");
		if (opt3Dred != 0)
			blendTextureToScreen(opt3Dred, 45, 40, 393, 157, 0xff);
		textHelper->writeText(SANSERIF_SMALL_REGULAR, "3D mode red/cyan", 250, 215);
		break;
	case 2:
		if (opt3Dgreen == 0)
			opt3Dgreen = textureMgr->loadFromPNG("option_3D_gp.png\0");
		if (opt3Dgreen != 0)
			blendTextureToScreen(opt3Dgreen, 45, 40, 393, 157, 0xff);
		textHelper->writeText(SANSERIF_SMALL_REGULAR, "3D mode pink/green", 245, 215);
		break;
	}
	blendTextureToScreen(arrowRight, 415, 205, 10, 10, 0xff);
	blendTextureToScreen(arrowLeft, 200, 205, 10, 10, 0xff);


	blendFrameToScreen(0, 257, 480, 15, 0x7fff0000);
	if (buttons[BTN_TRIANGLE] != 0)
		blendTextureToScreen(buttons[BTN_TRIANGLE], 410, 258, 13, 13, 0xff);
	textHelper->writeText(SANSERIF_SMALL_BOLD, "back", 430, 270);

	//handle the buttons
	sceCtrlPeekBufferPositive(&pad, 1);
	//button pressed ?
	if (pad.Buttons != 0 && pad.Buttons != lastPad.Buttons){
		if (pad.Buttons & PSP_CTRL_RIGHT){
			playerInfo.red_cyan = (playerInfo.red_cyan + 1) % 3;
			ClMp3Helper::playMP3("snd//whup11.mp3");
		}
		if (pad.Buttons & PSP_CTRL_LEFT){
			if (playerInfo.red_cyan==0) playerInfo.red_cyan = 3;
			playerInfo.red_cyan = (playerInfo.red_cyan - 1) % 3;
			ClMp3Helper::playMP3("snd//whup11.mp3");
		}
		if (pad.Buttons & PSP_CTRL_TRIANGLE){
			//go back to option menu
			currentState = MBA_SHOW_OPTIONS;
			ClMp3Helper::playMP3("snd//blubb1.mp3");
			//release some resources
			if (opt3Doff){
				textureMgr->freeTexture(opt3Doff->id);
				opt3Doff = 0;
			}
			if (opt3Dred){
				textureMgr->freeTexture(opt3Dred->id);
				opt3Dred = 0;
			}
			if(opt3Dgreen){
				textureMgr->freeTexture(opt3Dgreen->id);
				opt3Dgreen = 0;
			}
		}
	}
	lastPad = pad;
}

ClMagicBowlApp::~ClMagicBowlApp() {
	// TODO Auto-generated destructor stub
	delete(timer);
	//stop the music at least now...
	if (introMusicId)
		ClMp3Helper::stopMP3(introMusicId);

	//ClTextureMgr::freeInstance();
}

void ClMagicBowlApp::clean_instance(){
	if (_instance) {
		delete(_instance);
		_instance = 0;
	}
}

void ClMagicBowlApp::initRenderTarget(){
	//festlegen des normalen Zeichenpuffers als Render-Target
	sceGuDrawBufferList(GU_PSM_8888,(void*)drawBuff,BUF_WIDTH);
	sceGuOffset(2048 - (SCR_WIDTH/2),2048 - (SCR_HEIGHT/2));
	sceGuViewport(2048,2048,SCR_WIDTH,SCR_HEIGHT);

	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	//flipping the FOV to simulate anti aliasing
	sceGumPerspective(45.0f+0.25f*fovFlip, 480.0f/272.0f, 10.0f, 1500.0f);

	//sceGuDisable(GU_CLIP_PLANES);
	//sceGuEnable(GU_LINE_SMOOTH);
	//sceGuEnable(GU_PATCH_FACE);
	//fovFlip = fovFlip?false:true;
}


void ClMagicBowlApp::blendTextureToScreen(monzoom::Texture *texture, short  alpha){
	blendTextureToScreen(texture, 0, 0, 480, 272, alpha);
}

void ClMagicBowlApp::blendTextureToScreen(monzoom::Texture *texture,short x, short y, short width, short height, short  alpha){

	TexColIVertex* vertex;

	//set the Texture
	sceGuEnable(GU_TEXTURE_2D);
	sceGuTexImage(0, texture->width, texture->height, texture->stride, (void*) texture->data);
	sceGuTexMode(texture->type, 0, 0, texture->swizzled);
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	sceGuTexFilter(GU_NEAREST, GU_NEAREST);
	sceGuTexScale(1.0f/width, 1.0f/height);
	//sceGuAmbientColor(0xffffffff);
	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	sceGuDisable(GU_DEPTH_TEST);
	//renter the texture across the whole screen
	//do not render complete but in 64Pixel width blocks
	int w, slice;
	slice = 64;
	w = 0;
	while (w < width){

		vertex = (TexColIVertex*)sceGuGetMemory(2*sizeof(TexColIVertex));

		if (w+slice > width) slice = width - w;
		vertex[0].x = x+w;
		vertex[0].y = y;
		vertex[0].z = 0x00ff;
		vertex[0].u = w;
		vertex[0].v = 0;
		vertex[0].color = 0x00ffffff | ((alpha & 255) << 24);

		vertex[1].x = x+w+slice;
		vertex[1].y = y+height;
		vertex[1].z = 0x00ff;
		vertex[1].u = w+slice;
		vertex[1].v = height;
		vertex[1].color = 0x00ffffff | ((alpha & 255) << 24);

		//draw this part
		sceGuDrawArray(GU_SPRITES, GU_COLOR_8888 | GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, vertex);

		w+=slice;
	}
	sceGuEnable(GU_DEPTH_TEST);
	sceGuDisable(GU_TEXTURE_2D);
	sceGuDisable(GU_BLEND);
}

void ClMagicBowlApp::blendFrameToScreen(short  x, short  y, short  width, short  height, int color){
	ColorIVertex* vertex;

	sceGuDisable(GU_TEXTURE_2D);
	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

	vertex = (ColorIVertex*)sceGuGetMemory(sizeof(ColorIVertex)*2);
	vertex[0].x = x;
	vertex[0].y = y;
	vertex[0].z = 0xffff;
	vertex[0].color = color;

	vertex[1].x = x+width;
	vertex[1].y = y+height;
	vertex[1].z = 0xffff;
	vertex[1].color = color;

	sceGuDisable(GU_DEPTH_TEST);
	sceGuDrawArray(GU_SPRITES, GU_TRANSFORM_2D | GU_VERTEX_16BIT | GU_COLOR_8888, 2, 0, vertex);
	sceGuDisable(GU_BLEND);
	sceGuEnable(GU_DEPTH_TEST);
}

void ClMagicBowlApp::displayProgressBar(short  x, short  y, short  width, short  progress){
	blendFrameToScreen(x, y, width, 2, 0xee111111);
	blendFrameToScreen(x, y+1, width, 9, 0xee3d3d3d);
	blendFrameToScreen(x, y+10, width, 3, 0xee717171);

	blendFrameToScreen(x, y, progress, 2, 0xeedf7777);
	blendFrameToScreen(x, y+1, progress, 9, 0xeedf2222);
	blendFrameToScreen(x, y+10, progress, 3, 0xeedf0000);
}

void ClMagicBowlApp::initSaveDialogue(SceUtilitySavedataParam* saveData, PspUtilitySavedataMode mode){
	memset(saveData, 0, sizeof(SceUtilitySavedataParam));
	saveData->base.size = sizeof(SceUtilitySavedataParam);
	saveData->base.language = PSP_SYSTEMPARAM_LANGUAGE_ENGLISH;
	saveData->base.buttonSwap = PSP_UTILITY_ACCEPT_CROSS;
	saveData->base.graphicsThread = 0x11;
	saveData->base.accessThread = 0x13;
	saveData->base.fontThread = 0x12;
	saveData->base.soundThread = 0x10;

	saveData->mode = mode;
	saveData->overwrite = 1;
	saveData->focus = PSP_UTILITY_SAVEDATA_FOCUS_LATEST; // Set initial focus to the newest file (for loading)

	#if _PSP_FW_VERSION >= 200
	strncpy(saveData->key, key, 16);
	#endif

	strcpy(saveData->gameName,"MAGICBOWLV1");
	strcpy(saveData->saveName,"0000");
	strcpy(saveData->fileName,"DATA.BIN");
	saveData->saveNameList = nameMultiple;

	memset(&saveGame, 0, sizeof(saveGame));
	saveData->dataBuf = &saveGame;
	saveData->dataBufSize = sizeof(saveGame);
	saveData->dataSize = sizeof(saveGame);

	if (mode == PSP_UTILITY_SAVEDATA_LISTSAVE)
	{
		//store data saved in the save game
		saveGame.level = currLevelId;
		saveGame.mana = playerInfo.mana;

		strcpy(saveData->sfoParam.title,"MagicBowlSave");
		strcpy(saveData->sfoParam.savedataTitle,"Single player mode");
		sprintf(saveData->sfoParam.detail, "Level:%d\nMana:%d", saveGame.level, saveGame.mana);
		//strcpy(saveData->sfoParam.detail,"Details");
		saveData->sfoParam.parentalLevel = 1;
		//icon data need to be a full PNG file stream.....
		FILE* savePng = fopen("img/savedat.png", "rb");
		if (savePng){
			fseek(savePng, 0, SEEK_END);
			unsigned long fileSize = ftell(savePng);
			rewind(savePng);
			saveData->icon0FileData.buf = malloc(fileSize);
			fread(saveData->icon0FileData.buf, sizeof(char), fileSize, savePng);
			saveData->icon0FileData.bufSize = fileSize;
			saveData->icon0FileData.size = fileSize;
			fclose(savePng);
		}

		saveData->icon1FileData.buf = 0;
		saveData->icon1FileData.bufSize = 0;
		saveData->icon1FileData.size = 0;

		//I do not know where this will be shown...but leave it from the example
		saveData->pic1FileData.buf = 0;
		saveData->pic1FileData.bufSize = 0;
		saveData->pic1FileData.size = 0;

		saveData->snd0FileData.buf = 0;
		saveData->snd0FileData.bufSize = 0;
		saveData->snd0FileData.size = 0;

		gameData.title = "new Save Game";
		gameData.icon0.buf = 0;
		gameData.icon0.bufSize = 0;
		gameData.icon0.size = 0;

		saveData->newData = &gameData;

		saveData->focus = PSP_UTILITY_SAVEDATA_FOCUS_FIRSTEMPTY;
	}

}

void ClMagicBowlApp::showSaveDialogue(PspUtilitySavedataMode mode){
	//init the SaveDialogue data...
	SceUtilitySavedataParam saveData;
	initSaveDialogue(&saveData, mode);

	int saveInit = sceUtilitySavedataInitStart(&saveData);

	int saveStatus;
	bool saveRunning = true;
	//handling the save dialogue
	//stop started drawing from GU
	this->finishGu();
	while (saveRunning){
		//TODO: draw some background images for Save-Dialogue if needed...
		this->startGu();
		sceGuClearColor(0xff000000);
		sceGuClearDepth(0);
		sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
		this->finishGu();

		saveStatus = sceUtilitySavedataGetStatus();
		switch(saveStatus){
			case PSP_UTILITY_DIALOG_VISIBLE :

				sceUtilitySavedataUpdate(1);
				break;

			case PSP_UTILITY_DIALOG_QUIT :

				sceUtilitySavedataShutdownStart();
				break;

			case PSP_UTILITY_DIALOG_FINISHED :
				if (mode == PSP_UTILITY_SAVEDATA_LISTLOAD){
					this->currLevelId = this->saveGame.level;
					this->playerInfo.mana = this->saveGame.mana;
				}
			case PSP_UTILITY_DIALOG_NONE :
				saveRunning = false;
		}
		sceDisplayWaitVblankStart();
		sceGuSwapBuffers();
		sceKernelDelayThread(10000);
	}

	//clean up some data
	if (saveData.icon0FileData.buf)
		free(saveData.icon0FileData.buf);

	//start drawing again with gu...
	this->startGu();
	return;
}


