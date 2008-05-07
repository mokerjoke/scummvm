/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#include "kyra/kyra_v1.h"
#include "kyra/screen.h"
#include "kyra/script.h"
#include "kyra/text.h"
#include "kyra/animator_v1.h"
#include "kyra/sound.h"
#include "kyra/gui_v1.h"

#include "common/config-manager.h"
#include "common/savefile.h"
#include "common/events.h"
#include "common/system.h"

namespace Kyra {

void KyraEngine_v1::initMainButtonList() {
	_buttonList = &_buttonData[0];
	for (int i = 0; _buttonDataListPtr[i]; ++i)
		_buttonList = _gui->addButtonToList(_buttonList, _buttonDataListPtr[i]);
}

int KyraEngine_v1::buttonInventoryCallback(Button *caller) {
	int itemOffset = caller->index - 2;
	uint8 inventoryItem = _currentCharacter->inventoryItems[itemOffset];
	if (_itemInHand == -1) {
		if (inventoryItem == 0xFF) {
			snd_playSoundEffect(0x36);
			return 0;
		} else {
			_screen->hideMouse();
			_screen->fillRect(_itemPosX[itemOffset], _itemPosY[itemOffset], _itemPosX[itemOffset] + 15, _itemPosY[itemOffset] + 15, 12);
			snd_playSoundEffect(0x35);
			setMouseItem(inventoryItem);
			updateSentenceCommand(_itemList[inventoryItem], _takenList[0], 179);
			_itemInHand = inventoryItem;
			_screen->showMouse();
			_currentCharacter->inventoryItems[itemOffset] = 0xFF;
		}
	} else {
		if (inventoryItem != 0xFF) {
			snd_playSoundEffect(0x35);
			_screen->hideMouse();
			_screen->fillRect(_itemPosX[itemOffset], _itemPosY[itemOffset], _itemPosX[itemOffset] + 15, _itemPosY[itemOffset] + 15, 12);
			_screen->drawShape(0, _shapes[216+_itemInHand], _itemPosX[itemOffset], _itemPosY[itemOffset], 0, 0);
			setMouseItem(inventoryItem);
			updateSentenceCommand(_itemList[inventoryItem], _takenList[1], 179);
			_screen->showMouse();
			_currentCharacter->inventoryItems[itemOffset] = _itemInHand;
			_itemInHand = inventoryItem;
		} else {
			snd_playSoundEffect(0x32);
			_screen->hideMouse();
			_screen->drawShape(0, _shapes[216+_itemInHand], _itemPosX[itemOffset], _itemPosY[itemOffset], 0, 0);
			_screen->setMouseCursor(1, 1, _shapes[0]);
			updateSentenceCommand(_itemList[_itemInHand], _placedList[0], 179);
			_screen->showMouse();
			_currentCharacter->inventoryItems[itemOffset] = _itemInHand;
			_itemInHand = -1;
		}
	}
	_screen->updateScreen();
	// XXX clearKyrandiaButtonIO
	return 0;
}

int KyraEngine_v1::buttonAmuletCallback(Button *caller) {
	if (!(_deathHandler & 8))
		return 1;
	int jewel = caller->index - 0x14;
	if (_currentCharacter->sceneId == 210) {
		if (_beadStateVar == 4 || _beadStateVar == 6)
			return 1;
	}
	if (!queryGameFlag(0x2D))
		return 1;
	if (_itemInHand != -1) {
		assert(_putDownFirst);
		characterSays(2000, _putDownFirst[0], 0, -2);
		return 1;
	}
	if (queryGameFlag(0xF1)) {
		assert(_waitForAmulet);
		characterSays(2001, _waitForAmulet[0], 0, -2);
		return 1;
	}
	if (!queryGameFlag(0x55+jewel)) {
		assert(_blackJewel);
		_animator->makeBrandonFaceMouse();
		drawJewelPress(jewel, 1);
		characterSays(2002, _blackJewel[0], 0, -2);
		return 1;
	}
	drawJewelPress(jewel, 0);
	drawJewelsFadeOutStart();
	drawJewelsFadeOutEnd(jewel);

	_emc->init(&_scriptClick, &_scriptClickData);
	_scriptClick.regs[3] = 0;
	_scriptClick.regs[6] = jewel;
	_emc->start(&_scriptClick, 4);

	while (_emc->isValid(&_scriptClick))
		_emc->run(&_scriptClick);

	if (_scriptClick.regs[3])
		return 1;

	_unkAmuletVar = 1;
	switch (jewel-1) {
	case 0:
		if (_brandonStatusBit & 1) {
			seq_brandonHealing2();
		} else if (_brandonStatusBit == 0) {
			seq_brandonHealing();
			assert(_healingTip);
			characterSays(2003, _healingTip[0], 0, -2);
		}
		break;

	case 1:
		seq_makeBrandonInv();
		break;

	case 2:
		if (_brandonStatusBit & 1) {
			assert(_wispJewelStrings);
			characterSays(2004, _wispJewelStrings[0], 0, -2);
		} else {
			if (_brandonStatusBit & 2) {
				// XXX
				seq_makeBrandonNormal2();
				// XXX
			} else {
				// do not check for item in hand again as in the original since some strings are missing
				// in the cd version
				if (_currentCharacter->sceneId >= 109 && _currentCharacter->sceneId <= 198) {
					snd_playWanderScoreViaMap(1, 0);
					seq_makeBrandonWisp();
					snd_playWanderScoreViaMap(17, 0);
				} else {
					seq_makeBrandonWisp();
				}
				setGameFlag(0x9E);
			}
		}
		break;

	case 3:
		seq_dispelMagicAnimation();
		assert(_magicJewelString);
		characterSays(2007, _magicJewelString[0], 0, -2);
		break;

	default:
		break;
	}
	_unkAmuletVar = 0;
	// XXX clearKyrandiaButtonIO (!used before every return in this function!)
	return 1;
}

#pragma mark -

GUI_v1::GUI_v1(KyraEngine_v1 *vm, Screen_v1 *screen) : GUI(vm), _vm(vm), _screen(screen) {
	_menu = 0;
	initStaticResource();
	_scrollUpFunctor = BUTTON_FUNCTOR(GUI_v1, this, &GUI_v1::scrollUp);
	_scrollDownFunctor = BUTTON_FUNCTOR(GUI_v1, this, &GUI_v1::scrollDown);
}

GUI_v1::~GUI_v1() {
	delete[] _menu;
}

int GUI_v1::processButtonList(Button *list, uint16 inputFlag) {
	if (_haveScrollButtons) {
		if (_mouseWheel < 0)
			scrollUp(&_scrollUpButton);
		else if (_mouseWheel > 0)
			scrollDown(&_scrollDownButton);
	}
	while (list) {
		if (list->flags & 8) {
			list = list->nextButton;
			continue;
		}

		int x = list->x;
		int y = list->y;
		assert(_screen->getScreenDim(list->dimTableIndex) != 0);
		if (x < 0) {
			x += _screen->getScreenDim(list->dimTableIndex)->w << 3;
		}
		x += _screen->getScreenDim(list->dimTableIndex)->sx << 3;

		if (y < 0) {
			y += _screen->getScreenDim(list->dimTableIndex)->h;
		}
		y += _screen->getScreenDim(list->dimTableIndex)->sy;

		Common::Point mouse = _vm->getMousePos();
		if (mouse.x >= x && mouse.y >= y && x + list->width >= mouse.x && y + list->height >= mouse.y) {
			int processMouseClick = 0;
			if (list->flags & 0x400) {
				if (_vm->_mousePressFlag) {
					if (!(list->flags2 & 1)) {
						list->flags2 |= 1;
						list->flags2 |= 4;
						processButton(list);
						_screen->updateScreen();
					}
				} else {
					if (list->flags2 & 1) {
						list->flags2 &= 0xFFFE;
						processButton(list);
						processMouseClick = 1;
					}
				}
			} else if (_vm->_mousePressFlag) {
				processMouseClick = 1;
			}

			if (processMouseClick) {
				if (list->buttonCallback) {
					if ((*list->buttonCallback.get())(list)) {
						break;
					}
				}
			}
		} else {
			if (list->flags2 & 1) {
				list->flags2 &= 0xFFFE;
				processButton(list);
			}
			if (list->flags2 & 4) {
				list->flags2 &= 0xFFFB;
				processButton(list);
				_screen->updateScreen();
			}
			list = list->nextButton;
			continue;
		}

		list = list->nextButton;
	}
	return 0;
}

void GUI_v1::processButton(Button *button) {
	if (!button)
		return;

	int processType = 0;
	const uint8 *shape = 0;
	Button::Callback callback;

	int flags = (button->flags2 & 5);
	if (flags == 1) {
		processType = button->data2Val1;
		if (processType == 1)
			shape = button->data2ShapePtr;
		else if (processType == 4)
			callback = button->data2Callback;
	} else if (flags == 4 || flags == 5) {
		processType = button->data1Val1;
		if (processType == 1)
			shape = button->data1ShapePtr;
		else if (processType == 4)
			callback = button->data1Callback;
	} else {
		processType = button->data0Val1;
		if (processType == 1)
			shape = button->data0ShapePtr;
		else if (processType == 4)
			callback = button->data0Callback;
	}

	int x = button->x;
	int y = button->y;
	assert(_screen->getScreenDim(button->dimTableIndex) != 0);
	if (x < 0)
		x += _screen->getScreenDim(button->dimTableIndex)->w << 3;

	if (y < 0)
		y += _screen->getScreenDim(button->dimTableIndex)->h;

	if (processType == 1 && shape)
		_screen->drawShape(_screen->_curPage, shape, x, y, button->dimTableIndex, 0x10);
	else if (processType == 4 && callback)
		(*callback.get())(button);
}

void GUI_v1::setGUILabels() {
	int offset = 0;
	int offsetOptions = 0;
	int offsetMainMenu = 0;
	int offsetOn = 0;

	int walkspeedGarbageOffset = 36;
	int menuLabelGarbageOffset = 0;

	if (_vm->gameFlags().isTalkie) {
		if (_vm->gameFlags().lang == Common::EN_ANY)
			offset = 52;
		else if (_vm->gameFlags().lang == Common::DE_DEU)
			offset = 30;
		else if (_vm->gameFlags().lang == Common::FR_FRA || _vm->gameFlags().lang == Common::IT_ITA)
			offset = 6;
		offsetOn = offsetMainMenu = offsetOptions = offset;
		walkspeedGarbageOffset = 48;
	} else if (_vm->gameFlags().lang == Common::ES_ESP) {
		offsetOn = offsetMainMenu = offsetOptions = offset = -4;
		menuLabelGarbageOffset = 72;
	} else if (_vm->gameFlags().lang == Common::DE_DEU) {
		offset = offsetMainMenu = offsetOn = offsetOptions = 24;
	} else if (_vm->gameFlags().platform == Common::kPlatformFMTowns || _vm->gameFlags().platform == Common::kPlatformPC98) {
		offset = 1;
		offsetOptions = 10;
		offsetOn = 0;
		walkspeedGarbageOffset = 0;
	}

	assert(offset + 27 < _vm->_guiStringsSize);

	// The Legend of Kyrandia
	_menu[0].menuNameString = _vm->_guiStrings[0];
	// Load a Game
	_menu[0].item[0].itemString = _vm->_guiStrings[1];
	// Save a Game
	_menu[0].item[1].itemString = _vm->_guiStrings[2];
	// Game controls
	_menu[0].item[2].itemString = _vm->_guiStrings[3];
	// Quit playing
	_menu[0].item[3].itemString = _vm->_guiStrings[4];
	// Resume game
	_menu[0].item[4].itemString = _vm->_guiStrings[5];

	// Cancel
	_menu[2].item[5].itemString = _vm->_guiStrings[10];

	// Enter a description of your saved game:
	_menu[3].menuNameString = _vm->_guiStrings[11];
	// Save
	_menu[3].item[0].itemString = _vm->_guiStrings[12];
	// Cancel
	_menu[3].item[1].itemString = _vm->_guiStrings[10];

	// Rest in peace, Brandon
	_menu[4].menuNameString = _vm->_guiStrings[13];
	// Load a game
	_menu[4].item[0].itemString = _vm->_guiStrings[1];
	// Quit playing
	_menu[4].item[1].itemString = _vm->_guiStrings[4];

	// Game Controls
	_menu[5].menuNameString = _vm->_guiStrings[6];
	// Yes
	_menu[1].item[0].itemString = _vm->_guiStrings[22 + offset];
	// No
	_menu[1].item[1].itemString = _vm->_guiStrings[23 + offset];

	// Music is
	_menu[5].item[0].labelString = _vm->_guiStrings[26 + offsetOptions];
	// Sounds are
	_menu[5].item[1].labelString = _vm->_guiStrings[27 + offsetOptions];
	// Walk speed
	_menu[5].item[2].labelString = &_vm->_guiStrings[24 + offsetOptions][walkspeedGarbageOffset];
	// Text speed
	_menu[5].item[4].labelString = _vm->_guiStrings[25 + offsetOptions];
	// Main Menu
	_menu[5].item[5].itemString = &_vm->_guiStrings[19 + offsetMainMenu][menuLabelGarbageOffset];

	if (_vm->gameFlags().isTalkie)
		// Text & Voice
		_voiceTextString = _vm->_guiStrings[28 + offset];

	_textSpeedString = _vm->_guiStrings[25 + offsetOptions];
	_onString =  _vm->_guiStrings[20 + offsetOn];
	_offString =  _vm->_guiStrings[21 + offset];
	_onCDString = _vm->_guiStrings[21];
}

int GUI_v1::buttonMenuCallback(Button *caller) {
	_displayMenu = true;

	assert(_vm->_guiStrings);
	assert(_vm->_configStrings);

	/*
	for (int i = 0; i < _vm->_guiStringsSize; i++)
		debug("GUI string %i: %s", i, _vm->_guiStrings[i]);

	for (int i = 0; i < _vm->_configStringsSize; i++)
		debug("Config string %i: %s", i, _vm->_configStrings[i]);
	*/

	setGUILabels();
	if (_vm->_currentCharacter->sceneId == 210 && _vm->_deathHandler == 0xFF) {
		_vm->snd_playSoundEffect(0x36);
		return 0;
	}
	// XXX
	_screen->setPaletteIndex(0xFE, 60, 60, 0);
	for (int i = 0; i < 6; i++) {
		_menuButtonData[i].data0Val1 = _menuButtonData[i].data1Val1 = _menuButtonData[i].data2Val1 = 4;
		_menuButtonData[i].data0Callback = _redrawShadedButtonFunctor;
		_menuButtonData[i].data1Callback = _redrawButtonFunctor;
		_menuButtonData[i].data2Callback = _redrawButtonFunctor;
	}

	_screen->savePageToDisk("SEENPAGE.TMP", 0);
	fadePalette();

	for (int i = 0; i < 5; i++)
		initMenuLayout(_menu[i]);

	_menuRestoreScreen = true;
	_keyPressed.reset();
	_vm->_mousePressFlag = false;

	_toplevelMenu = 0;
	if (_vm->_menuDirectlyToLoad) {
		loadGameMenu(0);
	} else {
		if (!caller)
			_toplevelMenu = 4;

		initMenu(_menu[_toplevelMenu]);
		updateAllMenuButtons();
	}

	while (_displayMenu && !_vm->_quitFlag) {
		Common::Point mouse = _vm->getMousePos();
		processHighlights(_menu[_toplevelMenu], mouse.x, mouse.y);
		processButtonList(_menuButtonList, 0);
		getInput();
	}

	if (_menuRestoreScreen) {
		restorePalette();
		_screen->loadPageFromDisk("SEENPAGE.TMP", 0);
		_vm->_animator->_updateScreen = true;
	} else {
		_screen->deletePageFromDisk(0);
	}

	return 0;
}

void GUI_v1::getInput() {
	Common::Event event;
	static uint32 lastScreenUpdate = 0;
	uint32 now = _vm->_system->getMillis();

	_mouseWheel = 0;
	while (_vm->_eventMan->pollEvent(event)) {
		switch (event.type) {
		case Common::EVENT_QUIT:
			_vm->quitGame();
			break;
		case Common::EVENT_LBUTTONDOWN:
			_vm->_mousePressFlag = true;
			break;
		case Common::EVENT_LBUTTONUP:
			_vm->_mousePressFlag = false;
			break;
		case Common::EVENT_MOUSEMOVE:
			_vm->_system->updateScreen();
			lastScreenUpdate = now;
			break;
		case Common::EVENT_WHEELUP:
			_mouseWheel = -1;
			break;
		case Common::EVENT_WHEELDOWN:
			_mouseWheel = 1;
			break;
		case Common::EVENT_KEYDOWN:
			_keyPressed = event.kbd;
			break;
		default:
			break;
		}
	}

	if (now - lastScreenUpdate > 50) {
		_vm->_system->updateScreen();
		lastScreenUpdate = now;
	}

	_vm->_system->delayMillis(3);
}

int GUI_v1::resumeGame(Button *button) {
	debugC(9, kDebugLevelGUI, "GUI_v1::resumeGame()");
	updateMenuButton(button);
	_displayMenu = false;

	return 0;
}

void GUI_v1::setupSavegames(Menu &menu, int num) {
	Common::InSaveFile *in;
	static char savenames[5][31];
	uint8 startSlot;
	assert(num <= 5);

	if (_savegameOffset == 0) {
		menu.item[0].itemString = _specialSavegameString;
		menu.item[0].enabled = 1;
		menu.item[0].saveSlot = 0;
		startSlot = 1;
	} else {
		startSlot = 0;
	}

	for (int i = startSlot; i < num; ++i)
		menu.item[i].enabled = 0;

	KyraEngine::SaveHeader header;
	for (int i = startSlot; i < num && uint(_savegameOffset + i) < _saveSlots.size(); i++) {
		if ((in = _vm->openSaveForReading(_vm->getSavegameFilename(_saveSlots[i + _savegameOffset]), header))) {
			strncpy(savenames[i], header.description.c_str(), 31);
			menu.item[i].itemString = savenames[i];
			menu.item[i].enabled = 1;
			menu.item[i].saveSlot = _saveSlots[i + _savegameOffset];
			delete in;
		}
	}
}

int GUI_v1::saveGameMenu(Button *button) {
	debugC(9, kDebugLevelGUI, "GUI_v1::saveGameMenu()");
	updateSaveList();

	updateMenuButton(button);
	_menu[2].item[5].enabled = true;

	_screen->loadPageFromDisk("SEENPAGE.TMP", 0);
	_screen->savePageToDisk("SEENPAGE.TMP", 0);

	_menu[2].menuNameString = _vm->_guiStrings[8]; // Select a position to save to:
	_specialSavegameString = _vm->_guiStrings[9]; // [ EMPTY SLOT ]
	for (int i = 0; i < 5; i++)
		_menu[2].item[i].callback = BUTTON_FUNCTOR(GUI_v1, this, &GUI_v1::saveGame);

	_savegameOffset = 0;
	setupSavegames(_menu[2], 5);

	initMenu(_menu[2]);
	updateAllMenuButtons();

	_displaySubMenu = true;
	_cancelSubMenu = false;

	while (_displaySubMenu && !_vm->_quitFlag) {
		getInput();
		Common::Point mouse = _vm->getMousePos();
		processHighlights(_menu[2], mouse.x, mouse.y);
		processButtonList(_menuButtonList, 0);
	}

	_screen->loadPageFromDisk("SEENPAGE.TMP", 0);
	_screen->savePageToDisk("SEENPAGE.TMP", 0);

	if (_cancelSubMenu) {
		initMenu(_menu[0]);
		updateAllMenuButtons();
	} else {
		_displayMenu = false;
	}
	return 0;
}

int GUI_v1::loadGameMenu(Button *button) {
	debugC(9, kDebugLevelGUI, "GUI_v1::loadGameMenu()");
	updateSaveList();

	if (_vm->_menuDirectlyToLoad) {
		_menu[2].item[5].enabled = false;
	} else {
		updateMenuButton(button);
		_menu[2].item[5].enabled = true;
	}

	_screen->loadPageFromDisk("SEENPAGE.TMP", 0);
	_screen->savePageToDisk("SEENPAGE.TMP", 0);

	_specialSavegameString = _vm->_newGameString[0]; //[ START A NEW GAME ]
	_menu[2].menuNameString = _vm->_guiStrings[7]; // Which game would you like to reload?
	for (int i = 0; i < 5; i++)
		_menu[2].item[i].callback = BUTTON_FUNCTOR(GUI_v1, this, &GUI_v1::loadGame);

	_savegameOffset = 0;
	setupSavegames(_menu[2], 5);

	initMenu(_menu[2]);
	updateAllMenuButtons();

	_displaySubMenu = true;
	_cancelSubMenu = false;

	_vm->_gameToLoad = -1;

	while (_displaySubMenu && !_vm->_quitFlag) {
		getInput();
		Common::Point mouse = _vm->getMousePos();
		processHighlights(_menu[2], mouse.x, mouse.y);
		processButtonList(_menuButtonList, 0);
	}

	_screen->loadPageFromDisk("SEENPAGE.TMP", 0);
	_screen->savePageToDisk("SEENPAGE.TMP", 0);

	if (_cancelSubMenu) {
		initMenu(_menu[_toplevelMenu]);
		updateAllMenuButtons();
	} else {
		restorePalette();
		if (_vm->_gameToLoad != -1)
			_vm->loadGame(_vm->getSavegameFilename(_vm->_gameToLoad));
		_displayMenu = false;
		_menuRestoreScreen = false;
	}
	return 0;
}

void GUI_v1::redrawTextfield() {
	_screen->fillRect(38, 91, 287, 102, 250);
	_text->printText(_savegameName, 38, 92, 253, 0, 0);

	_screen->_charWidth = -2;
	int width = _screen->getTextWidth(_savegameName);
	_screen->fillRect(39 + width, 93, 45 + width, 100, 254);
	_screen->_charWidth = 0;

	_screen->updateScreen();
}

void GUI_v1::updateSavegameString() {
	int length;

	if (_keyPressed.keycode) {
		length = strlen(_savegameName);

		if (_keyPressed.ascii > 31 && _keyPressed.ascii < 127) {
			if (length < 31) {
				_savegameName[length] = _keyPressed.ascii;
				_savegameName[length+1] = 0;
				redrawTextfield();
			}
		} else if (_keyPressed.keycode == Common::KEYCODE_BACKSPACE ||
		           _keyPressed.keycode == Common::KEYCODE_DELETE) {
			if (length > 0) {
				_savegameName[length-1] = 0;
				redrawTextfield();
			}
		} else if (_keyPressed.keycode == Common::KEYCODE_RETURN ||
		           _keyPressed.keycode == Common::KEYCODE_KP_ENTER) {
			_displaySubMenu = false;
		}
	}

	_keyPressed.reset();
}

int GUI_v1::saveGame(Button *button) {
	debugC(9, kDebugLevelGUI, "GUI_v1::saveGame()");
	updateMenuButton(button);
	_vm->_gameToLoad = _menu[2].item[button->index-0xC].saveSlot;

	_screen->loadPageFromDisk("SEENPAGE.TMP", 0);
	_screen->savePageToDisk("SEENPAGE.TMP", 0);

	initMenu(_menu[3]);
	updateAllMenuButtons();

	_displaySubMenu = true;
	_cancelSubMenu = false;

	if (_savegameOffset == 0 && _vm->_gameToLoad == 0) {
		_savegameName[0] = 0;
	} else {
		for (int i = 0; i < 5; i++) {
			if (_menu[2].item[i].saveSlot == _vm->_gameToLoad) {
				strncpy(_savegameName, _menu[2].item[i].itemString, 31);
				break;
			}
		}
	}
	redrawTextfield();

	while (_displaySubMenu && !_vm->_quitFlag) {
		getInput();
		updateSavegameString();
		Common::Point mouse = _vm->getMousePos();
		processHighlights(_menu[3], mouse.x, mouse.y);
		processButtonList(_menuButtonList, 0);
	}

	if (_cancelSubMenu) {
		_displaySubMenu = true;
		_cancelSubMenu = false;
		initMenu(_menu[2]);
		updateAllMenuButtons();
	} else {
		if (_savegameOffset == 0 && _vm->_gameToLoad == 0)
			_vm->_gameToLoad = getNextSavegameSlot();
		if (_vm->_gameToLoad > 0)
			_vm->saveGame(_vm->getSavegameFilename(_vm->_gameToLoad), _savegameName);
	}

	return 0;
}

int GUI_v1::savegameConfirm(Button *button) {
	debugC(9, kDebugLevelGUI, "GUI_v1::savegameConfirm()");
	updateMenuButton(button);
	_displaySubMenu = false;

	return 0;
}

int GUI_v1::loadGame(Button *button) {
	debugC(9, kDebugLevelGUI, "GUI_v1::loadGame()");
	updateMenuButton(button);
	_displaySubMenu = false;
	_vm->_gameToLoad = _menu[2].item[button->index-0xC].saveSlot;

	return 0;
}

int GUI_v1::cancelSubMenu(Button *button) {
	debugC(9, kDebugLevelGUI, "GUI_v1::cancelSubMenu()");
	updateMenuButton(button);
	_displaySubMenu = false;
	_cancelSubMenu = true;

	return 0;
}

int GUI_v1::quitPlaying(Button *button) {
	debugC(9, kDebugLevelGUI, "GUI_v1::quitPlaying()");
	updateMenuButton(button);

	if (quitConfirm(_vm->_guiStrings[14])) { // Are you sure you want to quit playing?
		_vm->quitGame();
	} else {
		initMenu(_menu[_toplevelMenu]);
		updateAllMenuButtons();
	}

	return 0;
}

bool GUI_v1::quitConfirm(const char *str) {
	debugC(9, kDebugLevelGUI, "GUI_v1::quitConfirm()");

	_screen->loadPageFromDisk("SEENPAGE.TMP", 0);
	_screen->savePageToDisk("SEENPAGE.TMP", 0);

	_menu[1].menuNameString = str;
	initMenuLayout(_menu[1]);
	initMenu(_menu[1]);

	_displaySubMenu = true;
	_cancelSubMenu = true;

	while (_displaySubMenu && !_vm->_quitFlag) {
		getInput();
		Common::Point mouse = _vm->getMousePos();
		processHighlights(_menu[1], mouse.x, mouse.y);
		processButtonList(_menuButtonList, 0);
	}

	_screen->loadPageFromDisk("SEENPAGE.TMP", 0);
	_screen->savePageToDisk("SEENPAGE.TMP", 0);

	return !_cancelSubMenu;
}

int GUI_v1::quitConfirmYes(Button *button) {
	debugC(9, kDebugLevelGUI, "GUI_v1::quitConfirmYes()");
	updateMenuButton(button);
	_displaySubMenu = false;
	_cancelSubMenu = false;

	return 0;
}

int GUI_v1::quitConfirmNo(Button *button) {
	debugC(9, kDebugLevelGUI, "GUI_v1::quitConfirmNo()");
	updateMenuButton(button);
	_displaySubMenu = false;
	_cancelSubMenu = true;

	return 0;
}

int GUI_v1::gameControlsMenu(Button *button) {
	debugC(9, kDebugLevelGUI, "GUI_v1::gameControlsMenu()");

	_vm->readSettings();

	_screen->loadPageFromDisk("SEENPAGE.TMP", 0);
	_screen->savePageToDisk("SEENPAGE.TMP", 0);

	if (_vm->gameFlags().isTalkie) {
		//_menu[5].width = 230;

		for (int i = 0; i < 5; i++) {
			//_menu[5].item[i].labelX = 24;
			//_menu[5].item[i].x = 115;
			//_menu[5].item[i].width = 94;
		}

		_menu[5].item[3].labelString = _voiceTextString; //"Voice / Text "
		_menu[5].item[3].callback = BUTTON_FUNCTOR(GUI_v1, this, &GUI_v1::controlsChangeVoice);

	} else {
		//_menu[5].height = 136;
		//_menu[5].item[5].y = 110;
		_menu[5].item[4].enabled = 0;
		_menu[5].item[3].labelString = _textSpeedString; // "Text speed "
		_menu[5].item[3].callback = BUTTON_FUNCTOR(GUI_v1, this, &GUI_v1::controlsChangeText);
	}

	setupControls(_menu[5]);

	updateAllMenuButtons();

	_displaySubMenu = true;
	_cancelSubMenu = false;

	while (_displaySubMenu && !_vm->_quitFlag) {
		getInput();
		Common::Point mouse = _vm->getMousePos();
		processHighlights(_menu[5], mouse.x, mouse.y);
		processButtonList(_menuButtonList, 0);
	}

	_screen->loadPageFromDisk("SEENPAGE.TMP", 0);
	_screen->savePageToDisk("SEENPAGE.TMP", 0);

	if (_cancelSubMenu) {
		initMenu(_menu[_toplevelMenu]);
		updateAllMenuButtons();
	}
	return 0;
}

void GUI_v1::setupControls(Menu &menu) {
	debugC(9, kDebugLevelGUI, "GUI_v1::setupControls()");

	switch (_vm->_configMusic) {
		case 0:
			menu.item[0].itemString = _offString; //"Off"
			break;
		case 1:
			menu.item[0].itemString = _onString; //"On"
			break;
		case 2:
			menu.item[0].itemString = _onCDString; //"On + CD"
			break;
	}

	if (_vm->_configSounds)
		menu.item[1].itemString = _onString; //"On"
	else
		menu.item[1].itemString = _offString; //"Off"


	switch (_vm->_configWalkspeed) {
	case 0:
		menu.item[2].itemString = _vm->_configStrings[0]; //"Slowest"
		break;
	case 1:
		menu.item[2].itemString = _vm->_configStrings[1]; //"Slow"
		break;
	case 2:
		menu.item[2].itemString = _vm->_configStrings[2]; //"Normal"
		break;
	case 3:
		menu.item[2].itemString = _vm->_configStrings[3]; //"Fast"
		break;
	case 4:
		menu.item[2].itemString = _vm->_configStrings[4]; //"Fastest"
		break;
	default:
		menu.item[2].itemString = "ERROR";
		break;
	}

	int textControl = 3;
	int clickableOffset = 8;
	if (_vm->gameFlags().isTalkie) {
		textControl = 4;
		clickableOffset = 11;

		if (_vm->_configVoice == 0) {
			menu.item[4].enabled = 1;
			menu.item[4].labelString = _textSpeedString;
		} else {
			menu.item[4].enabled = 0;
			menu.item[4].labelString = 0;
		}

		switch (_vm->_configVoice) {
		case 0:
			menu.item[3].itemString = _vm->_configStrings[5]; //"Text only"
			break;
		case 1:
			menu.item[3].itemString = _vm->_configStrings[6]; //"Voice only"
			break;
		case 2:
			menu.item[3].itemString = _vm->_configStrings[7]; //"Voice & Text"
			break;
		default:
			menu.item[3].itemString = "ERROR";
			break;
		}
	} else {
		menu.item[4].enabled = 0;
		menu.item[4].labelString = 0;
	}

	switch (_vm->_configTextspeed) {
	case 0:
		menu.item[textControl].itemString = _vm->_configStrings[1]; //"Slow"
		break;
	case 1:
		menu.item[textControl].itemString = _vm->_configStrings[2]; //"Normal"
		break;
	case 2:
		menu.item[textControl].itemString = _vm->_configStrings[3]; //"Fast"
		break;
	case 3:
		menu.item[textControl].itemString = _vm->_configStrings[clickableOffset]; //"Clickable"
		break;
	default:
		menu.item[textControl].itemString = "ERROR";
		break;
	}

	initMenuLayout(menu);
	initMenu(menu);
}

int GUI_v1::controlsChangeMusic(Button *button) {
	debugC(9, kDebugLevelGUI, "GUI_v1::controlsChangeMusic()");
	updateMenuButton(button);

	_vm->_configMusic = ++_vm->_configMusic % ((_vm->gameFlags().platform == Common::kPlatformFMTowns || _vm->gameFlags().platform == Common::kPlatformPC98) ? 3 : 2);
	setupControls(_menu[5]);
	return 0;
}

int GUI_v1::controlsChangeSounds(Button *button) {
	debugC(9, kDebugLevelGUI, "GUI_v1::controlsChangeSounds()");
	updateMenuButton(button);

	_vm->_configSounds = !_vm->_configSounds;
	setupControls(_menu[5]);
	return 0;
}

int GUI_v1::controlsChangeWalk(Button *button) {
	debugC(9, kDebugLevelGUI, "GUI_v1::controlsChangeWalk()");
	updateMenuButton(button);

	_vm->_configWalkspeed = ++_vm->_configWalkspeed % 5;
	_vm->setWalkspeed(_vm->_configWalkspeed);
	setupControls(_menu[5]);
	return 0;
}

int GUI_v1::controlsChangeText(Button *button) {
	debugC(9, kDebugLevelGUI, "GUI_v1::controlsChangeText()");
	updateMenuButton(button);

	_vm->_configTextspeed = ++_vm->_configTextspeed % 4;
	setupControls(_menu[5]);
	return 0;
}

int GUI_v1::controlsChangeVoice(Button *button) {
	debugC(9, kDebugLevelGUI, "GUI_v1::controlsChangeVoice()");
	updateMenuButton(button);

	_vm->_configVoice = ++_vm->_configVoice % 3;
	setupControls(_menu[5]);
	return 0;
}

int GUI_v1::controlsApply(Button *button) {
	debugC(9, kDebugLevelGUI, "GUI_v1::controlsApply()");
	_vm->writeSettings();
	return cancelSubMenu(button);
}

int GUI_v1::scrollUp(Button *button) {
	debugC(9, kDebugLevelGUI, "GUI_v1::scrollUp()");
	updateMenuButton(button);

	if (_savegameOffset > 0) {
		_savegameOffset--;
		setupSavegames(_menu[2], 5);
		initMenu(_menu[2]);
	}
	return 0;
}

int GUI_v1::scrollDown(Button *button) {
	debugC(9, kDebugLevelGUI, "GUI_v1::scrollDown()");
	updateMenuButton(button);

	_savegameOffset++;
	if (uint(_savegameOffset + 5) >= _saveSlots.size())
		_savegameOffset = MAX<int>(_saveSlots.size() - 5, 0);
	setupSavegames(_menu[2], 5);
	initMenu(_menu[2]);

	return 0;
}

void GUI_v1::fadePalette() {
	if (_vm->gameFlags().platform == Common::kPlatformAmiga)
		return;

	static int16 menuPalIndexes[] = {248, 249, 250, 251, 252, 253, 254, -1};
	int index = 0;

	memcpy(_screen->getPalette(2), _screen->_currentPalette, 768);

	for (int i = 0; i < 768; i++)
		_screen->_currentPalette[i] >>= 1;

	while (menuPalIndexes[index] != -1) {
		memcpy(&_screen->_currentPalette[menuPalIndexes[index]*3], &_screen->getPalette(2)[menuPalIndexes[index]*3], 3);
		index++;
	}

	_screen->fadePalette(_screen->_currentPalette, 2);
}

void GUI_v1::restorePalette() {
	if (_vm->gameFlags().platform == Common::kPlatformAmiga)
		return;

	memcpy(_screen->_currentPalette, _screen->getPalette(2), 768);
	_screen->fadePalette(_screen->_currentPalette, 2);
}

#pragma mark -

void KyraEngine_v1::drawAmulet() {
	debugC(9, kDebugLevelMain, "KyraEngine_v1::drawAmulet()");
	static const int16 amuletTable1[] = {0x167, 0x162, 0x15D, 0x158, 0x153, 0x150, 0x155, 0x15A, 0x15F, 0x164, 0x145, -1};
	static const int16 amuletTable3[] = {0x167, 0x162, 0x15D, 0x158, 0x153, 0x14F, 0x154, 0x159, 0x15E, 0x163, 0x144, -1};
	static const int16 amuletTable2[] = {0x167, 0x162, 0x15D, 0x158, 0x153, 0x152, 0x157, 0x15C, 0x161, 0x166, 0x147, -1};
	static const int16 amuletTable4[] = {0x167, 0x162, 0x15D, 0x158, 0x153, 0x151, 0x156, 0x15B, 0x160, 0x165, 0x146, -1};

	resetGameFlag(0xF1);
	_screen->hideMouse();

	int i = 0;
	while (amuletTable1[i] != -1) {
		if (queryGameFlag(87))
			_screen->drawShape(0, _shapes[amuletTable1[i]], _amuletX[0], _amuletY[0], 0, 0);

		if (queryGameFlag(89))
			_screen->drawShape(0, _shapes[amuletTable2[i]], _amuletX[1], _amuletY[1], 0, 0);

		if (queryGameFlag(86))
			_screen->drawShape(0, _shapes[amuletTable3[i]], _amuletX[2], _amuletY[2], 0, 0);

		if (queryGameFlag(88))
			_screen->drawShape(0, _shapes[amuletTable4[i]], _amuletX[3], _amuletY[3], 0, 0);

		_screen->updateScreen();
		delayWithTicks(3);
		i++;
	}
	_screen->showMouse();
}

} // end of namespace Kyra

