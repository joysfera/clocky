/*
 * SetClock
 */

#include <cflib.h>
#include <ostruct.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>

#include "setclock.h"

#ifndef FALSE
#define TRUE	(1==1)
#define FALSE	(!TRUE)
#endif

#define SETCLOCK_VERSION	0x311

#include "jclkcook.h"

#define UBYTE				unsigned char

JCLKSTRUCT *jclk_config = NULL;

#define CLOCKPATH "C:\\AUTO\\"
#define CLOCKEXT  "*.PRG"
#define CLOCKNAME "CLOCKY.PRG"

#define SELECTCLOCKY		"Select Clocky"
#define VYBERKBD_STRING		"Select Keyboard"
#define SELECTEHC			"Choose any application"

#define JCLKTOOLSETNAME		"jclktool.set"

#define MAXDIR		260
#define MAXPATH		260
#define MAXNAME		260

#define MAX_CLOCKY_LEN	10240

size_t	data_offset = 0;
short	OS_version = 0;

int edit_resident_clocky = FALSE;
int clocky_in_buffer = FALSE;

char root_drive = 'C';

UBYTE clocky_buffer[MAX_CLOCKY_LEN];		/* max 10 kB */
long clocky_buffer_len = 0;

char	clkpath[MAXDIR] = CLOCKPATH, clkname[MAXNAME] = CLOCKNAME, clkfname[MAXPATH] = "";
char	kbdpath[MAXDIR], kbdname[MAXNAME]="", kbdfname[MAXPATH] = "";
char	jclktoolsetpath[MAXPATH] = "";
char	ehcpath[MAXPATH] = "", ehcname[MAXPATH] = "", ehcfname[MAXPATH] = "";
char	*homedir = NULL, *homedefaultsdir = NULL;

#define TOTALKBDLEN			(KBDLEN + 3*ALT_LEN)
UBYTE tmp_kbd[TOTALKBDLEN];

char infos[][80]={"", "", "", "", "", "", "", "", "", ""};

typedef enum
{
/* public */
    KAPPFL_VA_START     = 0x0001,
	KAPPFL_VA_START_REQ = 0x0002,
	KAPPFL_GEMMSG		= 0x0004,
	KAPPFL_GEMMSG2		= 0x0008,
	KAPPFL_PASSTHROUGH	= 0x0100,
	KAPPFL_ALLOWSHIFTS	= 0x0200,
/* private */
	KAPPFL_ASCIIKEY		= 0x0400,
}   KAPPFLAGS;

struct	_kapp {
	int	scancode;
	KAPPFLAGS flags;
	char path[2*MAXPATH];
};
typedef struct _kapp KAPP;

/* --------------------------------------------------------------------------- */

OBJECT	*menu, *aboutdial, *vektorydial, *displaydial, *miscdial, *keyboarddial, *timedatedial,
		*saverdial, *konverzedial, *popups, *layoutdial, *deadkeysdial, *ehcdial,
		*ehceditdial, *inthotkeysdial;
int		quit;
int		msg[8];
int		event, msx, msy, mbutton, kstate, mclick, kreturn, vdi_handle, win_handle;

int		id = 1, pts = 10;

static void handle_msg(int *msg);
void ehc_dial(void);
void icc_dial(void);

/* --------------------------------------------------------------------------- */

void collect_infos(void)
{
	long value;
	JCLKSTRUCT *adr = NULL;
	if (getcookie("JCLK", &value) && value != NULL) {
		int version;
		adr = (JCLKSTRUCT *)value;
		version = adr->version;
		sprintf(infos[0], "Clocky found in memory, interface version %x.%02x", version / 0x100, version % 0x100);
	}
	else
		sprintf(infos[0], "Clocky not found in memory.");

	if (getcookie("DATE", NULL) && getcookie("TIME", NULL))
		strcpy(infos[1], "Date/Time Cookies found - Clocky's time will be updated correctly.");
	else
		strcpy(infos[1], "DTCOOKIE not found - please enable 'Detect Time Change' in 'Time Special'.");

	if (appl_find("JCLKTOOL") > 0)
		strcpy(infos[2], "JCLKTool is running - this gives you the universal external hotkeys.");
	else
		strcpy(infos[2], "JCLKTOOL.ACC is not running - please install it to get additional benefits.");

	if (adr != NULL) {
		char *ptr = adr->ehc_table;
		int i, cnt=0, total=0;
		for(i=0; i<128; i++) {
			int a = ptr[i];
			if (a)
				total++;
			if (a > cnt)
				cnt = a;
		}

		sprintf(infos[3], "%d hotkey clients installed. %d external hotkeys defined.", cnt, total);
	}
}

void force_redraw_win(int handle)
{
	int temp[4];
	short msg[8];
	wind_get(handle, WF_WORKXYWH, &temp[0], &temp[1], &temp[2], &temp[3]);
	temp[2] += temp[0] - 1;
	temp[3] += temp[1] - 1;
	msg[0] = WM_REDRAW;
	msg[1] = gl_apid;
	msg[2] = 0;
	msg[3] = handle;
	msg[4] = temp[0];
	msg[5] = temp[1];
	msg[6] = temp[2];
	msg[7] = temp[3];
	appl_write(gl_apid, 16, msg);
}

void Alert(const char *format, ...)
{
	va_list args;
	char buf[500], buf2[500];

	va_start(args, format);
	vsprintf(buf, format, args);
	va_end(args);

	sprintf(buf2, "[1][%s][Abort]", buf);
	do_walert(1, 1, buf2, "Error!");
}

void set_window_name(const char *format, ...)
{
	va_list args;
	static char buf[256];	/* must be static because wind_set_str doesn't copy that! */

	va_start(args, format);
	vsprintf(buf, format, args);
	va_end(args);

	wind_set_str(win_handle, WF_NAME, buf);
}

void aktivuj_menu()
{
	menu_icheck(menu, MRESIDENT, edit_resident_clocky ? CHECKED : NORMAL);

	menu_ienable(menu, MSAVE, clocky_in_buffer ? ENABLE : DISABLE);
	menu_ienable(menu, MSAVEAS, clocky_in_buffer ? ENABLE : DISABLE );

	menu_ienable(menu, MKBDAMYS, (jclk_config != NULL) ? ENABLE : DISABLE);
	menu_ienable(menu, MDISPLAY, (jclk_config != NULL) ? ENABLE : DISABLE);
	menu_ienable(menu, MTIMEDATE, (jclk_config != NULL) ? ENABLE : DISABLE);
	menu_ienable(menu, MSAVER, (jclk_config != NULL) ? ENABLE : DISABLE);
	menu_ienable(menu, MMISC, (jclk_config != NULL) ? ENABLE : DISABLE);
}

/* --------------------------------------------------------------------------- */

int load_data(const char *fname)
{
	FILE *f;
	long file_len;
	int text_seg_len;
	JCLKSTRUCT *jclk;

	if ((f = fopen(fname, "rb")) == NULL) {
		Alert("Cannot open '%s' |for reading.", fname);
		return FALSE;
	}

	/* budeme cist data do nasi struktury, uz neni krok zpet */
	jclk_config = NULL;
	edit_resident_clocky = FALSE;
	clocky_in_buffer = FALSE;

	file_len = fread(clocky_buffer, 1, MAX_CLOCKY_LEN, f);
	fclose( f );

	if (file_len == MAX_CLOCKY_LEN) {
		Alert("Clocky too long |(%d bytes).", file_len);
		return FALSE;
	}

	text_seg_len = *(long *)(clocky_buffer + 2);
	jclk = (JCLKSTRUCT *)(clocky_buffer + 28 + text_seg_len);

	/* zkontroluj verzi */
	if (jclk->name == CLOCKY_IDENT_NUM && jclk->version == JCLKSTRUCT_VERSION) {
		jclk_config = jclk;
		clocky_buffer_len = file_len;
		clocky_in_buffer = TRUE;
		set_window_name(" SetClock: editing file %s ", fname);
	}
	else {
		// debug("name = %04lx, version = %x\n", jclk->name, jclk->version);
		Alert("Wrong Clocky version |Detected: %d|Expected:%d", jclk->version, JCLKSTRUCT_VERSION);
	}

	aktivuj_menu();

	return clocky_in_buffer;
}

int select_and_load_data()
{
	if (! select_file(clkpath, clkname, CLOCKEXT, SELECTCLOCKY, NULL))
		return FALSE;

	strcpy( clkfname, clkpath );
	strcat( clkfname, clkname );
	return load_data(clkfname);
}

/* --------------------------------------------------------------------------- */

void read_resident_data(int fakt)
{
	if (fakt) {
		JCLKSTRUCT *adr;
		int clocky_ok = FALSE;

		if (getcookie(CLOCKY_IDENT_STR, (long *)&adr))
		{
			/* zkontroluj verzi */
			if (adr) {
				clocky_ok = (adr->name == CLOCKY_IDENT_NUM && adr->version == JCLKSTRUCT_VERSION);
				jclk_config = adr;
				edit_resident_clocky = TRUE;
				set_window_name(" SetClock: editing resident Clocky ");
				clocky_in_buffer = FALSE;	/* mozna by slo casem ulozit Clocky z RAM pres buffer a SaveAs na disk - odzkousena kombinace by se ulozila do AUTa - elegantni */
			}
		}
		menu_ienable(menu, MRESIDENT, clocky_ok ? ENABLE : DISABLE);
	}
	else {
		jclk_config = NULL;
		edit_resident_clocky = FALSE;
		set_window_name(" SetClock: no Clocky loaded ");
	}

	aktivuj_menu();
}

/* --------------------------------------------------------------------------- */

int save_data(const char *fname)
{
	FILE *f;
	int len = 0;

	if ( ( f = fopen( fname, "wb" ) ) == NULL ) {
		Alert("Cannot open '%s' |for writting.", fname);
		return FALSE;
	}

	len = fwrite( clocky_buffer, 1, clocky_buffer_len, f );
	fclose( f );
	if (len != clocky_buffer_len) {
		Alert("Data not stored correctly to |'%s'|(%d < %d)", fname, len, clocky_buffer_len);
		return FALSE;
	}

	set_window_name(" SetClock: editing file %s ", fname);

	return TRUE;
}

int save_data_as()
{
	if (! select_file(clkpath, clkname, CLOCKEXT, SELECTCLOCKY, NULL))
		return FALSE;

	strcpy( clkfname, clkpath );
	strcat( clkfname, clkname );
	return save_data(clkfname);
}

/* --------------------------------------------------------------------------- */

