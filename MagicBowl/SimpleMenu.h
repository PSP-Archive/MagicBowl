/*
 * SimpleMenu.h This provides a very simple graphic menu to be used
 *              within GU applications. If a menu will be rendered
 *              it will handle the buttons on its own. Therefore
 *              during Menu display some buttons will be reserved for
 *              the same. This are:
 *              	UP, DOWN, CROSS...
 */

#ifndef SIMPLEMENU_H_
#define SIMPLEMENU_H_

extern "C"{
#include <pspctrl.h>
}

#define MAX_MENU_ITEMS 20

class ClSimpleMenuItem {
public:
	ClSimpleMenuItem(const char* title, short action);
	virtual ~ClSimpleMenuItem();

	const char* title;
	short actionId;
};

class ClSimpleMenu {
public:
	ClSimpleMenu(short posX, short posY);
	virtual ~ClSimpleMenu();
	void addItem(ClSimpleMenuItem* item);

	/*
	 * this renders and handles all actions an the menu
	 * if an action item was selected and "executed" it will
	 * return the id of the action the menu item was created with
	 */
	short handleMenu();

protected:
	short pX, pY;
	ClSimpleMenuItem** menuItems;
	short selectedItem;
	short itemCount;
	SceCtrlData lastPad;

	short handlePad();
};

#endif /* SIMPLEMENU_H_ */
