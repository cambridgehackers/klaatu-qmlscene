#include "event_thread.h"
#include <EventHub.h>
#if defined(SHORT_PLATFORM_VERSION) && (SHORT_PLATFORM_VERSION < 41)
#include <ui/KeycodeLabels.h>
#else
#include <androidfw/KeycodeLabels.h>
#endif
#include <android/keycodes.h>
#include "screencontrol.h"
#include "sys/ioctl.h"
#include "linux/fb.h"
#include "klaatuapplication.h"
#include "cursorsignal.h"

#include <QDebug>
#if defined(SHORT_PLATFORM_VERSION) && (SHORT_PLATFORM_VERSION == 40)
#define CODE_FIELD scanCode
#else
#define CODE_FIELD code
#endif

#define DEBUG_INBOUND_EVENT_DETAILS 0

using namespace android;

static int displayWidth;
static int displayHeight;

const int keyMap[] =
{
    Qt::Key_unknown,              // AKEYCODE_UNKNOWN         = 0,
    Qt::Key_unknown,              // AKEYCODE_SOFT_LEFT       = 1,
    Qt::Key_unknown,              // AKEYCODE_SOFT_RIGHT      = 2,
    Qt::Key_Home,                 // AKEYCODE_HOME            = 3,
    Qt::Key_Back,                 // AKEYCODE_BACK            = 4,
    Qt::Key_Call,                 // AKEYCODE_CALL            = 5,
    Qt::Key_Hangup,               // AKEYCODE_ENDCALL         = 6,
    Qt::Key_0,                    // AKEYCODE_0               = 7,
    Qt::Key_1,                    // AKEYCODE_1               = 8,
    Qt::Key_2,                    // AKEYCODE_2               = 9,
    Qt::Key_3,                    // AKEYCODE_3               = 10,
    Qt::Key_4,                    // AKEYCODE_4               = 11,
    Qt::Key_5,                    // AKEYCODE_5               = 12,
    Qt::Key_6,                    // AKEYCODE_6               = 13,
    Qt::Key_7,                    // AKEYCODE_7               = 14,
    Qt::Key_8,                    // AKEYCODE_8               = 15,
    Qt::Key_9,                    // AKEYCODE_9               = 16,
    Qt::Key_Asterisk,             // AKEYCODE_STAR            = 17,
    Qt::Key_NumberSign,           // AKEYCODE_POUND           = 18,
    Qt::Key_Up,                   // AKEYCODE_DPAD_UP         = 19,
    Qt::Key_Down,                 // AKEYCODE_DPAD_DOWN       = 20,
    Qt::Key_Left,                 // AKEYCODE_DPAD_LEFT       = 21,
    Qt::Key_Right,                // AKEYCODE_DPAD_RIGHT      = 22,
    Qt::Key_unknown,              // AKEYCODE_DPAD_CENTER     = 23,
    Qt::Key_VolumeUp,             // AKEYCODE_VOLUME_UP       = 24,
    Qt::Key_VolumeDown,           // AKEYCODE_VOLUME_DOWN     = 25,
    Qt::Key_PowerOff,             // AKEYCODE_POWER           = 26,
    Qt::Key_Camera,               // AKEYCODE_CAMERA          = 27,
    Qt::Key_Clear,                // AKEYCODE_CLEAR           = 28,
    Qt::Key_A,                    // AKEYCODE_A               = 29,
    Qt::Key_B,                    // AKEYCODE_B               = 30,
    Qt::Key_C,                    // AKEYCODE_C               = 31,
    Qt::Key_D,                    // AKEYCODE_D               = 32,
    Qt::Key_E,                    // AKEYCODE_E               = 33,
    Qt::Key_F,                    // AKEYCODE_F               = 34,
    Qt::Key_G,                    // AKEYCODE_G               = 35,
    Qt::Key_H,                    // AKEYCODE_H               = 36,
    Qt::Key_I,                    // AKEYCODE_I               = 37,
    Qt::Key_J,                    // AKEYCODE_J               = 38,
    Qt::Key_K,                    // AKEYCODE_K               = 39,
    Qt::Key_L,                    // AKEYCODE_L               = 40,
    Qt::Key_M,                    // AKEYCODE_M               = 41,
    Qt::Key_N,                    // AKEYCODE_N               = 42,
    Qt::Key_O,                    // AKEYCODE_O               = 43,
    Qt::Key_P,                    // AKEYCODE_P               = 44,
    Qt::Key_Q,                    // AKEYCODE_Q               = 45,
    Qt::Key_R,                    // AKEYCODE_R               = 46,
    Qt::Key_S,                    // AKEYCODE_S               = 47,
    Qt::Key_T,                    // AKEYCODE_T               = 48,
    Qt::Key_U,                    // AKEYCODE_U               = 49,
    Qt::Key_V,                    // AKEYCODE_V               = 50,
    Qt::Key_W,                    // AKEYCODE_W               = 51,
    Qt::Key_X,                    // AKEYCODE_X               = 52,
    Qt::Key_Y,                    // AKEYCODE_Y               = 53,
    Qt::Key_Z,                    // AKEYCODE_Z               = 54,
    Qt::Key_Comma,                // AKEYCODE_COMMA           = 55,
    Qt::Key_Period,               // AKEYCODE_PERIOD          = 56,
    Qt::Key_Alt,                  // AKEYCODE_ALT_LEFT        = 57,
    Qt::Key_Alt,                  // AKEYCODE_ALT_RIGHT       = 58,
    Qt::Key_Shift,                // AKEYCODE_SHIFT_LEFT      = 59,
    Qt::Key_Shift,                // AKEYCODE_SHIFT_RIGHT     = 60,
    Qt::Key_Tab,                  // AKEYCODE_TAB             = 61,
    Qt::Key_Space,                // AKEYCODE_SPACE           = 62,
    Qt::Key_unknown,              // AKEYCODE_SYM             = 63,
    Qt::Key_Explorer,             // AKEYCODE_EXPLORER        = 64,
    Qt::Key_unknown,              // AKEYCODE_ENVELOPE        = 65,
    Qt::Key_Enter,                // AKEYCODE_ENTER           = 66,
    Qt::Key_Backspace,            // AKEYCODE_DEL             = 67,
    Qt::Key_QuoteLeft,            // AKEYCODE_GRAVE           = 68,
    Qt::Key_Minus,                // AKEYCODE_MINUS           = 69,
    Qt::Key_Equal,                // AKEYCODE_EQUALS          = 70,
    Qt::Key_BracketLeft,          // AKEYCODE_LEFT_BRACKET    = 71,
    Qt::Key_BracketRight,         // AKEYCODE_RIGHT_BRACKET   = 72,
    Qt::Key_Backslash,            // AKEYCODE_BACKSLASH       = 73,
    Qt::Key_Semicolon,            // AKEYCODE_SEMICOLON       = 74,
    Qt::Key_Apostrophe,           // AKEYCODE_APOSTROPHE      = 75,
    Qt::Key_Slash,                // AKEYCODE_SLASH           = 76,
    Qt::Key_At,                   // AKEYCODE_AT              = 77,
    Qt::Key_NumLock,              // AKEYCODE_NUM             = 78,
    Qt::Key_unknown,              // AKEYCODE_HEADSETHOOK     = 79,
    Qt::Key_CameraFocus,          // AKEYCODE_FOCUS           = 80,
    Qt::Key_Plus,                 // AKEYCODE_PLUS            = 81,
    Qt::Key_Menu,                 // AKEYCODE_MENU            = 82,
    Qt::Key_unknown,              // AKEYCODE_NOTIFICATION    = 83,
    Qt::Key_Search,               // AKEYCODE_SEARCH          = 84,
    Qt::Key_MediaTogglePlayPause, // AKEYCODE_MEDIA_PLAY_PAUSE= 85,
    Qt::Key_MediaStop,            // AKEYCODE_MEDIA_STOP      = 86,
    Qt::Key_MediaNext,            // AKEYCODE_MEDIA_NEXT      = 87,
    Qt::Key_MediaPrevious,        // AKEYCODE_MEDIA_PREVIOUS  = 88,
    Qt::Key_AudioRewind,          // AKEYCODE_MEDIA_REWIND    = 89,
    Qt::Key_AudioForward,         // AKEYCODE_MEDIA_FAST_FORWARD = 90,
    Qt::Key_VolumeMute,           // AKEYCODE_MUTE            = 91,
    Qt::Key_PageUp,               // AKEYCODE_PAGE_UP         = 92,
    Qt::Key_PageDown,             // AKEYCODE_PAGE_DOWN       = 93,
    Qt::Key_Pictures,             // AKEYCODE_PICTSYMBOLS     = 94,
    Qt::Key_Mode_switch,          // AKEYCODE_SWITCH_CHARSET  = 95,
    Qt::Key_unknown,              // AKEYCODE_BUTTON_A        = 96,
    Qt::Key_unknown,              // AKEYCODE_BUTTON_B        = 97,
    Qt::Key_unknown,              // AKEYCODE_BUTTON_C        = 98,
    Qt::Key_unknown,              // AKEYCODE_BUTTON_X        = 99,
    Qt::Key_unknown,              // AKEYCODE_BUTTON_Y        = 100,
    Qt::Key_unknown,              // AKEYCODE_BUTTON_Z        = 101,
    Qt::Key_unknown,              // AKEYCODE_BUTTON_L1       = 102,
    Qt::Key_unknown,              // AKEYCODE_BUTTON_R1       = 103,
    Qt::Key_unknown,              // AKEYCODE_BUTTON_L2       = 104,
    Qt::Key_unknown,              // AKEYCODE_BUTTON_R2       = 105,
    Qt::Key_unknown,              // AKEYCODE_BUTTON_THUMBL   = 106,
    Qt::Key_unknown,              // AKEYCODE_BUTTON_THUMBR   = 107,
    Qt::Key_unknown,              // AKEYCODE_BUTTON_START    = 108,
    Qt::Key_Select,               // AKEYCODE_BUTTON_SELECT   = 109,
    Qt::Key_unknown,              // AKEYCODE_BUTTON_MODE     = 110,
    Qt::Key_Escape,               // AKEYCODE_ESCAPE          = 111,
    Qt::Key_Delete,               // AKEYCODE_FORWARD_DEL     = 112,
    Qt::Key_Control,              // AKEYCODE_CTRL_LEFT       = 113,
    Qt::Key_Control,              // AKEYCODE_CTRL_RIGHT      = 114,
    Qt::Key_CapsLock,             // AKEYCODE_CAPS_LOCK       = 115,
    Qt::Key_ScrollLock,           // AKEYCODE_SCROLL_LOCK     = 116,
    Qt::Key_Meta,                 // AKEYCODE_META_LEFT       = 117,
    Qt::Key_Meta,                 // AKEYCODE_META_RIGHT      = 118,
    Qt::Key_unknown,              // AKEYCODE_FUNCTION        = 119,
    Qt::Key_SysReq,               // AKEYCODE_SYSRQ           = 120,
    Qt::Key_unknown,              // AKEYCODE_BREAK           = 121,
    Qt::Key_Home,                 // AKEYCODE_MOVE_HOME       = 122,
    Qt::Key_End,                  // AKEYCODE_MOVE_END        = 123,
    Qt::Key_Insert,               // AKEYCODE_INSERT          = 124,
    Qt::Key_Forward,              // AKEYCODE_FORWARD         = 125,
    Qt::Key_MediaPlay,            // AKEYCODE_MEDIA_PLAY      = 126,
    Qt::Key_MediaPause,           // AKEYCODE_MEDIA_PAUSE     = 127,
    Qt::Key_MediaStop,            // AKEYCODE_MEDIA_CLOSE     = 128,
    Qt::Key_Eject,                // AKEYCODE_MEDIA_EJECT     = 129,
    Qt::Key_MediaRecord,          // AKEYCODE_MEDIA_RECORD    = 130,
    Qt::Key_F1,                   // AKEYCODE_F1              = 131,
    Qt::Key_F2,                   // AKEYCODE_F2              = 132,
    Qt::Key_F3,                   // AKEYCODE_F3              = 133,
    Qt::Key_F4,                   // AKEYCODE_F4              = 134,
    Qt::Key_F5,                   // AKEYCODE_F5              = 135,
    Qt::Key_F6,                   // AKEYCODE_F6              = 136,
    Qt::Key_F7,                   // AKEYCODE_F7              = 137,
    Qt::Key_F8,                   // AKEYCODE_F8              = 138,
    Qt::Key_F9,                   // AKEYCODE_F9              = 139,
    Qt::Key_F10,                  // AKEYCODE_F10             = 140,
    Qt::Key_F11,                  // AKEYCODE_F11             = 141,
    Qt::Key_F12,                  // AKEYCODE_F12             = 142,
    Qt::Key_NumLock,              // AKEYCODE_NUM_LOCK        = 143,
    Qt::Key_0,                    // AKEYCODE_NUMPAD_0        = 144,
    Qt::Key_1,                    // AKEYCODE_NUMPAD_1        = 145,
    Qt::Key_2,                    // AKEYCODE_NUMPAD_2        = 146,
    Qt::Key_3,                    // AKEYCODE_NUMPAD_3        = 147,
    Qt::Key_4,                    // AKEYCODE_NUMPAD_4        = 148,
    Qt::Key_5,                    // AKEYCODE_NUMPAD_5        = 149,
    Qt::Key_6,                    // AKEYCODE_NUMPAD_6        = 150,
    Qt::Key_7,                    // AKEYCODE_NUMPAD_7        = 151,
    Qt::Key_8,                    // AKEYCODE_NUMPAD_8        = 152,
    Qt::Key_9,                    // AKEYCODE_NUMPAD_9        = 153,
    Qt::Key_division,             // AKEYCODE_NUMPAD_DIVIDE   = 154,
    Qt::Key_multiply,             // AKEYCODE_NUMPAD_MULTIPLY = 155,
    Qt::Key_Minus,                // AKEYCODE_NUMPAD_SUBTRACT = 156,
    Qt::Key_Plus,                 // AKEYCODE_NUMPAD_ADD      = 157,
    Qt::Key_Period,               // AKEYCODE_NUMPAD_DOT      = 158,
    Qt::Key_Comma,                // AKEYCODE_NUMPAD_COMMA    = 159,
    Qt::Key_Enter,                // AKEYCODE_NUMPAD_ENTER    = 160,
    Qt::Key_Equal,                // AKEYCODE_NUMPAD_EQUALS   = 161,
    Qt::Key_ParenLeft,            // AKEYCODE_NUMPAD_LEFT_PAREN = 162,
    Qt::Key_ParenRight,           // AKEYCODE_NUMPAD_RIGHT_PAREN = 163,
    Qt::Key_VolumeMute,           // AKEYCODE_VOLUME_MUTE     = 164,
    Qt::Key_Help,                 // AKEYCODE_INFO            = 165,
    Qt::Key_unknown,              // AKEYCODE_CHANNEL_UP      = 166,
    Qt::Key_unknown,              // AKEYCODE_CHANNEL_DOWN    = 167,
    Qt::Key_ZoomIn,               // AKEYCODE_ZOOM_IN         = 168,
    Qt::Key_ZoomOut,              // AKEYCODE_ZOOM_OUT        = 169,
    Qt::Key_unknown,              // AKEYCODE_TV              = 170,
    Qt::Key_unknown,              // AKEYCODE_WINDOW          = 171,
    Qt::Key_unknown,              // AKEYCODE_GUIDE           = 172,
    Qt::Key_unknown,              // AKEYCODE_DVR             = 173,
    Qt::Key_unknown,              // AKEYCODE_BOOKMARK        = 174,
    Qt::Key_unknown,              // AKEYCODE_CAPTIONS        = 175,
    Qt::Key_unknown,              // AKEYCODE_SETTINGS        = 176,
    Qt::Key_unknown,              // AKEYCODE_TV_POWER        = 177,
    Qt::Key_unknown,              // AKEYCODE_TV_INPUT        = 178,
    Qt::Key_unknown,              // AKEYCODE_STB_POWER       = 179,
    Qt::Key_unknown,              // AKEYCODE_STB_INPUT       = 180,
    Qt::Key_unknown,              // AKEYCODE_AVR_POWER       = 181,
    Qt::Key_unknown,              // AKEYCODE_AVR_INPUT       = 182,
    Qt::Key_unknown,              // AKEYCODE_PROG_RED        = 183,
    Qt::Key_unknown,              // AKEYCODE_PROG_GREEN      = 184,
    Qt::Key_unknown,              // AKEYCODE_PROG_YELLOW     = 185,
    Qt::Key_unknown,              // AKEYCODE_PROG_BLUE       = 186,
    Qt::Key_LaunchMedia,          // AKEYCODE_APP_SWITCH      = 187,
    Qt::Key_Launch0,              // AKEYCODE_BUTTON_1        = 188,
    Qt::Key_Launch1,              // AKEYCODE_BUTTON_2        = 189,
    Qt::Key_Launch2,              // AKEYCODE_BUTTON_3        = 190,
    Qt::Key_Launch3,              // AKEYCODE_BUTTON_4        = 191,
    Qt::Key_Launch4,              // AKEYCODE_BUTTON_5        = 192,
    Qt::Key_Launch5,              // AKEYCODE_BUTTON_6        = 193,
    Qt::Key_Launch6,              // AKEYCODE_BUTTON_7        = 194,
    Qt::Key_Launch7,              // AKEYCODE_BUTTON_8        = 195,
    Qt::Key_Launch8,              // AKEYCODE_BUTTON_9        = 196,
    Qt::Key_Launch9,              // AKEYCODE_BUTTON_10       = 197,
    Qt::Key_LaunchA,              // AKEYCODE_BUTTON_11       = 198,
    Qt::Key_LaunchB,              // AKEYCODE_BUTTON_12       = 199,
    Qt::Key_LaunchC,              // AKEYCODE_BUTTON_13       = 200,
    Qt::Key_LaunchD,              // AKEYCODE_BUTTON_14       = 201,
    Qt::Key_LaunchE,              // AKEYCODE_BUTTON_15       = 202,
    Qt::Key_LaunchF,              // AKEYCODE_BUTTON_16       = 203,
    Qt::Key_unknown,              // AKEYCODE_LANGUAGE_SWITCH = 204,
    Qt::Key_unknown,              // AKEYCODE_MANNER_MODE     = 205,
    Qt::Key_unknown,              // AKEYCODE_3D_MODE         = 206,
    Qt::Key_unknown,              // AKEYCODE_CONTACTS        = 207,
    Qt::Key_Calendar,             // AKEYCODE_CALENDAR        = 208,
    Qt::Key_Music,                // AKEYCODE_MUSIC           = 209,
    Qt::Key_Calculator,           // AKEYCODE_CALCULATOR      = 210,
    Qt::Key_Zenkaku_Hankaku,      // AKEYCODE_ZENKAKU_HANKAKU = 211,
    Qt::Key_Eisu_toggle,          // AKEYCODE_EISU            = 212,
    Qt::Key_Muhenkan,             // AKEYCODE_MUHENKAN        = 213,
    Qt::Key_Henkan,               // AKEYCODE_HENKAN          = 214,
    Qt::Key_Hiragana_Katakana,    // AKEYCODE_KATAKANA_HIRAGANA = 215,
    Qt::Key_yen,                  // AKEYCODE_YEN             = 216,
    Qt::Key_unknown,              // AKEYCODE_RO              = 217,
    Qt::Key_Kana_Lock,            // AKEYCODE_KANA            = 218,
    Qt::Key_unknown               // AKEYCODE_ASSIST          = 219,
};

