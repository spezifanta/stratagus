//   ___________		     _________		      _____  __
//   \_	  _____/______   ____   ____ \_   ___ \____________ _/ ____\/  |_
//    |    __) \_  __ \_/ __ \_/ __ \/    \  \/\_  __ \__  \\   __\\   __\ 
//    |     \   |  | \/\  ___/\  ___/\     \____|  | \// __ \|  |   |  |
//    \___  /   |__|    \___  >\___  >\______  /|__|  (____  /__|   |__|
//	  \/		    \/	   \/	     \/		   \/
//  ______________________                           ______________________
//			  T H E   W A R   B E G I N S
//	   FreeCraft - A free fantasy real time strategy game engine
//
/**@name menu_proc.c	-	The menu processing code. */
//
//	(c) Copyright 1999-2002 by Andreas Arens
//
//	FreeCraft is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published
//	by the Free Software Foundation; only version 2 of the License.
//
//	FreeCraft is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	$Id$

//@{

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#include "freecraft.h"

#include "video.h"
#include "font.h"
#include "interface.h"
#include "menus.h"
#include "cursor.h"
#include "network.h"
#include "netconnect.h"
#include "ui.h"
#include "sound_server.h"
#include "sound.h"
#include "ccl.h"

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

local EventCallback callbacks;

    /// Struct which specifies the buttons gfx
global struct {
	/// resource filename one for each race
    const char*	File[PlayerMaxRaces];
	/// Width of button
    int		Width, Height;
	/// sprite : FILLED
    Graphic*	Sprite;
} MenuButtonGfx
#ifndef laterUSE_CCL
= {
    { "ui/buttons 1.png" ,"ui/buttons 2.png" },
    300, 7632,

    NULL
}
#endif
    ;

/**
**	The currently processed menu
*/
global Menu *CurrentMenu;

/**
**	The background picture used by menus
*/
local Graphic *Menusbgnd;

/**
**	X, Y, Width, and Height of menu are to redraw
*/
local int MenuRedrawX;
local int MenuRedrawY;
local int MenuRedrawW;
local int MenuRedrawH;

local int MenuButtonUnderCursor = -1;
local int MenuButtonCurSel = -1;

/*----------------------------------------------------------------------------
--	Menu operation functions
----------------------------------------------------------------------------*/

/**
**	Find a menu by ident.
**
**	@param menu_id	Unique identifier for the menu.
**
**	@return		Pointer to the menu, NULL if menu is not found.
*/
global Menu *FindMenu(const char *menu_id)
{
    Menu **menu;

    if (!(menu = (Menu **) hash_find(MenuHash, (char *)menu_id))) {
	DebugLevel0Fn("Menu `%s' not found, probably a bug.\n" _C_ menu_id);
	return NULL;
    } else {
	return *menu;
    }
}

/**
**	Invalidate previously redrawn menu areas.
*/
global void InvalidateMenuAreas(void)
{
    InvalidateAreaAndCheckCursor(
	     MenuRedrawX,MenuRedrawY,MenuRedrawW,MenuRedrawH);
    MustRedraw&=~RedrawMenu;
}

/**
**	Set menu backgound and draw it.
*/
global void MenusSetBackground(void)
{
    DestroyCursorBackground();
    if (!Menusbgnd) {
	Menusbgnd = LoadGraphic(MenuBackground);
	VideoSetPalette(Menusbgnd->Pixels);
    }

    // VideoLockScreen();

    // FIXME: bigger window ?
    VideoDrawSubClip(Menusbgnd,0,0,
	Menusbgnd->Width,Menusbgnd->Height,
	(VideoWidth-Menusbgnd->Width)/2,(VideoHeight-Menusbgnd->Height)/2);

    // VideoUnlockScreen();
}

/**
**	Draw menu button 'button' on x,y
**
**	@param button	Button identifier
**	@param flags	State of Button (clicked, mouse over...)
**	@param w	Button width (for border)
**	@param h	Button height (for border)
**	@param x	X display position
**	@param y	Y display position
**	@param font	font number for text
**	@param text	text to print on button
*/
global void DrawMenuButton(MenuButtonId button,unsigned flags,unsigned w,unsigned h,unsigned x,unsigned y,
	const int font,const unsigned char *text)
{
    MenuButtonId rb;
    int s, nc, rc;

    GetDefaultTextColors(&nc, &rc);
    if (flags&MenuButtonDisabled) {
	rb = button - 1;
	s = 0;
	SetDefaultTextColors(FontGrey,FontGrey);
    } else if (flags&MenuButtonClicked) {
	rb = button + 1;
	s = 2;
	SetDefaultTextColors(rc,rc);
    } else {
	rb = button;
	s = 0;
	if (flags&MenuButtonActive) {
	    SetDefaultTextColors(rc,rc);
	}
    }
    if (rb < MenuButtonGfx.Sprite->NumFrames) {
	VideoDraw(MenuButtonGfx.Sprite, rb, x, y);
    } else {
	if (rb < button) {
	    VideoDrawRectangleClip(ColorGray,x+1,y+1,w-2,h-2);
	    VideoDrawRectangleClip(ColorGray,x+2,y+2,w-4,h-4);
	} else {
	    // FIXME: Temp-workaround for missing folder button in non-expansion gfx
	    VideoDrawRectangleClip(ColorYellow,x+1,y+1,w-2,h-2);
	    VideoDrawRectangleClip(ColorYellow,x+2,y+2,w-4,h-4);
	}
    }
    if (text) {
	if (button != MBUTTON_FOLDER) {
	    VideoDrawTextCentered(s+x+w/2,s+y+(font == GameFont ? 4 : 7),font,text);
	} else {
	    SetDefaultTextColors(nc,rc);
	    VideoDrawText(x+44,y+6,font,text);
	}
    }
    if (flags&MenuButtonSelected) {
	if (flags&MenuButtonDisabled) {
	    VideoDrawRectangleClip(ColorGray,x,y,w-1,h);
	} else {
	    VideoDrawRectangleClip(ColorYellow,x,y,w-1,h);
	}
    }
    SetDefaultTextColors(nc,rc);
}

