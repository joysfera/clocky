XBRA	= 'XBRA'
IDENTIFIER= 'JCLK'	identifik†tor v XBRA, Cookie i datovÇ struktury
VERSION	= $300	verze datovÇ struktury

ENGLISH	= 1	anglicka verze Clocku

DELKA_DNE	= 300	sekund
DELKA_1_RADKU	= 11	tolik znakñ m† mñj áasovò £daj na ®°©ku MAXim†lnà (kr†t dva)
MAX_POCET_LINEK	= 17	1+2*8 nebo 1+16 ©†dkñ m† mñj áasovò £daj na vò®ku MAXim†lnà
MAXCLKDEPTH	= 32	zkus°me z†lohovat aë po 32-bitovou grafiku (True Color)
SYSFNT_HEIGHT	= 16	pro porovnani jsme-li v 320x200 nebo ne
SECONDLINE_OFFSET	= 8	offset v linkach mezi prvnim a druhym vypisem
REFRESH	= 300	asi 6 sekund na registraci zmàn p©i startu GEMu

MOUSE	= 16
IKBD	= 32
_v_bas_ad	= $44E
conterm	= $484
savptr	= $4A2
_hz_200	= $4BA
_sysbase	= $4F2
_bconout	= $57e
_longframe= $59E
_p_cookies= $5A0
_videosync= $FF820A
_STacyvid	= $FF827E
_MegaSTe	= $FF8E21
_parallel	= $FFFA01
_SCC_A	= $FFFF8C81
_SCC_B	= $FFFF8C85
NOVA_REG	= $B01EEC		NOVA Falcon
hss	= $ffff828c
hht	= $ffff8282

* SM124 = Falcon ? ff8006 == 0 : ff8260 == 2
* VGA = Falcon && ff8006 == 2
* SCC = (_MCH == 1 && _MCHlo == $20) || _MCH == 2 || _MCH == 3
* MFP = !Falcon

*
_InvColor	= 0	invertovat barvy p©i bootu
_EngSys	= 1	emulovat anglickò systÇm
_XBSettime= 2	sledovat XBIOS Settime
_XBFixY2K	= 3	opravit chybu Y2K v XBIOSovÇm Gettime

_HookPrint= 11	povàsit se na Print a t°m umoënit konverzi
_HookMouse= 12	povàsit se na Mouse a t°m umoënit zrychlovaá
_HookKbd	= 13	povàsit se na KbdVec a t°m umoënit horkÇ/EHC/Alt/dead kl†vesy
_HookVBL	= 14	povàsit se na VBL a umoënit t°m zobrazov†n° áasu
_HookXBIOS= 15	povàsit se na XBIOS

* verejna struktura Joy Clocku, na kterou ukazuje CookieJar('JCLK') *
*-------------------------------------------------------------------*
* pojmenuj jednotlive bity v Cookie strukture

* 24-31 - definuje zobrazeni udaju na obrazovce
_ShowTime	= 31	zobrazovat na obrazovku (zavisi 30-24)
_ShowIDT	= 30	ridit se nastavenim v IDT cookie
_Showdat	= 29	zobrazovat i datum
_Showden	= 28	zobrazovat den v tydnu a symboly
_Showrok  = 27	zobrazovat rok a sekundy
_Showdigi	= 26	pouzit digitalni cisla
_ShowBig	= 25	pouzit plnou vysku systemoveho fontu
_ShowTrn	= 24	transparentni zobrazeni

* 16-23 - definuje akce klavesnice
_KbdAltK	= 23	©e®it Alt- a Alt-Shift kl†vesy
_KbdEHC	= 22	povolit Alt-Control pujcovani klaves
_Kbddead	= 21	povolit mrtve klavesy
_Kbdasci	= 20	povolit Alt-numpad ASCII kody
_Kbdcink  = 19	zakazat klikani klavesnice
_Kbdbell	= 18	zakazat zvonek klavesnice
* keyboard layout [16,17] - originalni, normalni, ceska klavesnice

* 8-15 - definuje ruzne pridavne veci
_Miscm4x	= 15	ctyrnasobny/dvojnasobny zrychlovac mysi
_Miscmys	= 14	zrychlovac mysi aktivni (zavisi 15)
_Miscprnt	= 13	Latin2 konverze na printer zapnuta
_Miscturb	= 12	turbo MegaSTE zapnuto
_Misctut	= 11	casove znameni kazdou hodinu
_unused2	= 10
_unused3	= 9
_unused4	= 8

* 0-7 - definuje vlastnosti Screen saveru
_Saveron	= 7	zapnout screen saver (zavisi savecount, 6-3)
_SaveMod1	= 6	kontrolovat CD Modemu1
_SaveMod2 = 5	kontrolovat CD Modemu2
_SaveSer2	= 4	kontrolovat CD Serialu2
_SaveVESA	= 3	tvrde vypnout obraz
_SaveSTacy= 2	STacy LED

* currently unused 0,1,2

	COMMENT	HEAD=7

*************************************************
zacatek	bra	init

	dc.l	XBRA,IDENTIFIER
ikbd_jmp	dc.l	0

my_ikbd	tst.b	d0
	bmi	ikbd_rts		pro released key do nothing
	
	bsr	roznout		klavesnice vzdy rozne obraz

	movem.l	a0-a2/d1-d4,-(A7)	first my IKBD routine
	move.l	kbshift(PC),a0

* zde zaá°naj° extern° hotkeje
	btst	#_KbdEHC-16,_KBD	vybirat EH z hn°zda?
	beq.s	prijety

	lea	ehc_scantable(PC),a1
	tst.b	(a1,d0)		je tato kl†vesa registrov†na ?
	beq.s	prijety

	lea	128(a1),a1	druh† polovina EHC tabulky
	move.b	(a0),d1		p©eáti KbShift
	btst	#1,(a1,d0)	zkus flag pro SHIFT_ALLOWED
	beq.s	.shall
	and.b	#%1100,d1		nech jen Alt+Control
.shall	cmp.b	#%1100,d1		porovnej s Alt+Control
	bne.s	prijety

	move.b	d0,act_key	zap°®u scancode pr†và stisknutÇ kl†vesy do actual_key
	move.b	(a0),act_shift	zap°®u stav p©e©azovaáñ

	btst	#0,(a1,d0)	zkus flag pro PASS_THROUGH
	bne	ikbd_ret
	bra	vyhodznak		a vyhodim z bufru

* zde se vyhodnocuj° hotkeje Clockñ
prijety	move.b	hotshift(PC),d1	jsou povolenÇ intern° hotkeje Clockñ?
	beq	ikbd3		0 = nepovolenÇ
	cmp.b	(a0),d1		drë°m kombinaci pro hotkey?
	bne	ikbd3

**********************************************************************
*** HorkÇ kl†vesy (scancode v D3) - vyhodnocen°
**********************************************************************
	tst.b	is_megae		Turbo jen pro MegaSTE!
	beq.s	.hotzamegou
	cmp.b	turbo_on,d0	'+' on numeric pad
	bne.s	.hot1
	bset	#_Miscturb-8,_MSC	Turbo ON
	bra.s	hot_end
*---
.hot1	cmp.b	turbo_off,d0	'-' on numeric pad
	bne.s	.hotzamegou
	bclr	#_Miscturb-8,_MSC	Turbo OFF
	bra.s	hot_end
*---
.hotzamegou
	cmp.b	hotk_inv,d0	'B'
	bne.s	.hot2
	bsr	invertuj
	bra.s	hot_end
*---
.hot2	cmp.b	hotk_klik,d0	'K'
	bne.s	.hotcyklus
	move.b	klikmask(PC),d1
	eor.b	d1,conterm\w
	bra.s	hot_end
*---
.hotcyklus
	lea	hotkeje(PC),a1
	moveq	#8,d1		hotkeje kontroluju odzadu
hotk1	cmp.b	(a1,d1),d0
	dbeq	d1,hotk1
	tst	d1
	bmi	ikbd_ret		ë†dn† z horkòch kl†ves

*********************************
*--- je to jedna z horkych klaves

	move.w	d1,d2
	subq.w	#5,d2		O = 1, N = 2, C = 3
	ble.s	hotk3
	and.b	#%11111100,_KBD
	and.b	#%00000011,d2
	or.b	d2,_KBD		zap°®u zmànu kl†vesnice
	bra.s	hot_end
*---
hotk3	moveq	#0,d2
	move.b	hotbity(PC,d1),d2
	move.l	STR(PC),d1
	bchg	d2,d1
	move.l	d1,STR

hot_end	bra	vyhodznak
*		 0   1   2   3   4   5   6   7   8
*                   'T','D','A','S','M','L','O','N','C'
*		$14,$20,$1E,$1F,$32,$26,$18,$31,$2E
hotbity	dc.b	_ShowTime,_Kbddead,_Kbdasci,_Saveron,_Miscmys,_Miscprnt

***************************************************************
* Alt-klavesy
ikbd3	btst	#_KbdAltK-16,_KBD
	beq.s	ikbd4
	tst.b	extended_kbd	pokud je to jiz v BIOSu tak nedelat
	bne.s	ikbd4
	move.b	(a0),d1		znovunacist kvuli CapsLocku
	btst	#3,d1		drë°m si Alt ?
	beq.s	ikbd4

	move.b	_KBD(PC),d2
	and.b	#%11,d2		jen typ kl†vesnice
	cmp.b	#2,d2
	beq.s	.normal
	cmp.b	#3,d2
	bne.s	ikbd4
	lea	altklav(PC),a1
	bra.s	.zanorm
.normal	lea	altkey(PC),a1
.zanorm	btst	#4,d1		je CapsLock?
	beq.s	.necaps
	lea	2*32(a1),a1
	bra.s	.smycka
.necaps	and.b	#%0011,d1		nàkterò z Shiftñ?
	beq.s	.smycka
	lea	32(a1),a1
.smycka	moveq	#30-1,d2
.loop	subq	#1,d2		SCAN na sudòch pozic°ch
	cmp.b	(a1,d2),d0
	dbeq	d2,.loop
	tst	d2
	bmi.s	ikbd4		ë†dn† z Alt kl†ves
	move.b	1(a1,d2),d0	vyt†hni z tabulky Alt znak
ikbd4
ikbd_ret	movem.l	(SP)+,a0-a2/d1-d4
ikbd_rts	move.l	ikbd_jmp(pc),-(sp)	then continue with original ikbd_key handler
	rts

vyhodznak	movem.l	(SP)+,a0-a2/d1-d4
	rts

***************************************
	dc.l	XBRA,IDENTIFIER