const char normalMap[] =
{
    0,            // AKEYCODE_UNKNOWN         = 0,
    0,            // AKEYCODE_SOFT_LEFT       = 1,
    0,            // AKEYCODE_SOFT_RIGHT      = 2,
    0,            // AKEYCODE_HOME            = 3,
    0,            // AKEYCODE_BACK            = 4,
    0,            // AKEYCODE_CALL            = 5,
    0,            // AKEYCODE_ENDCALL         = 6,
    '0',          // AKEYCODE_0               = 7,
    '1',          // AKEYCODE_1               = 8,
    '2',          // AKEYCODE_2               = 9,
    '3',          // AKEYCODE_3               = 10,
    '4',          // AKEYCODE_4               = 11,
    '5',          // AKEYCODE_5               = 12,
    '6',          // AKEYCODE_6               = 13,
    '7',          // AKEYCODE_7               = 14,
    '8',          // AKEYCODE_8               = 15,
    '9',          // AKEYCODE_9               = 16,
    '*',          // AKEYCODE_STAR            = 17,
    '#',          // AKEYCODE_POUND           = 18,
    0,            // AKEYCODE_DPAD_UP         = 19,
    0,            // AKEYCODE_DPAD_DOWN       = 20,
    0,            // AKEYCODE_DPAD_LEFT       = 21,
    0,            // AKEYCODE_DPAD_RIGHT      = 22,
    0,            // AKEYCODE_DPAD_CENTER     = 23,
    0,            // AKEYCODE_VOLUME_UP       = 24,
    0,            // AKEYCODE_VOLUME_DOWN     = 25,
    0,            // AKEYCODE_POWER           = 26,
    0,            // AKEYCODE_CAMERA          = 27,
    0,            // AKEYCODE_CLEAR           = 28,
    'a',          // AKEYCODE_A               = 29,
    'b',          // AKEYCODE_B               = 30,
    'c',          // AKEYCODE_C               = 31,
    'd',          // AKEYCODE_D               = 32,
    'e',          // AKEYCODE_E               = 33,
    'f',          // AKEYCODE_F               = 34,
    'g',          // AKEYCODE_G               = 35,
    'h',          // AKEYCODE_H               = 36,
    'i',          // AKEYCODE_I               = 37,
    'j',          // AKEYCODE_J               = 38,
    'k',          // AKEYCODE_K               = 39,
    'l',          // AKEYCODE_L               = 40,
    'm',          // AKEYCODE_M               = 41,
    'n',          // AKEYCODE_N               = 42,
    'o',          // AKEYCODE_O               = 43,
    'p',          // AKEYCODE_P               = 44,
    'q',          // AKEYCODE_Q               = 45,
    'r',          // AKEYCODE_R               = 46,
    's',          // AKEYCODE_S               = 47,
    't',          // AKEYCODE_T               = 48,
    'u',          // AKEYCODE_U               = 49,
    'v',          // AKEYCODE_V               = 50,
    'w',          // AKEYCODE_W               = 51,
    'x',          // AKEYCODE_X               = 52,
    'y',          // AKEYCODE_Y               = 53,
    'z',          // AKEYCODE_Z               = 54,
    ',',          // AKEYCODE_COMMA           = 55,
    '.',          // AKEYCODE_PERIOD          = 56,
    0,            // AKEYCODE_ALT_LEFT        = 57,
    0,            // AKEYCODE_ALT_RIGHT       = 58,
    0,            // AKEYCODE_SHIFT_LEFT      = 59,
    0,            // AKEYCODE_SHIFT_RIGHT     = 60,
    '\t',         // AKEYCODE_TAB             = 61,
    ' ',          // AKEYCODE_SPACE           = 62,
    0,            // AKEYCODE_SYM             = 63,
    0,            // AKEYCODE_EXPLORER        = 64,
    0,            // AKEYCODE_ENVELOPE        = 65,
    0,            // AKEYCODE_ENTER           = 66,
    0,            // AKEYCODE_DEL             = 67,
    '`',          // AKEYCODE_GRAVE           = 68,
    '-',          // AKEYCODE_MINUS           = 69,
    '=',          // AKEYCODE_EQUALS          = 70,
    '[',          // AKEYCODE_LEFT_BRACKET    = 71,
    ']',          // AKEYCODE_RIGHT_BRACKET   = 72,
    '\\',         // AKEYCODE_BACKSLASH       = 73,
    ';',          // AKEYCODE_SEMICOLON       = 74,
    '\'',         // AKEYCODE_APOSTROPHE      = 75,
    '/',          // AKEYCODE_SLASH           = 76,
    '@',          // AKEYCODE_AT              = 77,
    0,            // AKEYCODE_NUM             = 78,
    0,            // AKEYCODE_HEADSETHOOK     = 79,
    0,            // AKEYCODE_FOCUS           = 80,
    '+',          // AKEYCODE_PLUS            = 81,
    0,            // AKEYCODE_MENU            = 82,
    0,            // AKEYCODE_NOTIFICATION    = 83,
    0,            // AKEYCODE_SEARCH          = 84,
    0,            // AKEYCODE_MEDIA_PLAY_PAUSE= 85,
    0,            // AKEYCODE_MEDIA_STOP      = 86,
    0,            // AKEYCODE_MEDIA_NEXT      = 87,
    0,            // AKEYCODE_MEDIA_PREVIOUS  = 88,
    0,            // AKEYCODE_MEDIA_REWIND    = 89,
    0,            // AKEYCODE_MEDIA_FAST_FORWARD = 90,
    0,            // AKEYCODE_MUTE            = 91,
    0,            // AKEYCODE_PAGE_UP         = 92,
    0,            // AKEYCODE_PAGE_DOWN       = 93,
    0,            // AKEYCODE_PICTSYMBOLS     = 94,
    0,            // AKEYCODE_SWITCH_CHARSET  = 95,
    0,            // AKEYCODE_BUTTON_A        = 96,
    0,            // AKEYCODE_BUTTON_B        = 97,
    0,            // AKEYCODE_BUTTON_C        = 98,
    0,            // AKEYCODE_BUTTON_X        = 99,
    0,            // AKEYCODE_BUTTON_Y        = 100,
    0,            // AKEYCODE_BUTTON_Z        = 101,
    0,            // AKEYCODE_BUTTON_L1       = 102,
    0,            // AKEYCODE_BUTTON_R1       = 103,
    0,            // AKEYCODE_BUTTON_L2       = 104,
    0,            // AKEYCODE_BUTTON_R2       = 105,
    0,            // AKEYCODE_BUTTON_THUMBL   = 106,
    0,            // AKEYCODE_BUTTON_THUMBR   = 107,
    0,            // AKEYCODE_BUTTON_START    = 108,
    0,            // AKEYCODE_BUTTON_SELECT   = 109,
    0,            // AKEYCODE_BUTTON_MODE     = 110,
    0,            // AKEYCODE_ESCAPE          = 111,
    0,            // AKEYCODE_FORWARD_DEL     = 112,
    0,            // AKEYCODE_CTRL_LEFT       = 113,
    0,            // AKEYCODE_CTRL_RIGHT      = 114,
    0,            // AKEYCODE_CAPS_LOCK       = 115,
    0,            // AKEYCODE_SCROLL_LOCK     = 116,
    0,            // AKEYCODE_META_LEFT       = 117,
    0,            // AKEYCODE_META_RIGHT      = 118,
    0,            // AKEYCODE_FUNCTION        = 119,
    0,            // AKEYCODE_SYSRQ           = 120,
    0,            // AKEYCODE_BREAK           = 121,
    0,            // AKEYCODE_MOVE_HOME       = 122,
    0,            // AKEYCODE_MOVE_END        = 123,
    0,            // AKEYCODE_INSERT          = 124,
    0,            // AKEYCODE_FORWARD         = 125,
    0,            // AKEYCODE_MEDIA_PLAY      = 126,
    0,            // AKEYCODE_MEDIA_PAUSE     = 127,
    0,            // AKEYCODE_MEDIA_CLOSE     = 128,
    0,            // AKEYCODE_MEDIA_EJECT     = 129,
    0,            // AKEYCODE_MEDIA_RECORD    = 130,
    0,            // AKEYCODE_F1              = 131,
    0,            // AKEYCODE_F2              = 132,
    0,            // AKEYCODE_F3              = 133,
    0,            // AKEYCODE_F4              = 134,
    0,            // AKEYCODE_F5              = 135,
    0,            // AKEYCODE_F6              = 136,
    0,            // AKEYCODE_F7              = 137,
    0,            // AKEYCODE_F8              = 138,
    0,            // AKEYCODE_F9              = 139,
    0,            // AKEYCODE_F10             = 140,
    0,            // AKEYCODE_F11             = 141,
    0,            // AKEYCODE_F12             = 142,
    0,            // AKEYCODE_NUM_LOCK        = 143,
    '0',          // AKEYCODE_NUMPAD_0        = 144,
    '1',          // AKEYCODE_NUMPAD_1        = 145,
    '2',          // AKEYCODE_NUMPAD_2        = 146,
    '3',          // AKEYCODE_NUMPAD_3        = 147,
    '4',          // AKEYCODE_NUMPAD_4        = 148,
    '5',          // AKEYCODE_NUMPAD_5        = 149,
    '6',          // AKEYCODE_NUMPAD_6        = 150,
    '7',          // AKEYCODE_NUMPAD_7        = 151,
    '8',          // AKEYCODE_NUMPAD_8        = 152,
    '9',          // AKEYCODE_NUMPAD_9        = 153,
    '/',          // AKEYCODE_NUMPAD_DIVIDE   = 154,
    '*',          // AKEYCODE_NUMPAD_MULTIPLY = 155,
    '-',          // AKEYCODE_NUMPAD_SUBTRACT = 156,
    '+',          // AKEYCODE_NUMPAD_ADD      = 157,
    '.',          // AKEYCODE_NUMPAD_DOT      = 158,
    ',',          // AKEYCODE_NUMPAD_COMMA    = 159,
    0,            // AKEYCODE_NUMPAD_ENTER    = 160,
    '=',          // AKEYCODE_NUMPAD_EQUALS   = 161,
    '(',          // AKEYCODE_NUMPAD_LEFT_PAREN = 162,
    ')',          // AKEYCODE_NUMPAD_RIGHT_PAREN = 163,
    0,            // AKEYCODE_VOLUME_MUTE     = 164,
    0,            // AKEYCODE_INFO            = 165,
    0,            // AKEYCODE_CHANNEL_UP      = 166,
    0,            // AKEYCODE_CHANNEL_DOWN    = 167,
    0,            // AKEYCODE_ZOOM_IN         = 168,
    0,            // AKEYCODE_ZOOM_OUT        = 169,
    0,            // AKEYCODE_TV              = 170,
    0,            // AKEYCODE_WINDOW          = 171,
    0,            // AKEYCODE_GUIDE           = 172,
    0,            // AKEYCODE_DVR             = 173,
    0,            // AKEYCODE_BOOKMARK        = 174,
    0,            // AKEYCODE_CAPTIONS        = 175,
    0,            // AKEYCODE_SETTINGS        = 176,
    0,            // AKEYCODE_TV_POWER        = 177,
    0,            // AKEYCODE_TV_INPUT        = 178,
    0,            // AKEYCODE_STB_POWER       = 179,
    0,            // AKEYCODE_STB_INPUT       = 180,
    0,            // AKEYCODE_AVR_POWER       = 181,
    0,            // AKEYCODE_AVR_INPUT       = 182,
    0,            // AKEYCODE_PROG_RED        = 183,
    0,            // AKEYCODE_PROG_GREEN      = 184,
    0,            // AKEYCODE_PROG_YELLOW     = 185,
    0,            // AKEYCODE_PROG_BLUE       = 186,
    0,            // AKEYCODE_APP_SWITCH      = 187,
    0,            // AKEYCODE_BUTTON_1        = 188,
    0,            // AKEYCODE_BUTTON_2        = 189,
    0,            // AKEYCODE_BUTTON_3        = 190,
    0,            // AKEYCODE_BUTTON_4        = 191,
    0,            // AKEYCODE_BUTTON_5        = 192,
    0,            // AKEYCODE_BUTTON_6        = 193,
    0,            // AKEYCODE_BUTTON_7        = 194,
    0,            // AKEYCODE_BUTTON_8        = 195,
    0,            // AKEYCODE_BUTTON_9        = 196,
    0,            // AKEYCODE_BUTTON_10       = 197,
    0,            // AKEYCODE_BUTTON_11       = 198,
    0,            // AKEYCODE_BUTTON_12       = 199,
    0,            // AKEYCODE_BUTTON_13       = 200,
    0,            // AKEYCODE_BUTTON_14       = 201,
    0,            // AKEYCODE_BUTTON_15       = 202,
    0,            // AKEYCODE_BUTTON_16       = 203,
    0,            // AKEYCODE_LANGUAGE_SWITCH = 204,
    0,            // AKEYCODE_MANNER_MODE     = 205,
    0,            // AKEYCODE_3D_MODE         = 206,
    0,            // AKEYCODE_CONTACTS        = 207,
    0,            // AKEYCODE_CALENDAR        = 208,
    0,            // AKEYCODE_MUSIC           = 209,
    0,            // AKEYCODE_CALCULATOR      = 210,
    0,            // AKEYCODE_ZENKAKU_HANKAKU = 211,
    0,            // AKEYCODE_EISU            = 212,
    0,            // AKEYCODE_MUHENKAN        = 213,
    0,            // AKEYCODE_HENKAN          = 214,
    0,            // AKEYCODE_KATAKANA_HIRAGANA = 215,
    0,            // AKEYCODE_YEN             = 216,
    0,            // AKEYCODE_RO              = 217,
    0,            // AKEYCODE_KANA            = 218,
    0             // AKEYCODE_ASSIST          = 219,
};

