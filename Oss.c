/* Copyright 2000, 2009, 2011, 2012 Neil Edelman, distributed under the terms
 of the GNU General Public License, see copying.txt */

/* it's the OSS game, by Neil Edelman (Dec. 2000) */
/* let's make it playable on the web (Dec 2009) */

#include <stdlib.h> /* malloc free */
#include <stdio.h>  /* fprintf */
#include <stdarg.h>
#include <string.h> /* manipulating query strings */
#include <ctype.h>  /* isalnum */
#include <time.h>   /* time */
#include "Oss.h"
#include "Mac.h"
#include "Room.h"

/* constants */
static const char *year        = "2000, 2011, 2012";
static const int versionMajor  = 3;
static const int versionMinor  = 0;
#define SCORES                 8
extern const int nowhereroom;
extern const int startroom;   /* def in Room */
static const char *title     = "OSS Game";
static const char *inputname = "choice";
static const char *exec      = "oss.cgi";
static const char *scorefile = "badass";
static const unsigned long forget_time = 1024; /* about 15 minutes */
/* my sever name */
#ifdef NEIL
static const char *valid     = "http://neil.chaosnet.org/cgi-bin/oss-game/oss.cgi";
static const char *start     = "http://neil.chaosnet.org/cgi-bin/oss-game/";
#endif

/* private */
struct Hi {
	char          last_ip[16]; /* xxx.xxx.xxx.xxx */
	char          last_game[HISTSIZE];
	unsigned long last_time;
	struct Oss    score[SCORES];
	int           size;
};

/* C89 (in which we compile, maybe) doesn't have this prototype (pedantic) */
/*int snprintf(char *str, size_t size, const char *format, ...);*/

static void Oss(struct Oss *oss);
static struct Oss *history(const char *history);
static int osscomp(const struct Oss *a, const struct Oss *b);
static char *query(void);
static char *get_next(char **queryp, const char *want);
static char *status(const struct Oss *o);
static char load(struct Oss *oss);
static int simplify(char *str);
static void printoss(const struct Oss *oss);
static int Hi(struct Hi *hi);
static void HiPrint(struct Hi *hi);
static void HiSave(struct Oss *oss);
static void usage(const char *argvz);

/** initialise */
static void Oss(struct Oss *oss) {
	if(!oss) return;
	oss->room      = startroom;
	oss->flags     = 0;
	oss->points    = 0;
	oss->friends   = 0;
	oss->enemies   = 0;
	oss->education = 0;
	oss->trouble   = 0;
	oss->history[0]= '\0';
	oss->name[0]   = '\0';
}

/** options */
void OssMessage(const struct Oss *o, const char *message, ...) {
	va_list ap;
	char *option, letter = 'a';
	
	/* print the text */
	printf("<p>%s</p>\n\n", message);
	printf("<p><form action = \"%s\" method = \"post\">\n", exec);
	/* hidden args */
	printf("<input type = \"hidden\" name = \"room\" value = \"%d\">\n", o->room);
	printf("<input type = \"hidden\" name = \"flags\" value = \"%d\">\n", o->flags);
	printf("<input type = \"hidden\" name = \"points\" value = \"%d\">\n", o->points);
	printf("<input type = \"hidden\" name = \"friends\" value = \"%d\">\n", o->friends);
	printf("<input type = \"hidden\" name = \"enemies\" value = \"%d\">\n", o->enemies);
	printf("<input type = \"hidden\" name = \"education\" value = \"%d\">\n", o->education);
	printf("<input type = \"hidden\" name = \"trouble\" value = \"%d\">\n", o->trouble);
	printf("<input type = \"hidden\" name = \"history\" value = \"%s\">\n", o->history);
	/* start at the first argument */
	va_start(ap, message);
	while((option = va_arg(ap, char *))) {
		printf("<input type = \"radio\" name = \"%s\" value = \"%c\">", inputname, letter);
		printf("%s<br>\n", option);
		if(++letter == 'z') break;
	}
	va_end(ap);
	/* end the form */
	printf("<input type = \"submit\" value = \"Do It\">\n");
	printf("</form></p>\n");
}

