/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001  Ludvig Strigeus
 * Copyright (C) 2001/2002 The ScummVM project
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 *
 */

#include "stdafx.h"
#include "scumm.h"
#include "intern.h"
#include "resource.h"


void Scumm_v3::readIndexFile()
{
	uint16 blocktype;
	uint32 itemsize;
	int numblock = 0;
	int num, i;

	debug(9, "readIndexFile()");

	closeRoom();
	openRoom(0);

	while (!_fileHandle.eof()) {
		itemsize = _fileHandle.readUint32LE();
		blocktype = _fileHandle.readUint16LE();
		if (_fileHandle.ioFailed())
			break;

		switch (blocktype) {
		case 0x4E52:	// 'NR'
			_fileHandle.readUint16LE();
			break;
		case 0x5230:	// 'R0'
			_numRooms = _fileHandle.readUint16LE();
			break;
		case 0x5330:	// 'S0'
			_numScripts = _fileHandle.readUint16LE();
			break;
		case 0x4E30:	// 'N0'
			_numSounds = _fileHandle.readUint16LE();
			break;
		case 0x4330:	// 'C0'
			_numCostumes = _fileHandle.readUint16LE();
			break;
		case 0x4F30:	// 'O0'
			_numGlobalObjects = _fileHandle.readUint16LE();
			break;
		}
		_fileHandle.seek(itemsize - 8, SEEK_CUR);
	}

	_fileHandle.clearIOFailed();
	_fileHandle.seek(0, SEEK_SET);

	/* I'm not sure for those values yet, they will have to be rechecked */

	_numVariables = 800;				/* 800 */
	_numBitVariables = 4096;			/* 2048 */
	_numLocalObjects = 200;				/* 200 */
	_numArray = 50;
	_numVerbs = 100;
	_numNewNames = 0;
	_objectRoomTable = NULL;
	_numCharsets = 9;					/* 9 */
	_numInventory = 80;					/* 80 */
	_numGlobalScripts = 200;

	_shadowPaletteSize = 256;
	_shadowPalette = (byte *) calloc(_shadowPaletteSize, 1);	// stupid for now. Need to be removed later

	// Jamieson630: palManipulate variable initialization
	_palManipCounter = 0;
	_palManipPalette = 0; // Will allocate when needed
	_palManipIntermediatePal = 0; // Will allocate when needed

	_numFlObject = 50;
	allocateArrays();

	while (1) {
		itemsize = _fileHandle.readUint32LE();

		if (_fileHandle.ioFailed())
			break;

		blocktype = _fileHandle.readUint16LE();

		numblock++;

		switch (blocktype) {

		case 0x4E52:	// 'NR'
			_fileHandle.seek(itemsize - 6, SEEK_CUR);
			break;

		case 0x5230:	// 'R0'
			readResTypeList(rtRoom, MKID('ROOM'), "room");
			break;

		case 0x5330:	// 'S0'
			readResTypeList(rtScript, MKID('SCRP'), "script");
			break;

		case 0x4E30:	// 'N0'
			readResTypeList(rtSound, MKID('SOUN'), "sound");
			break;

		case 0x4330:	// 'C0'
			readResTypeList(rtCostume, MKID('COST'), "costume");
			break;

		case 0x4F30:	// 'O0'
			num = _fileHandle.readUint16LE();
			assert(num == _numGlobalObjects);
			for (i = 0; i != num; i++) {
				uint32 bits = _fileHandle.readByte();
				byte tmp;
				bits |= _fileHandle.readByte() << 8;
				bits |= _fileHandle.readByte() << 16;
				_classData[i] = bits;
				tmp = _fileHandle.readByte();
				_objectOwnerTable[i] = tmp & OF_OWNER_MASK;
				_objectStateTable[i] = tmp >> OF_STATE_SHL;
			}

			break;

		default:
			error("Bad ID %c%c found in directory!", blocktype & 0xFF, blocktype >> 8);
			return;
		}
	}

	closeRoom();
}

void Scumm_v3::loadCharset(int no)
{
	uint32 size;
	memset(_charsetData, 0, sizeof(_charsetData));

	checkRange(4, 0, no, "Loading illegal charset %d");
	closeRoom();

	openRoom(98 + no);

	size = _fileHandle.readUint16LE();

	_fileHandle.read(createResource(6, no, size), size);
	closeRoom();
}
