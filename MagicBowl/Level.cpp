/*
 * Level.cpp
 *
 *  Created on: Dec 30, 2009
 *      Author: andreborrmann
 */

extern "C"{
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <pspkernel.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspctrl.h>
#include <pspdebug.h>
}

#include "Level.h"
#include "SimpleTimer.h"
#include "MagicBowlApp.h"
#include <ClVectorHelper.h>
#include "TextHelper.h"
#include "Mp3Helper.h"
#include <ClAnimMaterialColor.h>
#include <ClParticleFlare.h>

#include <algorithm>


using namespace monzoom;
using namespace std;

ClLevel::ClLevel(unsigned short id, MDLFileData levelData, ClMagicBowlApp* app) {
	// TODO Auto-generated constructor stub
	currentState = MBL_INITIALIZING;

	this->id = id;
	//store the level DetailData
	lvlFileData = levelData;

	// for the rendering of this level we set some
	// gu states
	//flush all cache data
	sceKernelDcacheWritebackAll();
	//move the view to an initial position
	sceGumMatrixMode(GU_VIEW);
	sceGumLoadIdentity();
	//enable some lightning
	sceGuEnable(GU_LIGHTING);
	//disable previously set lights
	sceGuDisable(GU_LIGHT0);
	sceGuDisable(GU_LIGHT1);
	sceGuDisable(GU_LIGHT2);
	sceGuDisable(GU_LIGHT3);
	sceGuShadeModel(GU_SMOOTH);
	ClMagicBowlApp::initRenderTarget();

	//enable the analog pad
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
	switchSoundPlaying=false;

	timerTex = 0;
	runTime = 0;
	touchObject = 0;

	firstRender = true;
	viewAngle = 0.0f;
	r3dAngle = 0.75;//1.25f;
	viewDistance = 100.0f;

	initProgress = 0;
	this->app = app;

	//particle = 0;
}

void ClLevel::initLevel(bool triggerFlipped) {
	int initThread;
	/*
	 * some initialization not necessarily need to be performed in a seperate thread
	 */
	if (triggerFlipped)
		this->viewAngleChange = -1.25f;
	else
		this->viewAngleChange = 1.25f;

	/*
	 * setup and start init thread for asynchronous initialization...
	 */
	initThread = sceKernelCreateThread("initLevel", ClLevel::initThread, 0x12, 0x40000, THREAD_ATTR_USER | THREAD_ATTR_VFPU, 0);
	initProgress = 0;
	if (initThread >= 0){
		ThreadParams params;
		params.level = this;
		sceKernelStartThread(initThread, sizeof(ThreadParams), &params);
		//ClLevel::initThread(sizeof(ThreadParams), &params);
	} else {
		currentState = MBL_INIT_FAILED;
	}
}

ClLevel::~ClLevel() {
	// TODO Auto-generated destructor stub
	delete(timer);
	ClSceneMgr::freeInstance();
	if (timerTex)
		ClTextureMgr::getInstance()->freeTexture(timerTex->id);

	/*if (particle)
		delete(particle);
		*/
}