/* private */

/** given a history in choices, let it play out and return the result */
static struct Oss *history(const char *history) {
	void (*fn)(struct Oss *, const char, const int);
	static struct Oss oss;
	char              *h;
	
	Oss(&oss);
	for(h = (char *)history; *h; h++) {
		if(!(fn = RoomFn(oss.room))) break;
#ifdef DEBUG
		printf("<p>%s: %s(%c)</p>\n\n", RoomName(oss.room), h, *h);
		printoss(&oss);
#endif
		(*fn)(&oss, *h, 0);
	}
	return &oss;
}

/** compares the oss games, returns 0 if they're equal */
static int osscomp(const struct Oss *a, const struct Oss *b) {
	if(!a || !b) return 1;
	if(a->room      != b->room)      return -1;
	if(a->points    != b->points)    return -2;
	if(a->friends   != b->friends)   return -3;
	if(a->enemies   != b->enemies)   return -4;
	if(a->education != b->education) return -5;
	if(a->trouble   != b->trouble)   return -6;
	if(a->flags     != b->flags)     return -7;
	return 0;
}

/** get the query string */
static char *query(void) {
	/*'mac=[64]&room=xx&flags=xxxx&points=xxx&friends=xxx&ememies=xxx&education=xxx&trouble=xxx&choice=xx'*/
	/*'mac=[64]&room=xx&flags=xxxx&points=xxx&friends=xxx&ememies=xxx&education=xxx&trouble=xxx&name=xxxxxxx'*/
	static char post[192];
	char *query = 0, *method;

	if(!(method = getenv("REQUEST_METHOD"))) return 0;
	if(!strcmp("POST", method)) {
		fgets(post, sizeof(post), stdin);
		query = post;
	} else if(!strcmp("GET", method)) {
		query = getenv("QUERY_STRING");
	} else {
		printf("<p>Undefined request method, &quot;%s.&quot;</p>\n\n", method);
	}
	
	return query;
}

/** gets a query string "want" and advances the queryPtr, or does nothing if
 "want" is not arg */
static char *get_next(char **queryp, const char *want) {
	char *thing = 0, *query;

	/* paranoid */
	if(!queryp || !(query = *queryp) || !want) return 0;
	/* thing is at '=', make sure the 'want' is actually what we have */
	if(!(thing = strpbrk(query, "="))) return 0;
	if(strncmp(query, want, thing - query)) return 0;
	thing++;
	/* move to the next arg, return it and advance the query */
	if((query = strpbrk(thing, "&"))) {
		*query = '\0';
		query++;
		*queryp = query;
	} else {
		*queryp = 0;
	}

	return thing;
}

/** for MAC */
static char *status(const struct Oss *o) {
	static char status[64];
	/* rXXfXXpXXfXXeXXeXXtXX */
	snprintf(status, 64, "r%df%dp%df%de%de%dt%d", o->room, o->flags,
			 o->points, o->friends, o->enemies, o->education, o->trouble);
	return status;
}