mouse_jmp	dc.l	0
	btst	#_Miscmys-8,_MSC
	beq.s	mouse_ret
	movem.l	a0-a1/d0-d1,-(a7)
	lea	casovac(pc),a1
	move.b	(a0)+,d0
	cmpi.b	#$F8,d0		$F8 je my® bez tlaá°tek
	bmi.s	nezrychli
	cmpi.b	#$FB,d0		$FB je my® s obàma tlaá°tky
	bgt.s	nezrychli
	move.l	_hz_200\w,d1
	sub.l	(a1),d1
	move.l	_hz_200\w,(a1)	rozd°l od posledn° ud†losti my®i
	subq.l	#3,d1
	bcc.s	nezrychli		bylo to m°§ jak 20 ms ?

	btst	#_Miscm4x-8,_MSC
	bne.s	.ctyri

	move.b	(a0),d0
	add.b	d0,(a0)+		2x v obou smàrech
	move.b	(a0),d0
	add.b	d0,(a0)
	bra.s	nezrychli

.ctyri	move.b	(a0),d0
	add.b	d0,d0
	add.b	d0,d0
	move.b	d0,(a0)+
	move.b	(a0),d0
	add.b	d0,d0
	add.b	d0,d0
	move.b	d0,(a0)

nezrychli	movem.l	(SP)+,a0-a1/d0-d1
mouse_ret	move.l	mouse_jmp(pc),-(SP)
	rts
***************************************
	dc.l	XBRA,IDENTIFIER
kam2lat	dc.l	0
	btst	#_Miscprnt-8,_MSC
	beq.s	tiskni
	move.b	7(sp),d0
	bpl.s	tiskni
	and.w	#$7f,d0
	lea	latina(pc),a0
	move.b	(a0,d0),7(sp)
tiskni	move.l	kam2lat(pc),a0
	jmp	(a0)
***************************************
ctrl_vect	move.l	adr_mouse(pc),a0	a0 = ukazatel na strukturu ikbd
	lea	mouse_jmp+4(pc),a1	a1 = ukazatel na moji mysi funkci
	cmp.l	(a0),a1		je moje mys prvni na rade ?
	beq.s	mouse_ok            je => vsechno OK
				; kdyz neni, nekdo se nam napichl na vektor mysi (asi GEM)
	move.l	adr_oldms(pc),d0	d0 = stara hodnota ze struktury ikbd
	bne.s	kontrola		kdyz uz nejaka byla, musime zkontrolovat, jestli je to XBRA retez

	move.l	(a0),adr_oldms	jestli jeste nebyla zadna, zapamatuj aktualni a zarad do retezu
	bra.s	zaradit
kontrola	cmp.l	(a0),d0		porovnaji se adresy mysi a stare mysi
	bne.s	mouse_ok		pokud se lisi, NIC se NEDEJE !
				; kdyz jsou stejne, provede se zarazeni moji mysi
zaradit	move.l	(a0),-4(a1)	zapsat do XBRA retezu a mys opet na prvni misto
	move.l	a1,(a0)
	move.w	refresh(pc),mys_ok
mouse_ok	rts			registry obnov° rodiá
****************************************
invertuj	movem.l	a0-a1/d0-d1,-(sp)
	tst.b	is_falcon
	beq.s	.inv2

* inverze pro Falcona
	lea	$ffff9800.w,a0
	moveq	#4,d0
	move.w	bitplanes(PC),d1
	rol.w	d1,d0		takto bude fungovat 2,16 a 256 barev, ale nikoliv TC, coë je dob©e
	lea	-4(a0),a1
	add.l	d0,a1		offset zavisi na barevne hloubce
	move.l	(a0),d0
	move.l	(a1),(a0)
	move.l	d0,(a1)
	bra.s	.inv_end

* inverze pro ST
.inv2	tst.b	is_sm124
	beq.s	.inv_end
	not.w	$ffff8240.w
.inv_end	movem.l	(sp)+,a0-a1/d0-d1
	rts
****************************************
zhasnout	tst.b	zhasnuto
	bne	.zhasnute

* otestuj stupe§ zhasnut° - pokud m†lo, tak skoá p©°mo na .zhascern, jinak pokraáuj hardwarovou cestou

	tst.b	is_stacy
	beq.s	.is_falc
	btst	#_SaveSTacy,_SVR
	beq.s	.is_falc
	move.b	_STacyvid,orig_STacy_video	save original state
	btst	#_SaveVESA,_SVR
	beq.s	.soft_blank
	bset	#0,_STacyvid	vypni LCD
	bra.s	.uz_je
.soft_blank	bset	#2,_STacyvid	zhasni podsv°cen°
	bra.s	.uz_je

.is_falc	tst.b	is_falcon
	beq.s	.zhasnist
	tst.b	has_vga		jede na VGA monitoru?
	beq.s	.zhascern

	move	d0,-(sp)
	move.w	hss\w,zal_hss
	move.w	hht\w,d0
	addq	#2,d0
	move.w	d0,hss\w		VESA Powersaving mode
	move	(sp)+,d0

	tst.b	has_nova
	beq.s	.uz_je
	bset	#2,NOVA_REG	horizontal sync off
	bra.s	.uz_je

* vàtev hardwarovÇho zhasnut° pro TT a ST
.zhasnist	tst.b	is_sm124
	beq.s	.zhascern
	bset	#0,$ffff820a.w	zhasnout SM124 pomoci zapnuti externi synchronizace
	bra.s	.uz_je

.zhascern	nop			zalohuj barevne registry
	nop			vycerni barvove registry
	nop			zapis si za usi, ze's zcernal

.uz_je	st.b	zhasnuto
.zhasnute	rts
****************************************
roznout	clr.w	savecount		vynulovat pocitadlo delky svetla

	tst.b	zhasnuto
	beq.s	.roznute

	tst.b	is_stacy
	beq.s	.is_falc
	btst	#_SaveSTacy,_SVR
	beq.s	.is_falc
	move.b	orig_STacy_video,_STacyvid	restore original state

.is_falc	tst.b	is_falcon
	beq.s	.roznout1
	tst.b	has_vga
	beq.s	.rozcern

	move.w	zal_hss,hss\w
	tst.b	has_nova
	beq.s	.jiz_neni
	bclr	#2,NOVA_REG	horizontal sync on
	bra.s	.jiz_neni

.roznout1	tst.b	is_sm124
	beq.s	.rozcern
	bclr	#0,$ffff820a.w	roëni SM124 vypnut°m extern° synchronizace
	bra.s	.jiz_neni

.rozcern	nop	vrat zalohovane barvove registry
.jiz_neni	clr.b	zhasnuto
.roznute	rts
****************************************
Tutani	move.w	tut_wait,d0
	beq.s	.necekat
	subq.w	#1,d0
	move.w	d0,tut_wait
	bra.s	.po_tutu

.necekat	move.w	zatutat,d0
	beq.s	.po_tutu

	lea	tut_tab_end(pc),a0
	lea	(a0,d0),a0
	addq.w	#2,d0
	move.w	d0,zatutat
	clr.w	d1
	move.w	(a0),d1
	bmi.s	.cekaci_hodnota
	lea	$FFFF8800.w,a0
	movep.w	d1,(a0)
	bra.s	.po_tutu

.cekaci_hodnota
	and.w	#$00FF,d1
	move.w	d1,tut_wait
.po_tutu	rts

****************************************
ZvysitCas	tst.l	TIMECOOK
	bne.s	.mameDTCK
***
	move.l	abs_cas,d0
.zpozden	bsr	Inkrementuj_cas	p©iáti ruánà 1 sekundu k áasu
	add.l	#200,d0		p©iátu celou sekundu na p©°®t° áek†n°
	cmp.l	_hz_200\w,d0	uë jsme dobàhli TimerB?
	bcs.s	.zpozden
	move.l	d0,abs_cas	zapi® si to
	bra.s	.timer3
***
.mameDTCK	move.l	_hz_200\w,d0
	add.l	#200,d0
	move.l	d0,abs_cas	nejd©°v p©ipravit dal®° sekundu
	
	move.l	DATECOOK,a1
	moveq	#0,d0
	cmp.w	#0,a1
	beq.s	.datumne
	move.w	(a1),d0		"DATE"
	swap	d0
.datumne	move.l	TIMECOOK,a1
	move.w	(a1),d0		"TIME"
	cmp.l	DT_COOK,d0	porovnej s minulòm
	bne.s	.zmena
	bsr	Inkrementuj_cas	st†le nezmànàno => inkrementuj ruánà
	bra.s	.timer3
.zmena	move.l	d0,DT_COOK
	bsr	Prepar

.timer3
* t£tnout jedinà pokud sekundy=0 a minuty=0 a t£t†n° je povoleno
	btst	#_Misctut-8,_MSC
	beq.s	.netutat
	tst.b	has_yamaha
	beq.s	.netutat		nemame Yamahu => nemuzeme tutat
	tst.b	minuty
	bne.s	.netutat
	tst.b	sekundy
	bne.s	.netutat
	move.w	#(tut_table-tut_tab_end),zatutat		z†porn† dÇlka t£tac° tabulky
.netutat	rts

****************************************
ScreenSaver:
	btst	#_Saveron,_SVR	je screen saver pouzivan?
	beq.s	.scrsaver_end

* zkontrolujme Carrier Detecty modem portu
	btst	#_SaveMod1,_SVR	kontrolovat CD Modemu1?
	beq.s	.CD1kon
	tst.b	has_mfp		mame vubec MFP s CD signalem?
	beq.s	.CD1kon
	btst	#1,_parallel	jest DCD ?
	bne.s	.CD1kon
	bsr	roznout		je, takze rozni
	bra.s	.scrsaver_end
.CD1kon
	tst.b	has_scc		mame vubec SCC?
	beq.s	.CD2kon
	btst	#_SaveMod2,_SVR	kontrolovat CD Modemu2?
	beq.s	.SER2
	lea	_SCC_B\w,a0
	bsr.s	SCC_CD
	beq.s	.SER2
	bsr	roznout		je, takze rozni
	bra.s	.scrsaver_end

.SER2	btst	#_SaveSer2,_SVR	kontrolovat CD Serialu2?
	beq.s	.CD2kon
	lea	_SCC_A\w,a0
	bsr.s	SCC_CD
	beq.s	.CD2kon
	bsr	roznout		je, takze rozni
	bra.s	.scrsaver_end

.CD2kon

