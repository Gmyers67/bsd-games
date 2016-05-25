// Copyright (c) 1994 The Regents of the University of California.
// This file is free software, distributed under the BSD license.

#include "gomoku.h"
#include <curses.h>
#include <signal.h>

enum {
    USER,	// get input from standard input
    PROGRAM,	// get input from program
    INPUTF	// get input from a file
};

int interactive = 1;		// true if interactive
int debug = 0;			// true if debugging
static int test = 0;		// both moves come from 1: input, 2: computer
static char* prog = NULL;	// name of program
static FILE* debugfp = NULL;	// file for debug output
static FILE* inputfp = NULL;	// file for debug input

const char pdir[4] = "-\\|/";
char fmtbuf[128] = "";

struct spotstr board[BAREA];	// info for board
struct combostr frames[FAREA];	// storage for all frames
struct combostr *sortframes[2];	// sorted list of non-empty frames
unsigned char overlap[FAREA * FAREA];	// true if frame [a][b] overlap
short intersect[FAREA * FAREA];	// frame [a][b] intersection
int movelog[BSZ * BSZ];		// log of all the moves
int movenum = 0;		// current move number
const char* plyr[2] = { NULL };	// who's who

int main (int argc, char* const* argv)
{
    char buf[128];
    int color = 0, curmove = 0, i;
    int input[2];
    static const char *const fmt[2] = {
	"%3d %-6s",
	"%3d        %-6s"
    };

    // Revoke setgid privileges
    setregid(getgid(), getgid());

    prog = strrchr(argv[0], '/');
    if (prog)
	prog++;
    else
	prog = argv[0];

    for (int ch; (ch = getopt(argc, argv, "bcdD:u")) != -1;) {
	switch (ch) {
	    case 'b':	       // background
		interactive = 0;
		break;
	    case 'd':	       // debugging
		debug++;
		break;
	    case 'D':	       // log debug output to file
		if ((debugfp = fopen(optarg, "w")) == NULL) {
		    printf ("Error: failed to open debug log %s\n", optarg);
		    return EXIT_FAILURE;
		}
		break;
	    case 'u':	       // testing: user verses user
		test = 1;
		break;
	    case 'c':	       // testing: computer verses computer
		test = 2;
		break;
	}
    }
    argc -= optind;
    argv += optind;
    if (argc) {
	if ((inputfp = fopen(*argv, "r")) == NULL) {
	    printf ("Error: failed to open input file %s\n", argv[0]);
	    return EXIT_FAILURE;
	}
    }

    if (!debug)
	srandrand();
    if (interactive)
	cursinit();	       // initialize curses
  again:
    bdinit(board);	       // initialize board contents

    if (interactive) {
	plyr[BLACK] = plyr[WHITE] = "???";
	bdisp_init();	       // initialize display of board
#ifndef NDEBUG
	signal(SIGINT, whatsup);
#else
	signal(SIGINT, quitsig);
#endif

	if (inputfp == NULL && test == 0) {
	    for (;;) {
		ask("black or white? ");
		gomoku_getline(buf, sizeof(buf));
		if (buf[0] == 'b' || buf[0] == 'B') {
		    color = BLACK;
		    break;
		}
		if (buf[0] == 'w' || buf[0] == 'W') {
		    color = WHITE;
		    break;
		}
		move(22, 0);
		printw("Black moves first. Please enter `black' or `white'\n");
	    }
	    move(22, 0);
	    clrtoeol();
	}
    } else {
	setbuf(stdout, 0);
	gomoku_getline(buf, sizeof(buf));
	if (strcmp(buf, "black") == 0)
	    color = BLACK;
	else if (strcmp(buf, "white") == 0)
	    color = WHITE;
	else {
	    sprintf(fmtbuf, "Huh?  Expected `black' or `white', got `%s'\n", buf);
	    panic(fmtbuf);
	}
    }

    if (inputfp) {
	input[BLACK] = INPUTF;
	input[WHITE] = INPUTF;
    } else {
	switch (test) {
	    case 0:	       // user verses program
		input[color] = USER;
		input[!color] = PROGRAM;
		break;

	    case 1:	       // user verses user
		input[BLACK] = USER;
		input[WHITE] = USER;
		break;

	    case 2:	       // program verses program
		input[BLACK] = PROGRAM;
		input[WHITE] = PROGRAM;
		break;
	}
    }
    if (interactive) {
	plyr[BLACK] = input[BLACK] == USER ? "you" : prog;
	plyr[WHITE] = input[WHITE] == USER ? "you" : prog;
	bdwho(1);
    }

    for (color = BLACK;; color = !color) {
      top:
	switch (input[color]) {
	    case INPUTF:      // input comes from a file
		curmove = readinput(inputfp);
		if (curmove != ILLEGAL)
		    break;
		switch (test) {
		    case 0:   // user verses program
			input[color] = USER;
			input[!color] = PROGRAM;
			break;

		    case 1:   // user verses user
			input[BLACK] = USER;
			input[WHITE] = USER;
			break;

		    case 2:   // program verses program
			input[BLACK] = PROGRAM;
			input[WHITE] = PROGRAM;
			break;
		}
		plyr[BLACK] = input[BLACK] == USER ? "you" : prog;
		plyr[WHITE] = input[WHITE] == USER ? "you" : prog;
		bdwho(1);
		goto top;

	    case USER:	       // input comes from standard input
	      getinput:
		if (interactive)
		    ask("move? ");
		if (!gomoku_getline(buf, sizeof(buf))) {
		    curmove = RESIGN;
		    break;
		}
		if (buf[0] == '\0')
		    goto getinput;
		curmove = ctos(buf);
		if (interactive) {
		    if (curmove == SAVE) {
			FILE *fp;

			ask("save file name? ");
			gomoku_getline(buf, sizeof(buf));
			if ((fp = fopen(buf, "w")) == NULL) {
			    glog("cannot create save file");
			    goto getinput;
			}
			for (i = 0; i < movenum - 1; i++)
			    fprintf(fp, "%s\n", stoc(movelog[i]));
			fclose(fp);
			goto getinput;
		    }
		    if (curmove != RESIGN && board[curmove].s_occ != EMPTY) {
			glog("Illegal move");
			goto getinput;
		    }
		}
		break;

	    case PROGRAM:     // input comes from the program
		curmove = pickmove(color);
		break;
	}
	if (interactive) {
	    sprintf(fmtbuf, fmt[color], movenum, stoc(curmove));
	    glog(fmtbuf);
	}
	if ((i = makemove(color, curmove)) != MOVEOK)
	    break;
	if (interactive)
	    bdisp();
    }
    if (interactive) {
	move(22, 0);
	switch (i) {
	    case WIN:
		if (input[color] == PROGRAM)
		    addstr("Ha ha, I won");
		else
		    addstr("Rats! you won");
		break;
	    case TIE:
		addstr("Wow! its a tie");
		break;
	    case ILLEGAL:
		addstr("Illegal move");
		break;
	}
	clrtoeol();
	bdisp();
	if (i != RESIGN) {
	  replay:
	    ask("replay? ");
	    if (gomoku_getline(buf, sizeof(buf)) && (buf[0] == 'y' || buf[0] == 'Y'))
		goto again;
	    if (strcmp(buf, "save") == 0) {
		FILE *fp;

		ask("save file name? ");
		gomoku_getline(buf, sizeof(buf));
		if ((fp = fopen(buf, "w")) == NULL) {
		    glog("cannot create save file");
		    goto replay;
		}
		for (i = 0; i < movenum - 1; i++)
		    fprintf(fp, "%s\n", stoc(movelog[i]));
		fclose(fp);
		goto replay;
	    }
	}
    }
    quit();
    // NOTREACHED
    return EXIT_SUCCESS;
}

