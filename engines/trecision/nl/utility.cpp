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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/scummsys.h"
#include "trecision/nl/struct.h"
#include "trecision/nl/define.h"
#include "trecision/nl/message.h"
#include "trecision/nl/ll/llinc.h"
#include "trecision/nl/extern.h"
#include "trecision/trecision.h"

#include "common/config-manager.h"
#include "engines/engine.h"

namespace Trecision {

void SScriptFrame::sendFrame() {
	doEvent(_class, _event, MP_DEFAULT, _u16Param1, _u16Param2, _u8Param, _u32Param);
}

uint8 CurStack = 0;

#define MAXTEXTSTACK	3

struct StackText {
	uint16 x;
	uint16 y;
	uint16 tcol, scol;
	char  sign[256];
	bool  Clear;

	void DoText();
} TextStack[MAXTEXTSTACK];

int16 TextStackTop = -1;

/*-------------------------------------------------------------------------*/
/*                                ENDSCRIPT           					   */
/*-------------------------------------------------------------------------*/
void EndScript() {
	CurStack--;
	if (CurStack == 0) {
		Flagscriptactive = false;
		g_vm->_flagMouseEnabled = true;
		RepaintString();
	}
}

/*-------------------------------------------------------------------------*/
/*                               PLAYSCRIPT           					   */
/*-------------------------------------------------------------------------*/
void PlayScript(uint16 i) {
	CurStack++;
	Flagscriptactive = true;
	g_vm->_flagMouseEnabled = false;
	g_vm->_curScriptFrame[CurStack] = g_vm->_script[i]._firstFrame;

	SScriptFrame *curFrame = &g_vm->_scriptFrame[g_vm->_curScriptFrame[CurStack]];
	// If the event is empty, terminate the script
	if ((curFrame->_class == 0) && (curFrame->_event == 0)) {
		EndScript();
		return;
	}

	bool loop = true;
	while (loop) {
		loop = false;
		curFrame = &g_vm->_scriptFrame[g_vm->_curScriptFrame[CurStack]];
		SScriptFrame *nextFrame = &g_vm->_scriptFrame[g_vm->_curScriptFrame[CurStack] + 1];
		curFrame->sendFrame();
		if (curFrame->_noWait && !((nextFrame->_class == 0) && (nextFrame->_event == 0))) {
			g_vm->_curScriptFrame[CurStack]++;
			loop = true;
		}
	}
}

/*-------------------------------------------------------------------------*/
/*                               EVALSCRIPT           					   */
/*-------------------------------------------------------------------------*/
void EvalScript() {
	if (g_vm->_characterQueue.testEmptyCharacterQueue4Script() && g_vm->_gameQueue.testEmptyQueue(MC_DIALOG) && FlagScreenRefreshed) {
		g_vm->_curScriptFrame[CurStack]++;
		g_vm->_flagMouseEnabled = false;

		SScriptFrame *curFrame = &g_vm->_scriptFrame[g_vm->_curScriptFrame[CurStack]];
		if ((curFrame->_class == 0) && (curFrame->_event == 0)) {
			EndScript();
			return;
		}

		bool loop = true;
		while (loop) {
			loop = false;
			curFrame = &g_vm->_scriptFrame[g_vm->_curScriptFrame[CurStack]];
			SScriptFrame *nextFrame = &g_vm->_scriptFrame[g_vm->_curScriptFrame[CurStack] + 1];
			curFrame->sendFrame();
			if (curFrame->_noWait && !((nextFrame->_class == 0) && (nextFrame->_event == 0))) {
				g_vm->_curScriptFrame[CurStack]++;
				loop = true;
			}
		}
	}
}

/*-------------------------------------------------------------------------*/
/*                                   TEXT              					   */
/*-------------------------------------------------------------------------*/
void Text(uint16 x, uint16 y, const char *sign, uint16 tcol, uint16 scol) {
	TextStackTop++;
	if (TextStackTop >= MAXTEXTSTACK) {
		warning("MaxTextStackReached!");
		return;
	}

	TextStack[TextStackTop].x     = x;
	TextStack[TextStackTop].y     = y;
	TextStack[TextStackTop].tcol  = tcol;
	TextStack[TextStackTop].scol  = scol;
	TextStack[TextStackTop].Clear = false;
	strcpy(TextStack[TextStackTop].sign, sign);
}

/* -----------------08/07/97 22.13-------------------
						ClearText
 --------------------------------------------------*/
void ClearText() {
	if (TextStackTop >= 0) {
	// The stack isn't empty
		if (! TextStack[TextStackTop].Clear)
		// The previous is a string to write, return
			TextStackTop--;
	} else {
	// the stack is empty
		TextStackTop = 0;
		TextStack[TextStackTop].Clear = true;
	}
}

/* -----------------08/07/97 22.14-------------------
					PaintString
 --------------------------------------------------*/
void PaintString() {
	for (int16 i = 0; i <= TextStackTop; i++) {
		if (TextStack[i].Clear)
			DoClearText();
		else
			TextStack[i].DoText();
	}
}

/* -----------------08/07/97 22.15-------------------
						DoText
 --------------------------------------------------*/
void StackText::DoText() {
	curString.x = x;
	curString.y = y;
	curString.dx = TextLength(sign, 0);
	if ((y == MAXY - CARHEI) && (curString.dx > 600))
		curString.dx = curString.dx * 3 / 5;
	else if ((y != MAXY - CARHEI) && (curString.dx > 960))
		curString.dx = curString.dx * 2 / 5;
	else if ((y != MAXY - CARHEI) && (curString.dx > 320))
		curString.dx = curString.dx * 3 / 5;

	curString.sign = sign;
	curString.l[0] = 0;
	curString.l[1] = 0;
	curString.l[2] = curString.dx;
	uint16 hstring = curString.checkDText();
	curString.l[3] = hstring;
	curString.dy   = hstring;
	curString.tcol = tcol;
	curString.scol = scol;

	if (curString.y <= hstring)
		curString.y += hstring;
	else
		curString.y -= hstring;

	if (curString.y <= VIDEOTOP)
		curString.y = VIDEOTOP + 1;


	TextStatus |= TEXT_DRAW;
}


/* -----------------08/07/97 22.15-------------------
					DoClearString
 --------------------------------------------------*/
void DoClearText() {
	if ((oldString.sign == nullptr) && (curString.sign)) {
		oldString.set(curString);
		curString.sign = nullptr;

		TextStatus |= TEXT_DEL;
	}
}

/* -----------------21/01/98 10.22-------------------
 * 				DoSys
 * --------------------------------------------------*/
void DoSys(uint16 curObj) {
	switch (curObj) {
	case o00QUIT:
		if (QuitGame())
			doEvent(MC_SYSTEM, ME_QUIT, MP_SYSTEM, 0, 0, 0, 0);
		break;

	case o00EXIT:
		if (g_vm->_oldRoom == rSYS)
			break;
		doEvent(MC_SYSTEM, ME_CHANGEROOM, MP_SYSTEM, g_vm->_obj[o00EXIT]._goRoom, 0, 0, 0);
		break;

	case o00SAVE:
		if (g_vm->_oldRoom == rSYS)
			break;
		g_vm->_curRoom = g_vm->_obj[o00EXIT]._goRoom;
		if (!DataSave()) {
			g_vm->showInventoryName(NO_OBJECTS, false);
			doEvent(MC_INVENTORY, ME_SHOWICONNAME, MP_DEFAULT, mx, my, 0, 0);
			doEvent(MC_SYSTEM, ME_CHANGEROOM, MP_SYSTEM, g_vm->_obj[o00EXIT]._goRoom, 0, 0, 0);
		}
		g_vm->_curRoom = rSYS;
		break;

	case o00LOAD:
		if (!DataLoad()) {
			g_vm->showInventoryName(NO_OBJECTS, false);
			doEvent(MC_INVENTORY, ME_SHOWICONNAME, MP_DEFAULT, mx, my, 0, 0);
		}
		break;

	case o00SPEECHON:
		if (ConfMan.getBool("subtitles")) {
			g_vm->_obj[o00SPEECHON]._mode &= ~OBJMODE_OBJSTATUS;
			g_vm->_obj[o00SPEECHOFF]._mode |= OBJMODE_OBJSTATUS;
			ConfMan.setBool("speech_mute", true);
			g_vm->_curObj = o00SPEECHOFF;
			RegenRoom();
			ShowObjName(g_vm->_curObj, true);
		}
		break;

	case o00SPEECHOFF:
		g_vm->_obj[o00SPEECHOFF]._mode &= ~OBJMODE_OBJSTATUS;
		g_vm->_obj[o00SPEECHON]._mode |= OBJMODE_OBJSTATUS;
		ConfMan.setBool("speech_mute", false);
		g_vm->_curObj = o00SPEECHON;
		RegenRoom();
		ShowObjName(g_vm->_curObj, true);
		break;

	case o00TEXTON:
		if (!ConfMan.getBool("speech_mute")) {
			g_vm->_obj[o00TEXTON]._mode &= ~OBJMODE_OBJSTATUS;
			g_vm->_obj[o00TEXTOFF]._mode |= OBJMODE_OBJSTATUS;
			ConfMan.setBool("subtitles", false);
			g_vm->_curObj = o00TEXTOFF;
			RegenRoom();
			ShowObjName(g_vm->_curObj, true);
		}
		break;

	case o00TEXTOFF:
		g_vm->_obj[o00TEXTOFF]._mode &= ~OBJMODE_OBJSTATUS;
		g_vm->_obj[o00TEXTON]._mode |= OBJMODE_OBJSTATUS;
		ConfMan.setBool("subtitles", true);
		g_vm->_curObj = o00TEXTON;
		RegenRoom();
		ShowObjName(g_vm->_curObj, true);
		break;

	case o00SPEECH1D:
	case o00SPEECH2D:
	case o00SPEECH3D:
	case o00SPEECH4D:
	case o00SPEECH5D:
	case o00SPEECH6D:
	case o00MUSIC1D:
	case o00MUSIC2D:
	case o00MUSIC3D:
	case o00MUSIC4D:
	case o00MUSIC5D:
	case o00MUSIC6D:
	case o00SOUND1D:
	case o00SOUND2D:
	case o00SOUND3D:
	case o00SOUND4D:
	case o00SOUND5D:
	case o00SOUND6D:
		g_vm->_obj[curObj]._mode &= ~OBJMODE_OBJSTATUS;
		if ((curObj != o00SPEECH6D) && (curObj != o00MUSIC6D) && (curObj != o00SOUND6D))
			g_vm->_obj[curObj + 1]._mode &= ~OBJMODE_OBJSTATUS;
		g_vm->_obj[curObj - 1]._mode |= OBJMODE_OBJSTATUS;
		g_vm->_obj[curObj - 2]._mode |= OBJMODE_OBJSTATUS;
		RegenRoom();
		if (curObj < o00MUSIC1D)
			ConfMan.setInt("speech_volume", ((curObj - 2 - o00SPEECH1D) / 2) * 51);
		else if (curObj > o00MUSIC6D)
			ConfMan.setInt("sfx_volume", ((curObj - 2 - o00SOUND1D) / 2) * 51);
		else
			ConfMan.setInt("music_volume", ((curObj - 2 - o00MUSIC1D) / 2) * 51);
		break;

	case o00SPEECH1U:
	case o00SPEECH2U:
	case o00SPEECH3U:
	case o00SPEECH4U:
	case o00SPEECH5U:
	case o00MUSIC1U:
	case o00MUSIC2U:
	case o00MUSIC3U:
	case o00MUSIC4U:
	case o00MUSIC5U:
	case o00SOUND1U:
	case o00SOUND2U:
	case o00SOUND3U:
	case o00SOUND4U:
	case o00SOUND5U:
		g_vm->_obj[curObj]._mode &= ~OBJMODE_OBJSTATUS;
		g_vm->_obj[curObj - 1]._mode &= ~OBJMODE_OBJSTATUS;
		g_vm->_obj[curObj + 1]._mode |= OBJMODE_OBJSTATUS;
		if ((curObj != o00SPEECH5U) && (curObj != o00MUSIC5U) && (curObj != o00SOUND5U))
			g_vm->_obj[curObj + 2]._mode |= OBJMODE_OBJSTATUS;
		RegenRoom();
		if (curObj < o00MUSIC1D)
			ConfMan.setInt("speech_volume", ((curObj + 1 - o00SPEECH1D) / 2) * 51);
		else if (curObj > o00MUSIC6D)
			ConfMan.setInt("sfx_volume", ((curObj + 1 - o00SOUND1D) / 2) * 51);
		else
			ConfMan.setInt("music_volume", ((curObj + 1 - o00MUSIC1D) / 2) * 51);
		break;
	}

	g_engine->syncSoundSettings();
	ConfMan.flushToDisk();
}

/* -----------------09/02/98 15.44-------------------
 * 					SetRoom
 * --------------------------------------------------*/
void SetRoom(unsigned short r, bool b) {
	switch (r) {
	case r21:
		if (!b) {
			read3D("21.3d");
			g_vm->_room[r21]._flag &= ~OBJFLAG_EXTRA;
			setPosition(14);
			g_vm->_obj[oCATENAT21]._position = 5;
			g_vm->_obj[oUSCITA21]._position = 11;

			// if we can go beyond
			if (((g_vm->iconPos(iSBARRA21) != MAXICON) && ((_choice[436]._flag & OBJFLAG_DONE) || (_choice[466]._flag & OBJFLAG_DONE)))
					|| ((_choice[451]._flag & OBJFLAG_DONE) || (_choice[481]._flag & OBJFLAG_DONE))) {
				g_vm->_obj[od21TO23]._flag |= OBJFLAG_ROOMOUT;
				g_vm->_obj[od21TO23]._flag &= ~OBJFLAG_EXAMINE;
			} else {
				g_vm->_obj[od21TO23]._flag &= ~OBJFLAG_ROOMOUT;
				g_vm->_obj[od21TO23]._flag |= OBJFLAG_EXAMINE;
			}
			g_vm->_obj[od21TO23]._anim = 0;
			g_vm->_obj[oUSCITA21]._mode |= OBJMODE_OBJSTATUS;

			g_vm->_obj[od21TO22]._flag |= OBJFLAG_ROOMOUT;
			g_vm->_obj[od21TO22]._flag &= ~OBJFLAG_EXAMINE;
			g_vm->_obj[od21TO22]._anim = aWALKOUT;
			g_vm->_obj[oPORTAA21]._anim = a212;
			g_vm->_obj[oDOORC21]._anim = a219;

			g_vm->_obj[oCUNICOLO21]._mode |= OBJMODE_OBJSTATUS;
			g_vm->_obj[oCARTELLONE21]._mode |= OBJMODE_OBJSTATUS;
		} else {
			read3D("212.3d");
			g_vm->_room[r21]._flag |= OBJFLAG_EXTRA;
			setPosition(15);
			g_vm->_obj[oCATENAT21]._position = 6;
			g_vm->_obj[oUSCITA21]._position = 21;

			g_vm->_obj[od21TO23]._flag |= OBJFLAG_ROOMOUT;
			g_vm->_obj[od21TO23]._flag &= ~OBJFLAG_EXAMINE;
			g_vm->_obj[od21TO23]._anim = aWALKOUT;
			g_vm->_obj[oUSCITA21]._mode |= OBJMODE_OBJSTATUS;

			// If we can go beyond
			if (((g_vm->iconPos(iSBARRA21) != MAXICON) && ((_choice[436]._flag & OBJFLAG_DONE) || (_choice[466]._flag & OBJFLAG_DONE)))
					|| ((_choice[451]._flag & OBJFLAG_DONE) || (_choice[481]._flag & OBJFLAG_DONE))) {
				g_vm->_obj[od21TO22]._flag |= OBJFLAG_ROOMOUT;
				g_vm->_obj[od21TO22]._flag &= ~OBJFLAG_EXAMINE;
			} else {
				g_vm->_obj[od21TO22]._flag &= ~OBJFLAG_ROOMOUT;
				g_vm->_obj[od21TO22]._flag |= OBJFLAG_EXAMINE;
			}
			g_vm->_obj[od21TO22]._anim = 0;
			g_vm->_obj[od21TO22]._examine = 335;
			g_vm->_obj[od21TO22]._action = 335;
			g_vm->_obj[oPORTAA21]._anim = 0;
			g_vm->_obj[oDOORC21]._anim = 0;

			g_vm->_obj[oCUNICOLO21]._mode &= ~OBJMODE_OBJSTATUS;
			g_vm->_obj[oCARTELLONE21]._mode &= ~OBJMODE_OBJSTATUS;
		}
		break;
	case r24:
		if (!b) {
			read3D("24.3d");
			g_vm->_room[r24]._flag &= ~OBJFLAG_EXTRA;
			g_vm->_obj[oPASSAGE24]._position = 3;
			g_vm->_obj[oMACERIE24]._position = 3;
			g_vm->_obj[oDUMMY24]._mode &= ~OBJMODE_OBJSTATUS;
			g_vm->_obj[oDUMMY24A]._mode |= OBJMODE_OBJSTATUS;
		} else {
			read3D("242.3d");
			g_vm->_room[r24]._flag |= OBJFLAG_EXTRA;
			g_vm->_obj[od24TO26]._mode |= OBJMODE_OBJSTATUS;
			g_vm->_obj[oPASSAGE24]._position = 4;
			g_vm->_obj[oMACERIE24]._position = 4;
			g_vm->_obj[oDUMMY24A]._mode &= ~OBJMODE_OBJSTATUS;
			g_vm->_obj[oDUMMY24]._mode |= OBJMODE_OBJSTATUS;
		}
		break;

	case r2A:
		if (!b) {
			read3D("2A.3d");
			g_vm->_room[r2A]._flag &= ~OBJFLAG_EXTRA;
			g_vm->_obj[oDUMMY2A2]._mode |= OBJMODE_OBJSTATUS;
			g_vm->_obj[oDUMMY2A]._mode &= ~OBJMODE_OBJSTATUS;
		} else {
			read3D("2A2.3d");
			g_vm->_room[r2A]._flag |= OBJFLAG_EXTRA;
			g_vm->_obj[oDUMMY2A]._mode |= OBJMODE_OBJSTATUS;
			g_vm->_obj[oDUMMY2A2]._mode &= ~OBJMODE_OBJSTATUS;
		}
		break;
	case r2B:
		if (!b) {
			read3D("2B.3d");
			g_vm->_room[r2B]._flag &= ~OBJFLAG_EXTRA;
			g_vm->_obj[oPORTA2B]._mode |= OBJMODE_OBJSTATUS;
			g_vm->_obj[od2BALLA28]._mode &= ~OBJMODE_OBJSTATUS;
		} else {
			read3D("2B2.3d");
			g_vm->_room[r2B]._flag |= OBJFLAG_EXTRA;
			g_vm->_obj[oPORTA2B]._mode &= ~OBJMODE_OBJSTATUS;
			g_vm->_obj[od2BALLA28]._mode |= OBJMODE_OBJSTATUS;
		}
		break;
	case r2E:
		if (!b) {
			g_vm->_obj[oCATWALKA2E]._nbox = BACKGROUND;
			g_vm->_obj[oCATWALKA2E]._position = 2;
			g_vm->_obj[oCATWALKA2E]._anim = a2E2PRIMAPALLONTANANDO;
			read3D("2E.3d");
			g_vm->_room[r2E]._flag &= ~OBJFLAG_EXTRA;
			g_vm->_obj[oDUMMY2E]._mode &= ~OBJMODE_OBJSTATUS;
			g_vm->_obj[oENTRANCE2E]._flag &= ~OBJFLAG_EXAMINE;
			g_vm->_obj[oPASSERELLAB2E]._mode &= ~OBJMODE_OBJSTATUS;
			g_vm->_obj[oCRATERE2E]._mode &= ~OBJMODE_OBJSTATUS;
			g_vm->_obj[oARBUSTI2E]._mode &= ~OBJMODE_OBJSTATUS;
			g_vm->_obj[oCREPACCIO2E]._position = 6;
		} else {
			g_vm->_obj[oCATWALKA2E]._position = 3;
			g_vm->_obj[oCATWALKA2E]._anim = a2E3PRIMAPAVVICINANDO;
			read3D("2E2.3d");
			g_vm->_room[r2E]._flag |= OBJFLAG_EXTRA;
			g_vm->_obj[oDUMMY2E]._mode |= OBJMODE_OBJSTATUS;
			g_vm->_obj[oENTRANCE2E]._mode |= OBJMODE_OBJSTATUS;
			g_vm->_obj[oENTRANCE2E]._flag |= OBJFLAG_EXAMINE;
			g_vm->_obj[oPASSERELLAB2E]._mode |= OBJMODE_OBJSTATUS;
			g_vm->_obj[oCRATERE2E]._mode |= OBJMODE_OBJSTATUS;
			g_vm->_obj[oARBUSTI2E]._mode |= OBJMODE_OBJSTATUS;
			g_vm->_obj[oCREPACCIO2E]._position = 7;
			g_vm->_obj[oCATWALKA2E]._nbox = FOREGROUND;
		}
		break;
	case r2GV:
		if (!b) {
			g_vm->_obj[oVIADOTTO2GV]._position = 7;
			g_vm->_obj[oVIADOTTO2GV]._anim = a2G7ATTRAVERSAPONTICELLO;
			read3D("2GV.3d");
			g_vm->_room[r2GV]._flag &= ~OBJFLAG_EXTRA;
			g_vm->_obj[oDUMMY2GV]._mode &= ~OBJMODE_OBJSTATUS;
			g_vm->_obj[oRAGAZZOS2GV]._mode &= ~OBJMODE_OBJSTATUS;
			g_vm->_obj[oCOCCODRILLO2GV]._mode &= ~OBJMODE_OBJSTATUS;
		}
		break;
	default:
		break;
	}

	RegenRoom();
}

} // End of namespace Trecision