const char shiftMap[] =
{
    0,            // AKEYCODE_UNKNOWN         = 0,
    0,            // AKEYCODE_SOFT_LEFT       = 1,
    0,            // AKEYCODE_SOFT_RIGHT      = 2,
    0,            // AKEYCODE_HOME            = 3,
    0,            // AKEYCODE_BACK            = 4,
    0,            // AKEYCODE_CALL            = 5,
    0,            // AKEYCODE_ENDCALL         = 6,
    ')',          // AKEYCODE_0               = 7,
    '!',          // AKEYCODE_1               = 8,
    '@',          // AKEYCODE_2               = 9,
    '#',          // AKEYCODE_3               = 10,
    '$',          // AKEYCODE_4               = 11,
    '%',          // AKEYCODE_5               = 12,
    '^',          // AKEYCODE_6               = 13,
    '&',          // AKEYCODE_7               = 14,
    '*',          // AKEYCODE_8               = 15,
    '(',          // AKEYCODE_9               = 16,
    '*',          // AKEYCODE_STAR            = 17,
    '#',          // AKEYCODE_POUND           = 18,
    0,            // AKEYCODE_DPAD_UP         = 19,
    0,            // AKEYCODE_DPAD_DOWN       = 20,
    0,            // AKEYCODE_DPAD_LEFT       = 21,
    0,            // AKEYCODE_DPAD_RIGHT      = 22,
    0,            // AKEYCODE_DPAD_CENTER     = 23,
    0,            // AKEYCODE_VOLUME_UP       = 24,
    0,            // AKEYCODE_VOLUME_DOWN     = 25,
    0,            // AKEYCODE_POWER           = 26,
    0,            // AKEYCODE_CAMERA          = 27,
    0,            // AKEYCODE_CLEAR           = 28,
    'A',          // AKEYCODE_A               = 29,
    'B',          // AKEYCODE_B               = 30,
    'C',          // AKEYCODE_C               = 31,
    'D',          // AKEYCODE_D               = 32,
    'E',          // AKEYCODE_E               = 33,
    'F',          // AKEYCODE_F               = 34,
    'G',          // AKEYCODE_G               = 35,
    'H',          // AKEYCODE_H               = 36,
    'I',          // AKEYCODE_I               = 37,
    'J',          // AKEYCODE_J               = 38,
    'K',          // AKEYCODE_K               = 39,
    'L',          // AKEYCODE_L               = 40,
    'M',          // AKEYCODE_M               = 41,
    'N',          // AKEYCODE_N               = 42,
    'O',          // AKEYCODE_O               = 43,
    'P',          // AKEYCODE_P               = 44,
    'Q',          // AKEYCODE_Q               = 45,
    'R',          // AKEYCODE_R               = 46,
    'S',          // AKEYCODE_S               = 47,
    'T',          // AKEYCODE_T               = 48,
    'U',          // AKEYCODE_U               = 49,
    'V',          // AKEYCODE_V               = 50,
    'W',          // AKEYCODE_W               = 51,
    'X',          // AKEYCODE_X               = 52,
    'Y',          // AKEYCODE_Y               = 53,
    'Z',          // AKEYCODE_Z               = 54,
    '<',          // AKEYCODE_COMMA           = 55,
    '>',          // AKEYCODE_PERIOD          = 56,
    0,            // AKEYCODE_ALT_LEFT        = 57,
    0,            // AKEYCODE_ALT_RIGHT       = 58,
    0,            // AKEYCODE_SHIFT_LEFT      = 59,
    0,            // AKEYCODE_SHIFT_RIGHT     = 60,
    '\t',         // AKEYCODE_TAB             = 61,
    ' ',          // AKEYCODE_SPACE           = 62,
    0,            // AKEYCODE_SYM             = 63,
    0,            // AKEYCODE_EXPLORER        = 64,
    0,            // AKEYCODE_ENVELOPE        = 65,
    0,            // AKEYCODE_ENTER           = 66,
    0,            // AKEYCODE_DEL             = 67,
    '~',          // AKEYCODE_GRAVE           = 68,
    '_',          // AKEYCODE_MINUS           = 69,
    '+',          // AKEYCODE_EQUALS          = 70,
    '{',          // AKEYCODE_LEFT_BRACKET    = 71,
    '}',          // AKEYCODE_RIGHT_BRACKET   = 72,
    '|',          // AKEYCODE_BACKSLASH       = 73,
    ':',          // AKEYCODE_SEMICOLON       = 74,
    '\"',         // AKEYCODE_APOSTROPHE      = 75,
    '?',          // AKEYCODE_SLASH           = 76,
    '@',          // AKEYCODE_AT              = 77,
    0,            // AKEYCODE_NUM             = 78,
    0,            // AKEYCODE_HEADSETHOOK     = 79,
    0,            // AKEYCODE_FOCUS           = 80,
    '+',          // AKEYCODE_PLUS            = 81,
    0,            // AKEYCODE_MENU            = 82,
    0,            // AKEYCODE_NOTIFICATION    = 83,
    0,            // AKEYCODE_SEARCH          = 84,
    0,            // AKEYCODE_MEDIA_PLAY_PAUSE= 85,
    0,            // AKEYCODE_MEDIA_STOP      = 86,
    0,            // AKEYCODE_MEDIA_NEXT      = 87,
    0,            // AKEYCODE_MEDIA_PREVIOUS  = 88,
    0,            // AKEYCODE_MEDIA_REWIND    = 89,
    0,            // AKEYCODE_MEDIA_FAST_FORWARD = 90,
    0,            // AKEYCODE_MUTE            = 91,
    0,            // AKEYCODE_PAGE_UP         = 92,
    0,            // AKEYCODE_PAGE_DOWN       = 93,
    0,            // AKEYCODE_PICTSYMBOLS     = 94,
    0,            // AKEYCODE_SWITCH_CHARSET  = 95,
    0,            // AKEYCODE_BUTTON_A        = 96,
    0,            // AKEYCODE_BUTTON_B        = 97,
    0,            // AKEYCODE_BUTTON_C        = 98,
    0,            // AKEYCODE_BUTTON_X        = 99,
    0,            // AKEYCODE_BUTTON_Y        = 100,
    0,            // AKEYCODE_BUTTON_Z        = 101,
    0,            // AKEYCODE_BUTTON_L1       = 102,
    0,            // AKEYCODE_BUTTON_R1       = 103,
    0,            // AKEYCODE_BUTTON_L2       = 104,
    0,            // AKEYCODE_BUTTON_R2       = 105,
    0,            // AKEYCODE_BUTTON_THUMBL   = 106,
    0,            // AKEYCODE_BUTTON_THUMBR   = 107,
    0,            // AKEYCODE_BUTTON_START    = 108,
    0,            // AKEYCODE_BUTTON_SELECT   = 109,
    0,            // AKEYCODE_BUTTON_MODE     = 110,
    0,            // AKEYCODE_ESCAPE          = 111,
    0,            // AKEYCODE_FORWARD_DEL     = 112,
    0,            // AKEYCODE_CTRL_LEFT       = 113,
    0,            // AKEYCODE_CTRL_RIGHT      = 114,
    0,            // AKEYCODE_CAPS_LOCK       = 115,
    0,            // AKEYCODE_SCROLL_LOCK     = 116,
    0,            // AKEYCODE_META_LEFT       = 117,
    0,            // AKEYCODE_META_RIGHT      = 118,
    0,            // AKEYCODE_FUNCTION        = 119,
    0,            // AKEYCODE_SYSRQ           = 120,
    0,            // AKEYCODE_BREAK           = 121,
    0,            // AKEYCODE_MOVE_HOME       = 122,
    0,            // AKEYCODE_MOVE_END        = 123,
    0,            // AKEYCODE_INSERT          = 124,
    0,            // AKEYCODE_FORWARD         = 125,
    0,            // AKEYCODE_MEDIA_PLAY      = 126,
    0,            // AKEYCODE_MEDIA_PAUSE     = 127,
    0,            // AKEYCODE_MEDIA_CLOSE     = 128,
    0,            // AKEYCODE_MEDIA_EJECT     = 129,
    0,            // AKEYCODE_MEDIA_RECORD    = 130,
    0,            // AKEYCODE_F1              = 131,
    0,            // AKEYCODE_F2              = 132,
    0,            // AKEYCODE_F3              = 133,
    0,            // AKEYCODE_F4              = 134,
    0,            // AKEYCODE_F5              = 135,
    0,            // AKEYCODE_F6              = 136,
    0,            // AKEYCODE_F7              = 137,
    0,            // AKEYCODE_F8              = 138,
    0,            // AKEYCODE_F9              = 139,
    0,            // AKEYCODE_F10             = 140,
    0,            // AKEYCODE_F11             = 141,
    0,            // AKEYCODE_F12             = 142,
    0,            // AKEYCODE_NUM_LOCK        = 143,
    '0',          // AKEYCODE_NUMPAD_0        = 144,
    '1',          // AKEYCODE_NUMPAD_1        = 145,
    '2',          // AKEYCODE_NUMPAD_2        = 146,
    '3',          // AKEYCODE_NUMPAD_3        = 147,
    '4',          // AKEYCODE_NUMPAD_4        = 148,
    '5',          // AKEYCODE_NUMPAD_5        = 149,
    '6',          // AKEYCODE_NUMPAD_6        = 150,
    '7',          // AKEYCODE_NUMPAD_7        = 151,
    '8',          // AKEYCODE_NUMPAD_8        = 152,
    '9',          // AKEYCODE_NUMPAD_9        = 153,
    '/',          // AKEYCODE_NUMPAD_DIVIDE   = 154,
    '*',          // AKEYCODE_NUMPAD_MULTIPLY = 155,
    '-',          // AKEYCODE_NUMPAD_SUBTRACT = 156,
    '+',          // AKEYCODE_NUMPAD_ADD      = 157,
    '.',          // AKEYCODE_NUMPAD_DOT      = 158,
    ',',          // AKEYCODE_NUMPAD_COMMA    = 159,
    0,            // AKEYCODE_NUMPAD_ENTER    = 160,
    '=',          // AKEYCODE_NUMPAD_EQUALS   = 161,
    '(',          // AKEYCODE_NUMPAD_LEFT_PAREN = 162,
    ')',          // AKEYCODE_NUMPAD_RIGHT_PAREN = 163,
    0,            // AKEYCODE_VOLUME_MUTE     = 164,
    0,            // AKEYCODE_INFO            = 165,
    0,            // AKEYCODE_CHANNEL_UP      = 166,
    0,            // AKEYCODE_CHANNEL_DOWN    = 167,
    0,            // AKEYCODE_ZOOM_IN         = 168,
    0,            // AKEYCODE_ZOOM_OUT        = 169,
    0,            // AKEYCODE_TV              = 170,
    0,            // AKEYCODE_WINDOW          = 171,
    0,            // AKEYCODE_GUIDE           = 172,
    0,            // AKEYCODE_DVR             = 173,
    0,            // AKEYCODE_BOOKMARK        = 174,
    0,            // AKEYCODE_CAPTIONS        = 175,
    0,            // AKEYCODE_SETTINGS        = 176,
    0,            // AKEYCODE_TV_POWER        = 177,
    0,            // AKEYCODE_TV_INPUT        = 178,
    0,            // AKEYCODE_STB_POWER       = 179,
    0,            // AKEYCODE_STB_INPUT       = 180,
    0,            // AKEYCODE_AVR_POWER       = 181,
    0,            // AKEYCODE_AVR_INPUT       = 182,
    0,            // AKEYCODE_PROG_RED        = 183,
    0,            // AKEYCODE_PROG_GREEN      = 184,
    0,            // AKEYCODE_PROG_YELLOW     = 185,
    0,            // AKEYCODE_PROG_BLUE       = 186,
    0,            // AKEYCODE_APP_SWITCH      = 187,
    0,            // AKEYCODE_BUTTON_1        = 188,
    0,            // AKEYCODE_BUTTON_2        = 189,
    0,            // AKEYCODE_BUTTON_3        = 190,
    0,            // AKEYCODE_BUTTON_4        = 191,
    0,            // AKEYCODE_BUTTON_5        = 192,
    0,            // AKEYCODE_BUTTON_6        = 193,
    0,            // AKEYCODE_BUTTON_7        = 194,
    0,            // AKEYCODE_BUTTON_8        = 195,
    0,            // AKEYCODE_BUTTON_9        = 196,
    0,            // AKEYCODE_BUTTON_10       = 197,
    0,            // AKEYCODE_BUTTON_11       = 198,
    0,            // AKEYCODE_BUTTON_12       = 199,
    0,            // AKEYCODE_BUTTON_13       = 200,
    0,            // AKEYCODE_BUTTON_14       = 201,
    0,            // AKEYCODE_BUTTON_15       = 202,
    0,            // AKEYCODE_BUTTON_16       = 203,
    0,            // AKEYCODE_LANGUAGE_SWITCH = 204,
    0,            // AKEYCODE_MANNER_MODE     = 205,
    0,            // AKEYCODE_3D_MODE         = 206,
    0,            // AKEYCODE_CONTACTS        = 207,
    0,            // AKEYCODE_CALENDAR        = 208,
    0,            // AKEYCODE_MUSIC           = 209,
    0,            // AKEYCODE_CALCULATOR      = 210,
    0,            // AKEYCODE_ZENKAKU_HANKAKU = 211,
    0,            // AKEYCODE_EISU            = 212,
    0,            // AKEYCODE_MUHENKAN        = 213,
    0,            // AKEYCODE_HENKAN          = 214,
    0,            // AKEYCODE_KATAKANA_HIRAGANA = 215,
    0,            // AKEYCODE_YEN             = 216,
    0,            // AKEYCODE_RO              = 217,
    0,            // AKEYCODE_KANA            = 218,
    0             // AKEYCODE_ASSIST          = 219,
};