int load_kbd(void)
{
	FILE *f;
	int fl;	/* file length */
	int ret=TRUE;

	*kbdfname = '\0';
	if (! select_file(kbdpath, kbdname, "*.KBD", VYBERKBD_STRING, NULL) )
		return FALSE;
	strcpy( kbdfname, kbdpath );
	strcat( kbdfname, kbdname );
	if ( ( f = fopen( kbdfname, "rb" ) ) == NULL ) {
		Alert("Cannot open '%s' |for reading.", kbdfname);
		return FALSE;
	}

	memset( tmp_kbd, 0, TOTALKBDLEN);

	fseek(f, 0, SEEK_END);
	fl = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (fl == KBDLEN)
		fread( tmp_kbd, sizeof(UBYTE), KBDLEN, f );
	else if (fl == 2*KBDLEN) {
		int h;
		char *ptr;
		fread( tmp_kbd, sizeof(UBYTE), KBDLEN, f );

		/* specialni cyklus pro prevedeni Alt pole na sekvenci */
		ptr = tmp_kbd + KBDLEN;
		for(h = 0; h < 3; h++, ptr += ALT_LEN) {
			char tmparr[SCAN_CODES];
			int i, j = 0;
			fread( tmparr, sizeof(tmparr[0]), sizeof(tmparr), f);
			for(i=0;i < SCAN_CODES && j < ALT_LEN; i++) {
				char a = tmparr[i];
				if (a > 0) {
					ptr[j++] = i;
					ptr[j++] = a;
				}
			}
		}
	}
	else {
		Alert("Unrecognized keyboard file. |Supported are 384 and 768|bytes long files.");
		ret = FALSE;
	}
	fclose( f );
	return ret;
}

/* --------------------------------------------------------------------------- */

int save_kbd(int extended_alt)
{
	FILE *f;

	if ( ! select_file(kbdpath, kbdname, "*.KBD", VYBERKBD_STRING, NULL) )
		return FALSE;
	strcpy( kbdfname, kbdpath );
	strcat( kbdfname, kbdname );
	if ( ( f = fopen( kbdfname, "wb" ) ) == NULL ) {
		Alert("Cannot open '%s' |for writting.", kbdfname);
		return FALSE;
	}

	fwrite( tmp_kbd, sizeof(UBYTE), KBDLEN, f );

	if (extended_alt) {
		char *ptr = tmp_kbd + KBDLEN;
		int h;
		for(h = 0; h < 3; h++, ptr += ALT_LEN) {
			char tmparr[SCAN_CODES];
			int i;
			memset(tmparr, 0, SCAN_CODES);
			for(i=0; i < ALT_LEN; i+=2) {
				int a = ptr[i];
				if (a == 0)
					break;
				tmparr[a] = ptr[i+1];
			}
			fwrite( tmparr, sizeof(tmparr[0]), sizeof(tmparr), f);
		}
	}
	fclose( f );

	return TRUE;
}

/* --------------------------------------------------------------------------- */

void copy_alt_table(char *dest, char *src)
{
	int i;
	memcpy( dest, src, ALT_LEN );
	/* vynuluj zbytek balastu */
	for(i=0; i<ALT_LEN; i+=2) {
		if (dest[i] != 0)
			continue;
		memset(dest + i, 0, ALT_LEN - i);
	}
}

void get_actual_kbd(void)
{
	_KEYTAB *keytbl_ptr = Keytbl((void *)-1L,(void *)-1L,(void *)-1L);
	memcpy( tmp_kbd, keytbl_ptr->unshift, SCAN_CODES );
	memcpy( tmp_kbd+SCAN_CODES, keytbl_ptr->shift, SCAN_CODES );
	memcpy( tmp_kbd+2*SCAN_CODES, keytbl_ptr->caps, SCAN_CODES );

	/* copy Alt tables either from TOS or from Clocky (if installed) */
	if (OS_version >= 0x400) {
		char *dest = tmp_kbd + KBDLEN;
		copy_alt_table(dest, keytbl_ptr[1].unshift);
		copy_alt_table(dest+=ALT_LEN, keytbl_ptr[1].shift);
		copy_alt_table(dest+=ALT_LEN, keytbl_ptr[1].caps);
	}
	else {
		JCLKSTRUCT *clk;

		if (getcookie(CLOCKY_IDENT_STR, (long *)&clk) && clk != NULL)
		{
			/* zkontroluj verzi */
			if (clk->name == CLOCKY_IDENT_NUM && clk->version == JCLKSTRUCT_VERSION) {
				int klavesy = clk->switches.par.KbdLayout;
				char *dest = tmp_kbd + KBDLEN;
				int i;
				if (klavesy >= 2)
					for(i=0; i<2; i++, dest+=ALT_LEN)
						copy_alt_table(dest, klavesy == 2 ? clk->normal_alt[i] : clk->coding_alt[i]);
			}
		}
	}
}

int scancode_to_objectnr(int scan)
{
	int key;

	if (scan <= 0x3a)
		key = scan + KBDESC - 1;
	else if (scan == 0x4a)
		key = MINUS;
	else if (scan == 0x4e)
		key = PLUS;
	else if (scan == 0x60)
		key = KBD60;
	else if (scan >= 0x63 && scan <= 0x72)
		key = scan + KBD63 - 0x63;
	else
		return KBDESC;	/* nic */

	return key;
}

/* vypocitat scancode z cisla stisknuteho buttonu */
int objectnr_to_scancode(int obj)
{
	int scancode;

	if (obj > KBDESC && obj <= (0x3a + (KBDESC - 1)))
		scancode = obj - (KBDESC - 1);
	else if (obj == MINUS)
		scancode = 0x4a;
	else if (obj == PLUS)
		scancode = 0x4e;
	else if (obj == KBD60)
		scancode = 0x60;
	else if (obj >= KBD63 && obj <= KBD63+0x0f)
		scancode = obj - KBD63 + 0x63;
	else
		scancode = 0;

	return scancode;
}

int scancode_to_objectnr2(int scan)
{
	int key;

	if (scan <= 0x3a)
		key = scan + HOTKEY_ESC - 1;
	else if (scan >= 0x3b && scan <= 0x44)
		key = scan - 0x3b + HOTKEY_F1;
	else if (scan == 0x47)
		key = HOT_CLR;
	else if (scan == 0x48)
		key = HOT_ARUP;
	else if (scan == 0x4a)
		key = HOT_MINUS;
	else if (scan == 0x4b)
		key = HOT_ARLEFT;
	else if (scan == 0x4d)
		key = HOT_ARRIGHT;
	else if (scan == 0x4e)
		key = HOT_PLUS;
	else if (scan == 0x50)
		key = HOT_ARDOWN;
	else if (scan == 0x52)
		key = HOT_INS;
	else if (scan == 0x53)
		key = HOT_DEL;
	else if (scan == 0x60)
		key = HOT60;
	else if (scan == 0x61)
		key = HOT_UNDO;
	else if (scan == 0x62)
		key = HOT_HELP;
	else if (scan >= 0x63 && scan <= 0x72)
		key = scan - 0x63 + HOT63;
	else
		return HOTKEY_ESC;	/* nic */

	return key;
}

/* vypocitat scancode z cisla stisknuteho buttonu */
int objectnr_to_scancode2(int obj)
{
	int scancode;

	if (obj >= HOTKEY_ESC && obj <= (0x3a + (HOTKEY_ESC - 1)))
		scancode = obj - HOTKEY_ESC + 1;
	else if (obj >= HOTKEY_F1 && obj <= (HOTKEY_F1 + 9))
		scancode = obj - HOTKEY_F1 + 0x3b;
	else if (obj == HOT_CLR)
		scancode = 0x47;
	else if (obj == HOT_ARUP)
		scancode = 0x48;
	else if (obj == HOT_MINUS)
		scancode = 0x4a;
	else if (obj == HOT_ARLEFT)
		scancode = 0x4b;
	else if (obj == HOT_ARRIGHT)
		scancode = 0x4d;
	else if (obj == HOT_PLUS)
		scancode = 0x4e;
	else if (obj == HOT_ARDOWN)
		scancode = 0x50;
	else if (obj == HOT_INS)
		scancode = 0x52;
	else if (obj == HOT_DEL)
		scancode = 0x53;
	else if (obj == HOT60)
		scancode = 0x60;
	else if (obj == HOT_UNDO)
		scancode = 0x61;
	else if (obj == HOT_HELP)
		scancode = 0x62;
	else if (obj >= HOT63 && obj <= HOT63+0x0f)
		scancode = obj - HOT63 + 0x63;
	else
		scancode = 0;

	return scancode;
}

int nastav_znak_klavesnice(OBJECT *klavesnice, int scan, int ascii)
{
	int object_key = scancode_to_objectnr(scan);
	if (klavesnice[object_key].ob_type == G_BOXCHAR)
		klavesnice[object_key].ob_spec.obspec.character = ascii;

	return object_key;
}

void vymaz_znaky_klavesnice(OBJECT *klavesnice)
{
	int i;
	for(i=1; i<SCAN_CODES; i++) {
		int obj = nastav_znak_klavesnice(klavesnice, i, ' ');
		if (obj > 0 /*&& klavesnice[obj].ob_flags & SELECTABLE*/)
			set_state(klavesnice, obj, SELECTED, FALSE);		/* unselect */
	}
}

void newkbd(OBJECT *klavesnice, int shifts)
{
	int i;

	if (shifts < 0 || shifts > 5)
		shifts = 0;

	vymaz_znaky_klavesnice(klavesnice);

	if (shifts <= 2) {
		for(i = 1; i < SCAN_CODES; i++) {
			int ascii = tmp_kbd[shifts*SCAN_CODES+i];
			nastav_znak_klavesnice(klavesnice, i, ascii);
		}
	}
	else {
		for(i=0; i < ALT_LEN; i+=2) {
			int ptr = KBDLEN+(shifts-3)*ALT_LEN+i;
			int scan = tmp_kbd[ptr];
			int ascii = tmp_kbd[ptr+1];
			if (! scan)
				break;

			nastav_znak_klavesnice(klavesnice, scan, ascii);
		}
	}
}

/* --------------------------------------------------------------------------- */

void show_help(int co)
{
	int id = appl_find("ST-GUIDE");
	if (id > 0) {
		char *hyp_path = "*:\\clocky.hyp";
		struct _co2node {
			int co;
			char *kapitola;
		};
		struct _co2node kapitoly[] = {
			{0, "main"},
			{ABOUT, "D_About"},
			{MISC, "D_Miscellaneous"},
			{TIMEDATE, "D_TimeDate"},
			{KEYBOARD, "D_Keyboard"},
			{DISPLAY, "D_Display"},
			{SAVER, "D_Saver"},
			{LAYOUT, "D_Layout"},
			{DEADKEYS, "D_DeadKeys"},
			{EHC, "D_Ehc"},
			{EHC_EDIT, "D_EhcEdit"},
			{INTHOTKEYS, "D_IntHotKeys"}
		};
		int i;
		char *kapitol_name = "";
		char *complete_path;

		for(i=0; i<sizeof(kapitoly)/sizeof(kapitoly[0]); i++) {
			if (kapitoly[i].co == co) {
				kapitol_name = kapitoly[i].kapitola;
				break;
			}
		}

		complete_path = alloca(strlen(hyp_path)+1+strlen(kapitol_name)+1);
		strcpy(complete_path, hyp_path);
		if (*kapitol_name) {
			strcat(complete_path, " ");
			strcat(complete_path, kapitol_name);
		}
		send_vastart(id, complete_path);
	}
	else
		Alert("ST-Guide not found.|Please install it if you|want to read Clocky's help. ");
}

