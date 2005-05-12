/* Copyright (C) 1994-1998 Revolution Software Ltd.
 * Copyright (C) 2003-2005 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 */

// WALKER.CPP by James (14nov96)

// Functions for moving megas about the place & also for keeping tabs on them

#include "common/stdafx.h"
#include "sword2/sword2.h"
#include "sword2/defs.h"
#include "sword2/interpreter.h"
#include "sword2/logic.h"
#include "sword2/resman.h"
#include "sword2/router.h"

namespace Sword2 {

void Router::setStandbyCoords(int16 x, int16 y, uint8 dir) {
	assert(dir <= 7);

	_standbyX = x;
	_standbyY = y;
	_standbyDir = dir;
}

/**
 * Work out direction from start to dest.
 */

// Used in whatTarget(); not valid for all megas
#define	diagonalx 36
#define	diagonaly 8

int Router::whatTarget(int startX, int startY, int destX, int destY) {
	int deltaX = destX - startX;
	int deltaY = destY - startY;

	// 7 0 1
	// 6   2
	// 5 4 3

	// Flat route

	if (ABS(deltaY) * diagonalx < ABS(deltaX) * diagonaly / 2)
		return (deltaX > 0) ? 2 : 6;

	// Vertical route

	if (ABS(deltaY) * diagonalx / 2 > ABS(deltaX) * diagonaly)
		return (deltaY > 0) ? 4 : 0;

	// Diagonal route

	if (deltaX > 0)
		return (deltaY > 0) ? 3 : 1;

	return (deltaY > 0) ? 5 : 7;
}

/**
 * Walk meta to (x,y,dir). Set RESULT to 0 if it succeeded. Otherwise, set
 * RESULT to 1. Return true if the mega has finished walking.
 */

int Router::doWalk(ObjectLogic *ob_logic, ObjectGraphic *ob_graph, ObjectMega *ob_mega, ObjectWalkdata *ob_walkdata, int16 target_x, int16 target_y, uint8 target_dir) {
	// If this is the start of the walk, calculate the route.

	if (!ob_logic->looping) {
		// If we're already there, don't even bother allocating
		// memory and calling the router, just quit back & continue
		// the script! This avoids an embarassing mega stand frame
		// appearing for one cycle when we're already in position for
		// an anim eg. repeatedly clicking on same object to repeat
		// an anim - no mega frame will appear in between runs of the
		// anim.
		
		if (ob_mega->feet_x == target_x && ob_mega->feet_y == target_y && ob_mega->current_dir == target_dir) {
			Logic::_scriptVars[RESULT] = 0;
			return IR_CONT;
		}

		assert(target_dir <= 8);

		ob_mega->walk_pc = 0;

		// Set up mem for _walkData in route_slots[] & set mega's
		// 'route_slot_id' accordingly
		allocateRouteMem();

		int32 route = routeFinder(ob_mega, ob_walkdata, target_x, target_y, target_dir);

		// 0 = can't make route to target
		// 1 = created route
		// 2 = zero route but may need to turn

		if (route != 1 && route != 2) {
			freeRouteMem();
			Logic::_scriptVars[RESULT] = 1;
			return IR_CONT;
		}

		// Walk is about to start

		ob_mega->currently_walking = 1;
		ob_logic->looping = 1;
		ob_graph->anim_resource = ob_mega->megaset_res;
	} else if (Logic::_scriptVars[EXIT_FADING] && _vm->_screen->getFadeStatus() == RDFADE_BLACK) {
		// Double clicked an exit, and the screen has faded down to
		// black. Ok, that's it. Back to script and change screen.

		// We have to clear te EXIT_CLICK_ID variable in case there's a
		// walk instruction on the new screen, or it'd be cut short.

		freeRouteMem();

		ob_logic->looping = 0;
		ob_mega->currently_walking = 0;
		Logic::_scriptVars[EXIT_CLICK_ID] = 0;
		Logic::_scriptVars[RESULT] = 0;

		return IR_CONT;
	}

	// Get pointer to walkanim & current frame position

	WalkData *walkAnim = getRouteMem();
	int32 walk_pc = ob_mega->walk_pc;

	// If stopping the walk early, overwrite the next step with a
	// slow-out, then finish

	if (_vm->_logic->checkEventWaiting() && walkAnim[walk_pc].step == 0 && walkAnim[walk_pc + 1].step == 1) {
		// At the beginning of a step
		earlySlowOut(ob_mega, ob_walkdata);
	}

	// Get new frame of walk

	ob_graph->anim_pc = walkAnim[walk_pc].frame;
	ob_mega->current_dir = walkAnim[walk_pc].dir;
	ob_mega->feet_x = walkAnim[walk_pc].x;
	ob_mega->feet_y = walkAnim[walk_pc].y;

	// Is the NEXT frame is the end-marker (512) of the walk sequence?

	if (walkAnim[walk_pc + 1].frame != 512) {
		// No, it wasn't. Increment the walk-anim frame number and
		// come back next cycle.
		ob_mega->walk_pc++;
		return IR_REPEAT;
	}

	// We have reached the end-marker, which means we can return to the
	// script just as the final (stand) frame of the walk is set.

	freeRouteMem();
	ob_logic->looping = 0;
	ob_mega->currently_walking = 0;

	// If George's walk has been interrupted to run a new action script for
	// instance or Nico's walk has been interrupted by player clicking on
	// her to talk

	// There used to be code here for checking if two megas were colliding,
	// but it had been commented out, and it was only run if a function
	// that always returned zero returned non-zero.

	if (_vm->_logic->checkEventWaiting()) {
		_vm->_logic->startEvent();
		Logic::_scriptVars[RESULT] = 1;
		return IR_TERMINATE;
	}

	Logic::_scriptVars[RESULT] = 0;

	// CONTINUE the script so that RESULT can be checked! Also, if an anim
	// command follows the fnWalk command, the 1st frame of the anim (which
	// is always a stand frame itself) can replace the final stand frame of
	// the walk, to hide the slight difference between the shrinking on the
	// mega frames and the pre-shrunk anim start-frame.

	return IR_CONT;
}

/**
 * Walk mega to start position of anim
 */

int Router::walkToAnim(ObjectLogic *ob_logic, ObjectGraphic *ob_graph, ObjectMega *ob_mega, ObjectWalkdata *ob_walkdata, uint32 animRes) {
	int16 target_x = 0;
	int16 target_y = 0;
	uint8 target_dir = 0;

	// Walkdata is needed for earlySlowOut if player clicks elsewhere
	// during the walk.

	// If this is the start of the walk, read anim file to get start coords

	if (!ob_logic->looping) {
		byte *anim_file = _vm->_resman->openResource(animRes);
		AnimHeader *anim_head = _vm->fetchAnimHeader(anim_file);

		target_x = anim_head->feetStartX;
		target_y = anim_head->feetStartY;
		target_dir = anim_head->feetStartDir;

		_vm->_resman->closeResource(animRes);

		// If start coords not yet set in anim header, use the standby
		// coords (which should be set beforehand in the script).

		if (target_x == 0 && target_y == 0) {
			target_x = _standbyX;
			target_y = _standbyY;
			target_dir = _standbyDir;
		}

		assert(target_dir <= 7);
	}

	return doWalk(ob_logic, ob_graph, ob_mega, ob_walkdata, target_x, target_y, target_dir);
}

/**
 * Route to the left or right hand side of target id, if possible.
 */

int Router::walkToTalkToMega(ObjectLogic *ob_logic, ObjectGraphic *ob_graph, ObjectMega *ob_mega, ObjectWalkdata *ob_walkdata, uint32 megaId, uint32 separation) {
	int16 target_x = 0;
	int16 target_y = 0;
	uint8 target_dir = 0;

	// If this is the start of the walk, calculate the route.

	if (!ob_logic->looping)	{
		StandardHeader *head = (StandardHeader *)_vm->_resman->openResource(megaId);

		assert(head->fileType == GAME_OBJECT);

		// Call the base script. This is the graphic/mouse service
		// call, and will set _engineMega to the ObjectMega of mega we
		// want to route to.

		char *raw_script_ad = (char *)head;
		uint32 null_pc = 3;

		_vm->_logic->runScript(raw_script_ad, raw_script_ad, &null_pc);
		_vm->_resman->closeResource(megaId);

		ObjectMega *targetMega = _vm->_logic->getEngineMega();

		// Stand exactly beside the mega, ie. at same y-coord
		target_y = targetMega->feet_y;

		// Apply scale factor to walk distance. Ay+B gives 256 * scale
		// ie. 256 * 256 * true_scale for even better accuracy, ie.
		// scale = (Ay + B) / 256

		int scale = (ob_mega->scale_a * ob_mega->feet_y + ob_mega->scale_b) / 256;
		int mega_separation = (separation * scale) / 256;

		debug(4, "Target is at (%d, %d), separation %d", targetMega->feet_x, targetMega->feet_y, mega_separation);

		if (targetMega->feet_x < ob_mega->feet_x) {
			// Target is left of us, so aim to stand to their
			// right. Face down_left

			target_x = targetMega->feet_x + mega_separation;
			target_dir = 5;
		} else {
			// Ok, must be right of us so aim to stand to their
			// left. Face down_right.

			target_x = targetMega->feet_x - mega_separation;
			target_dir = 3;
		}
	}

	return doWalk(ob_logic, ob_graph, ob_mega, ob_walkdata, target_x, target_y, target_dir);
}
/**
 * Turn mega to the specified direction. Just needs to call doWalk() with
 * current feet coords, so router can produce anim of turn frames.
 */

int Router::doFace(ObjectLogic *ob_logic, ObjectGraphic *ob_graph, ObjectMega *ob_mega, ObjectWalkdata *ob_walkdata, uint8 target_dir) {
	int16 target_x = 0;
	int16 target_y = 0;

	// If this is the start of the turn, get the mega's current feet
	// coords + the required direction

	if (!ob_logic->looping) {
		assert(target_dir <= 7);

		target_x = ob_mega->feet_x;
		target_y = ob_mega->feet_y;
	}

	return doWalk(ob_logic, ob_graph, ob_mega, ob_walkdata, target_x, target_y, target_dir);
}

/**
 * Turn mega to face point (x,y) on the floor
 */

int Router::faceXY(ObjectLogic *ob_logic, ObjectGraphic *ob_graph, ObjectMega *ob_mega, ObjectWalkdata *ob_walkdata, int16 target_x, int16 target_y) {
	uint8 target_dir = 0;

	// If this is the start of the turn, get the mega's current feet
	// coords + the required direction

	if (!ob_logic->looping) {
		target_dir = whatTarget(ob_mega->feet_x, ob_mega->feet_y, target_x, target_y);
	}

	return doFace(ob_logic, ob_graph, ob_mega, ob_walkdata, target_dir);
}

/**
 * Turn mega to face another mega.
 */

int Router::faceMega(ObjectLogic *ob_logic, ObjectGraphic *ob_graph, ObjectMega *ob_mega, ObjectWalkdata *ob_walkdata, uint32 megaId) {
	uint8 target_dir = 0;

	// If this is the start of the walk, decide where to walk to.

	if (!ob_logic->looping) {
		StandardHeader *head = (StandardHeader *)_vm->_resman->openResource(megaId);

		assert(head->fileType == GAME_OBJECT);

		// Call the base script. This is the graphic/mouse service
		// call, and will set _engineMega to the ObjectMega of mega we
		// want to turn to face.

		char *raw_script_ad = (char *)head;
		uint32 null_pc = 3;

		_vm->_logic->runScript(raw_script_ad, raw_script_ad, &null_pc);
		_vm->_resman->closeResource(megaId);

		ObjectMega *targetMega = _vm->_logic->getEngineMega();

		target_dir = whatTarget(ob_mega->feet_x, ob_mega->feet_y, targetMega->feet_x, targetMega->feet_y);
	}

	return doFace(ob_logic, ob_graph, ob_mega, ob_walkdata, target_dir);
}

/**
 * Stand mega at (x,y,dir)
 * Sets up the graphic object, but also needs to set the new 'current_dir' in
 * the mega object, so the router knows in future
 */

void Router::standAt(ObjectGraphic *ob_graph, ObjectMega *ob_mega, int32 x, int32 y, int32 dir) {
	assert(dir >= 0 && dir <= 7);

	// Set up the stand frame & set the mega's new direction

	ob_mega->feet_x = x;
	ob_mega->feet_y = y;
	ob_mega->current_dir = dir;

	// Mega-set animation file
	ob_graph->anim_resource	= ob_mega->megaset_res;

	// Dir + first stand frame (always frame 96)
	ob_graph->anim_pc = dir + 96;
}

/**
 * stand mega at end position of anim
 */

void Router::standAfterAnim(ObjectGraphic *ob_graph, ObjectMega *ob_mega, uint32 animRes) {
	byte *anim_file = _vm->_resman->openResource(animRes);
	AnimHeader *anim_head = _vm->fetchAnimHeader(anim_file);

	int32 x = anim_head->feetEndX;
	int32 y = anim_head->feetEndY;
	int32 dir = anim_head->feetEndDir;

	_vm->_resman->closeResource(animRes);

	// If start coords not available either use the standby coords (which
	// should be set beforehand in the script)

	if (x == 0 && y == 0) {
		x = _standbyX;
		y = _standbyY;
		dir = _standbyDir;
	}

	standAt(ob_graph, ob_mega, x, y, dir);
}

void Router::standAtAnim(ObjectGraphic *ob_graph, ObjectMega *ob_mega, uint32 animRes) {
	byte *anim_file = _vm->_resman->openResource(animRes);
	AnimHeader *anim_head = _vm->fetchAnimHeader(anim_file);

	int32 x = anim_head->feetStartX;
	int32 y = anim_head->feetStartY;
	int32 dir = anim_head->feetStartDir;

	_vm->_resman->closeResource(animRes);

	// If start coords not available use the standby coords (which should
	// be set beforehand in the script)

	if (x == 0 && y == 0) {
		x = _standbyX;
		y = _standbyY;
		dir = _standbyDir;
	}

	standAt(ob_graph, ob_mega, x, y, dir);
}

} // End of namespace Sword2