* zkontrolovat a pripadne zvysit pocitadlo delky dne
	move	savecount(pc),d0
	move	saverlen(pc),d1
	cmp	d1,d0		porovnej dobu svetla s nastavenou dobou setreni obrazu
	bge.s	.zhasnout
	addq	#1,d0		zvysovat counter bileho dne az dokud nebude >= saverlen
	move.w	d0,savecount
	bra.s	.scrsaver_end
.zhasnout	bsr	zhasnout		zhasnout vzdy kdyz savecount >= saverlen (napr. i kvuli JCLKTOOL, ktery nastavuje $7FFF do savecount)
				; rozne za nas az pohyb klavesnice nebo mysi (proste IKBD aktivita), protoze VBL behem tmy nejede :-/
.scrsaver_end:
	rts
***************************************
* test SCC CD
SCC_CD	move.w	sr,d1
	or.w	#$700,sr		vypni preruseni, at nam nekdo nevleze do SCC programovani
	clr.b	(a0)		! snad ten, kdo programuje SCC zaroven s nami, ma zakazane VBL preruseni, at se nam nepohadaji SCC registry
	move.b	(a0),d0
	move.w	d1,sr		zapni preruseni, at moc dlouho nezdrzujeme
	btst	#3,d0		jest DCD ?
.konec	rts			ZERO bit znamena nebylo CD
***************************************
* zobraz udaje na obrazovku
DisplayUpdate:
	move.l	kbshift(PC),a0
	move.b	(a0),d0		p©eáti KbShift
	and.b	#%1111,d0		nech jen Shifty, Control a Alternate
	move.b	hottime(PC),d1	kombinace Shiftñ pro kuknut° na Clocky
	beq.s	.nezobraz		0 = nepovolenÇ kuknut°
	cmp.b	d0,d1		je stlaáena kombinace Kukkeje?
	beq.s	.zobraz
	btst	#_ShowTime-24,STR
	beq.s	.nezobraz
.zobraz	tst.w	mys_ok		dokud se to nezmeni, tak nepis
	bne.s	.nezobraz		(starym fontem do nove obrazovky)
	bsr	Print		zobraz aktu†ln° áas
.nezobraz	rts

***************************************
	dc.l	XBRA,IDENTIFIER
	dc.l	0
VBLtimer	movem.l	A0-A6/D0-D7,-(SP)
	bsr	Do_It		kaëdou 1/50-80s udàlej v®e promànnÇ
	bsr	ctrl_vect		zkontroluj vektor my®i
	tst.w	mys_ok		do®lo ke zmànà ?
	beq.s	.test_OK		ne...
	subq.w	#1,mys_ok		ano, odpoá°t†v†m REFRESH na inicializaci
	bne.s	.test_OK		TSR programñ a ACCessory váetnà XCONTROLu...

	bsr	Sys_Init		pokud uë je áas, zjist°m zmànu GEMovòch parametrñ a p©epoá°t†m Print_Size
	bsr	SaveBkg		uloëit novÇ pozad°

.test_OK	bsr	Tutani
	move.l	abs_cas,d0
	cmp.l	_hz_200\w,d0
	bcc.s	.tim_end		je®tà neuplynula cel† sekunda

* zde chodime az kazdou celou sekundu
	bsr	ZvysitCas
	bsr	ScreenSaver
	bsr.s	DisplayUpdate

* konec rutiny casovace
.tim_end	movem.l	(SP)+,A0-A6/D0-D7
	rts

***************************************
* Ruán° inkrementace áasu ©°zen† p©es 200Hz áasovaá
Inkrementuj_cas:
	lea	sekundy(pc),a1
	addq.b	#1,(a1)		inkrementuj sekundy
	cmp.b	#60,(a1)		pokud je to ®edes†t†
	blt.s	.konec
	clr.b	(a1)		vynuluj sekundy
	addq.b	#1,-(a1)		inkrementuj minuty
	cmp.b	#60,(a1)		pokud je to ®edes†t†
	blt.s	.konec
	clr.b	(a1)		vynuluj minuty
	addq.b	#1,-(a1)		inkrementuj hodiny
	cmp.b	#24,(a1)		pokud je to áty©iadvac†t†
	blt.s	.konec
	clr.b	(a1)		vynuluj hodiny
	addq.b	#1,-(a1)		inkrementuj dny
	move.b	(a1),d1		p©eáti dny
	lea	mesice(pc),a2
	clr	d2
	move.b	(a2),d2		... màs°ce
	lea	roky(pc),a3
	move.b	(a3),d3		... roky
	lea	tabmoon(pc),a4
	cmp.b	-1(a4,d2),d1	pokud je to den v dal®°m màs°ci
	blt.s	.day_over
	cmp.b	#2,d2		je to £nor ?
	bne.s	.neunor
	and.w	#03,d3
	bne.s	.neunor		je to p©estupnò rok ?
	cmp.b	#29,d1		pak je 29. £nor povolen
	beq.s	.day_over
.neunor	move.b	#1,(a1)		1. den v dal®°m màs°ci
	addq.b	#1,(a2)		inkrementuj màs°c
	cmp.b	#12,(a2)		pokud je dvan†ctò
	blt.s	.day_over
	move.b	#1,(a2)		1. màs°c v dal®°m roce
	addq.b	#1,(a3)		inkrementuj rok
	cmp.b	#100,(a3)		pokud je stò
	blt.s	.day_over
	clr.b	(a3)		tak je to rok 2000! :)
.day_over	bsr	denvtydnu
.konec	rts
***************************************
Compute_print_size:
	movem.l	d0-d1,-(sp)

	clr.b	dva_radky		p©edpokl†dejme vòstup do jednoho ©†dku
	move.l	fnt8x8(pc),showfont	a pouëit° malÇ znakovÇ sady
	move.w	#8,clk_lines	VíDY ZDE BùVALO #7 !!!!!!!
	move.w	v_cel_ht(pc),d0
	cmp	#SYSFNT_HEIGHT,d0	je velikost systÇmovÇho fontu 16 nebo v°ce bodñ?
	blt.s	.col0		ne, takëe nem†me fnt8x8 a mus°me do jednoho ©†dku
	btst	#_ShowBig-24,STR	velkòm fontem?
	bne.s	.velkym
	st.b	dva_radky		tak malòm fontem mñëeme do dvou ©†dkñ
	bra.s	.col0
.velkym	move.l	fntbig(pc),showfont	pouëijme velkou znakovou sadu
	move.w	#16,clk_lines

.col0	moveq	#DELKA_1_RADKU,d0
	btst	#_Showrok-24,STR
	bne.s	.col1
	subq	#3,d0
.col1	btst	#_Showden-24,STR
	bne.s	.col2
	subq	#3,d0
.col2	btst	#_Showdat-24,STR
	beq.s	.col3
	tst.b	dva_radky		bude datum do dvou ©†dkñ?
	bne.s	.col3
; piseme oboje do jednoho radku
	add	d0,d0		dvakrat sirsi
	addq	#1,d0		mezera mezi datumem a casem
.col3	move.w	d0,clk_cols

; compute xpos from xcol
	move.w	bitplanes,d1
	move.w	byt_lin,d0	®°©ka obrazovky v bajtech
	divu	d1,d0		d0 je poáet sloupcñ obrazovky
	sub.w	clk_cols,d0	posun o sirku vypisu doleva

	move.w	showpos,d1	kladnÇ á°slo je posun z prava, z†pornÇ z leva
	bpl.s	.z_prava
	neg.w	d1		zmànit na kladnÇ
	subq.w	#1,d1		-1 = levò roh
	cmp.w	d1,d0		nen° za pravò roh?
	bcs.s	.pos_ok
	move.w	d1,d0
	bra.s	.pos_ok

.z_prava	sub.w	d1,d0		posun o volitelnou vzdalenost, v podstate si uzivatel vybere pozici vypisu
	bpl.s	.pos_ok
	moveq	#0,d0		moc vlevo => zustat vlevo
.pos_ok	move	d0,clk_xpos	sloupec polohy hodin
	movem.l	(sp)+,d0-d1
	rts

***************************************
**** pouzite registry:
***** D0 pro predavani parametru
***** D2 je aktualni sloupec pri tisku
***** D3-D6 jsou pouzite v jednotlivych rutinach pro pomocne ucely
***** D7 je oddelovac datumu
Print	move.l	_v_bas_ad\w,A5	adresa videoram

	move.w	showpos,d0
	cmp.w	B_showpos,d0	nezmànila se pozice vòpisu
	beq.s	.reco
	st.b	recompute
	move.w	showpos,B_showpos

.reco	tst.b	recompute
	beq.s	.prnt

	clr.b	recompute
	bsr	RestBkg		vratit pozadi, bylo-li ulozene
	sne	d0		zapamatovat si, ma-li se pozadi znovu ulozit
	bsr	Compute_print_size	prepocitat
	tst.b	d0		bylo ulozene pozadi?
	beq.s	.prnt
	bsr	SaveBkg		ulozit nove rozmery

.prnt	move	clk_xpos(PC),D2	poá†teán° sloupec

	btst	#_Showdat-24,STR
	beq	Pis_cas

* pis DATUM
pis_datum	btst	#_Showden-24,STR
	beq.s	.za_tydnem

	lea	d_v_t(PC),A2	uloëenò den v tòdnu
	lea	tyden(PC),A3	tabulka PoótSt ...
	moveq	#0,D0
	move.b	(A2),D0
	add.l	D0,D0
	add.l	D0,A3
	move.b	(A3)+,D0
	bsr	Putchr		den
	move.b	(A3),D0
	bsr	Putchr		v tydnu
	moveq	#' ',D0		za dnem v tòdnu mezera
	bsr	Putchr

.za_tydnem:
	lea	dny(PC),A2	uloëenò datum
	moveq	#'.',d7		date separator
	btst	#_ShowIDT-24,STR
	beq.s	.jed
	move.l	#'_IDT',d0
	bsr	GetCookie
	bne.s	.jed
	move.b	d0,d7		international date separator

	btst	#12,d0		12 vs 24 hours
	seq	ampm

	asr.w	#8,d0
	and.w	#3,d0
	beq.s	.mmddyy
	cmp.w	#1,d0
	beq.s	.jed

	st.b	YY_sep_za		separator za
	bsr.s	put_YY
	bsr.s	put_MM
	bsr.s	put_sep
	bsr.s	put_DD
	bra.s	konec_datumu

.mmddyy	bsr.s	put_MM
	bsr.s	put_sep
	bsr.s	put_DD
	clr.b	YY_sep_za		separator pred
	bsr.s	put_YY
	bra.s	konec_datumu