/* --------------------------------------------------------------------------- */

void misc_dial(void)
{
	void	*mdial;
	int	close = FALSE;
	int	exit_obj;

	set_state(miscdial, ON_MOUSEACCEL, SELECTED, jclk_config->switches.par.MiscMouse);
	set_popup_item(miscdial, KBDPOP_MOUSE, popups, jclk_config->switches.par.MiscMS4x ? PTMYS_4X : PTMYS_2X);
	set_state(miscdial, ON_PRINTCONV, SELECTED, jclk_config->switches.par.MiscPrnt);
	set_state(miscdial, ON_KEYCLICK, SELECTED, jclk_config->switches.par.KbdClick || jclk_config->switches.par.KbdBell);
	switch(jclk_config->switches.par.KbdClick * 2 + jclk_config->switches.par.KbdBell) {
		case 3:
			set_popup_item(miscdial, MISCPOP_ZVUKY, popups, PTZVUK_OBA);
			break;
		case 2:
		case 0:		/* nothing should be disabled */
			set_popup_item(miscdial, MISCPOP_ZVUKY, popups, PTZVUK_KLIK);
		case 1:
			set_popup_item(miscdial, MISCPOP_ZVUKY, popups, PTZVUK_ZVON);
	}

	set_state(miscdial, ON_MEGATURBO, SELECTED, jclk_config->switches.par.MiscTurbo);

	set_state(miscdial, ON_INVCOLOR, SELECTED, jclk_config->boot.InvVideo);
	set_state(miscdial, ON_INVCOLOR, DISABLED, edit_resident_clocky);

	set_state(miscdial, ON_SIMUENG, SELECTED, jclk_config->boot.EngSys);
	set_state(miscdial, ON_SIMUENG, DISABLED, edit_resident_clocky);

	set_state(miscdial, ON_RESETEHC, SELECTED, jclk_config->boot.ResetEHC);
	set_state(miscdial, ON_RESETEHC, DISABLED, edit_resident_clocky);

	set_int(miscdial, AFTER_GEM_START, jclk_config->refresh);
	set_state(miscdial, AFTER_GEM_START, DISABLED, edit_resident_clocky);

	set_state(miscdial, MISC_APPLY, DISABLED, !edit_resident_clocky);

	mdial = open_mdial(miscdial, AFTER_GEM_START);
	while (!close)
	{
		exit_obj = do_mdial(mdial) & 0x7fff;
		switch (exit_obj)
		{
			case KBDPOP_MOUSE :
				handle_popup(miscdial, KBDPOP_MOUSE, popups, POPMYS, POP_OPEN);
				break;

			case MISCPOP_ZVUKY :
				handle_popup(miscdial, MISCPOP_ZVUKY, popups, POP_ZVUKEFEKTY, POP_OPEN);
				break;

			case MISC_CANCEL :
				close = TRUE;
				break;

			case MISC_HELP :
				show_help(MISC);
				break;

			case MISC_OK :
				close = TRUE;
				/* passthru */

			case MISC_APPLY:
				jclk_config->switches.par.MiscMouse = get_state(miscdial, ON_MOUSEACCEL, SELECTED);
				jclk_config->switches.par.MiscMS4x = is_selected_popup_item(miscdial, KBDPOP_MOUSE, popups, POPMYS, PTMYS_4X) ? 1 : 0;
				jclk_config->switches.par.MiscPrnt = get_state(miscdial, ON_PRINTCONV, SELECTED);
				jclk_config->switches.par.KbdClick = jclk_config->switches.par.KbdBell = FALSE;
				if (get_state(miscdial, ON_KEYCLICK, SELECTED)) {
					jclk_config->switches.par.KbdClick = ! is_selected_popup_item(miscdial, MISCPOP_ZVUKY, popups, POP_ZVUKEFEKTY, PTZVUK_ZVON);
					jclk_config->switches.par.KbdBell = ! is_selected_popup_item(miscdial, MISCPOP_ZVUKY, popups, POP_ZVUKEFEKTY, PTZVUK_KLIK);
				}
				jclk_config->switches.par.MiscTurbo = get_state(miscdial, ON_MEGATURBO, SELECTED);
				jclk_config->boot.InvVideo = get_state(miscdial, ON_INVCOLOR, SELECTED);
				jclk_config->boot.EngSys = get_state(miscdial, ON_SIMUENG, SELECTED);
				jclk_config->boot.ResetEHC = get_state(miscdial, ON_RESETEHC, SELECTED);
				jclk_config->refresh = get_int(miscdial, AFTER_GEM_START);

				break;
		}
		if (get_flag(miscdial, exit_obj, EXIT))
			set_state(miscdial, exit_obj, SELECTED, FALSE);
		if (!close)
			redraw_mdobj(mdial, exit_obj);
	}
	close_mdial(mdial);
}

int shift_to_object(int shifts)
{
	switch(shifts) {
		case 3:	return PTS_LSRS;
		case 5: return PTS_RSC;
		case 6: return PTS_LSC;
		case 9: return PTS_RSA;
		case 10: return PTS_LSA;
		case 12: return PTS_AC;
		case 13: return PTS_RSAC;
		case 14: return PTS_LSAC;
		default: return 0;
	}
}

int object_to_shift(int obj)
{
	switch(obj) {
		case PTS_LSRS: return 3;
		case PTS_RSC: return 5;
		case PTS_LSC: return 6;
		case PTS_RSA: return 9;
		case PTS_LSA: return 10;
		case PTS_AC: return 12;
		case PTS_RSAC: return 13;
		case PTS_LSAC: return 14;
		default: return 0;
	}
}

void timedate_dial(void)
{
	void	*mdial;
	int	close = FALSE;
	int	exit_obj;

	set_state(timedatedial, ON_Y2KFIX, SELECTED, jclk_config->boot.FixY2K);
	set_state(timedatedial, ON_Y2KFIX, DISABLED, edit_resident_clocky);
	set_state(timedatedial, ON_SETTIME, SELECTED, jclk_config->boot.Settime);
	set_state(timedatedial, ON_SETTIME, DISABLED, edit_resident_clocky);
	if (jclk_config->hottime) {
		set_state(timedatedial, ON_KUKNOUT, SELECTED, TRUE);
		set_popup_item(timedatedial, TDPOP_KUKSHIFT, popups, shift_to_object(jclk_config->hottime));
	}
	else {
		set_state(timedatedial, ON_KUKNOUT, SELECTED, FALSE);
		set_popup_item(timedatedial, TDPOP_KUKSHIFT, popups, PTS_LSC);
		set_state(timedatedial, TDPOP_KUKSHIFT, DISABLED, TRUE);
	}
	set_state(timedatedial, ON_CASZNAMENI, SELECTED, jclk_config->switches.par.MiscBeep);

	set_state(timedatedial, TIMEDATE_APPLY, DISABLED, ! edit_resident_clocky);

	mdial = open_mdial(timedatedial, -1/* EDIT1*/);
	while (!close)
	{
		exit_obj = do_mdial(mdial) & 0x7fff;
		switch (exit_obj)
		{
			case ON_KUKNOUT :
				set_state(timedatedial, TDPOP_KUKSHIFT, DISABLED, !get_state(timedatedial, ON_KUKNOUT, SELECTED));
				redraw_mdobj(mdial, TDPOP_KUKSHIFT);
				break;

			case TDPOP_KUKSHIFT :
				handle_popup(timedatedial, TDPOP_KUKSHIFT, popups, POPSHIFTY, POP_OPEN);
				break;

			case TIMEDATE_CANCEL :
				close = TRUE;
				break;

			case TIMEDATE_HELP :
				show_help(TIMEDATE);
				break;

			case TIMEDATE_OK :
				close = TRUE;
				/* passthru */

			case TIMEDATE_APPLY :
				jclk_config->boot.FixY2K = get_state(timedatedial, ON_Y2KFIX, SELECTED);
				jclk_config->boot.Settime = get_state(timedatedial, ON_SETTIME, SELECTED);
				/* zjisti popup TDPOP_KUKSHIFT */
				if (get_state(timedatedial, ON_KUKNOUT, SELECTED))
					jclk_config->hottime = object_to_shift(get_popup_item(timedatedial, TDPOP_KUKSHIFT, popups, POPSHIFTY));
				else
					jclk_config->hottime = 0;
				jclk_config->switches.par.MiscBeep = get_state(timedatedial, ON_CASZNAMENI, SELECTED);

				break;
		}
		if (get_flag(timedatedial, exit_obj, EXIT))
			set_state(timedatedial, exit_obj, SELECTED, FALSE);
		if (!close)
			redraw_mdobj(mdial, exit_obj);
	}
	close_mdial(mdial);
}

void editlayout_dial(int cesky_layout, int extended_alt);
void editdead_dial(void);

int get_selected_keyboard_layout()
{
	int layout = get_popup_item(keyboarddial, KBDPOP_LAYOUT, popups, POPLAYOUT);
	int result = 0;
	switch(layout) {
		case PTLAY_ORIG:	result = 1; break;
		case PTLAY_NORMAL:	result = 2; break;
		case PTLAY_CESKA:	result = 3; break;
	}

	return result;
}

int order_to_keyboard_layout(int layout)
{
	int ptlay;
	switch(layout) {
		default:
		case 1:	ptlay = PTLAY_ORIG; break;
		case 2: ptlay = PTLAY_NORMAL; break;
		case 3: ptlay = PTLAY_CESKA; break;
	}

	return ptlay;
}

