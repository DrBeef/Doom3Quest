//
// Created by simon on 04/03/2020.
//

#ifndef DOOMKEYS_H
#define DOOMKEYS_H

typedef enum {

    // DG: please don't change any existing constants for keyboard keys below (or recreate the tables in win_input.cpp)!

    K_TAB = 9,
    K_ENTER = 13,
    K_ESCAPE = 27,
    K_SPACE = 32,

    K_BACKSPACE = 127,

    K_COMMAND = 128,
    K_CAPSLOCK,
    K_SCROLL,
    K_POWER,
    K_PAUSE,

    K_UPARROW = 133,
    K_DOWNARROW,
    K_LEFTARROW,
    K_RIGHTARROW,

    // The 3 windows keys
            K_LWIN = 137,
    K_RWIN,
    K_MENU,

    K_ALT = 140,
    K_CTRL,
    K_SHIFT,
    K_INS,
    K_DEL,
    K_PGDN,
    K_PGUP,
    K_HOME,
    K_END,

    K_F1 = 149,
    K_F2,
    K_F3,
    K_F4,
    K_F5,
    K_F6,
    K_F7,
    K_F8,
    K_F9,
    K_F10,
    K_F11,
    K_F12,
    K_INVERTED_EXCLAMATION = 161,	// upside down !
    K_F13,
    K_F14,
    K_F15,

    K_KP_HOME = 165,
    K_KP_UPARROW,
    K_KP_PGUP,
    K_KP_LEFTARROW,
    K_KP_5,
    K_KP_RIGHTARROW,
    K_KP_END,
    K_KP_DOWNARROW,
    K_KP_PGDN,
    K_KP_ENTER,
    K_KP_INS,
    K_KP_DEL,
    K_KP_SLASH,
    K_SUPERSCRIPT_TWO = 178,		// superscript 2
    K_KP_MINUS,
    K_ACUTE_ACCENT = 180,			// accute accent
    K_KP_PLUS,
    K_KP_NUMLOCK,
    K_KP_STAR,
    K_KP_EQUALS,

    // DG: please don't change any existing constants above this one (or recreate the tables in win_input.cpp)!

    K_MASCULINE_ORDINATOR = 186,
    // K_MOUSE enums must be contiguous (no char codes in the middle)
            K_MOUSE1 = 187,
    K_MOUSE2,
    K_MOUSE3,
    K_MOUSE4,
    K_MOUSE5,
    K_MOUSE6,
    K_MOUSE7,
    K_MOUSE8,

    K_MWHEELDOWN = 195,
    K_MWHEELUP,

    K_JOY1 = 197,
    K_JOY2,
    K_JOY3,
    K_JOY4,
    K_JOY5,
    K_JOY6,
    K_JOY7,
    K_JOY8,
    K_JOY9,
    K_JOY10,
    K_JOY11,
    K_JOY12,
    K_JOY13,
    K_JOY14,
    K_JOY15,
    K_JOY16,
    K_JOY17,
    K_JOY18,
    K_JOY19,
    K_JOY20,
    K_JOY21,
    K_JOY22,
    K_JOY23,
    K_JOY24,
    K_JOY25,
    K_JOY26,
    K_JOY27,
    K_GRAVE_A = 224,	// lowercase a with grave accent
    K_JOY28,
    K_JOY29,
    K_JOY30,
    K_JOY31,
    K_JOY32,

    K_AUX1 = 230,
    K_CEDILLA_C = 231,	// lowercase c with Cedilla
    K_GRAVE_E = 232,	// lowercase e with grave accent
    K_AUX2,
    K_AUX3,
    K_AUX4,
    K_GRAVE_I = 236,	// lowercase i with grave accent
    K_AUX5,
    K_AUX6,
    K_AUX7,
    K_AUX8,
    K_TILDE_N = 241,	// lowercase n with tilde
    K_GRAVE_O = 242,	// lowercase o with grave accent
    K_AUX9,
    K_AUX10,
    K_AUX11,
    K_AUX12,
    K_AUX13,
    K_AUX14,
    K_GRAVE_U = 249,	// lowercase u with grave accent
    K_AUX15,
    K_AUX16,

    K_PRINT_SCR	= 252,	// SysRq / PrintScr
    K_RIGHT_ALT = 253,	// used by some languages as "Alt-Gr"
    K_LAST_KEY  = 254	// this better be < 256!
} keyNum_t;


#endif //DOOMKEYS_H