/**
**	Draw pulldown 'button' on menu mx, my
**
**	@param mi	menuitem pointer
**	@param mx	menu X display position (offset)
**	@param my	menu Y display position (offset)
*/
local void DrawPulldown(Menuitem *mi, unsigned mx, unsigned my)
{
    int i, nc, rc;
    char *text;
    unsigned flags;
    MenuButtonId rb;
    unsigned w, h, x, y, oh;

    x = mx+mi->xofs;
    y = my+mi->yofs;
    w = mi->d.pulldown.xsize;
    flags = mi->flags;
    rb = mi->d.pulldown.button;
    oh = h = mi->d.pulldown.ysize - 2;

    GetDefaultTextColors(&nc, &rc);
    if (flags&MenuButtonClicked) {
	// Make the menu inside of the screen (TOP)
	if (y <= mi->d.pulldown.curopt * h) {
	    y = 0;
	} else {
	    y -= mi->d.pulldown.curopt * h;
	}
	i = mi->d.pulldown.noptions;
	h *= i;
	while (i--) {
	    PushClipping();
	    SetClipping(0,0,x+w,VideoHeight-1);
	    VideoDrawClip(MenuButtonGfx.Sprite, rb, x-1, y-1 + oh*i);
	    PopClipping();
	    text = mi->d.pulldown.options[i];
	    if (text) {
		if (i == mi->d.pulldown.cursel)
		    SetDefaultTextColors(rc,rc);
		else
		    SetDefaultTextColors(nc,rc);
		VideoDrawText(x+2,y+2 + oh*i ,mi->font,text);
	    }
	}
	w += 2;
    } else {
	h = mi->d.pulldown.ysize;
	y = my+mi->yofs;
	if (flags&MenuButtonDisabled) {
	    rb--;
	    SetDefaultTextColors(FontGrey,FontGrey);
	} else {
	    if (flags&MenuButtonActive) {
		SetDefaultTextColors(rc,rc);
	    }
	}

	PushClipping();
	if (!(mi->d.pulldown.state & MI_PSTATE_PASSIVE)) {
	    SetClipping(0,0,x+w-20,VideoHeight-1);
	} else {
	    SetClipping(0,0,x+w-1,VideoHeight-1);
	}
	VideoDrawClip(MenuButtonGfx.Sprite, rb, x-1, y-1);
	PopClipping();
	if (!(mi->d.pulldown.state & MI_PSTATE_PASSIVE)) {
	    VideoDraw(MenuButtonGfx.Sprite, MBUTTON_DOWN_ARROW + rb - MBUTTON_PULLDOWN, x-1 + w-20, y-2);
	}
	text = mi->d.pulldown.options[mi->d.pulldown.curopt];
	if (text) {
	    VideoDrawText(x+2,y+2,mi->font,text);
	}
    }
    if (flags&MenuButtonSelected) {
	if (flags&MenuButtonDisabled) {
	    VideoDrawRectangleClip(ColorGray,x-2,y-2,w,h);
	} else {
	    VideoDrawRectangleClip(ColorYellow,x-2,y-2,w,h);
	}
    }
    SetDefaultTextColors(nc,rc);
}

/**
**	Draw listbox 'button' on menu mx, my
**
**	@param mi	menuitem pointer
**	@param mx	menu X display position (offset)
**	@param my	menu Y display position (offset)
*/
local void DrawListbox(Menuitem *mi, unsigned mx, unsigned my)
{
    int i, s, nc, rc;
    char *text;
    MenuButtonId rb = mi->d.listbox.button;
    unsigned flags = mi->flags;
    unsigned w, h, x, y;
    w = mi->d.listbox.xsize;
    h = mi->d.listbox.ysize;
    x = mx+mi->xofs;
    y = my+mi->yofs;

    GetDefaultTextColors(&nc, &rc);

    if (flags&MenuButtonDisabled) {
	rb--;
    }
    i = mi->d.listbox.nlines;
    s = mi->d.listbox.startline;
    while (i--) {
	PushClipping();
	SetClipping(0,0,x+w,VideoHeight-1);
	VideoDrawClip(MenuButtonGfx.Sprite, rb, x-1, y-1 + 18*i);
	PopClipping();
	if (!(flags&MenuButtonDisabled)) {
	    if (i < mi->d.listbox.noptions) {
		SetDefaultTextColors(nc,rc);
		text = (*mi->d.listbox.retrieveopt)(mi, i + s);
		if (text) {
		    if (i == mi->d.listbox.curopt)
			SetDefaultTextColors(rc,rc);
		    else
			SetDefaultTextColors(nc,rc);
		    VideoDrawText(x+2,y+2 + 18*i ,mi->font,text);
		}
	    }
	}
    }

    if (flags&MenuButtonSelected) {
	if (flags&MenuButtonDisabled) {
	    VideoDrawRectangleClip(ColorGray,x-2,y-2,w+1,h+2);
	} else {
	    VideoDrawRectangleClip(ColorYellow,x-2,y-2,w+1,h+2);
	}
    }
    SetDefaultTextColors(nc,rc);
}

/**
**	Draw vslider 'button' on menu mx, my
**
**	@param mi	menuitem pointer
**	@param mx	menu X display position (offset)
**	@param my	menu Y display position (offset)
*/
local void DrawVSlider(Menuitem *mi, unsigned mx, unsigned my)
{
    int p;
    unsigned flags = mi->flags;
    unsigned w, h, x, y;
    w = mi->d.vslider.xsize;
    h = mi->d.vslider.ysize;
    x = mx+mi->xofs;
    y = my+mi->yofs;

    if (flags&MenuButtonDisabled) {
	PushClipping();
	SetClipping(0,0,VideoWidth-1,y + h - 20);
	VideoDrawClip(MenuButtonGfx.Sprite, MBUTTON_S_VCONT - 1, x, y - 2);
	VideoDrawClip(MenuButtonGfx.Sprite, MBUTTON_S_VCONT - 1, x, y + h/2);
	PopClipping();
	VideoDraw(MenuButtonGfx.Sprite, MBUTTON_UP_ARROW - 1, x, y - 2);
	VideoDraw(MenuButtonGfx.Sprite, MBUTTON_DOWN_ARROW - 1, x, y + h - 20);
    } else {
	PushClipping();
	SetClipping(0,0,VideoWidth-1,y + h - 20);
	VideoDrawClip(MenuButtonGfx.Sprite, MBUTTON_S_VCONT, x, y - 2);
	VideoDrawClip(MenuButtonGfx.Sprite, MBUTTON_S_VCONT, x, y + h/2);
	PopClipping();
	if (mi->d.vslider.cflags&MI_CFLAGS_UP) {
	    VideoDraw(MenuButtonGfx.Sprite, MBUTTON_UP_ARROW + 1, x, y - 2);
	} else {
	    VideoDraw(MenuButtonGfx.Sprite, MBUTTON_UP_ARROW, x, y - 2);
	}
	if (mi->d.vslider.cflags&MI_CFLAGS_DOWN) {
	    VideoDraw(MenuButtonGfx.Sprite, MBUTTON_DOWN_ARROW + 1, x, y + h - 20);
	} else {
	    VideoDraw(MenuButtonGfx.Sprite, MBUTTON_DOWN_ARROW, x, y + h - 20);
	}
	p = (mi->d.vslider.percent * (h - 54)) / 100;
	VideoDraw(MenuButtonGfx.Sprite, MBUTTON_S_KNOB, x + 1, y + 18 + p);
    }

    if (flags&MenuButtonSelected) {
	if (flags&MenuButtonDisabled) {
	    VideoDrawRectangleClip(ColorGray,x,y-2,w,h+2);
	} else {
	    VideoDrawRectangleClip(ColorYellow,x,y-2,w,h+2);
	}
    }

}