/** load game */
static char load(struct Oss *oss) {
	char *env, *q;
	char *sRoom, *sFlags, *sPoints, *sFriends, *sEnemies, *sEducation;
	char *sTrouble, *sChoice, *sHistory, choice;
	char *sMac, *cMac, *sName;
	int histSize;
	struct Oss *check;
	
	/* initial game state */
	Oss(oss);
	/* check env vars */
	if(!(env = getenv("REQUEST_METHOD")) || strcmp(env, "POST")) return 0;
	/* not valid referrer, show the hi-scores */
#ifdef NEIL
	if(!(env = getenv("HTTP_REFERER"))   || strncmp(env, valid, strlen(valid))) {
		struct Hi hi;
		if(Hi(&hi)) HiPrint(&hi);
		else printf("<p>Error loading high scores.</p>\n\n");
		printf("<p>Go back to the start of <a href = \"%s\">OSS game</a>.</p>", start);
		printf("</body>\n\n");
		printf("</html>\n");
		exit(0);
	}
#endif
	if(!(q = query())) return 0;
	/* deconstruct query */
	if(!(sRoom = get_next(&q, "room"))) return 0;
	if(!(sFlags = get_next(&q, "flags"))) return 0;
	if(!(sPoints = get_next(&q, "points"))) return 0;
	if(!(sFriends = get_next(&q, "friends"))) return 0;
	if(!(sEnemies = get_next(&q, "enemies"))) return 0;
	if(!(sEducation = get_next(&q, "education"))) return 0;
	if(!(sTrouble = get_next(&q, "trouble"))) return 0;
	if(!(sHistory = get_next(&q, "history"))) return 0;
	/* history can only be HISTSIZE-1 */
	histSize = strlen(sHistory);
	if(histSize >= HISTSIZE)
		{ printf("<p>Too much history!</p>\n\n"); return 0; }
	oss->room      = atoi(sRoom);
	oss->flags     = atoi(sFlags);
	oss->points    = atoi(sPoints);
	oss->friends   = atoi(sFriends);
	oss->enemies   = atoi(sEnemies);
	oss->education = atoi(sEducation);
	oss->trouble   = atoi(sTrouble);
	if((sChoice = get_next(&q, "choice"))) {
		if(sChoice[0] == '\0' || sChoice[1] != '\0')
			{ printf("<p>Choice is not single character.</p>\n\n"); return 0; }
		if(histSize >= HISTSIZE - 1)
			{ printf("<p>Too much history.</p>\n\n"); return 0; }
		choice = sChoice[0];
		sprintf(oss->history, "%s%c", sHistory, choice);
	} else {
		strcpy(oss->history, sHistory);
		/* high score? */
		if(!(sName = get_next(&q, "name")) || !simplify(sName))
			{ printf("<p>Invalid entry; sorry try again.</p>\n\n"); return 0; }
		strcpy(oss->name, sName);
		/* check MAC */
		if(!(sMac = get_next(&q, "mac")))
			{ printf("<p>Submitted to high-score without Message Authentication Code.</p>\n\n"); return 0; }
		cMac = Mac(status(oss));
		if(strcmp(sMac, cMac))
			{ printf("<p>Message Authentication Code check failed!</p>\n\n"); return 0; }
		/* check history (muhaha!) */
		if(osscomp(oss, check = history(oss->history))) {
			printf("<p>Data given does not support score; how did you get here?</p>\n\n");
			printf("<p>You scored:</p>\n\n");
			printoss(oss);
			printf("<p>But the history says:</p>\n\n");
			printoss(check);
			return 0;
		}
		/* flags it as not an error */
		return -1;
	}
#ifdef DEBUG
	printf("Load: room:%d;flags:%d;points:%d;f%de%de%dt%d;h:%s;%c<br><br>\n",
		   oss->room, oss->flags, oss->points, oss->friends, oss->enemies,
		   oss->education, oss->trouble, oss->history, choice == 0 ? '0' : choice);
#endif
	return choice;
}

/** gets rid of all not isalnum, returns the size of the result */
static int simplify(char *str) {
	char *i, *o;
	for(i = o = str; ; i++) {
		if(*i == '\0') {
			*o = '\0';
			break;
		} else if(*i == '%') {
			if(!i[1] || !i[2]) continue;
			i += 2; /* forget fancy chars, too much work! */
		} else if(isalnum((int)*i)) {
			*o = *i;
			o++;
		}
	}
	return o - str;
}

