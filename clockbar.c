#include <u.h>
#include <libc.h>
#include <draw.h>
#include <bio.h>
#include <event.h>
#include <keyboard.h>

enum {
	Nsec = 1000*1000*1000,
};

int newwin(char*);

int nokill;
char *title = nil;
char *message = nil;
Biobuf *bout;
int delay = 1000;

Image *light;
Image *text;
Rectangle rtext;
Tzone *tz;

void
initcolor(void)
{
	text = display->black;
	light = allocimagemix(display, DPalegreen, DWhite);
	if(light == nil) sysfatal("initcolor: %r");
}

void
drawmsg(void)
{
	draw(screen, rtext, light, nil, ZP);
	string(screen, rtext.min, text, ZP, display->defaultfont, message);
	flushimage(display, 1);
}

void
eresized(int new)
{
	if(new && getwindow(display, Refnone) < 0)
		fprint(2,"can't reattach to window");
	rtext = screen->r;
	draw(screen, rtext, light, nil, ZP);
	rtext.min.x += 4;
	rtext.min.y += 4;
	if(title){
		string(screen, rtext.min, text, ZP, display->defaultfont, title);
		rtext.min.y += 8+display->defaultfont->height;
	}
	rtext.max.y = rtext.min.y + display->defaultfont->height;
	drawmsg();
}

char*
datestring(void)
{
	char *fmt = "WW MMM _D hh:mm:ss ZZZ YYYY";
	vlong s, ns;
	Tm tm;
	char *datestring;


	ns = nsec();
	s = ns/Nsec;
	ns = ns%Nsec;

	if(tmtimens(&tm, s, ns, tz) == nil)
		sysfatal("now: %r");

	datestring = smprint("%τ", tmfmt(&tm, fmt));
	if(!datestring)
		sysfatal("%r");
	return datestring;
}

void
msg(void)
{
	char *p;
	Event e;
	int k;

	for(;;){
		if(ecanmouse() || ecankbd()) {
			k = eread(Ekeyboard|Emouse, &e);
			if(k == Ekeyboard && (e.kbdc == Kdel || e.kbdc == Ketx))
				break;
		}
		p = datestring();
		snprint(message, Bsize, "%.*s", utflen(p), p);
		drawmsg();
		free(p);
		sleep(delay);
	}
}


void
usage(void)
{
	fprint(2, "usage: %s [-k] [-w minx,miny,maxx,maxy] [title]\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *p, *q;

	p = "0,0,200,60";
	tmfmtinstall();
	
	ARGBEGIN{
	case 'w':
		p = ARGF();
		break;
	case 'k':
		nokill = 1;
		break;
	default:
		usage();
	}ARGEND;

	switch(argc){
	default:
		usage();
	case 1:
		title = argv[0];
	case 0:
		break;
	}

	while(q = strchr(p, ','))
		*q = ' ';
	if((message = malloc(Bsize)) == nil)
		sysfatal("malloc: %r");
	memset(message, 0, Bsize);
	if((tz = tzload("local")) == nil)
		sysfatal("timezone: %r");
	if(newwin(p) < 0)
		sysfatal("newwin: unable to open");

	if(initdraw(0, 0, title ? title : argv0) < 0)
		sysfatal("initdraw: %r");
	initcolor();
	einit(Emouse|Ekeyboard);
	eresized(0);

	msg();

	exits(0);
}

int
newwin(char *win)
{
	char spec[100];
	int cons;

	if(win != nil){
		snprint(spec, sizeof(spec), "-r %s", win);
		win = spec;
	}
	if(newwindow(win) < 0){
		fprint(2, "%s: newwindow: %r", argv0);
		return -1;
	}
	if((cons = open("/dev/cons", OREAD)) < 0){
	NoCons:
		fprint(2, "%s: can't open /dev/cons: %r", argv0);
		return -1;
	}
	dup(cons, 0);
	close(cons);
	if((cons = open("/dev/cons", OWRITE)) < 0)
		goto NoCons;
	dup(cons, 1);
	dup(cons, 2);
	close(cons);
	return 0;
}