/**
**	Draw hslider 'button' on menu mx, my
**
**	@param mi	menuitem pointer
**	@param mx	menu X display position (offset)
**	@param my	menu Y display position (offset)
*/
local void DrawHSlider(Menuitem *mi, unsigned mx, unsigned my)
{
    int p;
    unsigned flags = mi->flags;
    unsigned w, h, x, y;
    w = mi->d.hslider.xsize;
    h = mi->d.hslider.ysize;
    x = mx+mi->xofs;
    y = my+mi->yofs;

    if (flags&MenuButtonDisabled) {
	PushClipping();
	SetClipping(0,0,x + w - 20,VideoHeight-1);
	VideoDrawClip(MenuButtonGfx.Sprite, MBUTTON_S_HCONT - 1, x - 2, y);
	VideoDrawClip(MenuButtonGfx.Sprite, MBUTTON_S_HCONT - 1, x + w/2, y);
	PopClipping();
	VideoDraw(MenuButtonGfx.Sprite, MBUTTON_LEFT_ARROW - 1, x - 2, y);
	VideoDraw(MenuButtonGfx.Sprite, MBUTTON_RIGHT_ARROW - 1, x + w - 20, y);
    } else {
	PushClipping();
	SetClipping(0,0,x + w - 20,VideoHeight-1);
	VideoDrawClip(MenuButtonGfx.Sprite, MBUTTON_S_HCONT, x - 2, y);
	VideoDrawClip(MenuButtonGfx.Sprite, MBUTTON_S_HCONT, x + w/2, y);
	PopClipping();
	if (mi->d.hslider.cflags&MI_CFLAGS_LEFT) {
	    VideoDraw(MenuButtonGfx.Sprite, MBUTTON_LEFT_ARROW + 1, x - 2, y);
	} else {
	    VideoDraw(MenuButtonGfx.Sprite, MBUTTON_LEFT_ARROW, x - 2, y);
	}
	if (mi->d.hslider.cflags&MI_CFLAGS_RIGHT) {
	    VideoDraw(MenuButtonGfx.Sprite, MBUTTON_RIGHT_ARROW + 1, x + w - 20, y);
	} else {
	    VideoDraw(MenuButtonGfx.Sprite, MBUTTON_RIGHT_ARROW, x + w - 20, y);
	}
	p = (mi->d.hslider.percent * (w - 54)) / 100;
	VideoDraw(MenuButtonGfx.Sprite, MBUTTON_S_KNOB, x + 18 + p, y + 1);
    }

    if (flags&MenuButtonSelected) {
	if (flags&MenuButtonDisabled) {
	    VideoDrawRectangleClip(ColorGray,x-2,y,w+2,h);
	} else {
	    VideoDrawRectangleClip(ColorYellow,x-2,y,w+2,h);
	}
    }

}

/**
**	Draw gem 'button' on menu mx, my
**
**	@param mi	menuitem pointer
**	@param mx	menu X display position (offset)
**	@param my	menu Y display position (offset)
*/
local void DrawGem(Menuitem *mi, unsigned mx, unsigned my)
{
    unsigned flags = mi->flags;
    MenuButtonId rb = mi->d.gem.button;
    unsigned x, y;

    x = mx+mi->xofs;
    y = my+mi->yofs;
    if ((mi->d.gem.state & MI_GSTATE_INVISIBLE)) {
	return;
    }
    if (flags&MenuButtonDisabled) {
	rb--;
    }
    else {
	if (flags&MenuButtonClicked) {
	    rb++;
	}
	if ((mi->d.gem.state & MI_GSTATE_CHECKED)) {
	    rb += 2;
	}
    }
    VideoDraw(MenuButtonGfx.Sprite, rb, x, y);
}

/**
**	Draw input 'button' on menu mx, my
**
**	@param mi	menuitem pointer
**	@param mx	menu X display position (offset)
**	@param my	menu Y display position (offset)
*/
local void DrawInput(Menuitem *mi, unsigned mx, unsigned my)
{
    int nc, rc;
    char *text;
    unsigned flags = mi->flags;
    MenuButtonId rb = mi->d.input.button;
    unsigned w, h, x, y;

    x = mx+mi->xofs;
    y = my+mi->yofs;
    w = mi->d.input.xsize;
    h = mi->d.input.ysize;

    GetDefaultTextColors(&nc, &rc);
    if (flags&MenuButtonDisabled) {
	rb--;
	SetDefaultTextColors(FontGrey,FontGrey);
    }

    PushClipping();
    SetClipping(0,0,x+w,VideoHeight-1);
    VideoDrawClip(MenuButtonGfx.Sprite, rb, x-1, y-1);
    PopClipping();
    text = mi->d.input.buffer;
    if (text) {
	VideoDrawText(x+2,y+2,mi->font,text);
    }
    if (flags&MenuButtonSelected) {
	if (flags&MenuButtonDisabled) {
	    VideoDrawRectangleClip(ColorGray,x-2,y-2,w+4,h);
	} else {
	    VideoDrawRectangleClip(ColorYellow,x-2,y-2,w+4,h);
	}
    }
    SetDefaultTextColors(nc,rc);
}


