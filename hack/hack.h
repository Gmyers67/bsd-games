// Copyright (c) 1985, Stichting Centrum voor Wiskunde en Informatica,
// Copyright (c) 1982 Jay Fenlason <hack@gnu.org>
// This file is free software, distributed under the BSD license.

#pragma once
#include "config.h"
#include "objclass.h"

struct coord {
    uint8_t x;
    uint8_t y;
};

#include "monst.h"
#include "gold.h"
#include "trap.h"
#include "obj.h"
#include "flag.h"

static inline const char* plur (unsigned x)
    { return x == 1 ? "" : "s"; }
static inline char* newstring (size_t sz)
    { return alloc (sz); }

enum {
    BUFSZ	= 256,	       // for getlin buffers
    PL_NSIZ	= 16	       // name of player, ghost, shopkeeper
};

#include "rm.h"
#include "permonst.h"
#include "onames.h"
#include "mkroom.h"

enum { OFF, ON };

// prop.p_flgs values
enum {
    TIMEOUT	= 007777,	// mask
    LEFT_RING	= W_RINGL,	// 010000L
    RIGHT_RING	= W_RINGR,	// 020000L
    INTRINSIC	= 040000L,
    LEFT_SIDE	= LEFT_RING,
    RIGHT_SIDE	= RIGHT_RING,
    BOTH_SIDES	= LEFT_SIDE| RIGHT_SIDE
};

struct prop {
    long p_flgs;
    void (*p_tofn) (void);	// called after timeout
};

// Values of you.uluck
enum {
    LUCKMAX = 10,	// on moonlit nights 11
    LUCKMIN = -10
};

// Values of you.utraptype
enum { TT_BEARTRAP, TT_PIT };

enum {
    TELEPAT = LAST_RING,
    FAST,
    CONFUSION,
    INVIS,
    GLIB,
    SICK = GLIB+2,
    BLIND,
    WOUNDED_LEGS,
    STONED
};

struct you {
    uint8_t	ux;
    uint8_t	uy;
    int8_t	dx;
    int8_t	dy;
    int8_t	dz;		// direction of move (or zap or ... )
    uint8_t	udisx;
    uint8_t	udisy;		// last display pos
    char	usym;		// usually '@'
    int8_t	uluck;
    uint8_t	dlevel;
    uint8_t	maxdlevel;	// dungeon level
    int8_t	ustr;
    uint8_t	ustrmax;
    int8_t	udaminc;
    int8_t	uac;
    uint8_t	uinvault;
    int16_t	uhp;
    uint16_t	uhpmax;
    unsigned	ugold;
    unsigned	ugold0;
    unsigned	uexp;
    unsigned	urexp;
    unsigned	moves;
    int		uhunger;	// refd only in eat.c and shk.c
    int		last_str_turn:3;// 0: none, 1: half turn, 2: full turn  +: turn right, -: turn left
    unsigned	udispl:1;	// @ on display
    unsigned	ulevel:4;	// 1 - 14
    unsigned	utrap:3;	// trap timeout
    unsigned	utraptype:1;	// defined if utrap nonzero
    unsigned	umconf:1;
    unsigned	uhs:3;		// hunger state - see hack.eat.c
    unsigned	uinshop:6;	// used only in shk.c - (roomno+1) of shop
    const char*	usick_cause;
    struct prop	uprops [LAST_RING+10];
};

// perhaps these #define's should also be generated by makedefs
#define	Telepat		_u.uprops[TELEPAT].p_flgs
#define	Fast		_u.uprops[FAST].p_flgs
#define	Confusion	_u.uprops[CONFUSION].p_flgs
#define	Invis		_u.uprops[INVIS].p_flgs
#define Invisible	(Invis && !See_invisible)
#define	Glib		_u.uprops[GLIB].p_flgs
#define	Sick		_u.uprops[SICK].p_flgs
#define	Blind		_u.uprops[BLIND].p_flgs
#define Wounded_legs	_u.uprops[WOUNDED_LEGS].p_flgs
#define Stoned		_u.uprops[STONED].p_flgs

// convert ring to index in uprops
static inline unsigned PROP (uint8_t x) { return x-RIN_ADORNMENT; }

static inline unsigned DIST (int8_t x1, int8_t y1, int8_t x2, int8_t y2)
    { return square(x1-x2) + square(y1-y2); }

enum {
    PL_CSIZ	= 20,	// sizeof pl_character
    MAX_CARR_CAP= 120,	// so that boulders can be heavier
    MAXLEVEL	= 40,
    FAR	= COLNO+2	// position outside screen
};

enum {
    DUST = 1,
    ENGRAVE,
    BURN
};

struct engr {
    struct engr* nxt_engr;
    char*	engr_txt;
    int8_t	engr_x;
    int8_t	engr_y;
    int8_t	engr_type;
    uint8_t	engr_lth;	// for save & restore; not length of text
    unsigned	engr_time;	// moment engraving was (will be) finished
};

struct level {
    struct monst*	monsters;
    struct gold*	money;
    struct obj*		objects;
    struct trap*	traps;
    struct obj*		billobjs;
    struct engr*	engravings;
    struct {
	struct coord	up;
	struct coord	dn;
    }			stair;
    unsigned		lastvisit;
    struct coord	doors [DOORMAX];
    struct mkroom	rooms [MAXNROFROOMS+1];
    struct rm		l [COLNO][ROWNO];	// map
};
#define LEVEL_INIT	{}

extern struct level* _level;
extern struct level _levels [MAXLEVEL];

extern bool in_mklev;
extern bool level_exists[];
extern char *CD;
extern const char *const hu_stat[];	// in eat.c
extern const char *nomovemsg;
extern const char *occtxt;
extern char *save_cm;
extern const char *killer;
extern const char *const traps[];
extern const char mlarge[];
extern char morc;
extern char plname[PL_NSIZ], pl_character[PL_CSIZ];
extern const char quitchars[];
extern char sdir[];		// defined in hack.c
extern const char shtypes[];	// = "=/)%?!["; 8 types: 7 specialized, 1 mixed
extern const char vowels[];
extern struct coord bhitpos;		// place where thrown weapon falls to the ground
extern int (*afternmv) (void);
extern int (*occupation) (void);
extern int CO, LI;		// usually COLNO and ROWNO+2
extern int doorindex;
extern int multi;
extern int nroom;
extern long wailmsg;
extern int8_t xdir[], ydir[];	// idem
extern struct monst youmonst;
extern struct obj *invent, *uwep, *uarm, *uarm2, *uarmh, *uarms, *uarmg;
extern struct obj *uleft, *uright, *fcobj;
extern const struct permonst pm_eel;
extern const struct permonst pm_ghost;
extern const struct permonst pm_mail_daemon;
extern const struct permonst pm_wizard;
extern struct you _u;
extern int8_t curx, cury;	// cursor location on screen
extern int8_t seehx, seelx, seehy, seely;	// where to see