void keyboard_dial(void)
{
	void	*mdial;
	int	close = FALSE;
	int	exit_obj;

	if (jclk_config->hotshift > 0) {
		set_state(keyboarddial, ON_HOTKEYS, SELECTED, TRUE);
		set_popup_item(keyboarddial, KBDPOP_HOTKEYS, popups, shift_to_object(jclk_config->hotshift));
	}
	else {
		set_state(keyboarddial, ON_HOTKEYS, SELECTED, FALSE);
		set_popup_item(keyboarddial, KBDPOP_HOTKEYS, popups, PTS_LSRS);
	}
	set_state(keyboarddial, ON_KBDLAYOUT, SELECTED, jclk_config->switches.par.KbdLayout > 0);
	set_popup_item(keyboarddial, KBDPOP_LAYOUT, popups, order_to_keyboard_layout(jclk_config->switches.par.KbdLayout));
	set_state(keyboarddial, ON_DEADKEYS, SELECTED, jclk_config->switches.par.KbdDead);
	set_state(keyboarddial, ON_ALTCHAR, SELECTED, jclk_config->switches.par.KbdAltChar);
	set_state(keyboarddial, ON_EHC, SELECTED, jclk_config->switches.par.KbdEHC);
	set_state(keyboarddial, ON_ALTKEYS, SELECTED, jclk_config->switches.par.KbdAltK);

	/* vypni editovatelnost originalni klavesnice */
	set_state(keyboarddial, KBDEDIT_LAYOUT, DISABLED, (get_selected_keyboard_layout() == 1));

	set_state(keyboarddial, KEYBOARD_APPLY, DISABLED, ! edit_resident_clocky);

	mdial = open_mdial(keyboarddial, -1/* EDIT1*/);
	while (!close)
	{
		exit_obj = do_mdial(mdial) & 0x7fff;
		switch (exit_obj)
		{
			case KBDPOP_HOTKEYS :
				handle_popup(keyboarddial, KBDPOP_HOTKEYS, popups, POPSHIFTY, POP_OPEN);
				break;

			case KBDEDIT_HOTKEYS:
				icc_dial();
				break;

			case KBDEDIT_LAYOUT :
				{
					int kbdlayout = get_selected_keyboard_layout();
					if (kbdlayout == 1)
						Alert("Can't edit TOS ROM keyboard");
					else
						editlayout_dial(kbdlayout == 3, get_state(keyboarddial, ON_ALTKEYS, SELECTED));
				}
				break;

			case KBDPOP_LAYOUT :
				handle_popup(keyboarddial, KBDPOP_LAYOUT, popups, POPLAYOUT, POP_OPEN);
				set_state(keyboarddial, KBDEDIT_LAYOUT, DISABLED, (get_selected_keyboard_layout() == 1));
				redraw_mdobj(mdial, KBDEDIT_LAYOUT);
				break;

			case KBDEDIT_DEADKEYS :
				editdead_dial();
				break;

			case KBDEDIT_EHC :
				ehc_dial();
				break;

			case KEYBOARD_HELP:
				show_help(KEYBOARD);
				break;

			case KEYBOARD_CANCEL :
				close = TRUE;
				break;

			case KEYBOARD_OK :
				close = TRUE;
				/* passthru */

			case KEYBOARD_APPLY :
				if (get_state(keyboarddial, ON_HOTKEYS, SELECTED)) {
					jclk_config->hotshift = object_to_shift(get_popup_item(keyboarddial, KBDPOP_HOTKEYS, popups, POPSHIFTY));
				}
				else
					jclk_config->hotshift = 0;

				if (get_state(keyboarddial, ON_KBDLAYOUT, SELECTED)) {
					jclk_config->switches.par.KbdLayout = get_selected_keyboard_layout();
				}
				else
					jclk_config->switches.par.KbdLayout = 0;

				jclk_config->switches.par.KbdDead = get_state(keyboarddial, ON_DEADKEYS, SELECTED);
				jclk_config->switches.par.KbdAltChar = get_state(keyboarddial, ON_ALTCHAR, SELECTED);
				jclk_config->switches.par.KbdEHC = get_state(keyboarddial, ON_EHC, SELECTED);
				jclk_config->switches.par.KbdAltK = get_state(keyboarddial, ON_ALTKEYS, SELECTED);

				break;
		}
		if (get_flag(keyboarddial, exit_obj, EXIT))
			set_state(keyboarddial, exit_obj, SELECTED, FALSE);
		if (!close)
			redraw_mdobj(mdial, exit_obj);
	}
	close_mdial(mdial);
}

int color2item(int color)
{
	if (color >= 0 && color < 16)
		return color + PTCOL_WHITE;

	return PTCOL_BLACK;
}

void about_dial(void)
{
	void	*mdial;
	int	close = FALSE;
	int	exit_obj;

	mdial = open_mdial(aboutdial, -1);
	while (!close)
	{
		exit_obj = do_mdial(mdial) & 0x7fff;
		switch (exit_obj)
		{
			case ABOUT_HELP:
				show_help(0);
				break;

			case ABOUT_INFO :
				force_redraw_win(win_handle);
				close = TRUE;
				break;

			case ABOUT_OK :
				close = TRUE;
				break;
		}
		if (get_flag(aboutdial, exit_obj, EXIT))
			set_state(aboutdial, exit_obj, SELECTED, FALSE);
		if (!close)
			redraw_mdobj(mdial, exit_obj);
	}
	close_mdial(mdial);
}

void display_dial(void)
{
	void	*mdial;
	int	close = FALSE;
	int	exit_obj;
	int iDays[7]={DOW_MONDAY, DOW_TUESDAY, DOW_WEDNESDAY, DOW_THURSDAY, DOW_FRIDAY, DOW_SATURDAY, DOW_SUNDAY};

	set_state(displaydial, ON_SHOWTIME, SELECTED, jclk_config->switches.par.ShowTime);
	set_state(displaydial, ON_SHOWSECYEAR, SELECTED, jclk_config->switches.par.ShowYear);
	set_state(displaydial, ON_SHOWDATELINE, SELECTED, jclk_config->switches.par.ShowDate);
	set_state(displaydial, ON_SHOWDOWSYM, SELECTED, jclk_config->switches.par.ShowDay);
	set_state(displaydial, ON_SHOWBYIDT, SELECTED, jclk_config->switches.par.ShowIDT);
	set_state(displaydial, ON_SHOWTRANSP, SELECTED, jclk_config->switches.par.ShowTrn);
	set_state(displaydial, ON_SHOWTRANSP, SELECTED, jclk_config->switches.par.ShowTrn);
	set_state(displaydial, ON_SHOWBIGFONT, SELECTED, jclk_config->switches.par.ShowBig);
	set_popup_item(displaydial, DISPPOP_CISLICE, popups, jclk_config->switches.par.ShowDigi ? PTCIF_DIGITAL : PTCIF_NORMAL);
	set_popup_item(displaydial, DISPPOP_COLOR, popups, color2item(jclk_config->ShowColorF));
	set_popup_item(displaydial, DISPPOP_COLORB, popups, color2item(jclk_config->ShowColorB));
	set_state(displaydial, DISPPOP_COLORB, DISABLED, jclk_config->switches.par.ShowTrn);
	if (jclk_config->ShowPos >= 0) {
		set_popup_item(displaydial, DISPPOP_OKRAJ, popups, PTOKR_PRAVY);
		set_int(displaydial, POZICE_VYPISU, jclk_config->ShowPos);
	}
	else {
		set_popup_item(displaydial, DISPPOP_OKRAJ, popups, PTOKR_LEVY);
		set_int(displaydial, POZICE_VYPISU, -jclk_config->ShowPos-1);
	}

	/* dny v tydnu */
	{
		int i;
		for(i=0; i<7; i++) {
			char str[3];
			strncpy(str, jclk_config->weekdays[i], 2);
			str[2]='\0';
			set_string(displaydial, iDays[i], str);
		}
	}

	set_state(displaydial, DISPLAY_APPLY, DISABLED, ! edit_resident_clocky);

	mdial = open_mdial(displaydial, POZICE_VYPISU);
	while (!close)
	{
		exit_obj = do_mdial(mdial) & 0x7fff;
		switch (exit_obj)
		{
			case DISPPOP_CISLICE :
				handle_popup(displaydial, DISPPOP_CISLICE, popups, POPCISLICE, POP_OPEN);
				break;

			case ON_SHOWTRANSP :
				set_state(displaydial, DISPPOP_COLORB, DISABLED, get_state(displaydial, ON_SHOWTRANSP, SELECTED));
				redraw_mdobj(mdial, DISPPOP_COLORB);
				break;


			case DISPPOP_COLOR :
				handle_popup(displaydial, DISPPOP_COLOR, popups, POPCOLOR, POP_OPEN);
				break;

			case DISPPOP_COLORB :
				handle_popup(displaydial, DISPPOP_COLORB, popups, POPCOLOR, POP_OPEN);
				break;

			case DISPPOP_OKRAJ :
				handle_popup(displaydial, DISPPOP_OKRAJ, popups, POPPOZICE, POP_OPEN);
				break;

			case DISPLAY_HELP:
				show_help(DISPLAY);
				break;

			case DISPLAY_CANCEL :
				close = TRUE;
				break;

			case DISPLAY_OK :
				close = TRUE;
				/* passthru */

			case DISPLAY_APPLY :
				jclk_config->switches.par.ShowTime = get_state(displaydial, ON_SHOWTIME, SELECTED);
				jclk_config->switches.par.ShowYear = get_state(displaydial, ON_SHOWSECYEAR, SELECTED);
				jclk_config->switches.par.ShowDate = get_state(displaydial, ON_SHOWDATELINE, SELECTED);
				jclk_config->switches.par.ShowDay = get_state(displaydial, ON_SHOWDOWSYM, SELECTED);
				jclk_config->switches.par.ShowIDT = get_state(displaydial, ON_SHOWBYIDT, SELECTED);
				jclk_config->switches.par.ShowTrn = get_state(displaydial, ON_SHOWTRANSP, SELECTED);
				jclk_config->switches.par.ShowBig = get_state(displaydial, ON_SHOWBIGFONT, SELECTED);
				jclk_config->switches.par.ShowDigi = is_selected_popup_item(displaydial, DISPPOP_CISLICE, popups, POPCISLICE, PTCIF_DIGITAL);
				jclk_config->ShowColorF = get_popup_item(displaydial, DISPPOP_COLOR, popups, POPCOLOR) - PTCOL_WHITE;
				jclk_config->ShowColorB = get_popup_item(displaydial, DISPPOP_COLORB, popups, POPCOLOR) - PTCOL_WHITE;
				{
					int new_pos = get_int(displaydial, POZICE_VYPISU);
					if (new_pos < 0)
						new_pos = 0;
					if (is_selected_popup_item(displaydial, DISPPOP_OKRAJ, popups, POPPOZICE, PTOKR_LEVY))
						new_pos = -new_pos-1;
					jclk_config->ShowPos = new_pos;
				}

				/* dny v tydnu */
				{
					int i;
					for(i=0; i<7; i++) {
						char str[3];
						char a;
						char *ptr=jclk_config->weekdays[i];
						get_string(displaydial, iDays[i], str);
						if ((a=str[0])) {
							ptr[0] = a;
							if ((a=str[1]))
								ptr[1] = a;
							else
								ptr[1] = ' ';
						}
						else
							ptr[0] = ptr[1] = ' ';
					}
				}

				break;
		}
		if (get_flag(displaydial, exit_obj, EXIT))
			set_state(displaydial, exit_obj, SELECTED, FALSE);
		if (!close)
			redraw_mdobj(mdial, exit_obj);
	}
	close_mdial(mdial);
}