bool ClLevel::SceneSortPredicate(monzoom::ClSceneObject* o1, monzoom::ClSceneObject* o2){
	//returns true if o1 is less than o2, which in my case is true
	//if o1 has none transparent material but o2 has
	#define IS_ALPHA(color) (((color)&0xff000000)==0xff000000?0:1)

	ClStaticSceneObject* so1 = (ClStaticSceneObject*)o1;
	ClStaticSceneObject* so2 = (ClStaticSceneObject*)o2;
	if (IS_ALPHA(so1->getMaterial()->getColor())){
		//o1 is transparent
		if (!IS_ALPHA(so2->getMaterial()->getColor()))
			//if o2 is not, o1 is "greater"
			return false;
		else
			//if o1 and o2 are transparent they are the "same"
			return false;
	} else {
		if (IS_ALPHA(so2->getMaterial()->getColor()))
			//if o1 is not transparent, but o2 --> o1 < o2!
			return true;
		else
			//if o1 and o2 are transparent they are the "same"
			return false;
	}

}
void ClLevel::render(short red_cyan){
	//if the renderer was called the first time
	//do some on-time stuff only valid at this point
	if (firstRender){
		firstRender = false;

		//if the first render pass is called we try to sort the object list
		//to make sure transparent objects are rendered last
		std::vector<ClSceneObject*>* scene;
		scene = levelScene->getChildsRef();
		std::sort((*scene).begin(), (*scene).end(), ClLevel::SceneSortPredicate);
		//if rendered first time deliver the
		//immediate events
		//trigger all immediate events
		ClEventManager::triggerEvent((void*)0);

		//set the ambient color a bit brighter if rendered using 3D
		if (red_cyan){
			levelScene->setAmbientLight(0xff999999);
		}
	}
	//initialize the timer
	int dummy = 1;
	if (!timer->isReset()) timer->reset();
	if(!runTime->isReset()) runTime->reset();
	//to enable constant application of physics and movement
	//appart from render speed we use the timer to handle this
	//movement etc. will be calculated 40 times per second
	//in the case we are running with in game menu we do not apply any movement/physics
	//TODO: the timer need to be set on hold...
	if (currentState != MBL_INGAME_MENU){
		if (timer->getDeltaSeconds() >= 0.025f) {
			//move the bowl dependent on the physics
			touchObject = sBowl->move(physics.gravity);//9.81f);//physics.gravity);
			timer->reset();

			//as we have moved the bowl now, check if we have collision with
			//an object and if so act dependend on this object
			if (touchObject){
				if (strcmp(touchObject->getName(), "Plane_Target")==0){
					currentState = MBL_TASK_FINISHED;
					//the time finished before end will be added as MANA
					app->playerInfo.mana+= (maxTime - runTime->getDeltaSeconds())*10;
					if (app->playerInfo.mana > MAX_MANA)
						app->playerInfo.mana = MAX_MANA;
					return;
				}
			}
		}
	}


	const ScePspFVector4* bowlPos;
	ScePspFMatrix4 lightProjInf;
	ScePspFMatrix4 lightView;

	bowlPos = sBowl->getPosition();

	//for subsequential render passes we place the camera to look
	//from a given distance at the bowl
	ScePspFVector3 camPos = {0.0f, 0.0f, -viewDistance};
	ScePspFVector3 lookAt = {-bowlPos->x, -bowlPos->y, -bowlPos->z};
	ScePspFMatrix4 viewM;
	//place the camera free around the bowl
	//looking at the same
	gumLoadIdentity(&viewM);
	//if (red_cyan)
		//rotate the camera by 1.0 degrees
		//gumRotateY(&viewM,GU_PI*(-r3dAngle)/180.0f);
	//keep a distance to the bowl
	gumTranslate(&viewM, &camPos);
	//rotate on X axis by 15?
	gumRotateX(&viewM, 50.0f*GU_PI/180.0f);
	//rotate by viewAngle on Y axis
	if (red_cyan)
		gumRotateY(&viewM, (viewAngle-r3dAngle)*GU_PI/180.0f);
	else
		gumRotateY(&viewM, viewAngle*GU_PI/180.0f);
	//move to the bowl position after all
	gumTranslate(&viewM, &lookAt);
	//set the camera of the scene looking at the bowl
	levelScene->getActiveCamera()->setViewMatrix(&viewM);

	//render the scene from camera point of view
	//using default settings
	//render the scene normal
	//in the case we would like to have red/cyan we set the render target to be offscreen
	const void* ptr = (void *)0x154000;
	const void* offScreen;
	if (red_cyan){
		offScreen = (void*)((int)sceGeEdramGetAddr() + (int)ptr);
		sceGuDrawBufferList(GU_PSM_8888, (void*)ptr, 512); //offscreen = 512x512
		sceGuClearColor(0xff000000);
		sceGuClearDepth(0);
		sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
	}

	levelScene->render();
/*
	if (id == 7){
		//NEW Particle Testing in level 7
		//being a waterfall
		if (particle == 0){
			particle = new ParticleContext_t();

			// Make a particle group
			int particle_handle = particle->GenParticleGroups(1, 800);

			particle->CurrentGroup(particle_handle);

		}
		particle->Velocity(PDBlob(pVec(0.03f, -0.001f, 0.001f), 0.002f));
		//creation color between light blue and white
		particle->Color(pVec(0.0f, 0.0f, 1.0f),1.0f);//PDLine(pVec(1.0f, 1.0f, 1.0f), pVec(1.0f, 1.0f, 1.0f)));
		//createn size
		particle->Size(4.0f);
		//creation of particles from a line
		particle->Source(5, PDLine(pVec(-20.0f, 42.0f, -50.0f), pVec(22.0f, 42.0f, -50.0f)));
		particle->Gravity(pVec(0.0f, -0.4f, 0.0f));

		//bounce the "water" at a rectangle
		particle->Bounce(0.0f, 0.0f, 0.0f,PDRectangle(pVec(-25.0f,29.0f, -35.0f), pVec(55.0f, 0.1f, 0.0f), pVec(0.0f, 0.4f, -40.0f)));
		particle->Bounce(-0.01f, 0.1f, 0.0f,PDRectangle(pVec(-30.0f,5.0f, -28.0f), pVec(40.0f, 0.0f, 0.0f), pVec(0.0f, 0.4f, -10.0f)));
		particle->Move(true, false);

		particle->KillOld(300.0f);
		particle->Sink(false, PDPlane(pVec(0.0f,0.0f,0.0f), pVec(0.0f,1.0f,0.0f)));


		//render the particles
		{
			ColorFVertex *verts;
			ScePspFMatrix4 viewMatrix;

			sceGumMatrixMode(GU_VIEW);
			sceGumStoreMatrix(&viewMatrix);
			sceGumMatrixMode(GU_MODEL);
			sceGumLoadIdentity();
			sceGuDisable(GU_CULL_FACE);

			//disabling the z-test does allow smooth particles across the scene
			//independent of there real depth where the transparency does have negative impact if
			//they are rendered not in the best z-order. However, the side effect would be that the
			//particles are visible through all objects and seem to be always in front of them
			//try to overcome this issue in enabling depth test on the scene but do not fill depth buffer with
			//particles - this solves both issues
			sceGuDepthMask(true);
			sceGuDisable(GU_TEXTURE_2D);
			sceGuEnable(GU_BLEND);
			sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

			//we would like to render a particle always as frontfacing to the view
			//therefore we would need to calculate the diamont to be created based on the view orientation
			//this one is given as the view matrix.
			ScePspFVector4 rl, fb;
			ScePspFMatrix4 viewInvers;

			gumFastInverse(&viewInvers, &viewMatrix);
			rl = viewInvers.x;
			fb = viewInvers.y;

			size_t cnt = particle->GetGroupCount();
			float *ptr;
			size_t flstride, pos3Ofs, posB3Ofs, size3Ofs, vel3Ofs, velB3Ofs, color3Ofs, alpha1Ofs, age1Ofs, up3Ofs, rvel3Ofs, upB3Ofs, mass1Ofs, data1Ofs;
			if (cnt > 0){
				cnt = particle->GetParticlePointer(ptr, flstride, pos3Ofs, posB3Ofs,
					size3Ofs, vel3Ofs, velB3Ofs, color3Ofs, alpha1Ofs, age1Ofs,
					up3Ofs, rvel3Ofs, upB3Ofs, mass1Ofs, data1Ofs);
				if(cnt > 0) {
					//cnt contains the number of particles neesd to be drawn
					//ptr is the pomter to the data and the ofs set the stzart position of the
					//data within the memory space
					//we need to convert the particles into vertices
					for (int p = 0; p < cnt; p++){
						verts = (ColorFVertex*)sceGuGetMemory(sizeof(ColorFVertex)*10);
						//float age = ptr[age1Ofs+p*flstride];
						//center of the particle being transparent dependend on the energy of the particle
						verts[0].x = ptr[pos3Ofs+p*flstride];
						verts[0].y = ptr[pos3Ofs+p*flstride+1];
						verts[0].z = ptr[pos3Ofs+p*flstride+2];
						verts[0].color = GU_COLOR(ptr[color3Ofs+p*flstride], ptr[color3Ofs+p*flstride+1], ptr[color3Ofs+p*flstride+2], ptr[alpha1Ofs+p*flstride]);//]1.0f);
						//verts[0].color |= 0xff000000;

						verts[1].x = verts[0].x + ptr[size3Ofs+p*flstride]*rl.x;
						verts[1].y = verts[0].y + ptr[size3Ofs+p*flstride]*rl.y;
						verts[1].z = verts[0].z + ptr[size3Ofs+p*flstride]*rl.z;
						verts[1].color = verts[0].color & 0x00ffffff;

						verts[2].x = verts[0].x + ptr[size3Ofs+p*flstride]*rl.x + ptr[size3Ofs+p*flstride]*fb.x;
						verts[2].y = verts[0].y + ptr[size3Ofs+p*flstride]*rl.y + ptr[size3Ofs+p*flstride]*fb.y;
						verts[2].z = verts[0].z + ptr[size3Ofs+p*flstride]*rl.z + ptr[size3Ofs+p*flstride]*fb.z;
						verts[2].color = verts[0].color & 0x00ffffff;

						verts[3].x = verts[0].x + ptr[size3Ofs+p*flstride]*fb.x;
						verts[3].y = verts[0].y + ptr[size3Ofs+p*flstride]*fb.y;
						verts[3].z = verts[0].z + ptr[size3Ofs+p*flstride]*fb.z;
						verts[3].color = verts[0].color & 0x00ffffff;

						verts[4].x = verts[0].x - ptr[size3Ofs+p*flstride]*rl.x + ptr[size3Ofs+p*flstride]*fb.x;
						verts[4].y = verts[0].y - ptr[size3Ofs+p*flstride]*rl.y + ptr[size3Ofs+p*flstride]*fb.y;
						verts[4].z = verts[0].z - ptr[size3Ofs+p*flstride]*rl.z + ptr[size3Ofs+p*flstride]*fb.z;
						verts[4].color = verts[0].color & 0x00ffffff;

						verts[5].x = verts[0].x - ptr[size3Ofs+p*flstride]*rl.x;
						verts[5].y = verts[0].y - ptr[size3Ofs+p*flstride]*rl.y;
						verts[5].z = verts[0].z - ptr[size3Ofs+p*flstride]*rl.z;
						verts[5].color = verts[0].color & 0x00ffffff;

						verts[6].x = verts[0].x - ptr[size3Ofs+p*flstride]*rl.x - ptr[size3Ofs+p*flstride]*fb.x;
						verts[6].y = verts[0].y - ptr[size3Ofs+p*flstride]*rl.y - ptr[size3Ofs+p*flstride]*fb.y;
						verts[6].z = verts[0].z - ptr[size3Ofs+p*flstride]*rl.z - ptr[size3Ofs+p*flstride]*fb.z;
						verts[6].color = verts[0].color & 0x00ffffff;

						verts[7].x = verts[0].x - ptr[size3Ofs+p*flstride]*fb.x;
						verts[7].y = verts[0].y - ptr[size3Ofs+p*flstride]*fb.y;
						verts[7].z = verts[0].z - ptr[size3Ofs+p*flstride]*fb.z;
						verts[7].color = verts[0].color & 0x00ffffff;

						verts[8].x = verts[0].x + ptr[size3Ofs+p*flstride]*rl.x - ptr[size3Ofs+p*flstride]*fb.x;
						verts[8].y = verts[0].y + ptr[size3Ofs+p*flstride]*rl.y - ptr[size3Ofs+p*flstride]*fb.y;
						verts[8].z = verts[0].z + ptr[size3Ofs+p*flstride]*rl.z - ptr[size3Ofs+p*flstride]*fb.z;
						verts[8].color = verts[0].color & 0x00ffffff;

						verts[9] = verts[1];

						sceGumDrawArray(GU_TRIANGLE_FAN, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 10, 0, verts);


					}
				}
			}
			sceGuEnable(GU_CULL_FACE);
			sceGuDepthMask(false);
		}
	}
*/
	//in case of red_cyan we need to pass the offscreen onto screen and render the scene a second
	//time with a bit different camera position
	if (red_cyan){
		ClMagicBowlApp::finishGu();
		ClMagicBowlApp::startGu();
		//setup default rendering onto screen
		ClMagicBowlApp::initRenderTarget();
		//clear screen
		sceGuClearColor(0xff000000);
		sceGuClearDepth(0);
		sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
		//2. render the texture across the whole screen with a color filter for cyan
		//red/cay or green/pink
		if (red_cyan == 1)
			//this->drawOffscreen2Screen(0xbfe7b600, offScreen);
			this->drawOffscreen2Screen(0xffffff00, offScreen);
		else
			//this->drawOffscreen2Screen(0xbfe700b6, offScreen);
			this->drawOffscreen2Screen(0xffff00ff, offScreen);

		sceGuEnable(GU_LIGHTING);
		gumLoadIdentity(&viewM);
		//rotate the camera by 1.0 degrees
		//gumRotateY(&viewM,GU_PI*r3dAngle/180.0f);
		//keep a distance to the bowl
		gumTranslate(&viewM, &camPos);
		//rotate on X axis by 15?
		gumRotateX(&viewM, 50.0f*GU_PI/180.0f);
		//rotate by viewAngle on Y axis
		gumRotateY(&viewM, (viewAngle+r3dAngle)*GU_PI/180.0f);
		//move to the bowl position after all
		gumTranslate(&viewM, &lookAt);
		levelScene->getActiveCamera()->setViewMatrix(&viewM);
		//3. second run into offscreen
		sceGuDrawBufferList(GU_PSM_8888, (void*)ptr, 512); //offscreen = 512x512
		sceGuClearColor(0xff000000);
		sceGuClearDepth(0);
		sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);

		levelScene->render();

		ClMagicBowlApp::finishGu();
		ClMagicBowlApp::startGu();
		//setup default rendering to screen
		ClMagicBowlApp::initRenderTarget();
		//do not clear the screen
		sceGuClearDepth(0);
		sceGuClear(GU_DEPTH_BUFFER_BIT);
		//4.render the texture across the whole screen with a color filter for red and 50% transparency
		if (red_cyan == 1){
			this->drawOffscreen2Screen(0x7f0000ff, offScreen);
		}else{
			this->drawOffscreen2Screen(0x7f00ff00, offScreen);
		}

	}
	/*
	 * at the very end we render the level state stuff
	 */
	//the red bar indicates the time left
	float timeLeft = maxTime - runTime->getDeltaSeconds();
	if (timeLeft < 0.0f){
		currentState = MBL_TASK_FAILED;
		return;
	}

	int barLen = (150*timeLeft / maxTime);
	app->blendFrameToScreen(175, 1, barLen, 2, 0xee7777df);
	app->blendFrameToScreen(175, 2, barLen, 9, 0xee2222df);
	app->blendFrameToScreen(175, 11, barLen, 3, 0xee0000df);
	app->blendTextureToScreen(timerTex, 150, 0, 180, 15, 0xff);
	barLen = (130*app->playerInfo.mana / MAX_MANA);
	app->blendFrameToScreen(4, 215-barLen, 2, barLen, 0xeedf77df);
	app->blendFrameToScreen(6, 215-barLen, 9, barLen, 0xeedf22df);
	app->blendFrameToScreen(15, 215-barLen, 3, barLen, 0xeedf00df);
	app->blendTextureToScreen(manaTex, 3, 80, 16, 150, 0xff);
	/*
	 * once the scene is completely rendered draw some other stuff
	 */
	if (touchObject){
/*		if (strcmp(touchObject->getName(), "Tipp_Level4_1")==0){
			//render some hint as we are on the training start platform
			this->app->blendFrameToScreen(95, 85, 240, 45, 0x99bb4400);
			clTextHelper::getInstance()->writeTextBlock(SERIF_SMALL_REGULAR, "You need to find the switch, to activate this bridge...", 100, 100, 240);
		}*/
		//trigger event for this object if registered
		ClEventManager::triggerEvent(touchObject);

	}
	//if the bowl position falls underneath -50 than we have failed the level
	if (bowlPos->y < -100.0f){
		currentState = MBL_TASK_FAILED;
		return;
	}
	/*
	 * everything was drawn to the screen,
	 * handle the pad....
	 */
	if (currentState != MBL_INGAME_MENU){
		//do not handle pad if we are in pause mode
		handlePad();
	}
}

