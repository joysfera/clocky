/* Joy Clocky Structure - a CookieJar public interface header file */

#define CLOCKY_IDENT		'JCLK'
#define CLOCKY_IDENT_STR	"JCLK"
#define CLOCKY_IDENT_NUM	0x4A434C4BUL	/* 'JCLK' in hexadecimal notation */
#define JCLKSTRUCT_VERSION	0x300

struct _switches {
	unsigned ShowTime:1;	/* 31 - General Display switch (bits 30-24 depend on this bit being set) */
	unsigned ShowIDT:1;		/* 30 - Follow the national settings (date&time format) */
	unsigned ShowDate:1;	/* 29 - Display also the date line */
	unsigned ShowDay:1;		/* 28 - Display Day of Week and special symbols */
	unsigned ShowYear:1;	/* 27 - Display Year (two digits) and seconds */
	unsigned ShowDigi:1;	/* 26 - Use digital numbers for displaying */
	unsigned ShowBig:1;		/* 25 - Use big font for displaying */
	unsigned ShowTrn:1;		/* 24 - Transparent displaying */

	unsigned KbdAltK:1;		/* 23 - Handle Alt-key and Alt-Shift-key combos */
	unsigned KbdEHC:1;		/* 22 - External HotKeys active */
	unsigned KbdDead:1;		/* 21 - Dead Key active */
	unsigned KbdAltChar:1;	/* 20 - Alt+number for entering unusual characters */
	unsigned KbdClick:1;	/* 19 - turn OFF clicking sound of typing */
	unsigned KbdBell:1;		/* 18 - turn OFF the warning bell sound */
	unsigned KbdLayout:2;	/* 16-17 - define keyboard layout: 0 = no change, 1 = TOS original, 2 = normal, 3 = coding */

	unsigned MiscMS4x:1;	/* 15 - accelerate mouse 4x (depends on MiscMouse being set) */
	unsigned MiscMouse:1;	/* 14 - mouse accelerator 2x active */
	unsigned MiscPrnt:1;	/* 13 - on-line conversion of printed data active */
	unsigned MiscTurbo:1;	/* 12 - MegaSTE's 16 MHz + data cache active */
	unsigned MiscBeep:1;	/* 11 - "time sign" - a beep every hour */
	unsigned unused1:1;
	unsigned unused2:1;
	unsigned unused3:1;

	unsigned SaverOn:1;		/*  7 - General Screen Saver switch (affects savecount, 6-2 depends on this bit being set) */
	unsigned SaveMod1:1;	/*  6 - Modem1 CD check */
	unsigned SaveMod2:1;	/*  5 - Modem2 CD check */
	unsigned SaveSer2:1;	/*  4 - Serial2 CD check */
	unsigned SaveVESA:1;	/*  3 - use VESA Powersaving features of modern VGA monitors */
	unsigned SaveSTacy:1;	/*  2 - save STacy LED */
	unsigned Unused:2;		/*  0-1 */
};

struct _bootsetup {
	unsigned unused3:1;		/* 15 - */
	unsigned HookVBL:1;		/* 14 - */
	unsigned HookKbd:1;		/* 13 - */
	unsigned HookMouse:1;	/* 12 - */
	unsigned HookPrint:1;	/* 11 - */
	unsigned unused2:3;
	/* */
	unsigned unused1:3;
	unsigned ResetEHC:1;	/* 4 - reset EHC table during AES restart */
	unsigned FixY2K:1;		/* 3 - enables a workaround in XBIOS Gettime() that fixes an Y2K bug in older TOS */
	unsigned Settime:1;		/* 2 - follow GEMDOS Tsettime()/Tsetdate for detecting date/time changes (even if DTCOOKIE is installed) */
	unsigned EngSys:1;		/* 1 - simulate English TOS */
	unsigned InvVideo:1;	/* 0 - invert colors on boot (white on black) */
};

typedef struct _bootsetup BOOTSETUP;

#define SCAN_CODES			128
#define KBDLEN				(3*SCAN_CODES)
#define ALT_LEN				(2*16)		/* up to 15 pairs of scan/ascii code + terminating pair [0,0] */
#define PRNTBLLEN			128
#define DEADTBLLEN			40
#define HOTKEYS				16

struct	_jclkstruct {
	long	name;		/* compare with CLOCKY_IDENT_NUM, must be equal */
	short	version;	/* compare with JCLKSTRUCT_VERSION, see below */
	union {
		struct	_switches par;
		long	lparam;
	} switches;
	short	SaverLen;			/* time of inactivity in seconds after which the screen saver is activated */
	short	SaverCount;			/* counts the time of inactivity - when Savercount >= SaverLen the saver is activated */
	char	hotshift;			/* bits 0..3, 0 = no hotkeys */
	char	hottime;			/* bits 0..3, 0 = no hottime */
	char	actual_shift;		/* current state of Alt/Ctrl/Shift/CapsLock keys  */
	char	actual_key;			/* currently pressed Alt-Ctrl hot key - scancode in range <1,127> */
	char	*ehc_table;			/* ptr to char ehc_table[2*128] */
	short	ShowPos;			/* position of clock on screen in columns from right border; if negative then it is the -(position from the left border+1) */
	unsigned char ShowColorB;	/* background color in the range <0,15> */
	unsigned char ShowColorF;	/* foreground color in the range <0,15> */
	BOOTSETUP	boot;
	short	refresh;
	char	normal_kbd[KBDLEN];
	char	normal_alt[3][ALT_LEN];
	char	coding_kbd[KBDLEN];
	char	coding_alt[3][ALT_LEN];
	char	prntable[PRNTBLLEN];
	char	deadkey;
	char	deadkeys_defined;
	char	deadtable1[DEADTBLLEN];
	char	deadtable2[DEADTBLLEN];
	char	deadtable3[DEADTBLLEN];
	char	hotkeys[HOTKEYS];
	char	weekdays[7][2];
};

typedef struct _jclkstruct JCLKSTRUCT;

/* how to check for the right Clocky struct version */
/*

if ((version >= JCLKSTRUCT_VERSION) && ((version / 0x100) == (JCLKSTRUCT_VERSION / 0x100)))
{
	version is OK
}
else
{
	get newer version
}

*/