.jed	bsr.s	put_DD
	bsr.s	put_sep
	bsr.s	put_MM
	clr.b	YY_sep_za		separator pred
	bsr.s	put_YY
	bra.s	konec_datumu

put_sep	move	d7,D0
	bra	Putchr

put_DD	move.b	(A2),D0
	bra	PutNo		DD

put_MM	move.b	-1(A2),D0
	bra	PutNo		MM

put_YY	btst	#_Showrok-24,STR
	beq.s	.YY_nee

	tst.b	YY_sep_za
	bne.s	.sep1ne
	move	d7,D0
	bsr	Putchr		separator pred
.sep1ne
	clr.w	D0
	move.b	-2(A2),D0
	cmp.w	#100,D0		Y2K?
	blt.s	.rokOK
	sub.w	#100,D0		2000-2107 -> 00 - 107
	cmp.w	#100,D0		Y2K1?
	blt.s	.rokOK
	sub.w	#100,D0		2100-2107 -> 00 - 07
.rokOK	bsr	PutNo		YY

	tst.b	YY_sep_za
	beq.s	.sep2ne
	move	d7,d0
	bsr	Putchr
.sep2ne
.YY_nee	rts

konec_datumu:
* konec vypisu DATUMu

	tst.b	dva_radky
	bne.s	.pis_druhy_radek
	moveq	#' ',d0
	bsr	Putchr		mezera mezi datumem a casem, jsou-li na jednom radku
	bra.s	Pis_cas

.pis_druhy_radek:
	move	byt_lin(PC),D0
	mulu	#SECONDLINE_OFFSET,D0
	add	D0,A5	 	posun na dal®° ©†dek
	move	clk_xpos(PC),D2	poá†teán° sloupec

* pis CAS
Pis_cas	btst	#_Showden-24,STR
	beq.s	za_symbolem

	moveq	#' ',D0
	btst	#_Miscprnt-8,_MSC
	beq.s	Prnt0
	move.b	#'',D0
Prnt0	bsr	Putchr		zde by se màl zobrazovat indik†tor kam2lat

	moveq	#' ',D0
	btst	#_Saveron,_SVR	test na poá°t†n° screen saveru
	beq.s	Prnt1
	moveq	#5,D0		znaáka poá°t†n° screen saveru
Prnt1	bsr	Putchr

	moveq	#' ',D0
	move.l	kbshift(PC),A2
	btst	#4,(a2)		testuj Caps Lock
	beq.s	Prnt2
	move.b	#249,D0		indikace Caps Locku
Prnt2	bsr.s	Putchr

za_symbolem:
	lea	hodiny(PC),A2	uloëenò áas
	move.b	(A2)+,D0
	tst.b	ampm
	beq.s	.pishodin
	tst.b	d0		0 hodin = pñlnoc?
	bne.s	.nepul
	move.b	#12,d0		pñlnoc je dvan†ct PM
	bra.s	.pishodin

.nepul	cmp.b	#12,d0
	ble.s	.pishodin
	sub.b	#12,d0		odeáti dopoledne

.pishodin	bsr.s	PutNo		HH
	moveq	#':',D0
	bsr.s	Putchr
	move.b	(A2)+,D0
	bsr.s	PutNo		MM

	btst	#_Showrok-24,STR
	beq.s	.konec_casu

	moveq	#':',D0
	bsr.s	Putchr
	move.b	(A2),D0		SS
	bsr.s	PutNo

.konec_casu:
	rts
****************************************
**** pouzite registry:
***** D0 prijede hodnota a je znicena
***** D1 pro pocitani
PutNo	moveq	#0,D1
	move.b	D0,D1
	divu	#10,D1
	moveq	#'0',D0
	btst	#_Showdigi-24,STR
	beq.s	.pu1
	sub	#32,D0
.pu1	add.b	D1,D0
	bsr.s	Putchr
	swap	D1
	moveq	#'0',D0
	btst	#_Showdigi-24,STR
	beq.s	.pu2
	sub	#32,D0
.pu2	add.b	D1,D0

*         fall-through         *

********************************
Putchr	movem.l	d1-d2,-(sp)
	move.l	a5,a6		uschovat ukazatel do videoram (budu ho inkrementovat)
	move.l	showfont(PC),a4
	and	#$ff,d0
	sub	#$100,d0
	add	d0,a4		ukazatel na (aktu†ln° p°smeno-$100) ve fontu

	tst.b	has_chunky
	bne	Chunky_Putchr

	move.w	bitplanes(pc),d0
	cmp	#16,d0		Falcon TrueColor ?
	beq	c16bit
	cmp	#8,d0		256 barev ?
	beq	c8bit
	cmp	#4,d0		16 barev ?
	beq.s	c4bit
	cmp	#2,d0		4 barvy ?
	beq.s	c2bit

c1bit	move	d0,d3		barevn† hloubka
	mulu	d2,d3		kr†t poá†teán° sloupec
	btst	#0,d2		je to lichò sloupec ?
	beq.s	.sudy_byt
	sub	d0,d3
	addq	#1,d3
.sudy_byt	add	d3,a6
	move	#$100,d0		offset ©†dkñ ve fontu
	move	byt_lin(PC),d3	®°©ka obrazovky v bajtech

	clr.b	(a6)
	move.w	clk_lines(pc),d4
	subq	#1,d4

.loop	add	d0,a4
	add	d3,a6
	move.b	(a4),(a6)		vlastn° znak
	dbra	d4,.loop

Putch_end	movem.l	(sp)+,d1-d2
	addq	#1,d2
	rts
****************************************
c2bit	move	d0,d3		barevn† hloubka
	mulu	d2,d3		kr†t poá†teán° sloupec
	btst	#0,d2		je to lichò sloupec ?
	beq.s	.sudy_byt
	sub	d0,d3
	addq	#1,d3
.sudy_byt	add	d3,a6
	move	#$100,d0		offset ©†dkñ ve fontu
	move	byt_lin(PC),d3	®°©ka obrazovky v bajtech

	clr.b	(a6)
	clr.b	2(a6)
	move.w	clk_lines(pc),d4
	subq	#1,d4

.loop	add	d0,a4
	add	byt_lin(PC),a6	®°©ka obrazovky v bajtech
	move.b	(a4),d3
	move.b	d3,(a6)		vlastn° znak
	move.b	d3,2(a6)
	dbra	d4,.loop
	bra.s	Putch_end

c4bit	move	d0,d3		barevn† hloubka
	mulu	d2,d3		kr†t poá†teán° sloupec
	btst	#0,d2		je to lichò sloupec ?
	beq.s	.sudy_byt
	sub	d0,d3
	addq	#1,d3
.sudy_byt	add	d3,a6
	move	#$100,d0		offset ©†dkñ ve fontu
	move	byt_lin(PC),d3	®°©ka obrazovky v bajtech

	clr.b	(a6)
	clr.b	2(a6)
	clr.b	4(a6)
	clr.b	6(a6)
	move.w	clk_lines(pc),d4
	subq	#1,d4

.loop	add	d0,a4
	add	byt_lin(PC),a6	®°©ka obrazovky v bajtech
	move.b	(a4),d3
	move.b	d3,(a6)		vlastn° znak
	move.b	d3,2(a6)
	move.b	d3,4(a6)
	move.b	d3,6(a6)
	dbra	d4,.loop
	bra	Putch_end

c8bit	move	d0,d3		barevn† hloubka
	mulu	d2,d3		kr†t poá†teán° sloupec
	btst	#0,d2		je to lichò sloupec ?
	beq.s	.sudy_byt
	sub	d0,d3
	addq	#1,d3
.sudy_byt	add	d3,a6
	move	#$100,d0		offset ©†dkñ ve fontu
	move	byt_lin(PC),d3	®°©ka obrazovky v bajtech

	clr.b	(a6)
	clr.b	2(a6)
	clr.b	4(a6)
	clr.b	6(a6)
	clr.b	8(a6)
	clr.b	10(a6)
	clr.b	12(a6)
	clr.b	14(a6)
	move.w	clk_lines(pc),d4
	subq	#1,d4

.loop	add	d0,a4
	add	byt_lin(PC),a6	®°©ka obrazovky v bajtech
	move.b	(a4),d3
	move.b	d3,(a6)		vlastn° znak
	move.b	d3,2(a6)
	move.b	d3,4(a6)
	move.b	d3,6(a6)
	move.b	d3,8(a6)
	move.b	d3,10(a6)
	move.b	d3,12(a6)
	move.b	d3,14(a6)
	dbra	d4,.loop
	bra	Putch_end

c16bit	move	d2,d0		aktu†ln° sloupec
	asl	#4,d0		kr†t 16
	add	d0,a6		d3 = sloupec, a6 = videoram
	
	bsr.s	napln_indexy_barev
	lea	tab_tc(pc),a0
	asl	#1,d4
	asl	#1,d5
	move.w	(a0,d4),d4	D4.w = vybran† barva p°sma
	move.w	(a0,d5),d5	D5.w = vybran† barva pozad°
	btst	#_ShowTrn-24,STR
	beq.s	.netr
	move.w	Bkg_space(PC),d5	vyber slovo ze schovanÇho pozad° a pouë°vej ho jako transparentn° pozad°
.netr	
	move.w	d5,d0
	swap	d0
	move.w	d5,d0
	move.l	d0,d1
	move.l	d0,d2
	move.l	d0,d3
	movem.l	d0-d3,(a6)	prvn° linka pr†zdn† (nedàlat pro druhò ©†dek!)
	move.w	clk_lines(pc),d1
	subq	#1,d1

.c16_1	lea	$100(a4),a4
	move.b	(a4),d3
	add	byt_lin(pc),a6	a na p©°®t° ©†dek...
	lea	16(a6),a6		pñjdeme zprava doleva
	moveq	#8-1,d2
.c16_2	move.w	d4,d0
	asr.b	#1,d3
	bcs.s	.pis
	move.w	d5,d0
.pis	move.w	d0,-(a6)
	dbf	d2,.c16_2
	dbf	d1,.c16_1
	bra	Putch_end
************************************
* napln D4 indexem barvy pisma a D5 indexem barvy pozadi
napln_indexy_barev:

	clr.l	d4
	move.b	showcolf(pc),d4	D4.b = index barvy p°sma
	cmp.w	#16,d4
	blt.s	.d4_ok
	moveq	#1,d4		D4.b = áernÇ p°smo, protoëe je ®patnò index

.d4_ok	clr.l	d5
	move.b	showcolb(pc),d5	D5.b = index barvy pozad°
	cmp.w	#16,d5
	blt.s	.d5_ok
	moveq	#0,d5		D5.b = b°lÇ pozad°, protoëe je ®patnò index