/*
 * handle any kind of pad inputs for this level
 */
void ClLevel::handlePad(){
	SceCtrlData pad;
	short padX, padY;

	sceCtrlPeekBufferPositive(&pad, 1);
	//the analog stick returns 0 for top, 255 for down
	//0 for left and 255 for right
	padX = (pad.Lx - 127) >> 5;
	padY = (pad.Ly - 127) >> 5;
	// as the pad's movement directions mapped into the
	//world space are dependend on the current view we first
	//get the ViewMatrix and pass it to the apply function
	ScePspFMatrix4 viewMatrix;
	sceGumMatrixMode(GU_VIEW);
	sceGumStoreMatrix(&viewMatrix);
	sBowl->applyAcceleration(padX, padY, &viewMatrix);

	if (pad.Buttons){
		if (pad.Buttons & PSP_CTRL_START){
			currentState = MBL_INGAME_MENU;
		}

		if (pad.Buttons & PSP_CTRL_UP){
			viewDistance-= 0.5f;
			if (viewDistance<=40.0f)
				viewDistance = 40.0f;
		}
		if (pad.Buttons & PSP_CTRL_DOWN){
			viewDistance+= 0.5f;
			if (viewDistance>=1000.0f)
				viewDistance = 1000.0f;
		}
/*
		if (pad.Buttons & PSP_CTRL_RIGHT){
			r3dAngle-= 0.1f;
		}
		if (pad.Buttons & PSP_CTRL_LEFT){
			r3dAngle+= 0.1f;
		}
		*/
		if (pad.Buttons & PSP_CTRL_LTRIGGER){
			viewAngle+= viewAngleChange;
		}
		if (pad.Buttons & PSP_CTRL_RTRIGGER){
			viewAngle-= viewAngleChange;
		}

		if(pad.Buttons & PSP_CTRL_CIRCLE){
			//activate some speed boost if enough mana exist
			if (app->playerInfo.mana > 0){
				app->playerInfo.mana--;
				sBowl->applyBoost(2.0f);
				//if boost is active start bowl animation (texture)
				ClEventManager::triggerEvent(sBowl);
				//as this event should be able to be re-triggered
				//we reset the triggered flag
				ClEventManager::resetTrigger(sBowl);
			}

		}
	}
}