/** entry point */
int main(int argc, char **argv) {
	char       choice;
	void       (*fn)(struct Oss *, const char, const int);
	struct Oss oss;

	if(argc > 1) { usage(argv[0]); return 0; }
	printf("Content-Type: text/html; encoding=us-ascii\n\n");
	printf("<!doctype html public \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n\n");
	printf("<html>\n\n");
	printf("<head>\n");
	printf("<title>%s</title>\n", title);
	printf("<meta name = \"Content-type\" http-equiv = \"Content-Type\" content = \"text/html; charset=us-ascii\">\n");
	printf("<link rel = \"top\" href = \"http://neil.chaosnet.org/\">\n");
	printf("<link rel = \"icon\" href = \"/favicon.ico\" type = \"image/x-icon\">\n");
	printf("<link rel = \"shortcut icon\" href = \"/favicon.ico\" type = \"image/x-icon\">\n");
	printf("<link rel = \"stylesheet\" href = \"/neil.css\">\n");
	printf("</head>\n\n");
	printf("<body>\n");
	printf("<h1>%s</h1>\n", title);
	/* load a game based on POST */
	choice = load(&oss);
	/* find the room */
	fn = RoomFn(oss.room);
	/* execute the room */
	if(fn && choice) {
		printf("<p>\n");
		(*fn)(&oss, choice, -1);
		printf("</p>\n");
		/* the room now is no longer valid, find the new room */
		fn = RoomFn(oss.room);
		if(strlen(oss.history) >= HISTSIZE - 1) {
			printf("<p>You pass out from exhaustion. You shouldh've eaten a hardier breakfast.</p>\n\n");
			fn = 0;
		}
	}
	/* print the new room */
	if(fn) {
		printoss(&oss);
		(*fn)(&oss, 0, -1);
	} else if(choice) {
		/* isn't in a room! the play has stopped, choice has been flagged as
		 valid in load(), end of the game! */
		HiSave(&oss);
	}
	printf("<p>Go back to <a href = \"index.html\">The Beginning</a>.</p>\n");
	printf("</body>\n\n");
	printf("</html>\n");
	
	return EXIT_SUCCESS;
}

/* Oss to html */
static void printoss(const struct Oss *oss) {
	printf("<table class = \"nice\">\n<tr><th>Score</th><th>Friends</th><th>Enemies</th><th>Education</th><th>Detentions</th></tr>\n");
	printf("<tr class = \"even\"><td>%d</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td></tr>\n",
		   oss->points, oss->friends, oss->enemies, oss->education, oss->trouble);
	printf("</table>\n\n");
}

/** loads the high scores */
static int Hi(struct Hi *hi) {
	struct Oss *s;
	FILE *fp;
	int i;

	if(!(fp = fopen(scorefile, "r"))) return 0;
	if(fscanf(fp, "%15s %lu %63s\n", hi->last_ip, &hi->last_time, hi->last_game) != 3)
		{ fclose(fp); return 0; }
	for(i = 0; i < SCORES; i++) {
		s = &hi->score[i];
		if(fscanf(fp, "%d, %d, %d, %d, %d, %d, %hd, %7s\n",
				  &s->room, &s->points, &s->friends, &s->enemies, &s->education,
				  &s->trouble, &s->flags, s->name) != 8) break;
	}
	fclose(fp);
	hi->size = i;
	return -1;
}

/** prints the high scores */
static void HiPrint(struct Hi *hi) {
	struct Oss *s;
	int i;

	printf("<table class = \"nice\">\n<caption>High-Score Table</caption>\n");
	printf("<tr><th>Name</th><th>Score</th><th>Friends</th><th>Enemies</th><th>Education</th><th>Detentions</th></tr>\n");
	for(i = 0; i < hi->size; i++) {
		s = &hi->score[i];
		printf("<tr class = \"%s\"><td>%s</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td></tr>\n",
			   i & 1 ? "odd" : "even", s->name, s->points, s->friends,
			   s->enemies, s->education, s->trouble);
	}
	printf("</table>\n\n");
}