.d5_ok	rts

************************************
Chunky_Putchr:
	move.w	bitplanes(pc),d0
	cmp	#1,d0
	beq	c1bit
	cmp	#4,d0		tu®°m ET4000 to m†
	beq	c4bit		je to ®patnà, j† v°m, ale aspo§ to nevlet° d†l

	move.w	d0,d6		D6 = barevn† hloubka (8,16,24,32)
	mulu	d2,d0		kr†t poá†teán° sloupec
	add	d0,a6		posu§ screen pointer na levò horn° roh vòpisu

	bsr.s	napln_indexy_barev	D4 a D5
	
	cmp	#8,d6		8bpp?
	bne.s	.ne8
	lea	tab_c8(pc),a0
	move.b	(a0,d4),d4	D4.b = vybran† barva p°sma
	move.b	(a0,d5),d5	D5.b = vybran† barva pozad°
	bra.s	transptst

.ne8	cmp	#15,d6
	bne.s	.ne15
	lea	tab_c15(pc),a0
	bra.s	.vyber_w
.ne15	cmp	#16,d6
	bne.s	.ne16
	lea	tab_c16(pc),a0
.vyber_w	asl	#1,d4
	asl	#1,d5
	move.w	(a0,d4),d4	D4.w = vybran† barva p°sma
	move.w	(a0,d5),d5	D5.w = vybran† barva pozad°
	bra.s	transptst

.ne16	cmp	#24,d6
	bne.s	.ne24
	lea	tab_c24(pc),a0
	bra.s	.vyber_l
.ne24	lea	tab_c32(pc),a0
.vyber_l	asl	#2,d4
	asl	#2,d5
	move.l	(a0,d4),d4	D4.l = vybran† barva p°sma
	move.l	(a0,d5),d5	D5.l = vybran† barva pozad°

transptst	btst	#_ShowTrn-24,STR
	beq.s	.netr
	move.l	Bkg_space(PC),d5	vyber slovo ze schovanÇho pozad° a pouë°vej ho jako transparentn° pozad°
.netr

* vymaë prvn° linku nad vòpisem (nemàlo by se dàlat pro druhò ©†dek!)

	moveq	#0,d3		©†dek niáeho
	bsr.s	put_line		horn° pr†zdn† linka

* vypi® samotnò znak
	move.w	clk_lines(pc),d1
	subq	#1,d1

.loop	lea	$100(a4),a4	posun ve fontu o ©†dek d†l
	sub.w	d6,a6		vr†tit to co jsem nainkrementoval
	add.w	byt_lin(PC),a6	posunout A6 na dal®° ©†dek
	move.b	(a4),d3		bajt z fontu
	bsr.s	put_line		D3 = bajt k vyps†n° na (A6)
	dbra	d1,.loop

	bra	Putch_end

***************************************
* vstup: D3 = bajt k vyps†n°
*        D4 = barva znaku
*        D5 = barva pozad°
*        D6 = barevn† hloubka (bpp)
*        A6 = adresa ve VideoRAM (je inkrementov†na)
*
* pouë°v† D0 a D2
*
put_line	moveq	#8-1,d2
.loop	move.l	d5,d0		d†t barvu pozad° do D0
	btst	d2,d3
	beq.s	.solto
	move.l	d4,d0		d†t barvu p°sma do D0
.solto	cmp	#8,d6
	beq.s	.b8
	cmp	#16,d6
	beq.s	.b16
	cmp	#24,d6
	beq.s	.b24

.b32	move.l	d0,(a6)+
	dbra	d2,.loop
	rts

.b16	move.w	d0,(a6)+
	dbra	d2,.loop
	rts

.b24	rol.l	#8,d0
	move.b	d0,(a6)+
	rol.l	#8,d0
	move.b	d0,(a6)+
	rol.l	#8,d0
.b8	move.b	d0,(a6)+
	dbra	d2,.loop
	rts
***************************************

SaveBkg	movem.l	a0-a2/d0-d4,Bkg_regs
	clr.l	d0		flag 0 for saving
	bra.s	Backgrnd

RestBkg	tst.b	bkg_valid
	bne.s	.restbkg
	rts			nastavit ZERO flag, protoze jsme nemeli zalohu
.restbkg	movem.l	a0-a2/d0-d4,Bkg_regs
	st	d0		flag 1 for restoring

Backgrnd	move.l	_v_bas_ad\w,a0	adresa videoram
	move.w	bitplanes(pc),d1
	tst	d0		uschovat nebo obnovit?
	bne.s	.resto
	st	bkg_valid		flag: v bufru m†me platnÇ pozad°
.resto	move	clk_xpos(pc),d2	poá†teán° sloupec Clockñ
	bclr	#0,d2		mus° bòt sudò!
	mulu	d1,d2
	lea	(a0,d2),a2	adresa 1. sudÇho sloupce Clockñ v RAM

	move.w	clk_cols,d3	poáet sloupcñ
	addq	#1,d3
	asr	#1,d3		p©eveden na 16-ti bitov† slova
	mulu	d1,d3		n†soben barevnou hloubkou
	subq	#1,d3		poáet slov -1 kvñli dbf

	lea	Bkg_space(pc),a1
	
* poáet ©†dkñ pro uloëen° nebo obnoven° je clk_lines+1 (horn° linka), ale protoëe dbf,
* tak tu '+1' ani nep©iá°t†m, ani nakonec neodeá°t†m
	move.w	clk_lines(pc),d2
	tst.b	dva_radky
	beq.s	.ne_dva
	add	d2,d2		dva ©†dky
.ne_dva	

Backloop	move.l	a2,a0
	move	d3,d4
	tst	d0
	beq.s	.saveloop

.restloop	move.w	(a1)+,(a0)+
	dbf	d4,.restloop
	bra.s	.endloop

.saveloop	move.w	(a0)+,(a1)+
	dbf	d4,.saveloop

.endloop	add	byt_lin(pc),a2
	dbf	d2,Backloop

SaveBend	moveq	#1,d0		smazat ZERO flag, protoze jsme zalohovali
	movem.l	Bkg_regs,a0-a2/d0-d4
	rts

***************************************
	dc.l	XBRA,IDENTIFIER
xbios_jmp	dc.l	0
	move.l	usp,a0
	btst	#5,(sp)
	beq.s	.nesup
	lea	6(sp),a0
	tst.w	_longframe\w
	beq.s	.nesup
	addq.l	#2,a0
.nesup
	btst	#_XBSettime,OnBoot1
	beq.s	.xb1

	tst.l	TIMECOOK
	bne.s	.xb1		pokud je TimeCookie, tak netrap XBIOS

	cmp.w	#22,(a0)		Settime
	bne.s	.xb1
; uloë si pr†và zmànànò áas
	movem.l	a1-a3/d0-d3,-(sp)
	move.l	2(a0),d0
	bsr.s	Prepar
	movem.l	(sp)+,a1-a3/d0-d3
	bra.s	.xb_end

.xb1	btst	#_XBFixY2K,OnBoot1
	beq.s	.xb_end

	cmp.w	#23,(a0)		Gettime
	bne.s	.xb_end
	move.l	2(sp),navrat
	move.l	#xbios_Gettime,2(sp)	return address

.xb_end	move.l	xbios_jmp,a0
	jmp	(a0)
navrat	dc.l	0

xbios_Gettime:
	move.l	d1,-(sp)
	move.l	d0,d1
	and.l	#$FE000000,d1
	cmp.l	#$60000000,d1
	bne.s	.xbios_y2k_ok
	and.l	#$01FFFFFF,d0	Y2K fix
	or.l	#$28000000,d0

.xbios_y2k_ok:
	move.l	(sp)+,d1
	move.l	navrat(PC),-(sp)
	rts

***************************************
GetTime	movem.l	A0-A3/D1-D3,-(sp)
	move	#$17,-(sp)
	trap	#14		áte áas z hardwaru (na strojich bez HW hodin (ST) to v preruseni tuhne)
	addq.l	#2,sp
	movem.l	(sp)+,A0-A3/D1-D3

	move.l	_hz_200\w,d1	pamatuju si okamëik pravdy, odtud budu p©iá°tat po sekundà
	add.l	#200,d1
	move.l	d1,abs_cas

Prepar	move.l	D0,D1
	lea	sekundy(PC),A1	p©ekop†n° áasu z XBIOS tvaru do BCD
	moveq	#$1F,D0
	and.b	D1,D0
	lsl.b	#1,D0
	move.b	D0,(A1) 		sekundy
	lsr.l	#5,D1
	moveq	#$3F,D0
	and.b	D1,D0
	move.b	D0,-(A1)		minuty
	lsr.l	#6,D1
	moveq	#$1F,D0
	and.b	D1,D0
	move.b	D0,-(A1)		hodiny
	lsr.l	#5,D1

	lea	datum(PC),A0
	cmp	(A0),d1		zmànil se od minulÇho P©epa©-en° datum?
	beq.s	dvt_end

	move	d1,(A0)		jo, tak p©epoá°tej datum
	moveq	#$1F,D0
	and.b	D1,D0
	move.b	D0,-(A1)		dny
	lsr	#5,D1
	moveq	#$F,D0
	and.b	D1,D0
	move.b	D0,-(A1)		màs°ce
	lsr	#4,D1
	and.b	#$7F,D1		jen 7 bitñ pro rok
	add.b	#$50,D1		p©iáti 80 k rokñm
	move.b	D1,-(A1)		roky uloë

* vòpoáet dne v tòdnu (d_v_t)
denvtydnu	moveq	#0,D0
	moveq	#0,D1
	moveq	#0,D2
	moveq	#0,D3
	lea	dny(PC),A0
	lea	tabmoon(PC),A1
	move.b	(A0),D0
	move.b	-(A0),D1
	move.b	-(A0),D2
	subq	#2,D1
	bmi.s	dvt3
dvt1	move.b	(A1,D1),D3
	add	D3,D0
	cmp.b	#1,D1
	bne.s	dvt2
	moveq	#3,D3
	and.b	D2,D3
	bne.s	dvt2
	addq	#1,D0
dvt2	dbf	D1,dvt1
dvt3	moveq	#$B,D1
	cmp.b	#$50,D2
	dble	D2,dvt1
	divu	#7,D0
	swap	D0
	move.b	D0,-(A0)
dvt_end	rts