const char capsMap[] =
{
    0,            // AKEYCODE_UNKNOWN         = 0,
    0,            // AKEYCODE_SOFT_LEFT       = 1,
    0,            // AKEYCODE_SOFT_RIGHT      = 2,
    0,            // AKEYCODE_HOME            = 3,
    0,            // AKEYCODE_BACK            = 4,
    0,            // AKEYCODE_CALL            = 5,
    0,            // AKEYCODE_ENDCALL         = 6,
    '0',          // AKEYCODE_0               = 7,
    '1',          // AKEYCODE_1               = 8,
    '2',          // AKEYCODE_2               = 9,
    '3',          // AKEYCODE_3               = 10,
    '4',          // AKEYCODE_4               = 11,
    '5',          // AKEYCODE_5               = 12,
    '6',          // AKEYCODE_6               = 13,
    '7',          // AKEYCODE_7               = 14,
    '8',          // AKEYCODE_8               = 15,
    '9',          // AKEYCODE_9               = 16,
    '*',          // AKEYCODE_STAR            = 17,
    '#',          // AKEYCODE_POUND           = 18,
    0,            // AKEYCODE_DPAD_UP         = 19,
    0,            // AKEYCODE_DPAD_DOWN       = 20,
    0,            // AKEYCODE_DPAD_LEFT       = 21,
    0,            // AKEYCODE_DPAD_RIGHT      = 22,
    0,            // AKEYCODE_DPAD_CENTER     = 23,
    0,            // AKEYCODE_VOLUME_UP       = 24,
    0,            // AKEYCODE_VOLUME_DOWN     = 25,
    0,            // AKEYCODE_POWER           = 26,
    0,            // AKEYCODE_CAMERA          = 27,
    0,            // AKEYCODE_CLEAR           = 28,
    'A',          // AKEYCODE_A               = 29,
    'B',          // AKEYCODE_B               = 30,
    'C',          // AKEYCODE_C               = 31,
    'D',          // AKEYCODE_D               = 32,
    'E',          // AKEYCODE_E               = 33,
    'F',          // AKEYCODE_F               = 34,
    'G',          // AKEYCODE_G               = 35,
    'H',          // AKEYCODE_H               = 36,
    'I',          // AKEYCODE_I               = 37,
    'J',          // AKEYCODE_J               = 38,
    'K',          // AKEYCODE_K               = 39,
    'L',          // AKEYCODE_L               = 40,
    'M',          // AKEYCODE_M               = 41,
    'N',          // AKEYCODE_N               = 42,
    'O',          // AKEYCODE_O               = 43,
    'P',          // AKEYCODE_P               = 44,
    'Q',          // AKEYCODE_Q               = 45,
    'R',          // AKEYCODE_R               = 46,
    'S',          // AKEYCODE_S               = 47,
    'T',          // AKEYCODE_T               = 48,
    'U',          // AKEYCODE_U               = 49,
    'V',          // AKEYCODE_V               = 50,
    'W',          // AKEYCODE_W               = 51,
    'X',          // AKEYCODE_X               = 52,
    'Y',          // AKEYCODE_Y               = 53,
    'Z',          // AKEYCODE_Z               = 54,
    ',',          // AKEYCODE_COMMA           = 55,
    '.',          // AKEYCODE_PERIOD          = 56,
    0,            // AKEYCODE_ALT_LEFT        = 57,
    0,            // AKEYCODE_ALT_RIGHT       = 58,
    0,            // AKEYCODE_SHIFT_LEFT      = 59,
    0,            // AKEYCODE_SHIFT_RIGHT     = 60,
    '\t',         // AKEYCODE_TAB             = 61,
    ' ',          // AKEYCODE_SPACE           = 62,
    0,            // AKEYCODE_SYM             = 63,
    0,            // AKEYCODE_EXPLORER        = 64,
    0,            // AKEYCODE_ENVELOPE        = 65,
    0,            // AKEYCODE_ENTER           = 66,
    0,            // AKEYCODE_DEL             = 67,
    '`',          // AKEYCODE_GRAVE           = 68,
    '-',          // AKEYCODE_MINUS           = 69,
    '=',          // AKEYCODE_EQUALS          = 70,
    '[',          // AKEYCODE_LEFT_BRACKET    = 71,
    ']',          // AKEYCODE_RIGHT_BRACKET   = 72,
    '\\',         // AKEYCODE_BACKSLASH       = 73,
    ';',          // AKEYCODE_SEMICOLON       = 74,
    '\'',         // AKEYCODE_APOSTROPHE      = 75,
    '/',          // AKEYCODE_SLASH           = 76,
    '@',          // AKEYCODE_AT              = 77,
    0,            // AKEYCODE_NUM             = 78,
    0,            // AKEYCODE_HEADSETHOOK     = 79,
    0,            // AKEYCODE_FOCUS           = 80,   // *Camera* focus
    '+',          // AKEYCODE_PLUS            = 81,
    0,            // AKEYCODE_MENU            = 82,
    0,            // AKEYCODE_NOTIFICATION    = 83,
    0,            // AKEYCODE_SEARCH          = 84,
    0,            // AKEYCODE_MEDIA_PLAY_PAUSE= 85,
    0,            // AKEYCODE_MEDIA_STOP      = 86,
    0,            // AKEYCODE_MEDIA_NEXT      = 87,
    0,            // AKEYCODE_MEDIA_PREVIOUS  = 88,
    0,            // AKEYCODE_MEDIA_REWIND    = 89,
    0,            // AKEYCODE_MEDIA_FAST_FORWARD = 90,
    0,            // AKEYCODE_MUTE            = 91,
    0,            // AKEYCODE_PAGE_UP         = 92,
    0,            // AKEYCODE_PAGE_DOWN       = 93,
    0,            // AKEYCODE_PICTSYMBOLS     = 94,
    0,            // AKEYCODE_SWITCH_CHARSET  = 95,
    0,            // AKEYCODE_BUTTON_A        = 96,
    0,            // AKEYCODE_BUTTON_B        = 97,
    0,            // AKEYCODE_BUTTON_C        = 98,
    0,            // AKEYCODE_BUTTON_X        = 99,
    0,            // AKEYCODE_BUTTON_Y        = 100,
    0,            // AKEYCODE_BUTTON_Z        = 101,
    0,            // AKEYCODE_BUTTON_L1       = 102,
    0,            // AKEYCODE_BUTTON_R1       = 103,
    0,            // AKEYCODE_BUTTON_L2       = 104,
    0,            // AKEYCODE_BUTTON_R2       = 105,
    0,            // AKEYCODE_BUTTON_THUMBL   = 106,
    0,            // AKEYCODE_BUTTON_THUMBR   = 107,
    0,            // AKEYCODE_BUTTON_START    = 108,
    0,            // AKEYCODE_BUTTON_SELECT   = 109,
    0,            // AKEYCODE_BUTTON_MODE     = 110,
    0,            // AKEYCODE_ESCAPE          = 111,
    0,            // AKEYCODE_FORWARD_DEL     = 112,
    0,            // AKEYCODE_CTRL_LEFT       = 113,
    0,            // AKEYCODE_CTRL_RIGHT      = 114,
    0,            // AKEYCODE_CAPS_LOCK       = 115,
    0,            // AKEYCODE_SCROLL_LOCK     = 116,
    0,            // AKEYCODE_META_LEFT       = 117,
    0,            // AKEYCODE_META_RIGHT      = 118,
    0,            // AKEYCODE_FUNCTION        = 119,
    0,            // AKEYCODE_SYSRQ           = 120,
    0,            // AKEYCODE_BREAK           = 121,
    0,            // AKEYCODE_MOVE_HOME       = 122,
    0,            // AKEYCODE_MOVE_END        = 123,
    0,            // AKEYCODE_INSERT          = 124,
    0,            // AKEYCODE_FORWARD         = 125,
    0,            // AKEYCODE_MEDIA_PLAY      = 126,
    0,            // AKEYCODE_MEDIA_PAUSE     = 127,
    0,            // AKEYCODE_MEDIA_CLOSE     = 128,
    0,            // AKEYCODE_MEDIA_EJECT     = 129,
    0,            // AKEYCODE_MEDIA_RECORD    = 130,
    0,            // AKEYCODE_F1              = 131,
    0,            // AKEYCODE_F2              = 132,
    0,            // AKEYCODE_F3              = 133,
    0,            // AKEYCODE_F4              = 134,
    0,            // AKEYCODE_F5              = 135,
    0,            // AKEYCODE_F6              = 136,
    0,            // AKEYCODE_F7              = 137,
    0,            // AKEYCODE_F8              = 138,
    0,            // AKEYCODE_F9              = 139,
    0,            // AKEYCODE_F10             = 140,
    0,            // AKEYCODE_F11             = 141,
    0,            // AKEYCODE_F12             = 142,
    0,            // AKEYCODE_NUM_LOCK        = 143,
    '0',          // AKEYCODE_NUMPAD_0        = 144,
    '1',          // AKEYCODE_NUMPAD_1        = 145,
    '2',          // AKEYCODE_NUMPAD_2        = 146,
    '3',          // AKEYCODE_NUMPAD_3        = 147,
    '4',          // AKEYCODE_NUMPAD_4        = 148,
    '5',          // AKEYCODE_NUMPAD_5        = 149,
    '6',          // AKEYCODE_NUMPAD_6        = 150,
    '7',          // AKEYCODE_NUMPAD_7        = 151,
    '8',          // AKEYCODE_NUMPAD_8        = 152,
    '9',          // AKEYCODE_NUMPAD_9        = 153,
    '/',          // AKEYCODE_NUMPAD_DIVIDE   = 154,
    '*',          // AKEYCODE_NUMPAD_MULTIPLY = 155,
    '-',          // AKEYCODE_NUMPAD_SUBTRACT = 156,
    '+',          // AKEYCODE_NUMPAD_ADD      = 157,
    '.',          // AKEYCODE_NUMPAD_DOT      = 158,
    ',',          // AKEYCODE_NUMPAD_COMMA    = 159,
    0,            // AKEYCODE_NUMPAD_ENTER    = 160,
    '=',          // AKEYCODE_NUMPAD_EQUALS   = 161,
    '(',          // AKEYCODE_NUMPAD_LEFT_PAREN = 162,
    ')',          // AKEYCODE_NUMPAD_RIGHT_PAREN = 163,
    0,            // AKEYCODE_VOLUME_MUTE     = 164,
    0,            // AKEYCODE_INFO            = 165,
    0,            // AKEYCODE_CHANNEL_UP      = 166,
    0,            // AKEYCODE_CHANNEL_DOWN    = 167,
    0,            // AKEYCODE_ZOOM_IN         = 168,
    0,            // AKEYCODE_ZOOM_OUT        = 169,
    0,            // AKEYCODE_TV              = 170,
    0,            // AKEYCODE_WINDOW          = 171,
    0,            // AKEYCODE_GUIDE           = 172,
    0,            // AKEYCODE_DVR             = 173,
    0,            // AKEYCODE_BOOKMARK        = 174,
    0,            // AKEYCODE_CAPTIONS        = 175,
    0,            // AKEYCODE_SETTINGS        = 176,
    0,            // AKEYCODE_TV_POWER        = 177,
    0,            // AKEYCODE_TV_INPUT        = 178,
    0,            // AKEYCODE_STB_POWER       = 179,
    0,            // AKEYCODE_STB_INPUT       = 180,
    0,            // AKEYCODE_AVR_POWER       = 181,
    0,            // AKEYCODE_AVR_INPUT       = 182,
    0,            // AKEYCODE_PROG_RED        = 183,
    0,            // AKEYCODE_PROG_GREEN      = 184,
    0,            // AKEYCODE_PROG_YELLOW     = 185,
    0,            // AKEYCODE_PROG_BLUE       = 186,
    0,            // AKEYCODE_APP_SWITCH      = 187,
    0,            // AKEYCODE_BUTTON_1        = 188,
    0,            // AKEYCODE_BUTTON_2        = 189,
    0,            // AKEYCODE_BUTTON_3        = 190,
    0,            // AKEYCODE_BUTTON_4        = 191,
    0,            // AKEYCODE_BUTTON_5        = 192,
    0,            // AKEYCODE_BUTTON_6        = 193,
    0,            // AKEYCODE_BUTTON_7        = 194,
    0,            // AKEYCODE_BUTTON_8        = 195,
    0,            // AKEYCODE_BUTTON_9        = 196,
    0,            // AKEYCODE_BUTTON_10       = 197,
    0,            // AKEYCODE_BUTTON_11       = 198,
    0,            // AKEYCODE_BUTTON_12       = 199,
    0,            // AKEYCODE_BUTTON_13       = 200,
    0,            // AKEYCODE_BUTTON_14       = 201,
    0,            // AKEYCODE_BUTTON_15       = 202,
    0,            // AKEYCODE_BUTTON_16       = 203,
    0,            // AKEYCODE_LANGUAGE_SWITCH = 204,
    0,            // AKEYCODE_MANNER_MODE     = 205,
    0,            // AKEYCODE_3D_MODE         = 206,
    0,            // AKEYCODE_CONTACTS        = 207,
    0,            // AKEYCODE_CALENDAR        = 208,
    0,            // AKEYCODE_MUSIC           = 209,
    0,            // AKEYCODE_CALCULATOR      = 210,
    0,            // AKEYCODE_ZENKAKU_HANKAKU = 211,
    0,            // AKEYCODE_EISU            = 212,
    0,            // AKEYCODE_MUHENKAN        = 213,
    0,            // AKEYCODE_HENKAN          = 214,
    0,            // AKEYCODE_KATAKANA_HIRAGANA = 215,
    0,            // AKEYCODE_YEN             = 216,
    0,            // AKEYCODE_RO              = 217,
    0,            // AKEYCODE_KANA            = 218,
    0             // AKEYCODE_ASSIST          = 219,
};