void saver_dial(void)
{
	void	*mdial;
	int	close = FALSE;
	int	exit_obj;

	set_state(saverdial, ON_SCREENSAVER, SELECTED, jclk_config->switches.par.SaverOn);
	set_state(saverdial, ON_SAVERCDM1, SELECTED, jclk_config->switches.par.SaveMod1);
	set_state(saverdial, ON_SAVERCDM2, SELECTED, jclk_config->switches.par.SaveMod2);
	set_state(saverdial, ON_SAVERCDS2, SELECTED, jclk_config->switches.par.SaveSer2);
	set_int(saverdial, AKTIVACE_SAVERU, jclk_config->SaverLen);
	set_state(saverdial, ON_SAVERVESA, SELECTED, jclk_config->switches.par.SaveVESA);
	set_state(saverdial, ON_SAVERSTACY, SELECTED, jclk_config->switches.par.SaveSTacy);

	set_state(saverdial, SAVER_APPLY, DISABLED, ! edit_resident_clocky);

	mdial = open_mdial(saverdial, AKTIVACE_SAVERU);
	while (!close)
	{
		exit_obj = do_mdial(mdial) & 0x7fff;
		switch (exit_obj)
		{
			case SAVER_HELP:
				show_help(SAVER);
				break;

			case SAVER_CANCEL :
				close = TRUE;
				break;

			case SAVER_OK :
				close = TRUE;
				/* passthru */

			case SAVER_APPLY :
				jclk_config->switches.par.SaverOn = get_state(saverdial, ON_SCREENSAVER, SELECTED);
				jclk_config->switches.par.SaveMod1 = get_state(saverdial, ON_SAVERCDM1, SELECTED);
				jclk_config->switches.par.SaveMod2 = get_state(saverdial, ON_SAVERCDM2, SELECTED);
				jclk_config->switches.par.SaveSer2 = get_state(saverdial, ON_SAVERCDS2, SELECTED);
				jclk_config->SaverLen = get_int(saverdial, AKTIVACE_SAVERU);
				jclk_config->switches.par.SaveVESA = get_state(saverdial, ON_SAVERVESA, SELECTED);
				jclk_config->switches.par.SaveSTacy = get_state(saverdial, ON_SAVERSTACY, SELECTED);

				break;
		}
		if (get_flag(saverdial, exit_obj, EXIT))
			set_state(saverdial, exit_obj, SELECTED, FALSE);
		if (!close)
			redraw_mdobj(mdial, exit_obj);
	}
	close_mdial(mdial);
}

void editlayout_dial(int cesky_layout, int extended_alt)
{
	void	*mdial;
	int	close = FALSE;
	int	exit_obj;
	int klavesnice = PTLAY_NORMAL;
	int prerazovac = 0;
	int pop_ret = 0;
	UBYTE *kbd_ptr;
	char s[80];

	/* you can't select Original keyboard layout for editting */
	set_flag(popups, PTLAY_ORIG, SELECTABLE, FALSE);
	set_state(popups, PTLAY_ORIG, DISABLED, TRUE);

	/* you can't edit Alt keyboards if no extended flag */
	set_state(popups, POP_SALTERNATE, DISABLED, !extended_alt);
	set_state(popups, POP_ALTSHIFT, DISABLED, !extended_alt);
	set_state(popups, POP_ALTCAPS, DISABLED, !extended_alt);

	klavesnice = cesky_layout ? PTLAY_CESKA : PTLAY_NORMAL;
	set_popup_item(layoutdial, LAYOUT_KBD, popups, klavesnice);

	kbd_ptr = klavesnice == PTLAY_CESKA ? jclk_config->coding_kbd : jclk_config->normal_kbd;
	memcpy( tmp_kbd, kbd_ptr, TOTALKBDLEN );	/* load */

	newkbd(layoutdial, prerazovac);
	get_string(popups, POP_BEZSHIFTU /* protoze prerazovac=0 */, s);
	set_string(layoutdial, LAYOUT_RADIC, s);

	set_state(layoutdial, LAYOUT_APPLY, DISABLED, ! edit_resident_clocky);

	mdial = open_mdial(layoutdial, -1/* EDIT1*/);
	while (!close)
	{
		exit_obj = do_mdial(mdial) & 0x7fff;
		switch (exit_obj)
		{
			case LAYOUT_KBD :
				pop_ret = handle_popup(layoutdial, LAYOUT_KBD, popups, POPLAYOUT, POP_OPEN);
				if (pop_ret > 0) {
					if (klavesnice != pop_ret) {
						/* zmena */
						kbd_ptr = klavesnice == PTLAY_CESKA ? jclk_config->coding_kbd : jclk_config->normal_kbd;
						memcpy( kbd_ptr, tmp_kbd, TOTALKBDLEN );	/* save */

						klavesnice = pop_ret;

						kbd_ptr = klavesnice == PTLAY_CESKA ? jclk_config->coding_kbd : jclk_config->normal_kbd;
						memcpy( tmp_kbd, kbd_ptr, TOTALKBDLEN );	/* load */
						newkbd(layoutdial, prerazovac);
						redraw_mdobj(mdial, LAYOUT_PARENT);
					}
				}
				break;

			case LAYOUT_RADIC :
				pop_ret = handle_popup(layoutdial, LAYOUT_RADIC, popups, POPRADIC, POP_OPEN);
				if (pop_ret > 0) {
					switch(pop_ret) {
						case POP_BEZSHIFTU:
							prerazovac = 0; break;
						case POP_SSHIFTEM:
							prerazovac = 1; break;
						case POP_SCAPSLOCK:
							prerazovac = 2; break;
						case POP_SALTERNATE:
							prerazovac = 3; break;
						case POP_ALTSHIFT:
							prerazovac = 4; break;
						case POP_ALTCAPS:
							prerazovac = 5; break;
					}
					newkbd(layoutdial, prerazovac);
					redraw_mdobj(mdial, LAYOUT_PARENT);
				}
				break;

			case LAYOUT_OK :
				close = TRUE;
				/* passthru */

			case LAYOUT_APPLY :
				/* save changes */
				kbd_ptr = klavesnice == PTLAY_CESKA ? jclk_config->coding_kbd : jclk_config->normal_kbd;
				memcpy( kbd_ptr, tmp_kbd, TOTALKBDLEN );	/* save */

				break;

			case LAYOUT_CANCEL :
				close = TRUE;
				break;

			case LAYOUT_HELP:
				show_help(LAYOUT);
				break;

			case LAYOUT_UNDO :
				memcpy( tmp_kbd, kbd_ptr, TOTALKBDLEN );	/* reload */
				newkbd( layoutdial, prerazovac );
				redraw_mdobj(mdial, LAYOUT_PARENT);
				break;

			case GET_KBDLAYOUT :
				get_actual_kbd();
				newkbd(layoutdial, prerazovac);
				redraw_mdobj(mdial, LAYOUT_PARENT);
				break;

			case LOAD_KBDLAYOUT :
				load_kbd();
				newkbd(layoutdial, prerazovac);
				redraw_mdobj(mdial, LAYOUT_PARENT);
				break;

			case SAVE_KBDLAYOUT :
				save_kbd(extended_alt);
				break;

			default :
				{
					int key = exit_obj;
					int znak;
					int scancode;

					if (layoutdial[key].ob_type != G_BOXCHAR) {
						Alert("Unknown button %d", key);
						break;
					}

					/* vypocitat scancode z cisla stisknuteho buttonu */
					if (key > KBDESC && key <= (0x3a + (KBDESC - 1)))
						scancode = key - (KBDESC - 1);
					else if (key == MINUS)
						scancode = 0x4a;
					else if (key == PLUS)
						scancode = 0x4e;
					else if (key == KBD60)
						scancode = 0x60;
					else if (key >= KBD63 && key <= KBD63+0x0f)
						scancode = key - KBD63 + 0x63;
					else {
						Alert("Unknown scancode for object %d", key);
						scancode = 0;
					}

					if (scancode < 1 && scancode >= SCAN_CODES)
						break;

	/* scancode found so save ascii code at the scancode position to the table */
					znak = ascii_table(1, gl_hbox >= 19 ? 10 : 9);
					if (znak < 0)
						break;

					if (prerazovac < 3) {
						layoutdial[key].ob_spec.obspec.character = tmp_kbd[prerazovac*SCAN_CODES + scancode] = znak;
					}
					else {
						int i;
						int saved = FALSE;
						char *ptr = tmp_kbd+KBDLEN+(prerazovac-3)*ALT_LEN;
						if (znak > 0) {
							/* add character */
							for(i=0; i<ALT_LEN; i+=2) {
								if (ptr[i] == 0 && i < ALT_LEN-2) {
									ptr[i] = scancode;
									/* ptr[i+1] will be set in the next step */
									ptr[i+2] = 0;
								}
								if (ptr[i] == scancode) {
									layoutdial[key].ob_spec.obspec.character = ptr[i+1] = znak;
									saved = TRUE;
									break;
								}
							}
							if (!saved) {
								Alert("Table for Alt characters is |already full.");
							}
						}
						else { /* ASCII(0) ==>  delete character */
							for(i=0; i<ALT_LEN; i+=2) {
								if (ptr[i] == scancode) {
									layoutdial[key].ob_spec.obspec.character = ' ';
									memmove(ptr+i, ptr+i+2, ALT_LEN-i-2);	/* prisunout zbytek */
								}
							}
						}
					}
				}
		}
		if (get_flag(layoutdial, exit_obj, EXIT))
			set_state(layoutdial, exit_obj, SELECTED, FALSE);
		if (!close)
			redraw_mdobj(mdial, exit_obj);
	}
	close_mdial(mdial);

	/* REVERSE: you can't select Original keyboard layout for editting */
	set_flag(popups, PTLAY_ORIG, SELECTABLE, TRUE);
	set_state(popups, PTLAY_ORIG, DISABLED, FALSE);
}