/** this does a high score, assumed they passed the hacker's test */
static void HiSave(struct Oss *oss) {
	struct Hi hi;
	struct Oss *s;
	FILE *fp;
	int p, i, named = oss->name[0] == '\0' ? 0 : -1;
	char *ip;

	if(!named) {
		printf("<p><em>Game over</em>; you: </p>\n\n");
		RoomReport(oss);
	}
	if(!Hi(&hi)) {
		printf("<p>Error loading the high-scores table.</p>\n\n");
		return;
	}
	if(!named) {
		HiPrint(&hi);
	}
	/* find the position in the high score table */
	for(p = 0; p < hi.size; p++) if(oss->points >= hi.score[p].points) break;
	/* did not make it */
	if(p >= SCORES) return;
	/* is not named */
	if(!named) {
		printf("<p>You made a high-score; enter your mark to live in glory forever:\n");
		printf("<form action = \"%s\" method = \"post\">\n", exec);
		printf("<input type = \"hidden\" name = \"room\" value = \"%d\">\n", oss->room);
		printf("<input type = \"hidden\" name = \"flags\" value = \"%d\">\n", oss->flags);
		printf("<input type = \"hidden\" name = \"points\" value = \"%d\">\n", oss->points);
		printf("<input type = \"hidden\" name = \"friends\" value = \"%d\">\n", oss->friends);
		printf("<input type = \"hidden\" name = \"enemies\" value = \"%d\">\n", oss->enemies);
		printf("<input type = \"hidden\" name = \"education\" value = \"%d\">\n", oss->education);
		printf("<input type = \"hidden\" name = \"trouble\" value = \"%d\">\n", oss->trouble);
		printf("<input type = \"hidden\" name = \"history\" value = \"%s\">\n", oss->history);
		printf("<input type = \"text\" name = \"name\" size = \"7\" maxlength = \"7\" value = \"Eve\">\n");
		printf("<input type = \"hidden\" name = \"mac\" value = \"%s\">\n", Mac(status(oss)));
		printf("<input type = \"submit\" value = \"Right\">\n");
		printf("</form></p>\n\n");
		return;
	}
	/* check if the score is new */
	for(i = p; i < hi.size; i++) {
		if(oss->points != hi.score[i].points) break;
		if(!strcmp(oss->name, hi.score[i].name)) {
			printf("<p>&quot;%s&quot; is already the high score with %d points.</p>\n\n", oss->name, oss->points);
			return;
		}
	}
	/* check if it's the same player entering different names */
	if(!(ip = getenv("REMOTE_ADDR")))
		{ printf("Your REMOTE_ADDR needs to be recorded to get on the high-scores table.\n"); return; }
	if(!strcmp(hi.last_ip, ip) && hi.last_time + forget_time > time(0) && !strcmp(hi.last_game, oss->history)) {
		printf("This REMOTE_ADDR has recently been added to the high-score table with that exact same game.\n");
		return;
	}
	/* move the scores down to make space */
	if(p < hi.size) {
		int size = (hi.size < SCORES) ? hi.size - p : SCORES - 1 - p;
		memmove(&hi.score[p+1], &hi.score[p], sizeof(struct Oss)*size);
	}
	if(hi.size < SCORES) hi.size++;
	memcpy(&hi.score[p], oss, sizeof(struct Oss));
	/* save score! */
	if(!(fp = fopen(scorefile, "w")))
		{ printf("<p>Error saving the score.</p>\n\n"); return; }
	/* time_t -> unsigned long is scetch */
	fprintf(fp, "%.15s %lu %.63s\n", getenv("REMOTE_ADDR"), (unsigned long)time(0), oss->history);
	for(i = 0; i < hi.size; i++) {
		s = &hi.score[i];
		fprintf(fp, "%d, %d, %d, %d, %d, %d, %hd, %7s\n",
				s->room, s->points, s->friends, s->enemies, s->education,
				s->trouble, s->flags, s->name);
	}
	fclose(fp);
	printf("<p>%s shall be known for all enternity with a high-score of %d.</p>\n\n", oss->name, oss->points);
}

static void usage(const char *argvz) {
	fprintf(stderr, "Usage: %s\n", argvz);
	fprintf(stderr, "Version %d.%d.\n\n", versionMajor, versionMinor);
	fprintf(stderr, "%s Copyright %s Neil Edelman\n", title, year);
	fprintf(stderr, "Uses HMAC-SHA-256 code Copyright (C) 2005 Olivier Gay.\n\n");
	fprintf(stderr, "This program comes with ABSOLUTELY NO WARRANTY.\n");
	fprintf(stderr, "This is free software, and you are welcome to redistribute it\n");
	fprintf(stderr, "under certain conditions; see copying.txt.\n\n");
}