class KlaatuInputListener: public DISPATCH_CLASS
{
private:
    ScreenControl *screen;
    QTouchDevice *mDevice;
    int xres, yres;

public:
    KlaatuInputListener(ScreenControl *s) : screen(s)
    {
        mDevice = new QTouchDevice;
        mDevice->setName("touchscreen");
        mDevice->setType(QTouchDevice::TouchScreen);
        mDevice->setCapabilities(QTouchDevice::Position | QTouchDevice::Area | QTouchDevice::Pressure);
        QWindowSystemInterface::registerTouchDevice(mDevice);

        struct fb_var_screeninfo fb_var;
        int fd = open("/dev/graphics/fb0", O_RDONLY);
        ioctl(fd, FBIOGET_VSCREENINFO, &fb_var);
        close(fd);
        qDebug("screen size is %d by %d\n", fb_var.xres, fb_var.yres);
        xres = fb_var.xres;
        yres = fb_var.yres;
        displayWidth = xres;
        displayHeight = yres;
    };

#if defined(SHORT_PLATFORM_VERSION) && (SHORT_PLATFORM_VERSION == 23)
    void dump(String8&)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    void dispatchOnce()
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    int32_t injectInputEvent(const InputEvent*, int32_t, int32_t, int32_t, int32_t)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
        return 0;
    }
    void setInputWindows(const Vector<InputWindow>&)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    void setFocusedApplication(const InputApplication*)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    void setInputDispatchMode(bool, bool)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    status_t registerInputChannel(const sp<InputChannel>&, bool)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
        return 0;
    }
    status_t unregisterInputChannel(const sp<InputChannel>&)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
        return 0;
    }
    void notifyConfigurationChanged(nsecs_t)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    void notifyKey(nsecs_t, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, int32_t, int32_t, nsecs_t)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    void notifyMotion(nsecs_t, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t, const int32_t*, const PointerCoords*, float, float, nsecs_t)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    void notifySwitch(nsecs_t, int32_t, int32_t, uint32_t)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