int vyber_klavesu(int orig_scancode, char *title)
{
	MDIAL	*mdial;
	int	close = FALSE;
	int	exit_obj;
	int orig_key = scancode_to_objectnr(orig_scancode);
	int new_scancode = orig_scancode;
	int height_offset;

	get_actual_kbd();
	newkbd(layoutdial, 0);

	if (layoutdial[orig_key].ob_type == G_BOXCHAR /* && ptr->ob_flags & TOUCHEXIT *//* SELECTABLE */ )
		set_state(layoutdial, orig_key, SELECTED, TRUE);

	/* uprava dialogu pro nezobrazovani OPTIONS */
	height_offset = layoutdial[OPTIONS_PARENT].ob_height;
	layoutdial[OPTIONS_PARENT].ob_flags |= HIDETREE;
	layoutdial[LAYOUT_OK].ob_y -= height_offset;
	layoutdial[LAYOUT_CANCEL].ob_y -= height_offset;
	layoutdial[LAYOUT_HELP].ob_y -= height_offset;
	layoutdial->ob_height -= height_offset;

	mdial = open_mdial(layoutdial, -1/* EDIT1*/);

	/* zmen jmeno okna */
	wind_set_str(mdial->win_handle, WF_NAME, title);

	while (!close)
	{
		exit_obj = do_mdial(mdial) & 0x7fff;
		switch (exit_obj)
		{
			case LAYOUT_OK :
				new_scancode = orig_scancode;
				close = TRUE;
				break;

			case LAYOUT_CANCEL :
				close = TRUE;
				break;

			case LAYOUT_HELP:
				show_help(LAYOUT);
				break;

			default :
				{
					int key = exit_obj;
					int scancode;

					if (layoutdial[key].ob_type != G_BOXCHAR) {
						Alert("Unknown button %d", key);
						break;
					}

					scancode = objectnr_to_scancode(key);
					if (! scancode)
						Alert("Unknown scancode for object %d", key);

					if (scancode < 1 && scancode >= SCAN_CODES)
						break;

					if (scancode != orig_scancode) {
						/* deselect old */
						if (layoutdial[orig_key].ob_type == G_BOXCHAR) {
							set_state(layoutdial, orig_key, SELECTED, FALSE);
							redraw_mdobj(mdial, orig_key);
						}

						/* select new */
						set_state(layoutdial, key, SELECTED, TRUE);
						redraw_mdobj(mdial, key);
						orig_scancode = scancode;
						orig_key = key;
					}
				}
		}
		if (get_flag(layoutdial, exit_obj, EXIT)) {
			set_state(layoutdial, exit_obj, SELECTED, FALSE);
			redraw_mdobj(mdial, exit_obj);
		}
	}
	close_mdial(mdial);

	/* deselect old */
	set_state(layoutdial, orig_key, SELECTED, FALSE);

	/* REVERSE: uprava dialogu pro nezobrazovani OPTIONS */
	layoutdial[OPTIONS_PARENT].ob_flags &= ~HIDETREE;
	layoutdial[LAYOUT_OK].ob_y += height_offset;
	layoutdial[LAYOUT_CANCEL].ob_y += height_offset;
	layoutdial[LAYOUT_HELP].ob_y += height_offset;
	layoutdial->ob_height += height_offset;

	return new_scancode;
}

int vyber_klavesu2(int orig_scancode, char *title)
{
	MDIAL	*mdial;
	int	close = FALSE;
	int	exit_obj;
	int orig_key = scancode_to_objectnr2(orig_scancode);
	int new_scancode = orig_scancode;

	get_actual_kbd();
	newkbd(ehcdial, 2);	/* CapsLock table */

	if (/*get_flag(ehcdial, orig_key, SELECTABLE) &&*/ get_flag(ehcdial, orig_key, TOUCHEXIT))
		set_state(ehcdial, orig_key, SELECTED, TRUE);

	mdial = open_mdial(ehcdial, -1);

	/* zmen jmeno okna */
	wind_set_str(mdial->win_handle, WF_NAME, title);

	while (!close)
	{
		exit_obj = do_mdial(mdial) & 0x7fff;
		switch (exit_obj)
		{
			case EHC_OK :
				new_scancode = orig_scancode;
				close = TRUE;
				break;

			case EHC_CANCEL :
				close = TRUE;
				break;

			case EHC_HELP:
				show_help(EHC);
				break;

			default :
				{
					int key = exit_obj;
					int scancode;

					scancode = objectnr_to_scancode2(key);
					if (! scancode)
						Alert("Unknown scancode for object %d", key);

					if (scancode < 1 && scancode >= SCAN_CODES)
						break;

					if (scancode != orig_scancode) {
						/* deselect old */
						set_state(ehcdial, orig_key, SELECTED, FALSE);
						redraw_mdobj(mdial, orig_key);

						/* select new */
						set_state(ehcdial, key, SELECTED, TRUE);
						redraw_mdobj(mdial, key);
						orig_scancode = scancode;
						orig_key = key;
					}
				}
		}
		if (get_flag(ehcdial, exit_obj, EXIT)) {
			set_state(ehcdial, exit_obj, SELECTED, FALSE);
			redraw_mdobj(mdial, exit_obj);
		}
	}
	close_mdial(mdial);

	/* deselect old */
	set_state(ehcdial, orig_key, SELECTED, FALSE);

	return new_scancode;
}

void editdead_dial(void)
{
	void	*mdial;
	int	close = FALSE;
	int	exit_obj;
	int deadkey = jclk_config->deadkey;
	int defined = jclk_config->deadkeys_defined;
	char deadstr[DEADTBLLEN+1];

	strncpy(deadstr, jclk_config->deadtable1, defined); deadstr[defined] = '\0';
	set_string(deadkeysdial, DK_NORMALNI, deadstr);
	strncpy(deadstr, jclk_config->deadtable2, defined); deadstr[defined] = '\0';
	set_string(deadkeysdial, DK_SCARKOU, deadstr);
	strncpy(deadstr, jclk_config->deadtable3, defined); deadstr[defined] = '\0';
	set_string(deadkeysdial, DK_SHACKEM, deadstr);

	mdial = open_mdial(deadkeysdial, NORMAL);
	while (!close)
	{
		exit_obj = do_mdial(mdial) & 0x7fff;
		switch (exit_obj)
		{
			case DK_CHANGE :
				deadkey = vyber_klavesu2(deadkey, " Choose the Dead key ");
				break;

			case DEADKEYS_HELP:
				show_help(DEADKEYS);
				break;

			case DEADKEYS_OK :
				jclk_config->deadkey = deadkey;
				get_string(deadkeysdial, DK_NORMALNI, deadstr);
				defined = strlen(deadstr);
				if (defined > DEADTBLLEN)
					defined = DEADTBLLEN;
				strncpy(jclk_config->deadtable1, deadstr, defined);
				get_string(deadkeysdial, DK_SCARKOU, deadstr);
				strncpy(jclk_config->deadtable2, deadstr, defined);
				get_string(deadkeysdial, DK_SHACKEM, deadstr);
				strncpy(jclk_config->deadtable3, deadstr, defined);
				jclk_config->deadkeys_defined = defined;
				close = TRUE;
				break;

			case DEADKEYS_CANCEL :
				close = TRUE;
				break;
		}
		if (get_flag(deadkeysdial, exit_obj, EXIT))
			set_state(deadkeysdial, exit_obj, SELECTED, FALSE);
		if (!close)
			redraw_mdobj(mdial, exit_obj);
	}
	close_mdial(mdial);
}

int ehc_edit_dial(KAPP *client)
{
	void	*mdial;
	int	close = FALSE;
	int ret = FALSE;
	int	exit_obj;
	int dblclick;
	int starttyp;
	char cesta[2*MAXPATH], cmdline[MAXPATH], *cmdp;

	if (client == NULL)
		return FALSE;

	starttyp = client->flags & (KAPPFL_VA_START | KAPPFL_VA_START_REQ);
	strcpy(cesta, client->path);

	// oddel cestu od kommandlajny
	if ((cmdp = strchr(cesta, ' ')) != NULL) {
		strncpy(cmdline, cmdp+1, sizeof(cmdline));
		cmdline[sizeof(cmdline)-1] = '\0';
	}
	else
		cmdline[0] = '\0';

	set_popup_item(ehceditdial, EHCEDIT_POP, popups, starttyp == 0 ? PEHC_NOSUPP : (starttyp == 1 ? PEHC_SUPPORTS : PEHC_REQUIRES));

	set_state(ehceditdial, EHCEDIT_PASSTHRU, SELECTED, client->flags & KAPPFL_PASSTHROUGH);
	set_state(ehceditdial, EHCEDIT_SHIFTS, SELECTED, client->flags & KAPPFL_ALLOWSHIFTS);

	set_string(ehceditdial, EHCEDIT_APP, cesta);
	set_string(ehceditdial, EHCEDIT_CMDLINE, cmdline);

	mdial = open_mdial(ehceditdial, EHCEDIT_APP);
	while (!close)
	{
		exit_obj = do_mdial(mdial);
		dblclick = (exit_obj & 0x8000);
		exit_obj &= 0x7fff;
		switch (exit_obj)
		{
			case EHCEDIT_HELP:
				show_help(EHC_EDIT);
				break;

			case EHCEDIT_CANCEL :
				close = TRUE;
				break;

			case EHCEDIT_OK :
				{
					char tmp[2*MAXPATH];

					get_string(ehceditdial, EHCEDIT_APP, tmp);
					strcpy(client->path, tmp);
					if (strlen(client->path)) {
						strcat(client->path, " ");
						get_string(ehceditdial, EHCEDIT_CMDLINE, tmp);
						strcat(client->path, tmp);
					}

					switch(get_popup_item(ehceditdial, EHCEDIT_POP, popups, POP_AVSTART)) {
						case PEHC_NOSUPP: starttyp = 0; break;
						case PEHC_SUPPORTS:	starttyp = KAPPFL_VA_START; break;
						case PEHC_REQUIRES:	starttyp = KAPPFL_VA_START | KAPPFL_VA_START_REQ; break;
						default: starttyp = 0;
					}
					client->flags = starttyp;
					if (get_state(ehceditdial, EHCEDIT_PASSTHRU, SELECTED))
						client->flags |= KAPPFL_PASSTHROUGH;
					if (get_state(ehceditdial, EHCEDIT_SHIFTS, SELECTED))
						client->flags |= KAPPFL_ALLOWSHIFTS;
				}
				close = TRUE;
				ret = TRUE;
				break;

			case EHCEDIT_DELETE:
				*client->path = '\0';
				close = TRUE;
				ret = TRUE;
				break;

			case EHCEDIT_POP:
				handle_popup(ehceditdial, EHCEDIT_POP, popups, POP_AVSTART, POP_OPEN);
				break;

			case EHCEDIT_APP:
				if (! dblclick) {
					set_state(ehceditdial, exit_obj, SELECTED, FALSE);
					redraw_mdobj(mdial, exit_obj);
					change_mdedit(mdial, EHCEDIT_APP);	/* single click for setting focus */
					continue;
				}

				/* double click for invoking fileselector box */
				get_string(ehceditdial, exit_obj, cesta);
				if (*cesta)
					split_filename(cesta, ehcpath, ehcname);

				if (select_file(ehcpath, ehcname, CLOCKEXT, SELECTEHC, NULL)) {
					if (strlen(ehcname)) {
						strcpy( ehcfname, ehcpath );
						strcat( ehcfname, ehcname );
						if (! file_exists(ehcfname))
							*ehcfname = '\0';
					}
					else
						*ehcfname = '\0';		/* make it empty */

					set_string(ehceditdial, exit_obj, ehcfname);
				}
				break;
		}

		if (get_flag(ehceditdial, exit_obj, EXIT))
			set_state(ehceditdial, exit_obj, SELECTED, FALSE);
		if (!close)
			redraw_mdobj(mdial, exit_obj);
	}
	close_mdial(mdial);

	return ret;
}