*****************************
Do_It	move.l	STR(PC),d0	registry A0-A6/D0-D3 jsou volnÇ
	move.l	B_structura(PC),d1
	cmp.l	d0,d1
	beq	do_end

	move.l	d0,B_structura	aby se zmàny zapamatovaly

	move.l	d1,d2
	eor.l	d0,d2		v D2 jsou nastavenÇ jen bity zmàn
**********
	btst	#_ShowTime,d2	zobrazen° áasu
	beq.s	.do2
	btst	#_ShowTime,d0
	bne.s	.do1
	bsr	RestBkg
	bra.s	.do2
.do1	bsr	SaveBkg
**********
.do2	btst	#_Showdat,d2
	sne	recompute
	bne.s	.do2_
	btst	#_Showden,d2
	sne	recompute
	bne.s	.do2_
	btst	#_Showrok,d2
	sne	recompute
	bne.s	.do2_
	btst	#_ShowBig,d2
	sne	recompute
**********
.do2_
	btst	#_Kbddead,d2	mrtv† kl†vesa
	beq.s	.do3
	clr.w	presmrt
**********
.do3	btst	#_Kbdasci,d2	ASCII num keypad
	beq.s	.do31
	clr.w	cislo
**********
* pokud je cink vypnuto, musim zapnout zvuk klavesnice a vypnout klikmask
* pokud je cink zapnuto, musim vypnout zvuk klavesnice a zapnout klikmask
.do31	btst	#_Kbdcink,d2	vse je podivne - ZMENIT!
	beq.s	.do32
	btst	#_Kbdcink,d0
	bne.s	.do31a
	bclr	#0,klikmask
	bset	#0,conterm\w
	bra.s	.do32
.do31a	bset	#0,klikmask
	bclr	#0,conterm\w

.do32	btst	#_Kbdbell,d2
	beq.s	.do4
	btst	#_Kbdbell,d0
	bne.s	.do32a
	bclr	#2,klikmask
	bset	#2,conterm\w
	bra.s	.do4
.do32a	bset	#2,klikmask
	bclr	#2,conterm\w
**********
.do4	btst	#_Miscmys,d2	zrychlovaá my®i
	beq.s	.do5
	move.l	_hz_200\w,casovac
**********
.do5	swap	d0		kl†vesnice
	swap	d2
	and.b	#%11,d0
	beq	.do6		nemànit
	and.b	#%11,d2
	beq	.do6		ë†dnÇ zmàny, takëe nic

	move.l	KEYTAB(pc),a1
	cmp.b	#1,d0
	beq.s	.do5c		1 = origin†ln°
	lea	normkbd(pc),a0
	cmp.b	#2,d0		2 = norm†ln°
	beq.s	.do5b
	lea	cskbd(PC),A0	3 = áesk†
.do5b	move.l	a0,(a1)+		normal
	lea	$80(a0),a0
	move.l	a0,(a1)+		shifted
	lea	$80(a0),a0
	move.l	a0,(a1)+		capslock
	btst	#_KbdAltK-16,_KBD
	beq.s	.do5E
	tst.b	extended_kbd
	beq.s	.do5E
	lea	$80(a0),a0
	move.l	a0,(a1)+		alt
	lea	32(a0),a0
	move.l	a0,(a1)+		alt+shift
	lea	32(a0),a0
	move.l	a0,(a1)		alt+capslock
	bra.s	.do5E

.do5c	move.l	TUNSHIFT,(a1)+
	move.l	TSHIFTED,(a1)+
	move.l	TCAPSLOCK,(a1)+
	btst	#_KbdAltK-16,_KBD
	beq.s	.do5E
	tst.b	extended_kbd
	beq.s	.do5E
	move.l	TALTKEY,(a1)+
	move.l	TALTSHKEY,(a1)+
	move.l	TALTCLKEY,(a1)

.do5E
**********
.do6	btst	#_Miscturb,d2
	beq.s	.do7
	tst.b	is_megae
	beq.s	.do7
*** Turbo 16 MHz ái standardn° pomalost ? ;-)
	lea	_MegaSTe,A0
	btst	#_Miscturb,D0
	beq.s	.do6b

	or.b	#%011,(A0)
	bra.s	.do7

.do6b	and.b	#~%011,(A0)
**********
.do7
do_end	rts
*****************************
Simulate_English_TOS
	btst	#_EngSys,OnBoot1
	beq.s	.end

	move.l	_sysbase\w,A0
	lea	SYSHDR(PC),A1
	moveq	#23,D0
.engloop	move.w	(A0)+,(A1)+
	dbra	D0,.engloop

.uz_copy	lea	SYSHDR(PC),A1
	and.w	#~%010,28(A1)	to je ta angliátina
	move.w	#$4EF9,48(A1)	a JuMP
	move.l	4(A1),50(A1)	na start TOS ROM (kvñli ColdBootu)
	move.l	A1,_sysbase\w

	move.l	#'_AKP',d0
	bsr	GetCookie
	move.l	d0,d1
	bmi.s	.cookneni
	and.l	#~$0000FF00,d1	save keyboard setting, set language to USA
	bra.s	.cookwrit
.cookneni	clr.l	d1		both language and keyboard are set to USA
.cookwrit	move.l	#'_AKP',d0
	bsr	SetCookie

.end	rts
*****************************
Sys_Init	movem.l	D0-D2/A0-A2,-(SP)
	dc	$A000		LineA
	move	(a0),d1
	move.w	d1,bitplanes
	move	-2(a0),d0
	move	d0,byt_lin	®°©ka obrazovky v bajtech
	move.w	-46(a0),v_cel_ht	vò®ka pole znaku
	move.l	4(A1),A0
	move.l	$4C(A0),fnt8x8	malò systÇmovò font
	move.l	8(A1),A0
	move.l	$4C(A0),fntbig
	bsr	Compute_print_size	p©epoá°tat promànnÇ clk_cols, clk_xpos

* Cookies DATE a TIME		kontrolujeme dvakr†t - p©i spu®tàn° Clockñ a po startu GEMu
	move.l	#'TIME',d0
	bsr.s	GetCookie
	bne.s	.cook_neni	neni TIME cookie, skoda
	move.l	d0,TIMECOOK

	move.l	#'DATE',d0
	bsr.s	GetCookie
	bne.s	.cook_neni
	move.l	d0,DATECOOK
.cook_neni

* zkus°m tady nahodit vàci, co GEM pr†và p©epsal (zvuky, English, inverze)
	bclr	#_Kbdcink-16,B_structura+1
	bclr	#_Kbdbell-16,B_structura+1

	bsr	Simulate_English_TOS

	btst	#_InvColor,OnBoot1
	beq.s	sys_konec
	bsr	invertuj

sys_konec	movem.l	(SP)+,D0-D2/A0-A2
	rts
***************************************
GetCookie	movem.l	D1-D2/A0,getcookie_regs
	MOVE.L	D0,D2		CookieJar
	MOVE.L	$5A0.W,D0
	BEQ.S	L00A2
	MOVEA.L	D0,A0
L00A1	MOVE.L	(A0)+,D1
	MOVE.L	(A0)+,D0
	CMP.L	D2,D1
	BEQ.S	L00A3
	TST.L	D1
	BNE.S	L00A1
L00A2	MOVEQ	#-1,D1		set NotEqual a Negative bity
L00A3	movem.l	getcookie_regs,D1-D2/A0
	RTS
***************************************
* D0 = key, D1 = value
SetCookie	MOVE.L	$5A0.W,D3
	BEQ.S	L00FF
	MOVEA.L	D3,A1
	MOVEQ	#0,D4
L00FE	ADDQ.W	#1,D4
	MOVEM.L	(A1)+,D2-D3
	CMP.L	D0,D2
	BEQ.S	COOKFOUND
	TST.L	D2
	BNE.S	L00FE
	CMP.L	D3,D4
	BEQ.S	L00FF
	MOVEM.L	D0-D3,-8(A1)
	BRA.S	L00FF
COOKFOUND	MOVEM.L	D0-D1,-8(A1)
L00FF	RTS
***************************************
RemoveCookie
	move.l	$5a0.w,d3
	beq.s	.end
	move.l	d3,a1
.findloop	movem.l	(a1)+,d2-d3
	tst.l	d2
	beq.s	.end
	cmp.l	d0,d2
	bne.s	.findloop
.moveloop	movem.l	(a1)+,d2-d3
	movem.l	d2-d3,-16(a1)
	tst.l	d2
	bne.s	.moveloop
.end	rts
***************************************
InsertVBL	MOVE.W	$454.W,D0
	LSL.W	#2,D0
	MOVEA.L	$456.W,A1
	MOVEQ	#4,D1
L010F	TST.L	(A1,D1.W)
	BEQ.S	L0111
	ADDQ.W	#4,D1
	CMP.W	D0,D1
	BNE.S	L010F
	MOVEQ	#-1,D0
	bra.s	L112
L0111	MOVE.L	a0,(A1,D1.W)
	MOVEQ	#0,D0
L112	rts
***************************************
RemoveVBL	move.w	$454.w,d0		1) remove VBL
	move.l	$456.w,a1

.vbl1	subq.w	#1,d0
	beq.s	.vbl_end
	addq	#4,a1
	move.l	(a1),a0
	cmp.l	#XBRA,-12(a0)
	bne.s	.vbl1
	cmp.l	#IDENTIFIER,-8(a0)
	bne.s	.vbl1
	clr.l	(a1)		JCLK nalezeno a zru®eno

.vbl3	subq.w	#1,d0
	beq.s	.vbl_end
	addq	#4,a1
	move.l	(a1),-4(a1)	p©isunout zbytek VBL slotñ
	bra.s	.vbl3
.vbl_end	rts
***************************************
RemoveXBRA
.loop	move.l	(a0),a1
	cmp.l	#XBRA,-12(a1)
	bne.s	.end		bez XBRA jsem bezmocnò
	cmp.l	#IDENTIFIER,-8(a1)
	beq.s	.found
	lea	-4(a1),a0		dal®° XBRA álen
	bra.s	.loop

.found	move.l	-4(a1),(a0)	vy©ad°me JCLK z XBRA ©etàzu

.end	rts
***************************************
UninstallVectors
	move.w	sr,-(sp)
	or.w	#$700,sr		zak†zat p©eru®en°

	bsr.s	RemoveVBL		1) odstranit VBL prerusovaci vektor

	move	#34,-(sp)		2) odstranit IKBD vektor
	trap	#14
	addq	#2,sp
	move.l	d0,a2
	lea	MOUSE(a2),a0
	bsr.s	RemoveXBRA	3) odstranit mouse accelerator
	lea	IKBD(a2),a0
	bsr.s	RemoveXBRA	4) odstranit keyboard handler

	lea	_bconout\w,a0
	bsr.s	RemoveXBRA	5) odstranit printer conversion

	lea	$b8.w,a0
	bsr.s	RemoveXBRA	6) odstanit XBIOS vektor

	move.l	#IDENTIFIER,d0
	bsr	RemoveCookie	7) odstranit CookieJar

	move.w	(sp)+,sr		povolit p©eru®en°
	rts