#else
    void notifyConfigurationChanged(const NotifyConfigurationChangedArgs*)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    void notifyKey(const NotifyKeyArgs* args)
    {
#ifdef INPUT_DEBUG
        qDebug("Keyboard event: time %lld keycode=%s action=%d ",
               args->eventTime, KEYCODES[args->keyCode-1].literal,
               args->action);
#endif
	int keycode = 0;
	char text = 0;
	Qt::KeyboardModifiers mod = Qt::NoModifier;
	static bool leftShift = false;
	static bool rightShift = false;
	static bool capsLock = false;

	switch(args->keyCode) {
	case AKEYCODE_POWER:
            screen->powerKey(!args->action);
	    keycode = Qt::Key_PowerOff;
	    break;
        case AKEYCODE_SHIFT_LEFT:
            if (args->action == AKEY_EVENT_ACTION_DOWN)
            {
                leftShift = true;
            }
            else
            {
                leftShift = false;
            }
            break;
        case AKEYCODE_SHIFT_RIGHT:
            if (args->action == AKEY_EVENT_ACTION_DOWN)
            {
                rightShift = true;
            }
            else
            {
                rightShift = false;
            }
            break;
        case AKEYCODE_CAPS_LOCK:
            if (args->action == AKEY_EVENT_ACTION_DOWN)
            {
                capsLock = !capsLock;
            }
            break;
	default:
            if (args->keyCode > AKEYCODE_ASSIST)
                qWarning("Keyboard Device: unrecognized keycode=%d",
                         args->keyCode);
            else
            {
                keycode = keyMap[args->keyCode];
                if (leftShift || rightShift)
                    text = shiftMap[args->keyCode];
                else if (capsLock)
                    text = capsMap[args->keyCode];
                else
                    text = normalMap[args->keyCode];
            }
	    break;
	}
        if (leftShift || rightShift || capsLock)
            mod |= Qt::ShiftModifier;
	if (keycode != 0)
	    QWindowSystemInterface::handleKeyEvent(0,
              (args->action == AKEY_EVENT_ACTION_DOWN ? QEvent::KeyPress
                                                      : QEvent::KeyRelease),
              keycode, mod, QString(text), false);
    }

    void notifyMotion(const NotifyMotionArgs* args)
    {
        float x=0.0, y=0.0;
        QList<QWindowSystemInterface::TouchPoint> mTouchPoints;
	mTouchPoints.clear();
        QWindowSystemInterface::TouchPoint tp;
#if DEBUG_INBOUND_EVENT_DETAILS
    ALOGD("notifyMotion - eventTime=%lld, deviceId=%d, source=0x%x, policyFlags=0x%x, "
            "action=0x%x, flags=0x%x, metaState=0x%x, buttonState=0x%x, edgeFlags=0x%x, "
            "xPrecision=%f, yPrecision=%f, downTime=%lld",
            args->eventTime, args->deviceId, args->source, args->policyFlags,
            args->action, args->flags, args->metaState, args->buttonState,
            args->edgeFlags, args->xPrecision, args->yPrecision, args->downTime);
    for (uint32_t i = 0; i < args->pointerCount; i++) {
        ALOGD("  Pointer %d: id=%d, toolType=%d, "
                "x=%f, y=%f, pressure=%f, size=%f, "
                "touchMajor=%f, touchMinor=%f, toolMajor=%f, toolMinor=%f, "
                "orientation=%f",
                i, args->pointerProperties[i].id,
                args->pointerProperties[i].toolType,
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_X),
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_Y),
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_PRESSURE),
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_SIZE),
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MAJOR),
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MINOR),
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOOL_MAJOR),
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOOL_MINOR),
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_ORIENTATION));
    }