void ehc_dial(void)
{
	MDIAL	*mdial;
	int exists = FALSE;
	int	close = FALSE;
	int save = FALSE;
	int	exit_obj;
	const int max_pocet_clientu = 128;
	int pocet_clientu = 0;
	char title[70];
	int title_len;

	KAPP *clienti = calloc(max_pocet_clientu, sizeof(KAPP));
	if (clienti == NULL)
		return;

	/* load */
	if (homedefaultsdir) {
		strcpy(jclktoolsetpath, homedefaultsdir);
		strcat(jclktoolsetpath, "\\"JCLKTOOLSETNAME);
		exists = file_exists(jclktoolsetpath);
	}

	if (!exists && homedir) {
		strcpy(jclktoolsetpath, homedir);
		strcat(jclktoolsetpath, "\\"JCLKTOOLSETNAME);
		exists = file_exists(jclktoolsetpath);
	}

	if (!exists) {
		strcpy(jclktoolsetpath, "C:\\"JCLKTOOLSETNAME);
		jclktoolsetpath[0] = root_drive;
		exists = file_exists(jclktoolsetpath);
	}

	if (exists) {
		FILE *f = fopen(jclktoolsetpath, "rt");
		char radek[2*MAXPATH];
		if (f == NULL) {
			free(clienti);
			return;
		}
		while(! feof(f)) {
			char *ptr;
			int scancode, flags;
			char name[2*MAXPATH];
			fgets(radek, sizeof(radek)-1, f);
			/* zahod vse od # do konce radku */
			ptr = strchr(radek, '#');
			if (ptr != NULL)
				*ptr = '\0';
			/* roztokenizuj */
			if (sscanf(radek, "%x %x %s", &scancode, &flags, name) != 3)
				continue;

			clienti[pocet_clientu].scancode = scancode;
			clienti[pocet_clientu].flags = flags;
			strcpy(clienti[pocet_clientu].path, name);
			pocet_clientu++;
		}
		fclose(f);
	}
	else {
		if (homedefaultsdir != NULL && path_exists(homedefaultsdir)) {
			strcpy(jclktoolsetpath, homedefaultsdir);
			strcat(jclktoolsetpath, "\\"JCLKTOOLSETNAME);
		}
		else if (homedir != NULL && path_exists(homedir)) {
			strcpy(jclktoolsetpath, homedefaultsdir);
			strcat(jclktoolsetpath, "\\"JCLKTOOLSETNAME);
		}
		else {
			strcpy(jclktoolsetpath, "C:\\"JCLKTOOLSETNAME);
			jclktoolsetpath[0] = root_drive;
		}
		do_walert(1, 0, "[1][JCLKTOOL.SET not found. |Will be created.][OK]", "New JCLKTOOL.SET");
	}

	get_actual_kbd();	/* pro prevod scan->ascii */
	newkbd(ehcdial, 0);

	// oznacit pouzite hotkeje
	{
		int i;
		for(i=0; i<pocet_clientu; i++) {
			int scan = clienti[i].scancode;
			int orig_key = scancode_to_objectnr2(scan);
			set_state(ehcdial, orig_key, SELECTED, TRUE);
		}
	}

	mdial = open_mdial(ehcdial, -1);

	/* dopln jmeno okna o cestu k JCLKTOOL.SET souboru */
	strcpy(title, " External Hotkeys (");
	title_len = strlen(title);
	if (title_len < sizeof(title)-1) {
		make_shortpath(jclktoolsetpath, title+title_len, sizeof(title) - title_len-1);
		strcat(title, ")");
		wind_set_str(mdial->win_handle, WF_NAME, title);
	}

	while (!close)
	{
		exit_obj = do_mdial(mdial) & 0x7fff;
		switch (exit_obj)
		{
			case EHC_HELP:
				show_help(EHC);
				break;

			case EHC_CANCEL :
				close = TRUE;
				break;

			case EHC_OK :
				close = TRUE;
				save = TRUE;
				break;

			default:
			{
				int scancode = objectnr_to_scancode2(exit_obj);
				int i;

				if (scancode) {
					if (get_state(ehcdial, exit_obj, SELECTED)) {
						for(i=0; i<pocet_clientu; i++) {
							if (clienti[i].scancode == scancode) {
								if (ehc_edit_dial(clienti + i) && !strlen(clienti[i].path)) {
									// delete hotkey
									set_state(ehcdial, exit_obj, SELECTED, FALSE);
									memmove(clienti + i, clienti + i + 1, sizeof(clienti[0]) * (pocet_clientu - i - 1));
									pocet_clientu--;
								}
								break;
							}
						}
					}
					else {
						// create a new hotkey
						if (pocet_clientu < max_pocet_clientu) {
							clienti[pocet_clientu].scancode = scancode;
							set_state(ehcdial, exit_obj, SELECTED, TRUE);
							redraw_mdobj(mdial, exit_obj);
							if (ehc_edit_dial(clienti + pocet_clientu) && strlen(clienti[pocet_clientu].path) > 0)
								pocet_clientu++;
							else
								set_state(ehcdial, exit_obj, SELECTED, FALSE);
						}
					}
				}
			}

		}

		if (get_flag(ehcdial, exit_obj, EXIT))
			set_state(ehcdial, exit_obj, SELECTED, FALSE);
		if (!close)
			redraw_mdobj(mdial, exit_obj);
	}
	close_mdial(mdial);

	if (save) {
		FILE *f = fopen(jclktoolsetpath, "wt");	/* !!! */
		int i;
		if (f == NULL) {
			/* !!! error saving */
			Alert("Can't open '%s' |for writting.", jclktoolsetpath);
			free(clienti);
			return;
		}
		{
			time_t timer;
			struct tm *tblock;
			timer = time(NULL);
			tblock = localtime(&timer);
			fprintf(f, "# JCLKTOOL.SET generated by SETCLOCK IIIø on %s\n", asctime(tblock));
		}

		for(i=0; i < pocet_clientu; i++) {
			KAPP *client = &(clienti[i]);
			if (strlen(client->path))
				fprintf(f, "0x%02x 0x%x %s\n", client->scancode, client->flags, client->path);
		}
		fclose(f);

		{
			int ap_id = appl_find("JCLKTOOL");
			if (ap_id > 0) {
				short msg[8];
				msg[0] = 1024;
				msg[1] = gl_apid;
				msg[2] = 1;	/* means: re-read JCLKTOOL.SET */
				msg[3] = msg[4] = msg[5] = msg[6] = msg[7] = 0;
				appl_write(ap_id, 16, msg);

				force_redraw_win(win_handle);
			}
		}
	}

	free(clienti);
}

/* predtim musis zavolat get_actual_kbd() */
int get_key_name(int scancode, char *name)
{
	int a;

	if (scancode < 1 || scancode >= SCAN_CODES)
		return FALSE;

	a = tmp_kbd[scancode + 2*SCAN_CODES];	/* CAPSLOCK tabulka */
	if (scancode == 0x01)
		strcpy(name, "Esc");
	else if (scancode == 0x0e)
		strcpy(name, "Bsp");
	else if (scancode == 0x0f)
		strcpy(name, "Tab");
	else if (scancode == 0x1c)
		strcpy(name, "Ret");
	else if (scancode == 0x53)
		strcpy(name, "Del");
	else if (scancode == 0x52)
		strcpy(name, "Ins");
	else if (scancode == 0x47)
		strcpy(name, "Clr");
	else if (scancode == 0x48)
		strcpy(name, "Aup");
	else if (scancode == 0x50)
		strcpy(name, "Adn");
	else if (scancode == 0x4b)
		strcpy(name, "Ale");
	else if (scancode == 0x4d)
		strcpy(name, "Ari");
	else if (scancode == 0x61)
		strcpy(name, "Und");
	else if (scancode == 0x62)
		strcpy(name, "Hel");
	else if (scancode >= 0x3b && scancode <= 0x44)
		sprintf(name, "F%d", scancode-0x3a);
	else if (scancode == 0x72)
		strcpy(name, "Ent");
	else if (a > 0)
		sprintf(name, " %c ", a);
	else
		sprintf(name, "N/A");

	return TRUE;
}

void icc_dial(void)
{
	void	*mdial;
	int	close = FALSE;
	int	exit_obj;
	int objs[]={HOTKEY1, HOTKEY2, HOTKEY3, HOTKEY4, HOTKEY5, HOTKEY6, HOTKEY7, HOTKEY8, HOTKEY9, HOTKEY10, HOTKEY11, HOTKEY12, HOTKEY13, HOTKEY14, HOTKEY15, HOTKEY16};
	int hotkeys[HOTKEYS];
	int i;
	char tmp[6];

	get_actual_kbd();	/* volej vzdy pred get_key_name() */
	newkbd(ehcdial, 2);	/* CapsLock tabulka */

	for(i=0; i<HOTKEYS; i++) {
		TEDINFO *ted = (TEDINFO *)get_obspec(inthotkeysdial, objs[i]);
		hotkeys[i] = jclk_config->hotkeys[i];
		if (! get_key_name(hotkeys[i], tmp))
			strcpy(tmp, "   ");
		ted->te_font = (tmp[0] != ' ') ? 5 : 3;	/* upravit velikost textu */
		set_string(inthotkeysdial, objs[i], tmp);
	}

	set_state(inthotkeysdial, INTHOTKEY_APPLY, DISABLED, ! edit_resident_clocky);

	mdial = open_mdial(inthotkeysdial, -1);
	while (!close)
	{
		exit_obj = do_mdial(mdial) & 0x7fff;
		switch (exit_obj)
		{
			case INTHOTKEY_HELP:
				show_help(INTHOTKEYS);
				break;

			case INTHOTKEY_CANCEL :
				close = TRUE;
				break;

			case INTHOTKEY_OK :
				close = TRUE;
				/* passthru */

			case INTHOTKEY_APPLY :
				for(i=0; i<HOTKEYS; i++)
					jclk_config->hotkeys[i] = hotkeys[i];

				break;

			default:
				for(i=0; i<HOTKEYS; i++) {
					if (exit_obj == objs[i]) {
						int orig_key = hotkeys[i];
						int new_key = vyber_klavesu2(orig_key, " Choose new Hotkey ");
						if (new_key != orig_key) {
							TEDINFO *ted = (TEDINFO *)get_obspec(inthotkeysdial, exit_obj);
							hotkeys[i] = new_key;
							get_key_name(new_key, tmp);
							ted->te_font = (tmp[0] != ' ') ? 5 : 3;	/* upravit velikost textu */
							set_string(inthotkeysdial, exit_obj, tmp);
						}
						break;
					}
				}
		}

		if (get_flag(inthotkeysdial, exit_obj, EXIT) || get_flag(inthotkeysdial, exit_obj, TOUCHEXIT))
			set_state(inthotkeysdial, exit_obj, SELECTED, FALSE);
		if (!close)
			redraw_mdobj(mdial, exit_obj);
	}

	close_mdial(mdial);
}


/* --------------------------------------------------------------------------- */

