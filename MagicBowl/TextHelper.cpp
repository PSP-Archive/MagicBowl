/*
 * TextHelper.cpp
 *
 *  Created on: 28.10.2009
 *      Author: andreborrmann
 */

#include "TextHelper.h"

extern "C"{
#include <pspgu.h>
#include <stdio.h>
}

clTextHelper* clTextHelper::_instance = 0;

clTextHelper* clTextHelper::getInstance() {
	if (!_instance){
		_instance = new clTextHelper();
	}

	return _instance;
}

clTextHelper::clTextHelper() {
	// TODO Auto-generated constructor stub
	intraFontInit();
	char file[40];
	for (short i=0;i<16;i++){
		sprintf(file, "flash0:/font/ltn%d.pgf", i);
		font[i] = intraFontLoad(file,0);
		intraFontSetStyle(font[i], 1.0f, 0xffffffff, 0xff3f3f3f, 0);
	}

	symbols = intraFontLoad("flash0:/font/arib.pgf",0);
	intraFontSetStyle(symbols, 0.8f, 0xffffffff, 0xff3f3f3f, 0);

	//fontStdA = intraFontLoad("flash0:/font/ltn4.pgf",0);
}

clTextHelper::~clTextHelper() {
	// TODO Auto-generated destructor stub
	for (short i=0;i<16;i++){
		intraFontUnload(font[i]);
	}
	//intraFontUnload(fontStdA);
	intraFontShutdown();
}

float clTextHelper::writeText(fontType ft, const char *text, short tx, short ty){
	//assuming sceGuSart was already called ...

	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

	float newX = intraFontPrint(font[ft], tx, ty, text);
	//intraFont call does destroy the kind of texture handle
	//restore is needed:
	sceGuTexFilter(GU_NEAREST, GU_NEAREST);
	sceGuTexMode(GU_PSM_8888, 0, 0, 0);

	sceGuDisable(GU_BLEND);
	return newX;
}

float clTextHelper::writeSymbol(unsigned short* sym, short tx, short ty){
	//assuming sceGuSart was already called ...

	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

	float newX = intraFontPrintUCS2(symbols, tx, ty, sym);
	//intraFont call does destroy the kind of texture handle
	//restore is needed:
	sceGuTexFilter(GU_NEAREST, GU_NEAREST);
	sceGuTexMode(GU_PSM_8888, 0, 0, 0);

	sceGuDisable(GU_BLEND);
	return newX;
}

void clTextHelper::writeTextBlock(fontType ft, const char *text, short tx, short ty, short width){
	//assuming sceGuSart was already called ...

	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

	intraFontPrintColumn(font[ft], tx, ty, width, text);
	//intraFont call does destroy the kind of texture handle
	//restore is needed:
	sceGuTexFilter(GU_NEAREST, GU_NEAREST);
	sceGuTexMode(GU_PSM_8888, 0, 0, 0);

	sceGuDisable(GU_BLEND);
}


