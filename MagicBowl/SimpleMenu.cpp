/*
 * SimpleMenu.cpp
 *
 *  Created on: Feb 26, 2010
 *      Author: andreborrmann
 */

extern "C"{
#include <pspgu.h>
#include <malloc.h>
}
#include "SimpleMenu.h"
#include "TextHelper.h"
#include "Mp3Helper.h"
#include <psptypes3d.h>

ClSimpleMenu::ClSimpleMenu(short posX, short posY) {
	// TODO Auto-generated constructor stub
	this->menuItems = (ClSimpleMenuItem**)malloc(sizeof(ClSimpleMenuItem*) * MAX_MENU_ITEMS);
	itemCount = 0;
	selectedItem = 0;
	pX = posX;
	pY = posY;

	//during initialization get the last key pressed
	sceCtrlPeekBufferPositive(&lastPad, 1);
}

ClSimpleMenu::~ClSimpleMenu() {
	// TODO Auto-generated destructor stub
	for (int i=0;i<itemCount;i++){
		delete(menuItems[i]);
	}
	free(menuItems);
}

void ClSimpleMenu::addItem(ClSimpleMenuItem *item){
	menuItems[itemCount] = item;
	itemCount++;
	//the first item is always the selected one during initialization
	if (!selectedItem) selectedItem = itemCount;
}

short ClSimpleMenu::handleMenu(){
	//each menu item will contain a background
	//represented by a transparent background and a text on top of the some
	clTextHelper* textHlp = clTextHelper::getInstance();
	//the background is stored within 2 vertices (SPRITE)
	ColorIVertex* vertex;
	//sceGuDisable(GU_TEXTURE);

	for (int i=0;i<itemCount;i++){
		vertex = (ColorIVertex*)sceGuGetMemory(sizeof(ColorIVertex)*2);
		vertex[0].x = pX-10;
		vertex[0].y = pY+32*i - 20;
		vertex[0].z = 0xffff;

		vertex[1].x = pX+250;
		vertex[1].y = pY+32*i + 10;
		vertex[1].z = 0xffff;
		if (i == (this->selectedItem - 1)){
			vertex[0].color = vertex[1].color = 0x66ff3333;
		}else{
			vertex[0].color = vertex[1].color = 0x66551111;
		}
		sceGuEnable(GU_BLEND);
		sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
		sceGuDisable(GU_TEXTURE_2D);
		sceGuDrawArray(GU_SPRITES, GU_TRANSFORM_2D | GU_COLOR_8888 | GU_VERTEX_16BIT, 2, 0, vertex);

		textHlp->writeText(SANSERIF_REGULAR, menuItems[i]->title, pX, pY+32*i);
	}
	sceGuDisable(GU_BLEND);

	return handlePad();
}

ClSimpleMenuItem::ClSimpleMenuItem(const char* title, short action) {
	// TODO Auto-generated constructor stub
	this->title = title;
	this->actionId = action;
}

ClSimpleMenuItem::~ClSimpleMenuItem() {
	// TODO Auto-generated destructor stub
}

short ClSimpleMenu::handlePad(){
	SceCtrlData pad;

	sceCtrlPeekBufferPositive(&pad, 1);
		//button pressed ?
	if (pad.Buttons != 0 && lastPad.Buttons != pad.Buttons){
		if (pad.Buttons & PSP_CTRL_DOWN){
			if (selectedItem < this->itemCount){
				//if item change play change sound
				ClMp3Helper::playMP3("snd//whup11.mp3");
				selectedItem++;
			}
		}
		if (pad.Buttons & PSP_CTRL_UP){
			if (selectedItem > 1){
				//if item change play change sound
				ClMp3Helper::playMP3("snd//whup11.mp3");
				selectedItem--;
			}
		}
		if (pad.Buttons & PSP_CTRL_CROSS){
			//if item selected play select sound
			ClMp3Helper::playMP3("snd//blubb1.mp3");
			return this->menuItems[selectedItem-1]->actionId;
		}
	}
	lastPad = pad;

	return -1;
}