#endif


	for (unsigned int i = 0; i < args->pointerCount; i++) {
            const PointerCoords pc = args->pointerCoords[i];

            tp.id = args->pointerProperties[i].id;
            tp.flags = 0;  // We are not a pen, check pointerProperties.toolType
            tp.pressure = pc.getAxisValue(AMOTION_EVENT_AXIS_PRESSURE);

            screen->userActivity();
            x = pc.getX();
            y = pc.getY();

            // Store the HW coordinates for now, will be updated later.
            tp.area = QRectF(0, 0,
                             pc.getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MAJOR),
                             pc.getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MINOR));
            tp.area.moveCenter(QPoint((int)x, (int)y));
	
            // Get a normalized position in range 0..1.
            tp.normalPosition = QPointF(x / xres, y / yres);

            mTouchPoints.append(tp);
        }

        int id = (args->action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
            AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        switch (args->action & AMOTION_EVENT_ACTION_MASK)
	{
	case AMOTION_EVENT_ACTION_DOWN:
            mTouchPoints[id].state = Qt::TouchPointPressed;
	  break;
	case AMOTION_EVENT_ACTION_UP:
	  mTouchPoints[id].state = Qt::TouchPointReleased;
	  break;
	case AMOTION_EVENT_ACTION_MOVE:
          // err on the side of caution and mark all points as moved
          for (unsigned int i = 0; i < args->pointerCount; i++) {
            mTouchPoints[i].state = Qt::TouchPointMoved;
        }
	  break;
	case AMOTION_EVENT_ACTION_POINTER_DOWN:
	  mTouchPoints[id].state = Qt::TouchPointPressed;
	  break;
	case AMOTION_EVENT_ACTION_POINTER_UP:
  	  mTouchPoints[id].state = Qt::TouchPointReleased;
	  break;
	case AMOTION_EVENT_ACTION_HOVER_MOVE:
            if (args->pointerProperties[0].toolType ==
                AMOTION_EVENT_TOOL_TYPE_MOUSE)
            {
                // For some reason, Qt only wants to respond to
                // HOVER_MOVE events as mouse events, not touch events.
                QPointF coords(x, y);
                QWindowSystemInterface::handleMouseEvent(0, coords, coords,
                                                         Qt::NoButton);
                return;
            }
	  break;
	case AMOTION_EVENT_ACTION_SCROLL:
	  qDebug("unhandled event: AMOTION_EVENT_ACTION_SCROLL\n");
	  break;
	case AMOTION_EVENT_ACTION_HOVER_ENTER:
	  qDebug("unhandled event: AMOTION_EVENT_ACTION_HOVER_ENTER\n");
	  break;
	case AMOTION_EVENT_ACTION_HOVER_EXIT:
	  qDebug("unhandled event: AMOTION_EVENT_ACTION_HOVER_EXIT\n");
	  break;
	default:
	  qDebug("unrecognized touch event: %d, index %d\n",
                 args->action & AMOTION_EVENT_ACTION_MASK, id);
	  break;
	}

#ifdef INPUT_DEBUG
        qDebug() << "Reporting touch events" << mTouchPoints.count();
        for (int i = 0 ; i < mTouchPoints.count() ; i++) {
            QWindowSystemInterface::TouchPoint& tp(mTouchPoints[i]);
            qDebug() << "  " << tp.id << tp.area << (int) tp.state << tp.normalPosition;
        }
#endif
        QWindowSystemInterface::handleTouchEvent(0, mDevice, mTouchPoints);
    }
    void notifySwitch(const NotifySwitchArgs*)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    void notifyDeviceReset(const NotifyDeviceResetArgs*)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
