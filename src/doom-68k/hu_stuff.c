// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:  Heads-up displays
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: hu_stuff.c,v 1.4 1997/02/03 16:47:52 b1 Exp $";


#include "doomdef.h"

#include "z_zone.h"

#include "m_swap.h"

#include "hu_stuff.h"
#include "hu_lib.h"
#include "w_wad.h"

#include "s_sound.h"

#include "doomstat.h"

// Data.
#include "dstrings.h"
#include "sounds.h"

//
// Locally used constants, shortcuts.
//
#define HU_TITLE	(mapnames[(gameepisode-1)*9+gamemap-1])
#define HU_TITLEHEIGHT	1
#define HU_TITLEX	0
#define HU_TITLEY	(167 - SHORT(hu_font[0]->height))

#define HU_INPUTTOGGLE	't'
#define HU_INPUTX	HU_MSGX
#define HU_INPUTY	(HU_MSGY + HU_MSGHEIGHT*(SHORT(hu_font[0]->height) +1))
#define HU_INPUTWIDTH	64
#define HU_INPUTHEIGHT	1



char* chat_macros[] = {
    HUSTR_CHATMACRO0,
    HUSTR_CHATMACRO1,
    HUSTR_CHATMACRO2,
    HUSTR_CHATMACRO3,
    HUSTR_CHATMACRO4,
    HUSTR_CHATMACRO5,
    HUSTR_CHATMACRO6,
    HUSTR_CHATMACRO7,
    HUSTR_CHATMACRO8,
    HUSTR_CHATMACRO9
};

char* player_names[] = {
    HUSTR_PLRGREEN,
    HUSTR_PLRINDIGO,
    HUSTR_PLRBROWN,
    HUSTR_PLRRED
};


char chat_char; // remove later.
static player_t* plr;
patch_t* hu_font[HU_FONTSIZE];
static hu_textline_t w_title;
doomboolean chat_on;
static hu_itext_t w_chat;
static doomboolean always_off = false;
static char chat_dest[MAXPLAYERS];
static hu_itext_t w_inputbuffer[MAXPLAYERS];

static doomboolean message_on;
doomboolean message_dontfuckwithme;
static doomboolean message_nottobefuckedwith;

static hu_stext_t w_message;
static int message_counter;

extern int showMessages;
extern doomboolean automapactive;

static doomboolean headsupactive = false;

//
// Builtin map names.
// The actual names can be found in DStrings.h.
//

char* mapnames[] = // DOOM shareware/registered/retail (Ultimate) names.
{

    HUSTR_E1M1,
    HUSTR_E1M2,
    HUSTR_E1M3,
    HUSTR_E1M4,
    HUSTR_E1M5,
    HUSTR_E1M6,
    HUSTR_E1M7,
    HUSTR_E1M8,
    HUSTR_E1M9,

    HUSTR_E2M1,
    HUSTR_E2M2,
    HUSTR_E2M3,
    HUSTR_E2M4,
    HUSTR_E2M5,
    HUSTR_E2M6,
    HUSTR_E2M7,
    HUSTR_E2M8,
    HUSTR_E2M9,

    HUSTR_E3M1,
    HUSTR_E3M2,
    HUSTR_E3M3,
    HUSTR_E3M4,
    HUSTR_E3M5,
    HUSTR_E3M6,
    HUSTR_E3M7,
    HUSTR_E3M8,
    HUSTR_E3M9,

    HUSTR_E4M1,
    HUSTR_E4M2,
    HUSTR_E4M3,
    HUSTR_E4M4,
    HUSTR_E4M5,
    HUSTR_E4M6,
    HUSTR_E4M7,
    HUSTR_E4M8,
    HUSTR_E4M9,

    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL"
};


const char* shiftxform;

const char english_shiftxform[] = {

    0,
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    31,
    ' ', '!', '"', '#', '$', '%', '&',
    '"', // shift-'
    '(', ')', '*', '+',
    '<', // shift-,
    '_', // shift--
    '>', // shift-.
    '?', // shift-/
    ')', // shift-0
    '!', // shift-1
    '@', // shift-2
    '#', // shift-3
    '$', // shift-4
    '%', // shift-5
    '^', // shift-6
    '&', // shift-7
    '*', // shift-8
    '(', // shift-9
    ':',
    ':', // shift-;
    '<',
    '+', // shift-=
    '>', '?', '@',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '[', // shift-[
    '!', // shift-backslash - OH MY GOD DOES WATCOM SUCK
    ']', // shift-]
    '"', '_',
    '\'', // shift-`
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '{', '|', '}', '~', 127
};

void HU_Init(void) {

    int i;
    int j;
    char buffer[9];

    /*
    sys_sprintf(buffer, "STCFN%.3d", 8);
    gConsPrint("fontax: ");
    gAppendString(buffer);*/

    shiftxform = english_shiftxform;

    // load the heads-up font
    j = HU_FONTSTART;
    for (i = 0; i < HU_FONTSIZE; i++) {
        //sys_printf("x: \n");
        std_sprintf(buffer, "STCFN%.3d", j++);
        //buffer[15] = 0;
        //sys_printf("y:%s \n", buffer);
        hu_font[i] = (patch_t *) W_CacheLumpName(buffer, PU_STATIC);
    }

}

void HU_Stop(void) {
    headsupactive = false;
}