/**
**	Draw a menu.
**
**	@param menu_id	The menu number to display
*/
global void DrawMenu(Menu *menu)
{
    int i, n, l;
    Menuitem *mi, *mip;

    if (menu == NULL) {
	MustRedraw &= ~RedrawMenu;
	return;
    }

    switch (menu->image) {
	case ImagePanel1:
	    VideoDrawSub(TheUI.GameMenuePanel.Graphic,0,0,
		    VideoGraphicWidth(TheUI.GameMenuePanel.Graphic),
		    VideoGraphicHeight(TheUI.GameMenuePanel.Graphic),
		    menu->x,menu->y);
	    break;
	case ImagePanel2:
	    VideoDrawSub(TheUI.Menue1Panel.Graphic,0,0,
		    VideoGraphicWidth(TheUI.Menue1Panel.Graphic),
		    VideoGraphicHeight(TheUI.Menue1Panel.Graphic),
		    menu->x,menu->y);
	    break;
	case ImagePanel3:
	    VideoDrawSub(TheUI.Menue2Panel.Graphic,0,0,
		   VideoGraphicWidth(TheUI.Menue2Panel.Graphic),
		   VideoGraphicHeight(TheUI.Menue2Panel.Graphic),
		   menu->x,menu->y);
	    break;
	case ImagePanel4:
	    VideoDrawSub(TheUI.VictoryPanel.Graphic,0,0,
		   VideoGraphicWidth(TheUI.VictoryPanel.Graphic),
		   VideoGraphicHeight(TheUI.VictoryPanel.Graphic),
		   menu->x,menu->y);
	    break;
	case ImagePanel5:
	    VideoDrawSub(TheUI.ScenarioPanel.Graphic,0,0,
		   VideoGraphicWidth(TheUI.ScenarioPanel.Graphic),
		   VideoGraphicHeight(TheUI.ScenarioPanel.Graphic),
		   menu->x,menu->y);
	default:
	    break;
    }
    n = menu->nitems;
    mi = menu->items;
    mip = NULL;
    for (i = 0; i < n; i++) {
	switch (mi->mitype) {
	    case MI_TYPE_TEXT:
		if (!mi->d.text.text)
		    break;
		if (mi->d.text.tflags&MI_TFLAGS_CENTERED)
		    VideoDrawTextCentered(menu->x+mi->xofs,menu->y+mi->yofs,
			    mi->font,mi->d.text.text);
		else if (mi->d.text.tflags&MI_TFLAGS_RALIGN) {
		    l = VideoTextLength(mi->font,mi->d.text.text);
		    VideoDrawText(menu->x+mi->xofs-l,menu->y+mi->yofs,
			    mi->font,mi->d.text.text);
		} else
		    VideoDrawText(menu->x+mi->xofs,menu->y+mi->yofs,
			    mi->font,mi->d.text.text);
		break;
	    case MI_TYPE_BUTTON:
		DrawMenuButton(mi->d.button.button,mi->flags,
			mi->d.button.xsize,mi->d.button.ysize,
			menu->x+mi->xofs,menu->y+mi->yofs,
			mi->font,mi->d.button.text);
		break;
	    case MI_TYPE_PULLDOWN:
		if (mi->flags&MenuButtonClicked) {
		    mip = mi;	// Delay, due to possible overlaying!
		} else {
		    DrawPulldown(mi,menu->x,menu->y);
		}
		break;
	    case MI_TYPE_LISTBOX:
		DrawListbox(mi,menu->x,menu->y);
		break;
	    case MI_TYPE_VSLIDER:
		DrawVSlider(mi,menu->x,menu->y);
		break;
	    case MI_TYPE_HSLIDER:
		DrawHSlider(mi,menu->x,menu->y);
		break;
	    case MI_TYPE_DRAWFUNC:
		if (mi->d.drawfunc.draw) {
		    (*mi->d.drawfunc.draw)(mi);
		}
		break;
	    case MI_TYPE_INPUT:
		DrawInput(mi,menu->x,menu->y);
		break;
	    case MI_TYPE_GEM:
		DrawGem(mi,menu->x,menu->y);
		break;
	    default:
		break;
	}
	mi++;
    }
    if (mip) {
	DrawPulldown(mip,menu->x,menu->y);
    }

    MenuRedrawX = menu->x;
    MenuRedrawY = menu->y;
    MenuRedrawW = menu->xsize;
    MenuRedrawH = menu->ysize;
}
/**
**	Handle keys in menu mode.
**
**	@param key	Key scancode.
**	@param keychar	ASCII character code of key.
**
**	@todo FIXME: Should be MenuKeyDown(), and act on _new_ MenuKeyUp() !!!
**      to implement button animation (depress before action)
*/
local void MenuHandleKeyDown(unsigned key,unsigned keychar)
{
    int i, n;
    Menuitem *mi;
    Menu *menu;

    HandleKeyModifiersDown(key, keychar);

    if (CurrentMenu == NULL) {
	return;
    }
    menu = CurrentMenu;
    if (MenuButtonCurSel != -1 && menu->items[MenuButtonCurSel].mitype == MI_TYPE_INPUT) {
	mi = menu->items + MenuButtonCurSel;
	if (!(mi->flags & MenuButtonDisabled)) {
inkey:
	    if (key >= 0x80 && key < 0x100) {
		// FIXME ARI: ISO->WC2 Translation here!
		key = 0;
	    }
	    switch(key) {
		case '\b': case '\177':
		    if (mi->d.input.nch > 0) {
			strcpy(mi->d.input.buffer + (--mi->d.input.nch), "~!_");
			MustRedraw |= RedrawMenu;
		    }
		    break;
		case 9:
		    goto normkey;
		case '~':		// ~ are quotes
		    return;		// Just ignore them
		case 'x':
		case 'X':
		    if( (KeyModifiers&ModifierAlt) ) {
			goto normkey;
		    }
		    /* FALL THROUGH */
		default:
		    if (key >= 32 && key < 0x100) {
			if (mi->d.input.nch < mi->d.input.maxch) {
			    mi->d.input.buffer[mi->d.input.nch++] = keychar;
			    strcpy(mi->d.input.buffer + mi->d.input.nch, "~!_");
			    MustRedraw |= RedrawMenu;
			}
		    }
		    break;
	    }
	    if (mi->d.input.action) {
		(*mi->d.input.action)(mi, key);
	    }
	    return;
	}
    }

normkey:
    if( !(KeyModifiers&ModifierAlt) ) {
	mi = menu->items;
	i = menu->nitems;
	while (i--) {
	    switch (mi->mitype) {
		case MI_TYPE_BUTTON:
		    if (key == mi->d.button.hotkey) {
			if (!(mi->flags & MenuButtonDisabled) && mi->d.button.handler) {
			    (*mi->d.button.handler)();
			}
			return;
		    }
		default:
		    break;
	    }
	    mi++;
	}
    }
    switch (key) {
	case 10: case 13:			// RETURN
	    if (MenuButtonCurSel != -1) {
		mi = menu->items + MenuButtonCurSel;
		switch (mi->mitype) {
		    case MI_TYPE_BUTTON:
			if (mi->d.button.handler) {
			    (*mi->d.button.handler)();
			}
			return;
		    case MI_TYPE_LISTBOX:
			if (mi->d.listbox.handler) {
			    (*mi->d.listbox.handler)();
			}
			return;
		    case MI_TYPE_VSLIDER:
			if (mi->d.vslider.handler) {
			    (*mi->d.vslider.handler)();
			}
			return;
		    case MI_TYPE_HSLIDER:
			if (mi->d.hslider.handler) {
			    (*mi->d.hslider.handler)();
			}
			return;
		    default:
			break;
		}
	    }
	    break;
	case KeyCodeUp: case KeyCodeDown:
	    if (MenuButtonCurSel != -1) {
		mi = menu->items + MenuButtonCurSel;
		if (!(mi->flags&MenuButtonClicked)) {
		    switch (mi->mitype) {
			case MI_TYPE_PULLDOWN:
			    if (key == KeyCodeDown) {
				if (mi->d.pulldown.curopt + 1 < mi->d.pulldown.noptions)
				    mi->d.pulldown.curopt++;
				else
				    break;
			    } else {
				if (mi->d.pulldown.curopt > 0)
				    mi->d.pulldown.curopt--;
				else
				    break;
			    }
			    MustRedraw |= RedrawMenu;
			    if (mi->d.pulldown.action) {
				(*mi->d.pulldown.action)(mi, mi->d.pulldown.curopt);
			    }
			    break;
			case MI_TYPE_LISTBOX:
			    if (key == KeyCodeDown) {
				if (mi->d.listbox.curopt+mi->d.listbox.startline+1 < mi->d.pulldown.noptions) {
				    mi->d.listbox.curopt++;
				    if (mi->d.listbox.curopt >= mi->d.listbox.nlines) {
					mi->d.listbox.curopt--;
					mi->d.listbox.startline++;
				    }
				} else {
				    break;
				}
			    } else {
				if (mi->d.listbox.curopt+mi->d.listbox.startline > 0) {
				    mi->d.listbox.curopt--;
				    if (mi->d.listbox.curopt < 0) {
					mi->d.listbox.curopt++;
					mi->d.listbox.startline--;
				    }
				} else {
				    break;
				}
			    }
			    if (mi->d.listbox.action) {
				(*mi->d.listbox.action)(mi, mi->d.listbox.curopt + mi->d.listbox.startline);
			    }
			    MustRedraw |= RedrawMenu;
			    break;
			case MI_TYPE_VSLIDER:
			    if (key == KeyCodeDown) {
				mi->d.vslider.cflags |= MI_CFLAGS_DOWN;
			    } else {
				mi->d.vslider.cflags |= MI_CFLAGS_UP;
			    }
			    if (mi->d.vslider.action) {
				(*mi->d.vslider.action)(mi, 2);		// 0 indicates key down
			    }
			    MustRedraw |= RedrawMenu;
			    break;
			default:
			    break;
		    }
		}
	    }
	    break;
	case 9:				// TAB			// FIXME: Add Shift-TAB
	    if (MenuButtonCurSel != -1 && !(menu->items[MenuButtonCurSel].flags&MenuButtonClicked)) {
		n = menu->nitems;
		for (i = 0; i < n; ++i) {
		    mi = menu->items + ((MenuButtonCurSel + i + 1) % n);
		    switch (mi->mitype) {
			case MI_TYPE_PULLDOWN:
			    if ((mi->d.pulldown.state & MI_PSTATE_PASSIVE)) {
				continue;
			    }
			    /* FALL THROUGH */
			case MI_TYPE_BUTTON:
			case MI_TYPE_LISTBOX:
			case MI_TYPE_VSLIDER:
			case MI_TYPE_HSLIDER:
			case MI_TYPE_INPUT:
			    if (mi->flags & MenuButtonDisabled) {
				break;
			    }
			    mi->flags |= MenuButtonSelected;
			    menu->items[MenuButtonCurSel].flags &= ~MenuButtonSelected;
			    MenuButtonCurSel = mi - menu->items;
			    MustRedraw |= RedrawMenu;
			    return;
			default:
			    break;
		    }
		}
	    }
	    break;
	case 'x':
	case 'X':
	    if( (KeyModifiers&ModifierAlt) ) {
		Exit(0);
	    }
	default:
	    mi = menu->items;
	    i = menu->nitems;
	    while (i--) {
		switch (mi->mitype) {
		    case MI_TYPE_INPUT:
			if (!(mi->flags & MenuButtonDisabled)) {
			    menu->items[MenuButtonCurSel].flags &= ~MenuButtonSelected;
			    mi->flags |= MenuButtonSelected;
			    MenuButtonCurSel = mi - menu->items;
			    MustRedraw |= RedrawMenu;
			    goto inkey;
			}
		    default:
			break;
		}
		mi++;
	    }
	    DebugLevel3("Key %d\n" _C_ key);
	    return;
    }
    return;
}