#endif // not 2.3
};

// --- FakePointerController for mouse support ---
// a straight copy of the Android code in frameworks/base/services/input/tests/InputReader_test.cpp
class FakePointerController : public PointerControllerInterface {
    bool mHaveBounds;
    float mMinX, mMinY, mMaxX, mMaxY;
    float mX, mY;
    int32_t mButtonState;

protected:
    virtual ~FakePointerController() { }

public:
    FakePointerController() :
        mHaveBounds(false), mMinX(0), mMinY(0), mMaxX(0), mMaxY(0), mX(0), mY(0),
        mButtonState(0) {
    }

    void setBounds(float minX, float minY, float maxX, float maxY) {
        mHaveBounds = true;
        mMinX = minX;
        mMinY = minY;
        mMaxX = maxX;
        mMaxY = maxY;
    }

    virtual void setPosition(float x, float y) {
        mX = x;
        mY = y;
    }

    virtual void setButtonState(int32_t buttonState) {
        mButtonState = buttonState;
    }

    virtual int32_t getButtonState() const {
        return mButtonState;
    }

    virtual void getPosition(float* outX, float* outY) const {
        *outX = mX;
        *outY = mY;
    }

private:
    virtual bool getBounds(float* outMinX, float* outMinY, float* outMaxX, float* outMaxY) const {
        *outMinX = mMinX;
        *outMinY = mMinY;
        *outMaxX = mMaxX;
        *outMaxY = mMaxY;
        return mHaveBounds;
    }

    virtual void move(float deltaX, float deltaY) {
        mX += deltaX;
        if (mX < mMinX) mX = mMinX;
        if (mX > mMaxX) mX = mMaxX;
        mY += deltaY;
        if (mY < mMinY) mY = mMinY;
        if (mY > mMaxY) mY = mMaxY;
    }

    virtual void fade(Transition) {
    }

    virtual void unfade(Transition) {
    }

    virtual void setPresentation(Presentation) {
    }

    virtual void setSpots(const PointerCoords*,
            const uint32_t*, BitSet32) {
    }

    virtual void clearSpots() {
    }
};

class KlaatuReaderPolicy: public InputReaderPolicyInterface {
    sp<FakePointerController> mPointerControllers;
    EventHub *mHub;
    CursorSignal *cursorSignal;

#if defined(SHORT_PLATFORM_VERSION) && (SHORT_PLATFORM_VERSION == 23)
    bool getDisplayInfo(int32_t, int32_t*, int32_t*, int32_t*)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
        return true;
    }
    bool filterTouchEvents()
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
        return true;
    }
    bool filterJumpyTouchEvents()
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
        return true;
    }
    nsecs_t getVirtualKeyQuietTime()
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
        return 0;
    }
    void getVirtualKeyDefinitions(const String8&, Vector<VirtualKeyDefinition>&)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    void getInputDeviceCalibration(const String8&, InputDeviceCalibration&)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    void getExcludedDeviceNames(Vector<String8>&)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
#else
    void getReaderConfiguration(InputReaderConfiguration* outConfig)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
        struct fb_var_screeninfo fb_var;
        int fd = open("/dev/graphics/fb0", O_RDONLY);
        ioctl(fd, FBIOGET_VSCREENINFO, &fb_var);
        close(fd);
        qDebug("screen size is %d by %d\n", fb_var.xres, fb_var.yres);

#if defined(SHORT_PLATFORM_VERSION) && (SHORT_PLATFORM_VERSION == 42)
	static DisplayViewport vport;
        vport.displayId = 0;
        vport.orientation = 0;
        vport.logicalLeft = 0;
        vport.logicalTop = 0;
        vport.logicalRight = fb_var.xres;
        vport.logicalBottom = fb_var.yres;
        vport.physicalLeft = 0;
        vport.physicalTop = 0;
        vport.physicalRight = fb_var.xres;
        vport.physicalBottom = fb_var.yres;
        vport.deviceWidth = fb_var.xres;
        vport.deviceHeight = fb_var.yres;

        outConfig->setDisplayInfo(false, vport);
#else
        outConfig->setDisplayInfo(0, false, fb_var.xres, fb_var.yres, 0);
#endif
    }
    sp<PointerControllerInterface> obtainPointerController(int32_t deviceId)
    {
        qDebug("[%s:%d] %x\n", __FUNCTION__, __LINE__, deviceId);
        mPointerControllers =  new FakePointerController();
        mPointerControllers->setBounds(0, 0, displayWidth - 1, displayHeight - 1);
        return mPointerControllers;
    }
    String8 getDeviceAlias(const InputDeviceIdentifier& identifier)
    {
        qDebug("[%s:%d] '%s'\n", __FUNCTION__, __LINE__, identifier.name.string());
        return String8("foo");
    }
#if defined(SHORT_PLATFORM_VERSION) && (SHORT_PLATFORM_VERSION == 40)
#else
    sp<KeyCharacterMap> getKeyboardLayoutOverlay(const String8& inputDeviceDescriptor)
    {
        qDebug("[%s:%d] '%s'\n", __FUNCTION__, __LINE__, inputDeviceDescriptor.string());
        return NULL;
    }
#endif
    void notifyInputDevicesChanged(const Vector<InputDeviceInfo>& inputDevices)
    {
        bool cursorDevice = false;
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
#if defined(SHORT_PLATFORM_VERSION) && (SHORT_PLATFORM_VERSION == 40)
#else
        for (unsigned int i = 0; i < inputDevices.size(); i++) {
            qDebug("name %s\n", inputDevices[i].getIdentifier().name.string());
            if (mHub->getDeviceClasses(inputDevices[i].getId()) & INPUT_DEVICE_CLASS_CURSOR)
            {
                qDebug("Found cursor device\n");
                cursorDevice = true;
            }
#if 0
            qDebug("x axis %f - %f\n",
                   inputDevices[i].getMotionRange(AMOTION_EVENT_AXIS_X, 0)->min,
                   inputDevices[i].getMotionRange(AMOTION_EVENT_AXIS_X, 0)->max);
#endif
        }
        // send a signal to the GUI thread to cause it to change the cursor
        emit cursorSignal->showMouse(cursorDevice);
#endif
    }
#endif // not 2.3
    public:
    KlaatuReaderPolicy(EventHub *hub) : mHub(hub)
    {
        cursorSignal = new CursorSignal();
        KlaatuApplication *ka =
            ((KlaatuApplication *)(QCoreApplication::instance()));
        cursorSignal->connect(cursorSignal, SIGNAL(showMouse(bool)), ka, SLOT(showMouse(bool)));
    }
};

// --------------------------------------------------------------------------------
EventThread::EventThread(ScreenControl *s, EventHub *hub) :
        InputReaderThread(new InputReader(hub,
                           new KlaatuReaderPolicy(hub),
                           new KlaatuInputListener(s)))
{
    mHub = hub;
}