void ClLevel::drawOffscreen2Screen(unsigned int color, const void* offScreenPtr){

	TextureIVertex* vertex;
	int slice = 64;
	int w = 0;
	//use offscreen as texture
	sceGuEnable(GU_TEXTURE_2D);
	sceGuTexImage(0, 512, 512, 512, offScreenPtr);
	sceGuTexMode(GU_PSM_8888, 0, 0, 0);
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGB);
	sceGuTexFilter(GU_NEAREST, GU_NEAREST);
	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	//sceGuBlendFunc(GU_SUBTRACT, GU_SRC_COLOR, GU_FIX, 0, color);
	sceGuDisable(GU_DEPTH_TEST);
	sceGuDisable(GU_LIGHTING);
	//render the texture across the whole screen
	//do not render complete but in 64Pixel width blocks
	while (w < 480){

		vertex = (TextureIVertex*)sceGuGetMemory(2*sizeof(TextureIVertex));

		if (w+slice > 480)
			slice = 480 - w;
		vertex[0].x = w;
		vertex[0].y = 0;
		vertex[0].z = 0x00ff;
		vertex[0].u = w;
		vertex[0].v = 0;

		vertex[1].x = w+slice;
		vertex[1].y = 272;
		vertex[1].z = 0x00ff;
		vertex[1].u = w+slice;
		vertex[1].v = 272;

		//draw this part
		sceGuColor(color);
		sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, vertex);

		w+=slice;
	}
	sceGuEnable(GU_DEPTH_TEST);
	sceGuDisable(GU_TEXTURE_2D);
	sceGuDisable(GU_BLEND);
}