/**
**	Handle keys in menu mode.
**
**	@param key	Key scancode.
**	@param keychar	ASCII character code of key.
*/
local void MenuHandleKeyUp(unsigned key,unsigned keychar)
{
    HandleKeyModifiersUp(key,keychar);
}

/**
**	Handle keys repeated in menu mode.
**
**	@param key	Key scancode.
**	@param keychar	ASCII character code of key.
*/
local void MenuHandleKeyRepeat(unsigned key,unsigned keychar)
{
    HandleKeyModifiersDown(key,keychar);

    if (CurrentMenu == NULL) {
	return;
    }

    if (MenuButtonCurSel != -1 && CurrentMenu->items[MenuButtonCurSel].mitype == MI_TYPE_INPUT) {
	MenuHandleKeyDown(key,keychar);
    }
}

/**
**	Handle movement of the cursor.
**
**	@param x	Screen X position.
**	@param y	Screen Y position.
*/
local void MenuHandleMouseMove(int x,int y)
{
    int h, w, i, j, n, xs, ys;
    Menuitem *mi;
    Menu *menu;
    int ox;
    int oy;
    int redraw_flag;

    ox = CursorX;
    oy = CursorY;			// Old position for rel movement.
    HandleCursorMove(&x,&y);

    if (CurrentMenu == NULL) {
	return;
    }

    menu = CurrentMenu;

    n = menu->nitems;
    MenuButtonUnderCursor = -1;
    redraw_flag = 0;

    // check active (popped-up) pulldown first, as it may overlay other menus!
    mi = menu->items;
    for (i = 0; i < n; ++i) {
	if (!(mi->flags&MenuButtonDisabled)) {
	    if (mi->mitype == MI_TYPE_PULLDOWN && (mi->flags&MenuButtonClicked)) {
		xs = menu->x + mi->xofs;
		ys = menu->y + mi->yofs;
		h = mi->d.pulldown.ysize - 2;
		if (ys <= mi->d.pulldown.curopt * h) {
		    ys = 0;
		} else {
		    ys -= mi->d.pulldown.curopt * h;
		}
		if (!(x<xs || x>xs + mi->d.pulldown.xsize || y<ys || y>ys + h*mi->d.pulldown.noptions)) {
		    j = (y - ys) / h;
		    if (j >= 0 && j < mi->d.pulldown.noptions && j != mi->d.pulldown.cursel) {
			mi->d.pulldown.cursel = j;
			redraw_flag = 1;
			if (mi->d.pulldown.action) {
			    (*mi->d.pulldown.action)(mi, mi->d.pulldown.cursel);
			}
		    }
		}
		MenuButtonUnderCursor = i;
		break;
	    }
	}
	mi++;
    }
    if (MenuButtonUnderCursor == -1) {
	for (i = 0; i < n; ++i) {
	    mi = menu->items + i;
	    if (!(mi->flags&MenuButtonDisabled)) {
		switch (mi->mitype) {
		    case MI_TYPE_GEM:
			xs = menu->x + mi->xofs;
			ys = menu->y + mi->yofs;
			if (x < xs || x > xs + mi->d.gem.xsize || y < ys || y > ys + mi->d.gem.ysize) {
			    if (!(mi->flags&MenuButtonClicked)) {
				if (mi->flags&MenuButtonActive) {
				    redraw_flag = 1;
				    mi->flags &= ~MenuButtonActive;
				}
			    }
			    continue;
			}
			break;
		    case MI_TYPE_BUTTON:
			xs = menu->x + mi->xofs;
			ys = menu->y + mi->yofs;
			if (x < xs || x > xs + mi->d.button.xsize || y < ys || y > ys + mi->d.button.ysize) {
			    if (!(mi->flags&MenuButtonClicked)) {
				if (mi->flags&MenuButtonActive) {
				    redraw_flag = 1;
				    mi->flags &= ~MenuButtonActive;
				}
			    }
			    continue;
			}
			break;
		    case MI_TYPE_INPUT:
			xs = menu->x + mi->xofs;
			ys = menu->y + mi->yofs;
			if (x<xs || x>xs + mi->d.input.xsize
				|| y<ys || y>ys + mi->d.input.ysize) {
			    if (!(mi->flags&MenuButtonClicked)) {
				if (mi->flags&MenuButtonActive) {
				    redraw_flag = 1;
				    mi->flags &= ~MenuButtonActive;
				}
			    }
			    continue;
			}
			break;
		    case MI_TYPE_PULLDOWN:
			if ((mi->d.pulldown.state & MI_PSTATE_PASSIVE)) {
			    continue;
			}
			// Clicked-state already checked above - there can only be one!
			xs = menu->x + mi->xofs;
			ys = menu->y + mi->yofs;
			if (x<xs || x>xs + mi->d.pulldown.xsize || y<ys || y>ys + mi->d.pulldown.ysize) {
			    if (!(mi->flags&MenuButtonClicked)) {
				if (mi->flags&MenuButtonActive) {
				    redraw_flag = 1;
				    mi->flags &= ~MenuButtonActive;
				}
			    }
			    continue;
			}
			break;
		    case MI_TYPE_LISTBOX:
			xs = menu->x + mi->xofs;
			ys = menu->y + mi->yofs;
			if (x < xs || x > xs + mi->d.listbox.xsize || y < ys || y > ys + mi->d.listbox.ysize) {
			    if (!(mi->flags&MenuButtonClicked)) {
				if (mi->flags&MenuButtonActive) {
				    redraw_flag = 1;
				    mi->flags &= ~MenuButtonActive;
				}
			    }
			    continue;
			}
			j = (y - ys) / 18;
			if (j != mi->d.listbox.cursel) {
			    mi->d.listbox.cursel = j;	// just store for click
			}
			if (mi->flags&MenuButtonClicked && mi->flags&MenuButtonActive) {
			    if (mi->d.listbox.cursel != mi->d.listbox.curopt) {
				mi->d.listbox.dohandler = 0;
				mi->d.listbox.curopt = mi->d.listbox.cursel;
				redraw_flag = 1;
				if (mi->d.listbox.action) {
				    (*mi->d.listbox.action)(mi, mi->d.listbox.curopt + mi->d.listbox.startline);
				}
			    }
			}
			break;
		    case MI_TYPE_VSLIDER:
			xs = menu->x + mi->xofs;
			ys = menu->y + mi->yofs;
			if (x < xs || x > xs + mi->d.vslider.xsize || y < ys || y > ys + mi->d.vslider.ysize) {
			    if (!(mi->flags&MenuButtonClicked)) {
				if (mi->flags&MenuButtonActive) {
				    redraw_flag = 1;
				    mi->flags &= ~MenuButtonActive;
				}
			    }
			    mi->d.vslider.cursel = 0;
			    continue;
			}
			j = y - ys;
			mi->d.vslider.cursel = 0;

			if (j < 20) {
			    mi->d.vslider.cursel |= MI_CFLAGS_UP;
			} else if (j > mi->d.vslider.ysize - 20) {
			    mi->d.vslider.cursel |= MI_CFLAGS_DOWN;
			} else {
			    mi->d.vslider.cursel &= ~(MI_CFLAGS_UP|MI_CFLAGS_DOWN);
			    h = (mi->d.vslider.percent * (mi->d.vslider.ysize - 54)) / 100 + 18;
			    if (j > h && j < h + 18) {
				mi->d.vslider.cursel |= MI_CFLAGS_KNOB;
			    } else {
				mi->d.vslider.cursel |= MI_CFLAGS_CONT;
				if (j <= h) {
				    mi->d.vslider.cursel |= MI_CFLAGS_UP;
				} else {
				    mi->d.vslider.cursel |= MI_CFLAGS_DOWN;
				}
			    }
			    j -= 8;
			    if (j < 20) j=20;

			    mi->d.vslider.curper = ((j - 20) * 100) / (mi->d.vslider.ysize - 54);
			    if (mi->d.vslider.curper > 100) {
				mi->d.vslider.curper = 100;
			    }
			}
			if (mi->d.vslider.action) {
			    (*mi->d.vslider.action)(mi, 1);		// 1 indicates move
			}
			break;
		    case MI_TYPE_HSLIDER:
			xs = menu->x + mi->xofs;
			ys = menu->y + mi->yofs;
			if (x < xs || x > xs + mi->d.hslider.xsize || y < ys || y > ys + mi->d.hslider.ysize) {
			    if (!(mi->flags&MenuButtonClicked)) {
				if (mi->flags&MenuButtonActive) {
				    redraw_flag = 1;
				    mi->flags &= ~MenuButtonActive;
				}
			    }
			    mi->d.hslider.cursel = 0;
			    continue;
			}
			j = x - xs;
			j -= 6;
			if (j < 20) {
			    mi->d.hslider.cursel |= MI_CFLAGS_LEFT;
			} else if (j > mi->d.hslider.xsize - 20) {
			    mi->d.hslider.cursel |= MI_CFLAGS_RIGHT;
			} else {
			    mi->d.hslider.cursel &= ~(MI_CFLAGS_LEFT|MI_CFLAGS_RIGHT);
			    w = (mi->d.hslider.percent * (mi->d.hslider.xsize - 54)) / 100 + 18;
			    mi->d.hslider.curper = ((j - 20) * 100) / (mi->d.hslider.xsize - 54);
			    if (mi->d.hslider.curper > 100) {
				mi->d.hslider.curper = 100;
			    }
			    if (j > w && j < w + 18) {
				mi->d.hslider.cursel |= MI_CFLAGS_KNOB;
			    } else {
				mi->d.hslider.cursel |= MI_CFLAGS_CONT;
				if (j <= w) {
				    mi->d.hslider.cursel |= MI_CFLAGS_LEFT;
				} else {
				    mi->d.hslider.cursel |= MI_CFLAGS_RIGHT;
				}
			    }
			}
			if (mi->d.hslider.action) {
			    (*mi->d.hslider.action)(mi, 1);		// 1 indicates move
			}
			break;
		    default:
			continue;
			// break;
		}
		switch (mi->mitype) {
		    case MI_TYPE_GEM:
			if ((mi->d.gem.state & MI_GSTATE_PASSIVE)) {
			    break;
			}
			/* FALL THROUGH */
		    case MI_TYPE_BUTTON:
		    case MI_TYPE_PULLDOWN:
		    case MI_TYPE_LISTBOX:
		    case MI_TYPE_VSLIDER:
		    case MI_TYPE_HSLIDER:
			if (!(mi->flags&MenuButtonActive)) {
			    redraw_flag = 1;
			    mi->flags |= MenuButtonActive;
			}
			MenuButtonUnderCursor = i;
		    default:
			break;
		    case MI_TYPE_INPUT:
			if (!(mi->flags&MenuButtonActive)) {
			    redraw_flag = 1;
			    mi->flags |= MenuButtonActive;
			}
			if (MouseButtons & LeftButton
				&& mi->flags & MenuButtonSelected) {
			    if (mi->d.input.buffer && *mi->d.input.buffer) {
				char* s;

				j = strtol(mi->d.input.buffer, &s, 0);
				if (!*s || s[0]=='~' ) {
				    mi->d.input.nch =
					sprintf(mi->d.input.buffer, "%d~!_",
						j + x - ox
						+ (y - oy)*1000);
				    redraw_flag = 1;
				}
			    }
			}
			MenuButtonUnderCursor = i;
			break;
		}
	    }
	}
    }
    if (redraw_flag) {
	MustRedraw |= RedrawMenu;
    }
}