void HU_Start(void) {

    int i;
    char* s;

    if (headsupactive)
        HU_Stop();

    plr = &players[consoleplayer];
    message_on = false;
    message_dontfuckwithme = false;
    message_nottobefuckedwith = false;
    chat_on = false;

    // create the message widget
    HUlib_initSText(&w_message,
            HU_MSGX, HU_MSGY, HU_MSGHEIGHT,
            hu_font,
            HU_FONTSTART, &message_on);

    // create the map title widget
    HUlib_initTextLine(&w_title,
            HU_TITLEX, HU_TITLEY,
            hu_font,
            HU_FONTSTART);

    s = HU_TITLE;

    while (*s)
        HUlib_addCharToTextLine(&w_title, *(s++));

    // create the chat widget
    HUlib_initIText(&w_chat,
            HU_INPUTX, HU_INPUTY,
            hu_font,
            HU_FONTSTART, &chat_on);

    // create the inputbuffer widgets
    for (i = 0; i < MAXPLAYERS; i++)
        HUlib_initIText(&w_inputbuffer[i], 0, 0, 0, 0, &always_off);

    headsupactive = true;

}

void HU_Drawer(void) {

    HUlib_drawSText(&w_message);
    HUlib_drawIText(&w_chat);
    if (automapactive)
        HUlib_drawTextLine(&w_title, false);

}

void HU_Erase(void) {

    HUlib_eraseSText(&w_message);
    HUlib_eraseIText(&w_chat);
    HUlib_eraseTextLine(&w_title);

}

void HU_Ticker(void) {

    int i, rc;
    char c;

    // tick down message counter if message is up
    if (message_counter && !--message_counter) {
        message_on = false;
        message_nottobefuckedwith = false;
    }

    if (showMessages || message_dontfuckwithme) {

        // display message if necessary
        if ((plr->message && !message_nottobefuckedwith)
                || (plr->message && message_dontfuckwithme)) {
            HUlib_addMessageToSText(&w_message, 0, plr->message);
            plr->message = 0;
            message_on = true;
            message_counter = HU_MSGTIMEOUT;
            message_nottobefuckedwith = message_dontfuckwithme;
            message_dontfuckwithme = 0;
        }

    } // else message_on = false;

}

#define QUEUESIZE		128

static char chatchars[QUEUESIZE];
static int head = 0;
static int tail = 0;

void HU_queueChatChar(char c) {
    if (((head + 1) & (QUEUESIZE - 1)) == tail) {
        plr->message = HUSTR_MSGU;
    } else {
        chatchars[head] = c;
        head = (head + 1) & (QUEUESIZE - 1);
    }
}

char HU_dequeueChatChar(void) {
    char c;

    if (head != tail) {
        c = chatchars[tail];
        tail = (tail + 1) & (QUEUESIZE - 1);
    } else {
        c = 0;
    }

    return c;
}

doomboolean HU_Responder(event_t *ev) {

    static char lastmessage[HU_MAXLINELENGTH + 1];
    char* macromessage;
    doomboolean eatkey = false;
    static doomboolean shiftdown = false;
    static doomboolean altdown = false;
    unsigned char c;
    int i;
    int numplayers;

    static char destination_keys[MAXPLAYERS] = {
        HUSTR_KEYGREEN,
        HUSTR_KEYINDIGO,
        HUSTR_KEYBROWN,
        HUSTR_KEYRED
    };

    static int num_nobrainers = 0;

    numplayers = 0;
    for (i = 0; i < MAXPLAYERS; i++)
        numplayers += playeringame[i];

    if (ev->data1 == KEY_RSHIFT) {
        shiftdown = ev->type == ev_keydown;
        return false;
    } else if (ev->data1 == KEY_RALT || ev->data1 == KEY_LALT) {
        altdown = ev->type == ev_keydown;
        return false;
    }

    if (ev->type != ev_keydown)
        return false;

    if (!chat_on && ev->data1 == HU_MSGREFRESH)
    {
        message_on = true;
        message_counter = HU_MSGTIMEOUT;
        eatkey = true;
    }
	else
    {
        c = ev->data1;
        // send a macro
        if (altdown) {
            c = c - '0';
            if (c > 9)
                return false;
            // fprintf(stderr, "got here\n");
            macromessage = chat_macros[c];

            // kill last message with a '\n'
            HU_queueChatChar(KEY_ENTER); // DEBUG!!!

            // send the macro message
            while (*macromessage)
                HU_queueChatChar(*macromessage++);
            HU_queueChatChar(KEY_ENTER);

            // leave chat mode and notify that it was sent
            chat_on = false;
            std_strcpy(lastmessage, chat_macros[c]);
            plr->message = lastmessage;
            eatkey = true;
        } else {
            if (shiftdown || (c >= 'a' && c <= 'z'))
                c = shiftxform[c];
            eatkey = HUlib_keyInIText(&w_chat, c);
            if (eatkey) {
                // static unsigned char buf[20]; // DEBUG
                HU_queueChatChar(c);

                // sprintf(buf, "KEY: %d => %d", ev->data1, c);
                //      plr->message = buf;
            }
            if (c == KEY_ENTER) {
                chat_on = false;
                if (w_chat.l.len) {
                    std_strcpy(lastmessage, w_chat.l.l);
                    plr->message = lastmessage;
                }
            } else if (c == KEY_ESCAPE)
                chat_on = false;
        }
    }

    return eatkey;

}