void ClLevel::renderPassBowlNormal(){
	sceGuDisable(GU_BLEND);
	//sceGuDisable(GU_TEXTURE_2D);
	//render the bowl as usual
	//bowl->render(true);
}

void ClLevel::renderPassObjectsShadowReciever(ScePspFMatrix4* lightProj,ScePspFMatrix4* lightView){
	//now setup the texture derived from the shadowmap
	const void* ptr = (void *)0x48AA88;
	const void* offScreen = (void*)((int)sceGeEdramGetAddr() + (int)ptr);

	sceGuEnable(GU_TEXTURE_2D);
	sceGuTexMapMode(GU_TEXTURE_MATRIX, 0, 0); //texture mapping using matrix instead u, v
	sceGuTexProjMapMode(GU_POSITION); //texture mapped based on the position of the objects relative to the texture

	sceGuTexMode(GU_PSM_5650, 0,0,0);
	sceGuTexImage(0,256,256,256, offScreen);
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);//only apply RGB value of texture
	sceGuTexFilter(GU_LINEAR,GU_LINEAR); //interpolate to have smooth borders
	sceGuTexWrap(GU_CLAMP,GU_CLAMP); //do cut texture not repeat
	//calculate shadow projection matrix
	ScePspFMatrix4 shadowMatrix;
	ScePspFMatrix4 textScaleTrans;
	gumLoadIdentity(&textScaleTrans);
	textScaleTrans.x.x = 0.5f;
	textScaleTrans.y.y = -0.5f;
	textScaleTrans.w.x = 0.5f;
	textScaleTrans.w.y = 0.5f;

	gumMultMatrix(&shadowMatrix, lightProj, lightView);
	gumMultMatrix(&shadowMatrix, &textScaleTrans, &shadowMatrix);
	sceGuSetMatrix(GU_TEXTURE,&shadowMatrix);

	//pass only if the z component equals already rendered objects
	sceGuDepthFunc(GU_EQUAL);
	//do not reflect this pass within the Z buffer
	sceGuDepthMask(GU_TRUE);
	sceGuEnable(GU_BLEND);
	//sceGuBlendFunc(GU_ADD, GU_DST_COLOR, GU_ONE_MINUS_DST_COLOR, 0, 0);
	sceGuBlendFunc(GU_ADD, GU_DST_COLOR, GU_FIX, 0x00000000, 0x00000000);

	sceGumMatrixMode(GU_MODEL);
	sceGumLoadIdentity();

	levelScene->render();
	/*unsigned int szeneObjects = guList.objectCount;//szene->getObjectCount();
	for (unsigned int i=0;i<szeneObjects;i++){
		ClMzGu3dObject* object = guList.guObjects[i];
		//render without own texture
		object->render(false);
	}*/
	//store back Z-Buffer and DepthFunc
	sceGuDepthFunc(GU_GEQUAL);
	sceGuDepthMask(GU_FALSE);
}