***************************************
Usermode	move.l	d0,-(sp)
	move.w	#32,-(sp)
	trap	#1		go into User mode
	addq	#6,sp
	rts
***************************************
init	clr.l	-(sp)
	move.w	#32,-(sp)
	trap	#1		go into Supervisor mode
	addq	#6,sp
	move.l	d0,-(sp)		uschovat USERSTACK pointer na z†sobn°k

	move.l	#IDENTIFIER,d0
	bsr	GetCookie
	bne.s	init0

* Clocky jsou jië aktivn° v pamàti - mus°me deaktivovat a odinstaloval
	move.l	d0,a0
* zkontrolovat verzi
* pokud je novàj®°, tak p©es ukazatel j°t na bitovÇ pole do glob†ln° pamàti
	addq.l	#6,a0		toto je adresa na bitovÇ pole

* p©epni na prapñvodn° (origin†ln°) kl†vesnici
	bclr	#1,1(a0)
	bset	#0,1(a0)		takto zap°n†m origin†ln° kl†vesnici

* jestli je zobrazov†n° aktivn°, tak ho vypni, aby se vr†tilo pñvodn° pozad°
	bclr	#_ShowTime-24,(a0)	vypnout zobrazov†n° áasu
	beq.s	.nezobrazoval

* poákat jedno VBL, neë starÇ Clocky vr†t° pñvodn° obraz pod áas
	move.w	#32,-(sp)
	trap	#1		go into User mode
	addq	#6,sp		áekat v uëivatelskÇm m¢du, aü v®echno funguje norm†lnà

	move.w	#$25,-(sp)
	trap	#14		1. Vsync
	addq	#2,sp
	move.w	#$25,-(sp)
	trap	#14		2. Vsync, to by màlo staáit
	addq	#2,sp
.nezobrazoval

* odinstaluj v®echny vektory, kterÇ p©edt°m Clocky zabraly (IKBD, mouse, VBL, prntout a Cookie)
	move.l	#UninstallVectors,-(sp)
	move.w	#38,-(sp)
	trap	#14		Supexec(UninstallVectors)
	addq	#6,sp

* nesm°m uvolnit pamàü p©edchoz° instance Clockñ, protoëe:
* - ji pouë°vaj° EHC klienti (a nelze jim ©°ct, ëe uë je neplatn†)
* - tam leë° hlaviáka English TOSu (simulovanÇho)
* - TSR programy stejnà nikdy pamàü neuvol§uj°, protoëe nikdy nekoná°

	pea	unintext(pc)
	move	#9,-(sp)
	trap	#1		zobrazit
	addq	#6,sp

	clr	-(sp)
	trap	#1		zkonáit
***********************************************
init0	st.b	has_mfp		temer vsechny stroje maji MFP
	move.l	#'_MCH',d0
	bsr	GetCookie
	beq.s	.je_mach
; _MCH neni => je to starò TOS (ST, STacy nebo MegaST)
	st.b	is_stacy		predpokladej, ze vzdy mame STacy (coz je blbe!)
	bra.s	init1

; _MCH je, tak ho pouzij pro rozhodnuti
.je_mach	swap	d0
	cmp.w	#3,d0		je to Falcon?
	bne.s	.init0a
; je to Falcon030
	st.b	is_falcon
	sf.b	has_mfp		Falcon nema Modem1 port
	st.b	has_scc
	move	#$59,-(sp)
	trap	#14		VgetMonitor
	addq	#2,sp
	cmp	#2,d0
	bne.s	init1
	st.b	has_vga
	bra.s	init1

.init0a	cmp.w	#2,d0
	bne.s	.init0b
; je to TT030
	st.b	is_tt
	st.b	has_scc
	bra.s	init1

.init0b	cmp.l	#$00100001,d0	MegaSTE
	bne.s	init1
; je to MegaSTE
	st.b	is_megae
	st.b	has_scc

init1	tst.b	is_falcon
	bne.s	.okoSM124
	cmp.b	#2,$ffff8260.w
	seq	is_sm124		 is_sm124 = !Falcon && ff8260 == 2
.okoSM124
	move.l	#'NOVA',d0
	bsr	GetCookie
	bne.s	.poi_vga
	st.b	has_nova
	st.b	has_chunky
	bra.s	konec_video_testu
.poi_vga
	move.l	#'RDCT',d0	part of NVDI 4000 - ReDireCT
	bsr	GetCookie
	bne.s	.poi_vga1
;	st.b	has_et4000
	st.b	has_chunky
	bra.s	konec_video_testu
.poi_vga1
	move.l	#'_VDO',d0
	bsr	GetCookie
	bne.s	.vdoneni
	swap	d0
	cmp	#4,d0		Milan graphics - similar to NOVA gfx card
	bne.s	.vdoneni
* je to Milan
	st.b	has_chunky
.vdoneni
konec_video_testu

	move.l	#'_SND',d0
	bsr	GetCookie
	bne.s	.snd_neni		takze Yamaha je
	btst	#0,d0
	beq.s	.yamaha_neni
.snd_neni	st.b	has_yamaha
.yamaha_neni

	move.l	_sysbase\w,A0
	move.w	2(A0),d0
	move.w	d0,sysver
	cmp.w	#$0100,d0
	bne.s	.ma_kbsh
	move.l	#$1EB,kbshift
	bra.s	.za_kbsh
.ma_kbsh	move.l	$24(A0),kbshift
.za_kbsh	cmp.w	#$0400,d0		od kvàtna 1992/TOSu 4.04 maj° TOSy novÇ pointery pro kl†vesnici
	blt.s	.neroz
	st.b	extended_kbd
.neroz:

	moveq	#-1,d0
	move.l	d0,-(sp)
	move.l	d0,-(sp)
	move.l	d0,-(sp)
	move	#16,-(sp)
	trap	#14
	lea	14(sp),sp
	move.l	d0,a0
	move.l	a0,KEYTAB
	move.l	(a0)+,TUNSHIFT
	move.l	(a0)+,TSHIFTED
	move.l	(a0)+,TCAPSLOCK
* nasledujici tri pouze pro TOS > 2.06
	tst.b	extended_kbd
	beq.s	.nema_alt
	move.l	(a0)+,TALTKEY
	move.l	(a0)+,TALTSHKEY
	move.l	(a0)+,TALTCLKEY
.nema_alt:
	pea	$e0001
	trap	#14		Iorec
	addq	#4,sp
	move.l	d0,iorec

****
	move	#34,-(sp)		Kbdvbase
	trap	#14
	addq	#2,sp
	move.l	d0,a2
	lea	MOUSE(a2),a0
	move.l	a0,adr_mouse	pamatuj, kde zaá°n† my®
	lea	mouse_jmp(pc),a1
	move.l	(a0),(a1)+
	move.l	a1,(a0)		zapnut zrychlovaá my®i

****
	lea	-4(a2),a0		-4 je undocumented
	lea	ikbd_jmp(pc),a1
	move.l	(a0),(a1)+
	move.l	a1,(a0)		ikbdsys

****
	btst	#_HookXBIOS-8,OnBoot
	beq.s	.ne_xbios
	lea	$b8.w,a0
	lea	xbios_jmp(pc),a1
	move.l	(a0),(a1)+
	move.l	a1,(a0)		XBIOS
.ne_xbios
****
	bsr	GetTime		uë áte Y2K opravenò áas

	bsr	Sys_Init

	move.l	#VBLtimer,a0
	bsr	InsertVBL

	move.l	#IDENTIFIER,d0
	move.l	#JoyCookie,d1
	bsr	SetCookie

*--------------------------------------*
* Vektor pro vòstup na tisk†rnu        *
*--------------------------------------*
	lea	_bconout\w,a0
	lea	kam2lat(pc),a1
	move.l	(a0),(a1)+
	move.l	a1,(a0)

*--------------------------------------*
* Konec instalace, zpàt do User m¢du   *
*--------------------------------------*
	move.w	#32,-(sp)
	trap	#1		go into User mode
	addq	#6,sp

	pea	infotext(PC)
	move	#9,-(SP)
	trap	#1
	addq	#6,SP

	clr	-(SP)
	pea	konec-zacatek+$100
	move	#49,-(SP)
	trap	#1


	section DATA
JoyCookie:
JoyClock	dc.l	IDENTIFIER
version	dc.w	VERSION
*                           v       v       v       v
STR	dc.l	%11111001011011100100100010001000
_KBD	= STR+1
_MSC	= STR+2
_SVR	= STR+3
saverlen	dc.w	DELKA_DNE		v sekund†ch
savecount	dc.w	0
hotshift	dc.b	%0011		DoubleShift
hottime	dc.b	%0110		Shift+Control
act_shift	dc.b	0		stav shiftu ve chvili zaznamenani EHC
act_key	dc.b	0		pr†và stisknut† EHC kl†vesa
ehc_table	dc.l	ehc_scantable	tabulka obsazen° kl†ves EHC [128] + tabulka flagu [128]
showpos	dc.w	0		kladn† zprava, z†porn† zleva
showcolb	dc.b	0		barva pozad° b°l†
showcolf	dc.b	1		barva p°sma áern†
*                           v       v
OnBoot	dc.w	%1111100000000000
OnBoot1	= OnBoot+1

refresh	dc.w	REFRESH		ve VBL

normkbd	dc.b	0,27,'1234567890=',39,8,9,'qwertzuiop',26,'+',13,0,'asdfghjkl[]#',0,'~yxcvbnm,.-',0,0,0,' ',0,0,0,0,0,0
	dc.b	0,0,0,0,0,0,0,0,0,0,'-',0,0,0,'+',0,0,0,0,127,0,0,0,0,0,0,0,0,0,0,0,0,'<',0,0,'()/*7894561230.',13,0,0,0,0,0,0,0,0,0,0,0,0,0
	dc.b	0,27,'!"#$%&/()=?`',8,9,'QWERTZUIOP\*',13,0,'ASDFGHJKL{}^',0,'|YXCVBNM;:_',0,0,0,' ',0,0,0,0,0,0
	dc.b	0,0,0,0,0,0,0,'78',0,'-4',0,'6+',0,'2',0,'0',127,0,0,0,0,0,0,0,0,0,0,0,0,'>',0,0,'()/*7894561230.',13,0,0,0,0,0,0,0,0,0,0,0,0,0
	dc.b	0,27,'1234567890=',39,8,9,'QWERTZUIOP',26,'+',13,0,'ASDFGHJKL[]#',0,'~YXCVBNM,.-',0,0,0,' ',0,0,0,0,0,0
	dc.b	0,0,0,0,0,0,0,0,0,0,'-',0,0,0,'+',0,0,0,0,127,0,0,0,0,0,0,0,0,0,0,0,0,'<',0,0,'()/*7894561230.',13,0,0,0,0,0,0,0,0,0,0,0,0,0