void handle_menu(int title, int item)
{
	switch (item)
	{
		case MABOUT :
			about_dial();
			break;

		case MRESIDENT :
			read_resident_data(! edit_resident_clocky);
			break;

		case MLOAD :
			select_and_load_data();
			break;

		case MSAVE :
			save_data(clkfname);
			break;

		case MSAVEAS :
			save_data_as();
			break;

		case MHELP :
			show_help(0);
			break;

		case MEND :
			quit = TRUE;
			break;

		case MVEKTORY :
			simple_mdial(vektorydial, 0);
			break;

		case MKBDAMYS :
			keyboard_dial();
			break;

		case MDISPLAY :
			display_dial();
			break;

		case MTIMEDATE :
			timedate_dial();
			break;

		case MSAVER :
			saver_dial();
			break;

		case MPRINTER :
			simple_mdial(konverzedial, 0);
			break;

		case MMISC :
			misc_dial();
			break;

/*
		default:
			if (title == TSCUT)
			{
				char str[50], s2[50];

				get_string(menu, item, str);
				sprintf(s2, "[1][%s][OK]", str);
				if (modal)
					do_alert(1, 0, s2);
				else
					do_walert(1, 0, s2, "MenÅ: Shortcuts");
			}
			break;
*/
	}
	menu_tnormal(menu, title, 1);
}

void print_informations(int x, int y, GRECT redrawsize)
{
	int cwc,chc,cwb,chb,d;
	int id=1;		/* system font */
	int pts=10;		/* standard size */
	int i;
	vst_font(vdi_handle, id);
	vst_point(vdi_handle, pts, &cwc, &chc, &cwb, &chb);
	vst_alignment(vdi_handle, TA_LEFT, TA_TOP, &d, &d);
	vswr_mode(vdi_handle, MD_REPLACE);
	vst_color(vdi_handle, 1);

	for(i=0; i<4; i++) {
		v_gtext(vdi_handle, x+4, y, infos[i]);
		y+=chb;
	}
}

void redraw_win(int handle, int xc, int yc, int wc, int hc)
{
	GRECT	t1,t2;
	int	temp[4];

	collect_infos();

	hide_mouse();
	wind_update(TRUE);
	t2.g_x = xc; t2.g_y = yc; t2.g_w = wc; t2.g_h = hc;
	wind_get_grect(handle, WF_FIRSTXYWH, &t1);
	vsf_color(vdi_handle, 0);

	wind_get(handle, WF_WORKXYWH, &temp[0], &temp[1], &temp[2], &temp[3]);
	temp[2] += temp[0] - 1;
	temp[3] += temp[1] - 1;

	while (t1.g_w && t1.g_h)
	{
		if (rc_intersect(&t2, &t1))
		{
			set_clipping(vdi_handle, t1.g_x, t1.g_y, t1.g_w, t1.g_h, TRUE);
			v_bar(vdi_handle, temp);	/* clear background */
			print_informations(temp[0], temp[1], t1);
		}
		wind_get(handle, WF_NEXTXYWH, &t1.g_x, &t1.g_y, &t1.g_w, &t1.g_h);
	}
	wind_update(FALSE);
	show_mouse();
}

void open_win(void)
{
	int	work_out[57];
	GRECT	r = {10, 50, 620, 150};

	vdi_handle = open_vwork(work_out);
	win_handle = wind_create_grect((NAME|MOVER|CLOSER|SIZER), &gl_desk);
	set_window_name(" SetClock ");
	wind_open_grect(win_handle, &r);
}

void win_msg(int *msg)
{
	if (msg[3] == win_handle)
	{
		switch (msg[0])
		{
			case WM_CLOSED :
				wind_close(win_handle);
				break;
			case WM_BOTTOMED:
				wind_set(win_handle, WF_BOTTOM, 0, 0, 0, 0);
				break;
			case WM_REDRAW:
				redraw_win(win_handle, msg[4], msg[5], msg[6], msg[7]);
				break;
			case WM_NEWTOP:
			case WM_TOPPED:
				wind_set(win_handle, WF_TOP, 0, 0, 0, 0);
				break;
			case WM_MOVED:
				wind_set(win_handle, WF_CURRXYWH, msg[4], msg[5], msg[6], msg[7]);
				break;
			case WM_SIZED:
				wind_set(win_handle, WF_CURRXYWH, msg[4], msg[5], msg[6], msg[7]);
				break;
		}
	}
}

static void handle_msg(int *msg)
{
	if (!message_wdial(msg))
	{
		switch (msg[0])
		{
			case MN_SELECTED:
				handle_menu(msg[3], msg[4]);
				break;

			case WM_CLOSED :
			case WM_BOTTOMED:
			case WM_REDRAW:
			case WM_NEWTOP:
			case WM_TOPPED:
			case WM_MOVED:
			case WM_SIZED :
				win_msg(msg);
				break;

			case AP_TERM :
				quit = TRUE;
				break;
		}
	}
}

/* --------------------------------------------------------------------------- */

int main( int argc, char *argv[] )
{
	// OBJECT	*tree;
	char	tmp[80];
	int		version = SETCLOCK_VERSION;
	long *ssp;
	short *syshdr;

#ifdef DEBUG
	debug_init("SETCLOCK", Datei, "setclock.log");
#endif

	debug("Begin:\n");
	ssp = (long *)Super(0L);
	debug("In Super(0L)\n");
	syshdr = *(short **)0x4f2UL;
	debug("After syshdr\n");
	Super(ssp);
	debug("In Usermode again\n");

	OS_version = syshdr[1];
	debug("OS_version read\n");

	// CHECK_CLOCKY_STRUCT;

	// debug_init("SetClockIII", Con, "emilek");

	root_drive = 'C';
	if (Dgetdrv() == 0)	{	/* asi mame jen disketovou jednotku a jsme bez harddisku */
		root_drive = 'A';
	}
	clkpath[0] = root_drive;
	debug("root drive set\n");

	if (argc == 2 && argv[1] != NULL) {
		strcpy( clkfname, argv[1] );
		debug("cmdline arg copied\n");
	}

	getcwd(kbdpath, sizeof(kbdpath)-5);
	debug("cwd read\n");

	homedir = getenv("HOME");
	debug("$HOME = '%s'\n", homedir);
	if (homedir != NULL && path_exists(homedir)) {
		strcpy(homedefaultsdir, homedir);
		strcat(homedefaultsdir, "\\defaults");
		debug("path_exists(%s)\n", homedefaultsdir);
		if (! path_exists(homedefaultsdir))
			homedefaultsdir = NULL;
	}
	else
		homedir = NULL;

	/* RSC init */
	debug("before RSC read\n");
	init_app("setclock.rsc");
	debug("RSC read succesfully\n");

	if (gl_naes)
		menu_register(gl_apid, "  SetClock IIIø");

	debug("Before RSC init\n");
	rsrc_gaddr(R_TREE, MENUTREE, &menu);
	create_menu(menu);
//	menu_icheck(menu, MMODAL, modal);
//	menu_icheck(menu, MFENSTER, !modal);

/* ------------------- */
	rsrc_gaddr(R_TREE, ABOUT, &aboutdial);
	fix_dial(aboutdial);

	sprintf(tmp, "%x.%02x", version / 0x100, version % 0x100);
	set_string(aboutdial, A_VERSION, tmp);
	strcpy(tmp, __DATE__);
	set_string(aboutdial, A_DATE, tmp);
	get_patchlev(__Ident_cflib, tmp);
	set_string(aboutdial, A_PL, tmp);

/* ------------------- */
	rsrc_gaddr(R_TREE, VEKTORY, &vektorydial);
	fix_dial(vektorydial);

/* ------------------- */
	rsrc_gaddr(R_TREE, MISC, &miscdial);
	fix_dial(miscdial);

/* ------------------- */
	rsrc_gaddr(R_TREE, KEYBOARD, &keyboarddial);
	fix_dial(keyboarddial);

/* ------------------- */
	rsrc_gaddr(R_TREE, DISPLAY, &displaydial);
	fix_dial(displaydial);

/* ------------------- */
	rsrc_gaddr(R_TREE, TIMEDATE, &timedatedial);
	fix_dial(timedatedial);

/* ------------------- */
	rsrc_gaddr(R_TREE, SAVER, &saverdial);
	fix_dial(saverdial);

/* ------------------- */
	rsrc_gaddr(R_TREE, KONVERZE, &konverzedial);
	fix_dial(konverzedial);

/* ------------------- */
	rsrc_gaddr(R_TREE, POPUPS, &popups);
	fix_menu(popups);

/* ------------------- */
	rsrc_gaddr(R_TREE, LAYOUT, &layoutdial);
	fix_dial(layoutdial);

/* ------------------- */
	rsrc_gaddr(R_TREE, DEADKEYS, &deadkeysdial);
	fix_dial(deadkeysdial);

/* ------------------- */
	rsrc_gaddr(R_TREE, EHC, &ehcdial);
	fix_dial(ehcdial);

/* ------------------- */
	rsrc_gaddr(R_TREE, EHC_EDIT, &ehceditdial);
	fix_dial(ehceditdial);

/* ------------------- */
	rsrc_gaddr(R_TREE, INTHOTKEYS, &inthotkeysdial);
	fix_dial(inthotkeysdial);

	/* RSC init END */
	debug("After RSC init\n");

	/* Callback fÅr modale Fensterdialoge, Fenster-Alerts usw. */
	set_mdial_wincb(handle_msg);
	debug("Callback set\n");

	open_win();
	debug("Main Window opened\n");

	set_asciitable_strings("Choose a char", "Cancel");
	debug("ASCII table set\n");

	if (*clkfname) {
		debug("Before load_data\n");
		load_data(clkfname);		/* zkus nacist co je na prikazove radce */
		debug("After load_data\n");
	}
	if (!clocky_in_buffer) {
		debug("Before read_resident\n");
		read_resident_data(TRUE);
		debug("After read_resident\n");
	}

	quit = FALSE;

	while (!quit)
	{
		mbutton = 0;
		event = evnt_multi(MU_MESAG|MU_BUTTON|MU_KEYBD,
									1, 1, 1,
									0, 0, 0, 0, 0,
									0, 0, 0, 0, 0,
									msg,
									0,
									&msx, &msy, &mbutton, &kstate,
									&kreturn, &mclick);

		if (event & MU_MESAG)
			handle_msg(msg);

		if (event & MU_BUTTON)
		{
			if (!click_wdial(mclick, msx, msy, kstate, mbutton))
				;
		}
		if (event & MU_KEYBD)
		{
			if (!key_wdial(kreturn, kstate))
			{
				int	title, item;

				if (is_menu_key(kreturn, kstate, &title, &item))
					handle_menu(title, item);
			}
		}
	}

	wind_close(win_handle);
	wind_delete(win_handle);
	v_clsvwk(vdi_handle);

	delete_menu();
	exit_app(0);
#ifdef DEBUG
	debug_exit();
#endif
	return 0;
}