void ClLevel::renderPassObjectsNormal(){
	sceGumMatrixMode(GU_MODEL);
	sceGumLoadIdentity();
	/*
	unsigned int szeneObjects = guList.objectCount;

	for (unsigned int i=0;i<szeneObjects;i++){
		ClMzGu3dObject* object = guList.guObjects[i];
		//render the object with the texture if available
		object->render(true);
	}
	*/
}

/*
 * render the bowl as shadow caster into an offscreen area
 * from the light point of view. This can be used as texture
 * in later calls for the shadow receiver
 */
void ClLevel::renderPassBowlShadowCast(ScePspFVector4* bowlPos, ScePspFMatrix4* lightProjInf,ScePspFMatrix4* lightView){
	/* first set the view position to the Light position and
	 * create the view as well as the projection matrix
	 */
	ScePspFMatrix4 lightProj;
	//set the view and projection matrix to render the shadow caster
	//from the lights point of view
	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	sceGumPerspective(45.0f, 1.0f, 0.1f, 500.0f);
	sceGumStoreMatrix(&lightProj);
	gumLoadIdentity(lightProjInf);
	gumPerspective(lightProjInf,45.0f, 1.0f, 0.0f, 500.0f);
	ScePspFVector3 lightPos = {lvlFileData.lightPosX, lvlFileData.lightPosY, lvlFileData.lightPosZ};
	ScePspFVector3 lightUp = {0.0f, 1.0f, 0.0f};
	ScePspFVector3 lightLookAt = {bowlPos->x, bowlPos->y,bowlPos->z};
	sceGumMatrixMode(GU_VIEW);
	sceGumLoadIdentity();
	sceGumLookAt( &lightPos, &lightLookAt, &lightUp);
	sceGumStoreMatrix(lightView);

	/*
	 * now we want to render from there to the offscreen buffer.
	 * setup the static offscreen pointer for this call
	 */
	const void* ptr = (void *)0x48AA88;
	const void* offScreen = (void*)((int)sceGeEdramGetAddr() + (int)ptr);
	sceGuDrawBufferList(GU_PSM_5650, (void*)ptr, 256); //shadow map will be 128x128

	//the viewport size need to fit the offscreen size which is 128x128
	sceGuOffset(2048 - 128,2048 - 128);
	sceGuViewport(2048, 2048, 256, 256);
	//clear the offscreen with white to render the shadow in black
	sceGuClearColor(0xffffffff);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
	//disable light and texture to force the object will
	//be rendered black
	//sceGuDisable(GU_LIGHTING);
	sceGuDisable(GU_LIGHT0);
	//sceGuDisable(GU_TEXTURE_2D);
	sceGuDisable(GU_BLEND);
	sceGuAmbientColor(0xff000000);

	//draw the shadow caster
	//bowl->render(false);

	//close the GU List to make sure the current rendering will be
	//committed to the GU and the result available in the offscreen area
	ClMagicBowlApp::finishGu();

	/*
	 * to enable the following calls working as expected we need to
	 * start a new GU list and restore the render target and some GU
	 * attributes
	 */
	ClMagicBowlApp::startGu();

	//now switch back to normal rendering into drawbuffer
	ClMagicBowlApp::initRenderTarget();
	sceGuClearColor(0xff000000);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
	//enable the lights
	//sceGuEnable(GU_LIGHTING);
	sceGuEnable(GU_LIGHT0);
	sceGuAmbientColor(0xff111111);
}

