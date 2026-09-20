/*temu: teensy eensy markup language
 (c) 5qr1 WTFPL 2026 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define LENGTH(x) sizeof(x)/sizeof(x[0])
#define BUFSIZE 2048

typedef int (*Parser)(char *, char *);
static void eprint(int err, const char *fmt, ...);
static void *emalloc(void *in, size_t n);
static char *lfiletobuf(FILE *in);
static void process(char *st, char *en, Parser pl[]);
static int code(char *st, char *en);
static int underlines(char *st, char *en);
static int blockquotes(char *st, char *en);
static int paragraphs(char *st, char *en);
static int inlinecode(char *st, char *en);
static int links(char *st, char *en);
static int replace(char *st, char *en);
Parser parsers[] = {code, underlines, blockquotes, paragraphs, inlinecode, links, replace};

int
main(int argc, char **argv) {
	FILE *s = stdin;
	if(argc > 1) {
	if(!(strcmp(argv[1], "-v")))
		eprint(0, "temu v%s\n", VERSION);
	if(!(s = fopen(argv[1], "r")))
		eprint(EXIT_FAILURE, "bad file\n");
	}
	
	char *buf = lfiletobuf(s);
	process(buf, &buf[strlen(buf)], 0);

	fclose(s);
	free(buf);
	exit(0);
}

static void
eprint(int err, const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	exit(err);
}

static void
*emalloc(void *in, size_t n) {
	void *out;
	if(n <= 0)
		n = 1;
	if(in)
		out = realloc(in, n);
	else
		out = malloc(n);
	if(!out)
		eprint(EXIT_FAILURE, "emalloc() failed\n");

	return out;
}

static char
*lfiletobuf(FILE *in) {
	if(!in)
		return 0;

	char *buf = emalloc(NULL, BUFSIZE);
	buf[0] = '\n';

	size_t s, len = 0, bsize = 2 * BUFSIZE;
	while((s = fread(buf + len + 1, 1, BUFSIZE, in))) {
		len += s;
		if(BUFSIZE + len + 1 > bsize) {
			bsize += BUFSIZE;
			buf = emalloc(buf, bsize);
		}
	}
	strcpy(buf + len + 1, "\n\n");
	return buf;
}

static void
process(char *st, char *en, Parser pl[]) {
	if(!(st) || !(en))
		return;
	
	for(char *p = st; p < en; p++) {
		int ch = 0;
		if(pl)
			for(size_t i = 0; pl[i] != NULL && !ch; i++)
				ch = pl[i](p, en);
		else 
			for(size_t i = 0; i < LENGTH(parsers) && !ch; i++)
				ch = parsers[i](p, en);
		if(ch)
			p += ch - 1;
		else
			putc(p[0], stdout);
	}
}

static int
code(char *st, char *en) {
	if(!(st) || st[0] != '\n' || st[1] != '`')
		return 0;
	
	char *p = st + 2;
	for(;p < en && p[0] != '`'; p++);
	if(p >= en)
		return 0;

	fputs("\n<pre><code>\n", stdout);
	process(st + 2, p, (Parser[]){replace, NULL});
	fputs("\n</code></pre>", stdout);
	return (p - st) + 1;
}

static int
underlines(char *st, char *en) {
	if(!(st) || st[0] != '\n')
		return 0;
	
	char *p = st + 1;
	for(;p < en && p[0] != '\n'; p++);
	if(p >= en)
		return 0;

	p++;
	char c = p[0];
	if(c != '=' && c != '-') 
		return 0;
	int l = p - (st + 1);

	for(; p < en && p[0] == c; p++);
	if(p[0] != '\n')
		return 0;

	if(p - (st + l) != l)
		return 0;

	if(c == '=') {
		fputs("\n<h1>\n", stdout);
		process(st + 1, st + l, (Parser[]){inlinecode, replace, NULL});
		fputs("\n</h1>\n", stdout);
	} else if(c == '-') {
		fputs("\n<h2>\n", stdout);
		process(st + 1, st + l, (Parser[]){inlinecode, replace, NULL});
		fputs("\n</h2>\n", stdout);
	}

	if((en - p) > 2 && p[1] == '\n')
		return (p - st) + 1;
	else
		return(p - st);
}

static int
blockquotes(char *st, char *en) {
	if(!(st) || st[0] != '\n' || st[1] != '\t')
		return 0;
	char *p = st + 1;

	for(;p < en; p++)
		if(p[0] == '\n' && p[1] == '\n')
			break;
	if(!p || p >= en)
		return 0;
	
	fputs("<blockquote>\n", stdout); /* intentionally missing leading \n */
	process(st + 1, p, (Parser[]){inlinecode, links, replace, NULL});
	fputs("\n</blockquote>\n", stdout);
	return (p - st) + 1;
}

static int
paragraphs(char *st, char *en) {
	if(!(st) || st[0] != '\n' || st[1] == '\n')
		return 0;
	char *p = st + 1;

	for(; p < en; p++)
		if((p[0] == '\n' && p[1] == '\n') || (p[0] == '\n' && p[1] == '\t'))
			break;
	if(!p || p >= en)
		return 0;

	fputs("\n<p>\n", stdout);
	process(st + 1, p, (Parser[]){inlinecode, links, replace, NULL});
	fputs("\n</p>\n", stdout);

	if(p[1] == '\t')
		return p - st;
	return (p - st) + 1;
}

static int
inlinecode(char *st, char *en) {
	if(!(st) || st[0] != '`')
		return 0;
	
	char *p = st + 1;
	for(;p < en && p[0] != '`'; p++);
	if(p >= en)
		return 0;

	fputs("<code>", stdout);
	process(st + 1, p, (Parser[]){replace, NULL});
	fputs("</code>", stdout);
	return (p - st) + 1;
}

/* todo: make less sloppy */
static int
links(char *st, char *en) {
	char *c, *buf, *p = st;
	
	for(; p < en; p++) {
		if(p[0] == '\n' || p[0] == ' ' || p[0] == '\t')
			return 0;
		if(p[0] == '!' || p[0] == ':') {
			c = p;
			break;
		}
	} if(p + 3 >= en || p[1] != '/' || p[2] != '/')
		return 0;
	for(p += 2; p < en && p[0] != '\n' && p[0] != ' '; p++);
	
	if(c == st) {
		buf = emalloc(NULL, (p - (c + 3)));
		memcpy(buf, c + 3, (p - (c + 3)));
		buf[(p - (c + 3))] = '\0';
	} else {
		buf = emalloc(NULL, (p - st));
		memcpy(buf, st, p - st);
		buf[(p - st)] = '\0';
		if(c[0] == '!')
			buf[c - st] = ':';
	}

	if(c[0] == '!')
		printf("<img src=\"%s\">", buf);
	else
		printf("<a href=\"%s\">%s</a>", buf, buf);
	free(buf);
	return (p - st);
}

static int
replace(char *st, char *en) {
	if(!(st))
		return 0;
	switch(st[0]) {
		case '&':
			fputs("&amp;", stdout);
			return 1;
		case '<':
			fputs("&lt;", stdout);
			return 1;
		case '>':
			fputs("&gt;", stdout);
			return 1;
		case '\"':
			fputs("&quot;", stdout);
			return 1;
		case '\'':
			fputs("&#39;", stdout);
			return 1;
		case '\t':
			/*putc(' ', stdout);*/
			return 1;
		default:
			return 0;
	}
}