/**
**	Called if mouse button pressed down.
**
**	@param b	button code
*/
local void MenuHandleButtonDown(unsigned b __attribute__((unused)))
{
    Menuitem *mi;
    Menu *menu;

    if (CurrentMenu == NULL) {
	return;
    }

    menu = CurrentMenu;

    if (MouseButtons&(LeftButton<<MouseHoldShift))
	return;

    if (MouseButtons&LeftButton) {
	if (MenuButtonUnderCursor != -1) {
	    mi = menu->items + MenuButtonUnderCursor;
	    if (!(mi->flags&MenuButtonClicked)) {
		switch (mi->mitype) {
		    case MI_TYPE_GEM:
		    case MI_TYPE_BUTTON:
		    case MI_TYPE_PULLDOWN:
		    case MI_TYPE_LISTBOX:
		    case MI_TYPE_VSLIDER:
		    case MI_TYPE_HSLIDER:
		    case MI_TYPE_INPUT:
			if (MenuButtonCurSel != -1) {
			    menu->items[MenuButtonCurSel].flags &= ~MenuButtonSelected;
			}
			MenuButtonCurSel = MenuButtonUnderCursor;
			mi->flags |= MenuButtonClicked|MenuButtonSelected;
			MustRedraw |= RedrawMenu;
		    default:
			break;
		}
	    }
	    PlayGameSound(GameSounds.Click.Sound,MaxSampleVolume);
	    switch (mi->mitype) {
		case MI_TYPE_VSLIDER:
		    mi->d.vslider.cflags = mi->d.vslider.cursel;
		    if (mi->d.vslider.action) {
			(*mi->d.vslider.action)(mi, 0);		// 0 indicates down
		    }
		    break;
		case MI_TYPE_HSLIDER:
		    mi->d.hslider.cflags = mi->d.hslider.cursel;
		    if (mi->d.hslider.action) {
			(*mi->d.hslider.action)(mi, 0);		// 0 indicates down
		    }
		    break;
		case MI_TYPE_PULLDOWN:
		    if (mi->d.pulldown.curopt >= 0 &&
					    mi->d.pulldown.curopt < mi->d.pulldown.noptions) {
			mi->d.pulldown.cursel = mi->d.pulldown.curopt;
		    }
		    break;
		case MI_TYPE_LISTBOX:
		    if (mi->d.listbox.cursel != mi->d.listbox.curopt) {
			mi->d.listbox.dohandler = 0;
			mi->d.listbox.curopt = mi->d.listbox.cursel;
			if (mi->d.listbox.action) {
			    (*mi->d.listbox.action)(mi, mi->d.listbox.curopt + mi->d.listbox.startline);
			}
		    }
		    else {
			mi->d.listbox.dohandler = 1;
		    }
		    break;
		default:
		    break;
	    }
	}
    }
}

