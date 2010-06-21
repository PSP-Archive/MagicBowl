/*
 * TextHelper.h Simple Texthelper class uses intraFont to display
 * text on PSP screen during 3D rendered scenes
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

#ifndef CLTEXTHELPER_H_
#define CLTEXTHELPER_H_

extern "C" {
#include "intraFont.h"
}

typedef enum {
	SANSERIF_REGULAR = 0,
	SANSERIF_ITALIC  = 2,
	SANSERIF_BOLD    = 4,
	SANSERIF_ITALIC_BOLD = 6,

	SANSERIF_SMALL_REGULAR = 8,
	SANSERIF_SMALL_ITALIC  = 10,
	SANSERIF_SMALL_BOLD    = 12,
	SANSERIF_SMALL_ITALIC_BOLD = 14,

	SERIF_REGULAR = 1,
	SERIF_ITALIC  = 3,
	SERIF_BOLD    = 5,
	SERIF_ITALIC_BOLD = 7,

	SERIF_SMALL_REGULAR = 9,
	SERIF_SMALL_ITALIC  = 11,
	SERIF_SMALL_BOLD    = 13,
	SERIF_SMALL_ITALIC_BOLD = 15,

}fontType;

class clTextHelper {
public:
	static clTextHelper* getInstance();
	virtual ~clTextHelper();
	float writeText(fontType ft, const char* text, short tx, short ty);
	float writeSymbol(unsigned short* sym, short tx, short ty);
	void writeTextBlock(fontType ft, const char* text, short tx, short ty, short width);
protected:
	static clTextHelper* _instance;
	clTextHelper();
	intraFont* font[16];
	intraFont* symbols;
};

#endif /* CLTEXTHELPER_H_ */