int ClLevel::initThread(SceSize args, void *argp){
	ThreadParams* params;
	char* mp3;
	params = (ThreadParams*)argp;

	ClLevel* level = params->level;

	ScePspFVector4 bowlPos;
	bowlPos.x = level->lvlFileData.bowlPosX;
	bowlPos.y = level->lvlFileData.bowlPosY;
	bowlPos.z = level->lvlFileData.bowlPosZ;
	bowlPos.w = 0.0f;

	level->initProgress = 10;

	level->timer = new ClSimpleTimer();
	level->runTime = new ClSimpleTimer();
	level->maxTime = level->lvlFileData.maxTime;

	level->physics.gravity = level->lvlFileData.gravity;

	level->camPos.x = level->camPos.y = level->camPos.z = 0.0f;
	//delay the current thread to allow main thread continuing
	sceKernelDelayThread(10000);//half a second

	level->initProgress = 25;
	//load the 3D Level Objects from Monzoom file and init the
	//scene
	level->levelScene = ClSceneMgr::getInstance();
	level->levelScene->loadSceneFromMON(level->lvlFileData.objectFile);

	//add additional object to the scene
	level->sBowl = new ClBowlPlayer(&bowlPos, 1.5f, level->levelScene);
/*	monzoom::ClMaterialTexture* bowlTex = new monzoom::ClMaterialTexture();
	bowlTex->setColor(0xffffffff);
	bowlTex->setTexture(monzoom::ClTextureMgr::getInstance()->loadFromPNG("Bowl.png"));
	level->sBowl->setMaterial(bowlTex);*/
	level->levelScene->addStaticSceneObject(level->sBowl);
	//delay the current thread to allow main thread continuing
	sceKernelDelayThread(10000);//half a second

	level->initProgress = 50;
	//do set some of the scene objects to cast shadows
	std::vector<ClSceneObject*>* objects = level->levelScene->getChildsRef();
	for (int o=0;o<objects->size();o++){
		level->initProgress = 50 + 30*o/objects->size();
		//all objects except the outbox casting shadows
		if ( strstr(objects->at(o)->getName(), "OutBox")==0 )
			objects->at(o)->setCastShadow(true);
	}
	level->initProgress = 80;

	level->timerTex = ClTextureMgr::getInstance()->loadFromPNG("Timer.png");
	level->manaTex = ClTextureMgr::getInstance()->loadFromPNG("Mana.png");

	sceKernelDelayThread(10000);

	level->initProgress = 85;
	//now setup the actions for good performance
	ClStaticSceneObject* target;
	for (unsigned short a = 0;a < level->lvlFileData.actions; a++){
		level->initProgress++;
		//search for the mentioned source object, if this is "NULL" this
		//this is an action executed immediately
		if (strcmp(level->lvlFileData.actionList[a].sourceObject, "NULL")==0){
			for (int t=0;t<level->levelScene->getChilds().size();t++){
				if (strcmp(level->levelScene->getChilds()[t]->getName(),level->lvlFileData.actionList[a].targetObject)==0){
					switch (level->lvlFileData.actionList[a].action){
					case 1:
						//material based action, get the material of the target object and
						//register the event
						target = (ClStaticSceneObject*)level->levelScene->getChilds()[t];
						ClEventManager::registerEvent(0,target->getMaterial(), ClAnimMaterialColor::eventReciever, 0);
						break;
					case 2:
						//object based action
						//we need to pass the action parameter as a "statefull" memory address
						//therefore it is a membervariable of the LEVEL-Class
						level->sosInactive = SOS_INACTIVE;
						ClEventManager::registerEvent(0,level->levelScene->getChilds()[t], ClStaticSceneObject::eventReciever, &level->sosInactive);
						break;
					case 3:
						ClEventManager::registerEvent(0,level->levelScene->getChilds()[t], ClAnimatedSceneObject::eventReciever, 0);
						break;
					}
				}
			}
		} else {
			//search for the source object and store the object pointer
			for (int o=0;o<level->levelScene->getChilds().size();o++){
				if (strcmp(level->levelScene->getChilds()[o]->getName(),level->lvlFileData.actionList[a].sourceObject)==0){
					//we do know the source object, find the target one,
					//but for some actions the "target" is not object in the scene
					//first check for them
					switch (level->lvlFileData.actionList[a].action){
					case 5:
						//in this case target points to the mp3 filename played as result of this action
						mp3 = (char*)malloc(strlen(level->lvlFileData.actionList[a].targetObject)+1);
						memcpy(mp3, level->lvlFileData.actionList[a].targetObject, strlen(level->lvlFileData.actionList[a].targetObject)+1);
						ClEventManager::registerEvent(level->levelScene->getChilds()[o],level, ClLevel::eventReciever, mp3);
						break;
					default:
						for (int t=0;t<level->levelScene->getChilds().size();t++){
							if (strcmp(level->levelScene->getChilds()[t]->getName(),level->lvlFileData.actionList[a].targetObject)==0){
								//we know the source and the target object, register this
								//event based on the action
								switch (level->lvlFileData.actionList[a].action){
								case 1:
									//material based action, get the material of the target object and
									//register the event
									target = (ClStaticSceneObject*)level->levelScene->getChilds()[t];
									ClEventManager::registerEvent(level->levelScene->getChilds()[o],target->getMaterial(), ClAnimMaterialColor::eventReciever, 0);
									break;
								case 2:
									//object based action
									level->sosVisible = SOS_VISIBLE;
									ClEventManager::registerEvent(level->levelScene->getChilds()[o],level->levelScene->getChilds()[t], ClStaticSceneObject::eventReciever, &level->sosVisible);
									break;
								case 3:
									ClEventManager::registerEvent(level->levelScene->getChilds()[o],level->levelScene->getChilds()[t], ClAnimatedSceneObject::eventReciever, 0);
									break;
								case 4:
									int* param = (int*)malloc(sizeof(int));
									*param = 1;
									ClEventManager::registerEvent(level->levelScene->getChilds()[o],level->levelScene->getChilds()[t], ClAnimatedSceneObject::eventReciever, param);
									break;
								}

							}
						}

						break;
					}
				}
			}
		}
		sceKernelDelayThread(10000);
	}
	level->initProgress = 90;
	level->currentState = MBL_INIT_SUCCESS;

	level->initProgress = 100;
	return sceKernelExitDeleteThread(0);
}

int ClLevel::getInitProgress(){
	return initProgress;
}
LevelStates ClLevel::getLevelState(){
	return currentState;
}
void ClLevel::setLevelState(LevelStates newState){
	currentState = newState;
}

void ClLevel::eventReciever(void* thisPtr, void* evtParam){
	ClLevel* mThis = (ClLevel*)thisPtr;
	if (evtParam != 0){
		char* mp3 = (char*)evtParam;
		ClMp3Helper::playMP3(mp3);

	}
}