/**
**	Called if mouse button released.
**
**	@param b	button code
*/
local void MenuHandleButtonUp(unsigned b)
{
    int i, n;
    Menuitem *mi;
    Menu *menu;
    int redraw_flag = 0;

    if (CurrentMenu == NULL) {
	return;
    }

    menu = CurrentMenu;

    if ((1<<b) == LeftButton) {
	n = menu->nitems;
	for (i = 0; i < n; ++i) {
	    mi = menu->items + i;
	    switch (mi->mitype) {
		case MI_TYPE_GEM:
		    if (mi->flags&MenuButtonClicked) {
			redraw_flag = 1;
			mi->flags &= ~MenuButtonClicked;
			if (MenuButtonUnderCursor == i) {
			    MenuButtonUnderCursor = -1;
			    if ((mi->d.gem.state & MI_GSTATE_CHECKED)) {
				mi->d.gem.state &= ~MI_GSTATE_CHECKED;
			    } else {
				mi->d.gem.state |= MI_GSTATE_CHECKED;
			    }
			    if (mi->d.gem.action) {
				(*mi->d.gem.action)(mi);
			    }
			}
		    }
		    break;
		case MI_TYPE_BUTTON:
		    if (mi->flags&MenuButtonClicked) {
			redraw_flag = 1;
			mi->flags &= ~MenuButtonClicked;
			if (MenuButtonUnderCursor == i) {
			    MenuButtonUnderCursor = -1;
			    if (mi->d.button.handler) {
				(*mi->d.button.handler)();
			    }
			}
		    }
		    break;
		case MI_TYPE_PULLDOWN:
		    if (mi->flags&MenuButtonClicked) {
			redraw_flag = 1;
			mi->flags &= ~MenuButtonClicked;
			if (MenuButtonUnderCursor == i) {
			    MenuButtonUnderCursor = -1;
			    if (mi->d.pulldown.cursel != mi->d.pulldown.curopt &&
					    mi->d.pulldown.cursel >= 0 &&
					    mi->d.pulldown.cursel < mi->d.pulldown.noptions) {
				mi->d.pulldown.curopt = mi->d.pulldown.cursel;
				if (mi->d.pulldown.action) {
				    (*mi->d.pulldown.action)(mi, mi->d.pulldown.curopt);
				}
			    }
			}
			mi->d.pulldown.cursel = 0;
		    }
		    break;
		case MI_TYPE_LISTBOX:
		    if (mi->flags&MenuButtonClicked) {
			redraw_flag = 1;
			mi->flags &= ~MenuButtonClicked;
			if (MenuButtonUnderCursor == i) {
			    MenuButtonUnderCursor = -1;
			    if (mi->d.listbox.dohandler && mi->d.listbox.handler) {
				(*mi->d.listbox.handler)();
			    }
			}
		    }
		    break;
		case MI_TYPE_INPUT:
		    if (mi->flags&MenuButtonClicked) {
			redraw_flag = 1;
			mi->flags &= ~MenuButtonClicked;
			// MAYBE ADD HERE
		    }
		    break;
		case MI_TYPE_VSLIDER:
		    if (mi->flags&MenuButtonClicked) {
			redraw_flag = 1;
			mi->flags &= ~MenuButtonClicked;
			mi->d.vslider.cflags = 0;
		    }
		    break;
		case MI_TYPE_HSLIDER:
		    if (mi->flags&MenuButtonClicked) {
			redraw_flag = 1;
			mi->flags &= ~MenuButtonClicked;
			mi->d.hslider.cflags = 0;
		    }
		    break;
		default:
		    break;
	    }
	}
    }
    if (redraw_flag) {
	MustRedraw |= RedrawMenu;

	MenuHandleMouseMove(CursorX,CursorY);
    }
}