* nasledujici tri tabulky maji po maximalne 15-ti parech hodnot ukoncenych 0,0 (tedy jsou 32 bajtu dlouhe)
altkey	dc.b	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
altshkey	dc.b	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
altclkey	dc.b	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

cskbd	dc.b	0,27,'¢à®á©ëò†°Ç=',39,8,9,'qwertzuiop£É',13,0,'asdfghjklñü#',0,'~yxcvbnm,.-',0,0,0,' ',0,0,0,0,0,0
	dc.b	0,0,0,0,0,0,0,0,0,0,'-',0,0,0,'+',0,0,0,0,127,0,0,0,0,0,0,0,0,0,0,0,0,'<',0,0,'()/*7894561230.',13,0,0,0,0,0,0,0,0,0,0,0,0,0
	dc.b	0,27,'ïâõÄûíùèãê?`',8,9,'QWERTZUIOPóÖ',13,0,'ASDFGHJKL¶Ü^',0,'|YXCVBNM;:_',0,0,0,' ',0,0,0,0,0,0
	dc.b	0,0,0,0,0,0,0,'78',0,'_4',0,'6+',0,'2',0,'0',127,0,0,0,0,0,0,0,0,0,0,0,0,'>',0,0,'{}/*\[]$%&!"#=.',13,0,0,0,0,0,0,0,0,0,0,0,0,0
	dc.b	0,27,'ïâõÄûíùèãê=',39,8,9,'QWERTZUIOPóÖ',13,0,'ASDFGHJKL¶Ü#',0,'~YXCVBNM,.-',0,0,0,' ',0,0,0,0,0,0
	dc.b	0,0,0,0,0,0,0,0,0,0,'-',0,0,0,'+',0,0,0,0,127,0,0,0,0,0,0,0,0,0,0,0,0,'<',0,0,'()/*7894561230.',13,0,0,0,0,0,0,0,0,0,0,0,0,0
* nasledujici tri tabulky maji po maximalne 15-ti parech hodnot ukoncenych 0,0
altklav	dc.b	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
altshklav	dc.b	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
altclklav	dc.b	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

* 128 bajtu prekodovaci tabulky Kamenici->Latin2
latina	dc.b	$ac,$81,$82,$d4,$84,$d2,$9b,$9f,$d8,$b7,$91,$d6,$96,$92,$8e,$b5
	dc.b	$90,$a7,$a6,$93,$94,$e0,$85,$e9,$ec,$99,$9a,$e6,$95,$ed,$fc,$9c
	dc.b	$a0,$a1,$a2,$a3,$e5,$d5,$de,$e2,$e7,$fd,$bf,$e8,$20,$f5,$ae,$af
	dc.b	$b0,$b1,$b2,$b3,$b4,$20,$20,$20,$20,$b9,$ba,$bb,$bc,$20,$20,$bf
	dc.b	$c0,$c1,$c2,$c3,$c4,$c5,$20,$20,$c8,$c9,$ca,$cb,$cc,$cd,$ce,$20
	dc.b	$20,$20,$20,$20,$20,$20,$20,$20,$20,$d9,$da,$db,$dc,$20,$20,$df
	dc.b	$20,$e1,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
	dc.b	$20,$20,$20,$20,$20,$20,$f6,$20,$f8,$20,$fa,$20,$20,$20,$fe,$20
DeadKey	dc.b	$0D
durazlen	dc.b	28	* v nasledujicich trech radkach je platnych jen 'durazlen' hodnot
produraz	dc.b	"'ertzuioasdycn`ERTZUIOASDYCN            "	* mus° bòt p©esnà 40 bajtñ
s_carou	dc.b	"'Çrtz£°¢†sdòcn'êRTZóãïèSDùCN            "
s_hakem	dc.b	"`à©üëñioa®Éyá§`âûÜí¶IOAõÖYÄ•            "
*                   'T','D','A','S','M','L','O','N','C'
hotkeje	dc.b	$14,$20,$1E,$1F,$32,$26,$18,$31,$2E
hotk_klik	dc.b	$25
hotk_inv	dc.b	$30
turbo_on	dc.b	$4A
turbo_off	dc.b	$4E
	dc.b	0,0,0				* horkòch kl†ves je celkem 16

	ifne	ENGLISH
tyden	dc.b	"MoTuWeThFrSaSu"
	else
tyden	dc.b	"PoótStÄtP†SoNe"
	endc

tabmoon	dc.b	31,28,31,30,31,30,31,31,30,31,30,31
* tabulky barev pro NOVA Mach64 a Falcon TC
tab_c8	dc.b	0,$ff,1,2,4,6,3,5,7,8,9,10,12,14,11,13
tab_c15	dc.w	$ff7f,$0000,$007c,$e003,$1f00,$ff03,$e07f,$1f7c,$f75e,$1042,$005c,$e002,$1700,$f702,$e05e,$175c
tab_c16	dc.w	$ffff,$0000,$00f8,$e007,$1f00,$ff07,$e0ff,$1ff8,$f7bd,$1084,$00b8,$e005,$1700,$f705,$e0bd,$17b8
tab_c24	dc.l	$ffffff00,$00000000,$0000ff00,$00ff0000,$ff000000,$ffff0000,$00ffff00,$ff00ff00,$bebebe00,$82828200,$0000be00,$00be0000,$be000000,$bebe0000,$00bebe00,$be00be00
tab_c32	dc.l	$ffffff,0,$ff,$ff00,$ff0000,$ffff00,$ffff,$ff00ff,$bebebe,$828282,$be,$be00,$be0000,$bebe00,$bebe,$be00be
tab_tc	dc.w	$ffdf,$0000,$f800,$07c0,$001f,$07df,$ffc0,$f81f,$d69a,$8410,$a800,$0540,$0015,$0555,$ad40,$a815

tut_table	dc.w	$7D,$100		set channel A frequency to 1000 Hz
	dc.w	$7FE		use channel A only
	dc.w	$80F		set channel A volume
	dc.w	$FF19		wait 25 vblanks
	dc.w	$3E		change frequence to 2000 Hz
	dc.w	$FF05		wait another 5 vblanks
	dc.w	$800		turn volume off
	dc.w	$FF05		wait another 5 vblanks
	dc.w	$80F		turn volume on
	dc.w	$FF05		wait another 5 vblanks
	dc.w	$800		turn volume off
	dc.w	$FF00		terminate
tut_tab_end:

	ifne	ENGLISH
infotext	dc.b	13,10,27,'p',"  Clockyø  version 3.10beta  2000/05/29 ",27,'q',13,10
	dc.b	       "     (c) 1991-2000  Petr Stehlik",13,10,10,0
unintext	dc.b	"Clocky has been deactivated and removed.",13,10,0
	else
infotext	dc.b	13,10,27,'p',"  Clockyø  verze 3.10beta  29.05.2000 ",27,'q',13,10
	dc.b	       "     (c) 1991-2000  Petr Stehl°k",13,10,10,0
unintext	dc.b	"Clocky byly vypnuty a odstranàny.",13,10,0
	endc

*******************************
	section BSS
klikmask	ds.b	1	%0101	hodnota masky je updatovana v zavislosti na _kbd.[45]
cislo	ds.w	1
iorec	ds.l	1
kbshift	ds.l	1
adr_mouse	ds.l	1
byt_lin	ds.w	1
bitplanes	ds.w	1
v_cel_ht	ds.w	1
clk_cols	ds.w	1
clk_xpos	ds.w	1
clk_lines	ds.w	1
sysver	ds.w	1

presmrt:
	even
hak	ds.b	1
cara	ds.b	1

zhasnuto	ds.b	1
recompute	ds.b	1
is_stacy	ds.b	1
is_falcon	ds.b	1
is_megae	ds.b	1
is_tt	ds.b	1
is_sm124	ds.b	1
has_mfp	ds.b	1
has_scc	ds.b	1
has_vga	ds.b	1
has_nova	ds.b	1
has_chunky	ds.b	1
has_yamaha	ds.b	1
dva_radky	ds.b	1

casovac	ds.l	1
abs_cas	ds.l	1

fnt8x8	ds.l	1
fntbig	ds.l	1
showfont	ds.l	1
cookie	ds.l	1

TIMECOOK	ds.l	1	udrëuje aktu†ln° áas
DATECOOK	ds.l	1	---""--- --""-- datum
DT_COOK	ds.l	1
KEYTAB	ds.l	1	ukazatel na trojici vektorñ pro kl†vesovÇ tabulky
TUNSHIFT	ds.l	1
TSHIFTED	ds.l	1
TCAPSLOCK	ds.l	1
TALTKEY	ds.l	1
TALTSHKEY	ds.l	1
TALTCLKEY	ds.l	1

SYSHDR	ds.w	27

B_structura:
	ds.l	1
B_showpos	ds.w	1

userstack	ds.l	1

datum	ds.w	1
d_v_t	ds.b	1
roky	ds.b	1
mesice	ds.b	1
dny	ds.b	1
hodiny	ds.b	1
minuty	ds.b	1
sekundy	ds.b	1
mys_ok	ds.w	1
adr_oldms	ds.l	1
zal_hss	ds.w	1
orig_STacy_video	ds.b	1
YY_sep_za	ds.b	1
ampm	ds.b	1
extended_kbd:
	ds.b	1

zatutat	ds.w	1	zaporny index od konce tabulky tutani
tut_wait	ds.w	1	cekani

Bkg_regs	ds.l	8
getcookie_regs	ds.l	3

	even
ehc_scantable
	ds.b	(2*128)	tabulka obsazen° jednotlivòch kl†ves EHC

* z†loha pozad° pod zobrazenòm áasem
	even
Bkg_space	ds.b	(2*(DELKA_1_RADKU+1))*(MAX_POCET_LINEK)*MAXCLKDEPTH
bkg_valid	ds.b	1

konec