int readinput (FILE* fp)
{
    char* cp = fmtbuf;
    for (int c; (c = getc(fp)) != EOF && c != '\n';)
	*cp++ = c;
    *cp = '\0';
    return ctos(fmtbuf);
}

#ifndef NDEBUG
// Handle strange situations.
void whatsup (int signum UNUSED)
{
    int i, n, s1, s2, d1, d2;
    struct spotstr *sp;
    FILE *fp;
    char *str;
    struct elist *ep;
    struct combostr *cbp;

    if (!interactive)
	quit();
  top:
    ask("cmd? ");
    if (!gomoku_getline(fmtbuf, sizeof(fmtbuf)))
	quit();
    switch (*fmtbuf) {
	case '\0':
	    goto top;
	case 'q':	       // conservative quit
	    quit();
	case 'd':	       // set debug level
	    debug = fmtbuf[1] - '0';
	    sprintf(fmtbuf, "Debug set to %d", debug);
	    dlog(fmtbuf);
	    sleep(1);
	case 'c':
	    break;
	case 'b':	       // back up a move
	    if (movenum > 1) {
		movenum--;
		board[movelog[movenum - 1]].s_occ = EMPTY;
		bdisp();
	    }
	    goto top;
	case 's':	       // suggest a move
	    i = fmtbuf[1] == 'b' ? BLACK : WHITE;
	    sprintf(fmtbuf, "suggest %c %s", i == BLACK ? 'B' : 'W', stoc(pickmove(i)));
	    dlog(fmtbuf);
	    goto top;
	case 'f':	       // go forward a move
	    board[movelog[movenum - 1]].s_occ = movenum & 1 ? BLACK : WHITE;
	    movenum++;
	    bdisp();
	    goto top;
	case 'l':	       // print move history
	    if (fmtbuf[1] == '\0') {
		for (i = 0; i < movenum - 1; i++)
		    dlog(stoc(movelog[i]));
		goto top;
	    }
	    if ((fp = fopen(fmtbuf + 1, "w")) == NULL)
		goto top;
	    for (i = 0; i < movenum - 1; i++) {
		fprintf(fp, "%s", stoc(movelog[i]));
		if (++i < movenum - 1)
		    fprintf(fp, " %s\n", stoc(movelog[i]));
		else
		    fputc('\n', fp);
	    }
	    bdump(fp);
	    fclose(fp);
	    goto top;
	case 'o':
	    n = 0;
	    for (str = fmtbuf + 1; *str; str++)
		if (*str == ',') {
		    for (d1 = 0; d1 < 4; d1++)
			if (str[-1] == pdir[d1])
			    break;
		    str[-1] = '\0';
		    sp = &board[s1 = ctos(fmtbuf + 1)];
		    n = (sp->s_frame[d1] - frames) * FAREA;
		    *str++ = '\0';
		    break;
		}
	    sp = &board[s2 = ctos(str)];
	    while (*str)
		str++;
	    for (d2 = 0; d2 < 4; d2++)
		if (str[-1] == pdir[d2])
		    break;
	    n += sp->s_frame[d2] - frames;
	    str = fmtbuf;
	    sprintf(str, "overlap %s%c,", stoc(s1), pdir[d1]);
	    str += strlen(str);
	    sprintf(str, "%s%c = %x", stoc(s2), pdir[d2], overlap[n]);
	    dlog(fmtbuf);
	    goto top;
	case 'p':
	    sp = &board[i = ctos(fmtbuf + 1)];
	    sprintf(fmtbuf, "V %s %x/%d %d %x/%d %d %d %x", stoc(i), sp->s_combo[BLACK].s, sp->s_level[BLACK], sp->s_nforce[BLACK], sp->s_combo[WHITE].s, sp->s_level[WHITE], sp->s_nforce[WHITE], sp->s_wval, sp->s_flg);
	    dlog(fmtbuf);
	    sprintf(fmtbuf, "FB %s %x %x %x %x", stoc(i), sp->s_fval[BLACK][0].s, sp->s_fval[BLACK][1].s, sp->s_fval[BLACK][2].s, sp->s_fval[BLACK][3].s);
	    dlog(fmtbuf);
	    sprintf(fmtbuf, "FW %s %x %x %x %x", stoc(i), sp->s_fval[WHITE][0].s, sp->s_fval[WHITE][1].s, sp->s_fval[WHITE][2].s, sp->s_fval[WHITE][3].s);
	    dlog(fmtbuf);
	    goto top;
	case 'e':	       // e {b|w} [0-9] spot
	    str = fmtbuf + 1;
	    if (*str >= '0' && *str <= '9')
		n = *str++ - '0';
	    else
		n = 0;
	    sp = &board[i = ctos(str)];
	    for (ep = sp->s_empty; ep; ep = ep->e_next) {
		cbp = ep->e_combo;
		if (n) {
		    if (cbp->c_nframes > n)
			continue;
		    if (cbp->c_nframes != n)
			break;
		}
		printcombo(cbp, fmtbuf);
		dlog(fmtbuf);
	    }
	    goto top;
	default:
	    dlog("Options are:");
	    dlog("q    - quit");
	    dlog("c    - continue");
	    dlog("d#   - set debug level to #");
	    dlog("p#   - print values at #");
	    goto top;
    }
}
#endif // NDEBUG

// Display debug info.
void dlog (const char *str)
{
    if (debugfp)
	fprintf (debugfp, "%s\n", str);
    if (interactive)
	dislog (str);
    else
	fprintf (stderr, "%s\n", str);
}

void glog (const char *str)
{
    if (debugfp)
	fprintf (debugfp, "%s\n", str);
    if (interactive)
	dislog (str);
    else
	printf ("%s\n", str);
}

void quit (void)
{
    if (interactive) {
	bdisp();	       // show final board
	cursfini();
    }
    exit (EXIT_SUCCESS);
}

void quitsig (int dummy UNUSED)
{
    quit();
}

// Die gracefully.
void panic (const char *str)
{
    fprintf (stderr, "%s: %s\n", prog, str);
    fputs ("resign\n", stdout);
    quit();
}