/**
**	End process menu
**
*/
global void EndMenu(void)
{
    CursorOn = CursorOnUnknown;
    CurrentMenu = NULL;

    MustRedraw = RedrawEverything;
    InterfaceState = IfaceStateNormal;
    UpdateDisplay();
    InterfaceState = IfaceStateMenu;
    MustRedraw = RedrawMenu;
}

/**
**	Process a menu.
**
**	@param menu_id	The menu number to process
**	@param loop	Indicates to setup handlers and really 'Process'
**
**	@todo FIXME: This function is called from the event handler!!
*/
global void ProcessMenu(const char *menu_id, int loop)
{
    int i, oldncr;
    Menuitem *mi;
    Menu *menu, *CurrentMenuSave = NULL;
    int MenuButtonUnderCursorSave = -1;
    int MenuButtonCurSelSave = -1;

    CancelBuildingMode();

    // Recursion protection:
    if (loop) {
	CurrentMenuSave = CurrentMenu;
	MenuButtonUnderCursorSave = MenuButtonUnderCursor;
	MenuButtonCurSelSave = MenuButtonCurSel;
    }

    InterfaceState = IfaceStateMenu;
    VideoLockScreen();
    HideAnyCursor();
    VideoUnlockScreen();
    DestroyCursorBackground();
    MustRedraw |= RedrawCursor;
    CursorState = CursorStatePoint;
    GameCursor = TheUI.Point.Cursor;
    menu = FindMenu(menu_id);
    if (menu == NULL) {
	return;
    }
    CurrentMenu = menu;
    MenuButtonCurSel = -1;
    for (i = 0; i < menu->nitems; ++i) {
	mi = menu->items + i;
	switch (mi->mitype) {
	    case MI_TYPE_BUTTON:
	    case MI_TYPE_PULLDOWN:
	    case MI_TYPE_LISTBOX:
	    case MI_TYPE_VSLIDER:
	    case MI_TYPE_HSLIDER:
	    case MI_TYPE_INPUT:
		mi->flags &= ~(MenuButtonClicked|MenuButtonActive
				|MenuButtonSelected);
		if (i == menu->defsel) {
		    mi->flags |= MenuButtonSelected;
		    MenuButtonCurSel = i;
		}
		break;
	}
	switch (mi->mitype) {
	    case MI_TYPE_PULLDOWN:
		mi->d.pulldown.cursel = 0;
		if (mi->d.pulldown.defopt != -1)
		    mi->d.pulldown.curopt = mi->d.pulldown.defopt;
		break;
	    case MI_TYPE_LISTBOX:
		mi->d.listbox.cursel = -1;
		mi->d.listbox.startline = 0;
		if (mi->d.listbox.defopt != -1)
		    mi->d.listbox.curopt = mi->d.listbox.defopt;
		break;
	    case MI_TYPE_VSLIDER:
		mi->d.vslider.cflags = 0;
		if (mi->d.vslider.defper != -1)
		    mi->d.vslider.percent = mi->d.vslider.defper;
		break;
	    case MI_TYPE_HSLIDER:
		mi->d.hslider.cflags = 0;
		if (mi->d.hslider.defper != -1)
		    mi->d.hslider.percent = mi->d.hslider.defper;
		break;
	    default:
		break;
	}
	if (mi->initfunc) {
	    (*mi->initfunc)(mi);
	}
    }
    MenuButtonUnderCursor = -1;
    if (loop) {
	SetVideoSync();
	MustRedraw = 0;
	MenuHandleMouseMove(CursorX,CursorY);	// This activates buttons as appropriate!
	MustRedraw |= RedrawCursor;
    }

    VideoLockScreen();
    DrawMenu(CurrentMenu);
    MustRedraw&=~RedrawMenu;
    VideoUnlockScreen();
    InvalidateAreaAndCheckCursor(MenuRedrawX,MenuRedrawY,MenuRedrawW,MenuRedrawH);

    if (loop) {
	while (CurrentMenu != NULL) {
	    DebugLevel3("MustRedraw: 0x%08x\n" _C_ MustRedraw);
	    if (MustRedraw) {
		UpdateDisplay();
	    }
	    RealizeVideoMemory();
	    oldncr = NetConnectRunning;
	    WaitEventsOneFrame(&callbacks);
	    if (NetConnectRunning == 2) {
		NetworkProcessClientRequest();
		MustRedraw |= RedrawMenu;
	    }
	    if (NetConnectRunning == 1) {
		NetworkProcessServerRequest();
	    }
	    // stopped by network activity?
	    if (oldncr == 2 && NetConnectRunning == 0) {
		if (menu->netaction) {
		    (*menu->netaction)();
		}
	    }
	}
    }

    for (i = 0; i < menu->nitems; ++i) {
	mi = menu->items + i;
	if (mi->exitfunc) {
	    (*mi->exitfunc)(mi);		// action/destructor
	}
    }

    if (loop) {
	CurrentMenu = CurrentMenuSave;
	MenuButtonUnderCursor = MenuButtonUnderCursorSave;
	MenuButtonCurSel = MenuButtonCurSelSave;
    }

    // FIXME: Johns good point?
    if (Menusbgnd) {
	VideoFree(Menusbgnd);
	Menusbgnd = NULL;
    }
}

/**
**	Init Menus for a specific race
**
**	@param race	The Race to set-up for
*/
global void InitMenus(unsigned int race)
{
    static int last_race = -1;
    const char *file;
    char *buf;

    if (race == last_race) {	// same race? already loaded!
	return;
    }

    if (last_race == -1) {
	InitMenuData();

	callbacks.ButtonPressed = &MenuHandleButtonDown;
	callbacks.ButtonReleased = &MenuHandleButtonUp;
	callbacks.MouseMoved = &MenuHandleMouseMove;
	callbacks.MouseExit = &HandleMouseExit;
	callbacks.KeyPressed = &MenuHandleKeyDown;
	callbacks.KeyReleased = &MenuHandleKeyUp;
	callbacks.KeyRepeated = &MenuHandleKeyRepeat;
	callbacks.NetworkEvent = NetworkEvent;
	callbacks.SoundReady = WriteSound;
    } else {
	// free previous sprites for different race
	VideoFree(MenuButtonGfx.Sprite);
    }
    last_race = race;
    file = MenuButtonGfx.File[race];
    buf = alloca(strlen(file) + 9 + 1);
    file = strcat(strcpy(buf, "graphics/"), file);
    MenuButtonGfx.Sprite = LoadSprite(file, 300, 144);	// 50/53 images!

    InitMenuFunctions();
    CurrentMenu = NULL;
}

#if 0
/**
**	Exit Menus code (freeing data)
**
**	// FIXME: NOT CALLED YET.....!!!!!
*/
global void ExitMenus(void)
{
    if (Menusbgnd) {
	VideoFree(Menusbgnd);
	Menusbgnd = NULL;
    }
}
#endif
