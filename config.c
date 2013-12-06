/*
 * config - retrieve platform specific file from config database.
 *
 * this file is part of the makeutil package:
 *   https://github.com/alanpost/makeutil
 *
 * this file is hereby placed in the public domain.
 * alyn.post@lodockikumazvati.org
 */


#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

static char host[MAXHOSTNAMELEN];

static char _program[]="config";
static char _usage[]="[-I file] [-p] file...";
static char _help[]=
"spec format:\n"
"  [os]-[ver]:[kern]-[ver]:[arch]:[cc]-[ver]:[ccdesc]:[pdesc]:[filename]"
"\n"
"\n"
"config options:\n"
"  -I\tsearch directory or file for config specs\n"
"  -p\tprint spec to standard output instead of writing it to a file\n"
"\n"
"miscellaneous:\n"
"  -h\tdisplay this help and exit";

/*
 * runtime variables
 */
static struct prop {
  char *name;
  char *value;
} prop[]={
  { "CC", (char*)0 },
  { "CC-HOST", (char*)0 },
  { "CFLAGS", (char*)0 },
  { "CFLAGS-HOST", (char*)0 },
  { "CFLAGS-PIC", (char*)0 },
  { "LD", (char*)0 },
  { "LD-HOST", (char*)0 },
  { "LDFLAGS", (char*)0 },
  { "LDFLAGS-HOST", (char*)0 },
  { "LDFLAGS-PIC", (char*)0 },
  { "LDFLAGS-STATIC", (char*)0 },
  { "LDFLAGS-SONAME", (char*)0 },
  { "LDFLAGS-RPATH", (char*)0 },
  { "home", (char*)0 },
  { "prefix", (char*)0 },
  { "user", (char*)0 },
  { "host", (char*)0 },
  { "os", (char*)0 },
  { "osversion", (char*)0 },
  { "kern", (char*)0 },
  { "kernversion", (char*)0 },
  { "arch", (char*)0 },
  { "cc", (char*)0 },
  { "ccversion", (char*)0 },
  { "ccdesc", (char*)0 },
  { "pdesc", (char*)0 },
  { (char*)0, (char*)0 },
};

static void usage();
static void help();
static void errorlex();
static void errornofile();
static void errornowrite();
static void dienoprop();
static void dienofile();
static void dienomem();


#define OS          0x00
#define OSVERSION   0x01
#define KERN        0x02
#define KERNVERSION 0x03
#define ARCH        0x04
#define CC          0x05
#define CCVERSION   0x06
#define CCDESC      0x07
#define PDESC       0x08
#define FILENAME    0x09
#define CSIZE       0x0a

struct string {
  char *s_base;
  unsigned s_size;
  unsigned s_alloc;
};

struct conf {
  unsigned c_weight;
  struct string c_field[CSIZE];
  struct string c_file;
};

struct db {
  struct conf **db_conf;
  unsigned db_size;
  unsigned db_alloc;
};


/* config tokens */
#define LABEL 0xff+1
#define FIELD 0xff+2

struct lex {
  int ch;
  int type;
  int expect;
  int error;
  int state;
  char *base;
  unsigned size;
  struct string la;
};


/* property manipulation */
static void putprop();
static char* getprop();

/* string manipulation */
static void strinit();
static void strdestroy();
static void strcopy();
static void strcopy0();
static int strdeepen();
static int strappend();
static int strappend0();
static int strexpand();
static int strconcat();
static int strconcat0();
static int strequal();

/* config manipulation */
static int confinit();
static void confdestroy();
static struct conf* confalloc();
static struct conf* confnew();
static void conffree();
static int confparse();

/* db manipulation */
static void dbinit();
static void dbdestroy();
static int dbappend();

/* lexer manipulation */
static void lexinit();
static void lexdestroy();
static int lexch();
static void lexnext();
static int lexmatch();
static char* lexpname();

/* construct config db */
static int parsefile();
static int readline();

/* parse spec */
static struct conf* readspec();

/* weigh spec against config db record */
static unsigned weight();


main(argc, argv)
  char *argv[];
{
  struct db db;
  struct conf *spec;
  int ch, p, retval=0;
  unsigned i;
  char *arg;

  dbinit(&db);

  /*
   * process command line options.
   */
  argc-=1; /*skip program name*/
  argv+=1;

  /*
   * store any custom compiler properties
   */
  putprop("CC", getenv("CC"));
  putprop("CC-HOST", getenv("CC_HOST"));

  putprop("CFLAGS", getenv("CFLAGS"));
  putprop("CFLAGS-HOST", getenv("CFLAGS_HOST"));
  putprop("CFLAGS-PIC", getenv("CFLAGS_PIC"));

  /*
   * store any custom linker properties
   */
  putprop("LD", getenv("LD"));
  putprop("LD-HOST", getenv("LD_HOST"));

  putprop("LDFLAGS", getenv("LDFLAGS"));
  putprop("LDFLAGS-HOST", getenv("LDFLAGS_HOST"));
  putprop("LDFLAGS-PIC", getenv("LDFLAGS_PIC"));
  putprop("LDFLAGS-STATIC", getenv("LDFLAGS_STATIC"));
  putprop("LDFLAGS-SONAME", getenv("LDFLAGS_SONAME"));
  putprop("LDFLAGS-RPATH", getenv("LDFLAGS_RPATH"));


  putprop("home", getenv("HOME"));
  putprop("prefix", getenv("PREFIX"));
  arg=getenv("USER"); if(!arg) arg=getenv("LOGNAME");
  putprop("user", arg);

  if(-1==gethostname(host, sizeof(host)/sizeof(char))) {
    fprintf(stderr, "%s: error: gethostname: %s\n",
        _program,
        strerror(errno));
  }
  host[(sizeof(host)/sizeof(char))-1]='\0';
  putprop("host", host);

  spec=readspec("systype");
  if(!spec) dienofile("systype");

  p=0;
  while((arg=*argv) && '-'==*arg++) {
    --argc;
    ++argv;

    /*
     * if we have a "--", stop processing.  (ignore trailing garbage.)
     */
    if('-'==*arg) break;

    /*
     * process each option in this argv element.
     */
    while(ch=*arg++) {
      char *filename;
      FILE *file;

      switch(ch) {
      case 'I':
        if(*arg) {
          filename=arg;
          arg="";
        } else if(*argv) {
          filename=*argv++;
        } else {
          usage();
          exit(1);
        }

        file=fopen(filename, "r");
        if(!file) dienofile(filename);

        switch(parsefile(filename, file, &db)) {
        case 1: dienofile(filename);
        case 2: dienomem();
        case 3: dienomem();
        }

        fclose(file);
        break;

      case 'p':
        p=1;
        break;

      case 'h':
        help();
        exit(0);

      default:
        usage();
        exit(1);
      }
    }
  }

  while(arg=*argv) {
    --argc;
    ++argv;

    spec->c_field[FILENAME].s_base=arg;
    spec->c_field[FILENAME].s_size=strlen(arg);
    spec->c_field[FILENAME].s_alloc=0U;

    /*
     * find every record with the highest weight
     */
    for(i=0U; i<db.db_size; ++i) {
      unsigned w;

      w=weight(db.db_conf[i], spec);
      db.db_conf[i]->c_weight=w;
      if(w>spec->c_weight) spec->c_weight=w;
    }

    if(!spec->c_weight) {
      fprintf(stderr, "%s: error: [%s]: no match in db\n",
          _program,
          arg);
      retval|=1;
      continue;
    }

    for(i=0; i<db.db_size; ++i) {
      struct conf *conf;

      conf=db.db_conf[i];

      if(spec->c_weight==conf->c_weight) {
        char *filename="-";
        FILE *file=stdout;
        unsigned s;

        if(!p) {
          filename=conf->c_field[FILENAME].s_base;
          file=fopen(filename, "w");
        }

        if(!file) {
          errornofile(filename);
          retval|=1;
          continue;
        }

        s=fwrite(conf->c_file.s_base, conf->c_file.s_size, 1U, file);
        if(1U!=s) {
          errornowrite(filename);
          retval|=1;
          goto close;
        }

        if(EOF==fflush(file)) {
          errornowrite(filename);
          retval|=1;
          goto close;
        }

close:
        if(!p && EOF==fclose(file)) {
          errornowrite(filename);
          retval|=1;
        }
      }
    }
  }

  if(p && EOF==fclose(stdout)) {
    errornowrite("-");
    retval|=1;
  }

  dbdestroy(&db);
  conffree(spec);

  exit(retval);
}

/*static*/ void
usage()
{
  fprintf(stderr, "usage: %s %s\ntry `%s -h' for more information.\n",
      _program,
      _usage,
      _program);
}

/*static*/ void
help()
{
  fprintf(stderr, "usage: %s %s\n\n%s\n", _program, _usage, _help);
}

/*static*/ void
errorlex(filename, line, expect, got)
  char *filename;
  unsigned line;
{
  char e[2], g[2];

  fprintf(stderr,
      "%s: %s: %u: parse error\nexpecting: %s\ngot:     %s\n",
      _program,
      filename,
      line,
      lexpname(expect, e),
      lexpname(got, g));
  fflush(stderr);
}

/*static*/ void
errornofile(filename)
  char *filename;
{
  fprintf(stderr, "%s: error: open: %s: %s\n",
      _program,
      filename,
      strerror(errno));
  fflush(stderr);
}

/*static*/ void
errornowrite(filename)
  char *filename;
{
  fprintf(stderr, "%s: error: write: %s: %s\n",
      _program,
      filename,
      strerror(errno));
  fflush(stderr);
}

/*static*/ void
dienoprop(prop)
  char *prop;
{
  fprintf(stderr, "%s: error: %s: property not defined\n",
      _program,
      prop);
  fflush(stderr);
  exit(1);
}

/*static*/ void
dienofile(filename)
  char *filename;
{
  errornofile(filename);
  exit(1);
}

/*static*/ void
dienomem()
{
  fprintf(stderr, "%s: error: brk: %s\n",
      _program,
      strerror(errno));
  fflush(stderr);
  exit(1);
}

/*static*/ void
putprop(n, v)
  char *n, *v;
{
  struct prop *m=&prop[0];

  while(m->name) {
    if(0==strcmp(m->name, n)) {
      if(v && *v) m->value=v;
      return;
    }
    ++m;
  }

  dienoprop(n);
}

/*static*/ char*
getprop(n)
  char *n;
{
  struct prop *m=&prop[0];

  while(m->name) {
    if(0==strcmp(m->name, n)) {
      if(m->value) return m->value;
      return "";
    }
    ++m;
  }

  dienoprop(n);
}

/*static*/ void
strinit(s)
  struct string *s;
{
  s->s_base=(char*)0;
  s->s_size=0U;
  s->s_alloc=0U;
}

/*static*/ void
strdestroy(s)
  struct string *s;
{
  if(s->s_alloc && s->s_base) free(s->s_base);
}

/*static*/ void
strcopy(d, s)
  struct string *d, *s;
{
  d->s_base=s->s_base;
  d->s_size=s->s_size;
  d->s_alloc=0;
}

/*static*/ void
strcopy0(d, s)
  struct string *d;
  char *s;
{
  d->s_base=s;
  d->s_size=strlen(s);
  d->s_alloc=0;
}

/*static*/
strdeepen(s)
  struct string *s;
{
  char *p;

  if(s->s_size) {
    p=malloc(s->s_size);
    if(!p) return 0;

    memcpy(p, s->s_base, s->s_size);

    s->s_base=p;
    s->s_alloc=s->s_size;
  }

  return 1;
}

/*static*/
strappend(s, ch)
  struct string *s;
{
  if(s->s_size>=s->s_alloc) {
    char *string;
    unsigned alloc;

    if(s->s_alloc) alloc=s->s_alloc*2U;
    else alloc=16U;
    if(alloc<s->s_alloc) return 0;

    string=realloc(s->s_base, alloc*sizeof(char));
    if(!string) return 0;

    s->s_base=string;
    s->s_alloc=alloc;
  }

  s->s_base[s->s_size++]=ch;
  return 1;
}

/*static*/
strappend0(s)
  struct string *s;
{
  if(s->s_size>=s->s_alloc) {
    char *string;
    unsigned alloc;

    if(s->s_alloc) alloc=s->s_alloc*2U;
    else alloc=16U;
    if(alloc<s->s_alloc) return 0;

    string=realloc(s->s_base, alloc*sizeof(char));
    if(!string) return 0;

    s->s_base=string;
    s->s_alloc=alloc;
  }

  s->s_base[s->s_size]='\0';
  return 1;
}

/*static*/
strexpand(s)
  struct string *s;
{
  struct string t;
  char *p;
  unsigned i, l;
  int status=0, ch, state;

  strinit(&t);

  strcopy(&t, s);
  if(!strdeepen(&t)) goto error;

  s->s_size=0U;
  state=0;
  for(i=0U; i<t.s_size; ++i) {
    switch(t.s_base[i]) {
    case '$':
      switch(state) {
      case '$':
        state=0;
        if(!strappend(s, '$')) goto error;
        if(!strappend(s, t.s_base[i])) goto error;
        break;

      case '{':
        ++l;
        break;

      case '}':
        ++l;
        break;

      default:
        state='$';
        break;
      }
      break;

    case '{':
      switch(state) {
      case '$':
        state='{';
        break;

      case '{':
        ++l;
        break;

      case '}':
        ++l;
        break;

      default:
        if(!strappend(s, t.s_base[i])) goto error;
        break;
      }
      break;

    case '}':
      switch(state) {
      case '$':
        state=0;
        if(!strappend(s, '$')) goto error;
        if(!strappend(s, t.s_base[i])) goto error;
        break;

      case '{':
        /* empty property */
        state=0;
        break;

      case '}':
        ch=p[l]; p[l]='\0';
        if(!strconcat0(s, getprop(p))) {
          p[l]=ch;
          goto error;
        }
        p[l]=ch;
        state=0;
        break;

      default:
        if(!strappend(s, t.s_base[i])) goto error;
        break;
      }
      break;

    default:
      switch(state) {
      case '$':
        state=0;
        if(!strappend(s, '$')) goto error;
        if(!strappend(s, t.s_base[i])) goto error;
        break;

      case '{':
        p=&t.s_base[i];
        l=1U;
        state='}';
        break;

      case '}':
        ++l;
        break;

      default:
        if(!strappend(s, t.s_base[i])) goto error;
        break;
      }
    }
  }

  if(!strappend0(s)) goto error;
  status=1;

error:
  strdestroy(&t);
  return status;
}

/*static*/
strconcat(d, s)
  struct string *d, *s;
{
  unsigned size, alloc;

  size=d->s_size+s->s_size;
  alloc=d->s_alloc;

  if(size<d->s_size) return 0;
  if(size<s->s_size) return 0;

  if(size>=alloc) {
    char *string;

    while(size>=alloc) {
      if(alloc) alloc*=2U;
      else alloc=16U;
    }
    if(alloc<d->s_alloc) return 0;

    string=realloc(d->s_base, alloc*sizeof(char));
    if(!string) return 0;

    d->s_base=string;
    d->s_alloc=alloc;
  }

  memcpy(d->s_base+d->s_size, s->s_base, s->s_size);
  d->s_size=size;
  return 1;
}

/*static*/
strconcat0(d, p)
  struct string *d;
  char *p;
{
  struct string s;

  strcopy0(&s, p);
  return strconcat(d, &s);
}

/*static*/
strequal(s1, s2)
  struct string *s1, *s2;
{
  return s1->s_size==s2->s_size &&
       0==memcmp(s1->s_base, s2->s_base, s2->s_size);
}

/*static*/
confinit(c, filename, line, p, s)
  struct conf *c;
  char *filename;
  unsigned line;
  char *p;
  unsigned s;
{
  unsigned i;

  c->c_weight=0U;
  for(i=0U; i<CSIZE; ++i) strinit(&c->c_field[i]);
  strinit(&c->c_file);

  return confparse(c, filename, line, p, s);
}

/*static*/ void
confdestroy(c)
  struct conf *c;
{
  unsigned i;

  for(i=0U; i<CSIZE; ++i) strdestroy(&c->c_field[i]);
  strdestroy(&c->c_file);
}

/*static*/ struct conf*
confalloc()
{
  return (struct conf*)malloc(sizeof(struct conf));
}

/*static*/ struct conf*
confnew(filename, line, p, s)
  char *filename;
  unsigned line;
  char *p;
  unsigned s;
{
  struct conf *c;

  c=confalloc();
  if(!c) return (struct conf*)0;
  if(!confinit(c, filename, line, p, s)) {
    conffree(c);
    c=(struct conf*)0;
  }
  return c;
}

/*static*/ void
conffree(c)
  struct conf *c;
{
  if(c) {
    confdestroy(c);
    free(c);
  }
}

/*static*/
confparse(c, filename, line, b, s)
  struct conf *c;
  char *filename;
  unsigned line;
  char *b;
  unsigned s;
{
  struct lex l;

  lexinit(&l, b, s);

  /*
   * os-version
   */
  if(LABEL==l.type) {
    if(!lexmatch(&l, LABEL, LABEL, &c->c_field[OS]))
      goto error;
  }
  if(!lexmatch(&l, '-', LABEL, (struct string*)0)) goto error;
  if(LABEL==l.type) {
    if(!lexmatch(&l, LABEL, LABEL, &c->c_field[OSVERSION]))
      goto error;
  }

  if(!lexmatch(&l, ':', LABEL, (struct string*)0)) goto error;

  /*
   * kern-version
   */
  if(LABEL==l.type) {
    if(!lexmatch(&l, LABEL, LABEL, &c->c_field[KERN]))
      goto error;
  }
  if(!lexmatch(&l, '-', LABEL, (struct string*)0)) goto error;
  if(LABEL==l.type) {
    if(!lexmatch(&l, LABEL, LABEL, &c->c_field[KERNVERSION]))
      goto error;
  }

  if(!lexmatch(&l, ':', LABEL, (struct string*)0)) goto error;

  /*
   * arch
   */
  if(LABEL==l.type) {
    if(!lexmatch(&l, LABEL, LABEL, &c->c_field[ARCH]))
      goto error;
  }

  if(!lexmatch(&l, ':', LABEL, (struct string*)0)) goto error;

  /*
   * cc-version
   */
  if(LABEL==l.type) {
    if(!lexmatch(&l, LABEL, LABEL, &c->c_field[CC]))
      goto error;
  }
  if(!lexmatch(&l, '-', LABEL, (struct string*)0)) goto error;
  if(LABEL==l.type) {
    if(!lexmatch(&l, LABEL, LABEL, &c->c_field[CCVERSION]))
      goto error;
  }

  if(!lexmatch(&l, ':', LABEL, (struct string*)0)) goto error;

  /*
   * ccdesc
   */
  if(LABEL==l.type) {
    if(!lexmatch(&l, LABEL, LABEL, &c->c_field[CCDESC]))
      goto error;
  }

  if(!lexmatch(&l, ':', LABEL, (struct string*)0)) goto error;

  /*
   * pdesc
   */
  if(LABEL==l.type) {
    if(!lexmatch(&l, LABEL, LABEL, &c->c_field[PDESC]))
      goto error;
  }

  if(!lexmatch(&l, ':', FIELD, (struct string*)0)) goto error;

  /*
   * filename
   */
  if(FIELD==l.type) {
    if(!lexmatch(&l, FIELD, LABEL, &c->c_field[FILENAME]))
      goto error;
  }

  if(!lexmatch(&l, EOF, LABEL, (struct string*)0)) goto error;

  lexdestroy(&l);
  return 1;

error:
  errorlex(filename, line, l.expect, l.type);
  lexdestroy(&l);
  return 0;
}

/*static*/ void
dbinit(db)
  struct db *db;
{
  db->db_conf=(struct conf**)0;
  db->db_size=0U;
  db->db_alloc=0U;
}

/*static*/ void
dbdestroy(db)
  struct db *db;
{
  unsigned i;
  for(i=0U; i<db->db_size; ++i) conffree(db->db_conf[i]);
  if(db->db_conf) free(db->db_conf);
}

/*static*/
dbappend(db, spec)
  struct db *db;
  struct conf *spec;
{
  if(db->db_size>=db->db_alloc) {
    struct conf **conf;
    unsigned alloc;

    if(db->db_alloc) alloc=db->db_alloc*2U;
    else alloc=16U;
    if(alloc<db->db_alloc) return 0;

    conf=(struct conf**)realloc((char*)db->db_conf, alloc*sizeof(struct db));
    if(!conf) return 0;

    db->db_conf=conf;
    db->db_alloc=alloc;
  }

  db->db_conf[db->db_size++]=spec;
  return 1;
}

/*static*/ void
lexinit(l, b, s)
  struct lex *l;
  char *b;
  unsigned s;
{
  l->type=0;
  l->expect=0;
  l->error=0;
  l->state=LABEL;
  l->base=b;
  l->size=s;

  strinit(&l->la);

  l->ch=lexch(l);
  lexnext(l);
}

/*static*/ void
lexdestroy(l)
  struct lex *l;
{
  strdestroy(&l->la);
}

/*static*/
lexch(l)
  struct lex *l;
{
  int ch=EOF;

  if(l->size) {
    ch=*l->base;
    ++l->base;
    --l->size;
  }

  return ch;
}

/*static*/ void
lexnext(l)
  struct lex *l;
{
  l->la.s_size=0U;

  if(EOF==l->ch) { l->type=EOF; return; }

  switch(l->state) {
  case LABEL:
    if(isalnum(l->ch) || '/'==l->ch || '.'==l->ch) {
      do {
        if(!strappend(&l->la, l->ch)) {
          l->type=0;
          l->error|=1;
          return;
        }
        l->ch=lexch(l);
        if(EOF==l->ch) break;
      } while(isalnum(l->ch) || '/'==l->ch || '.'==l->ch);

      if(!strappend0(&l->la)) {
        l->type=0;
        l->error|=1;
        return;
      }

      l->type=LABEL;
      return;
    }
    break;

  case FIELD:
    if(isalnum(l->ch) || ispunct(l->ch)) {
      do {
        if(!strappend(&l->la, l->ch)) {
          l->type=0;
          l->error|=1;
          return;
        }
        l->ch=lexch(l);
        if(EOF==l->ch) break;
      } while(isalnum(l->ch) || ispunct(l->ch));

      if(!strappend0(&l->la)) {
        l->type=0;
        l->error|=1;
        return;
      }

      l->type=FIELD;
      return;
    }
    break;
  }

  l->type=l->ch;
  l->ch=lexch(l);
}

/*static*/
lexmatch(l, type, state, t)
  struct lex *l;
  struct string *t;
{
  if(l->type==type) {
    l->state=state;

    if(t) {
      strcopy(t, &l->la);
      if(!strdeepen(t)) {
        l->type=0;
        l->error|=1;
        return 0;
      }
      if(!strappend0(t)) {
        l->type=0;
        l->error|=1;
        return 0;
      }
    }

    lexnext(l);
    return 1;
  } else {
    l->expect=type;
  }

  return 0;
}

/*static*/ char*
lexpname(type, s)
  char *s;
{
  switch(type) {
  case LABEL: return "<label>";
  case FIELD: return "<label>";
  }

  s[0]=type;
  s[1]='\0';

  if(isalnum(type)) return s;
  if(ispunct(type)) return s;
  if(EOF==type) return "<eof>";
  if('\0'==type) return "<eof>";
  return "<char>";
}

/*
 *
 */
/*static*/ struct conf*
readspec(filename)
  char *filename;
{
  FILE *file;
  struct string s;
  struct conf *spec=0;

  strinit(&s);

  file=fopen(filename, "r");
  if(!file) goto error;
  readline(filename, file, &s);
  spec=confnew(filename, 1U, s.s_base, s.s_size);
  fclose(file);

  putprop("os",          spec->c_field[OS].s_base);
  putprop("osversion",   spec->c_field[OSVERSION].s_base);
  putprop("kern",        spec->c_field[KERN].s_base);
  putprop("kernversion", spec->c_field[KERNVERSION].s_base);
  putprop("arch",        spec->c_field[ARCH].s_base);
  putprop("cc",          spec->c_field[CC].s_base);
  putprop("ccversion",   spec->c_field[CCVERSION].s_base);
  putprop("ccdesc",      spec->c_field[CCDESC].s_base);
  putprop("pdesc",       spec->c_field[PDESC].s_base);

error:
  strdestroy(&s);
  return spec;
}

/*static*/
parsefile(filename, file, db)
  char *filename;
  FILE *file;
  struct db *db;
{
  struct string s;
  struct conf *c=0;
  unsigned line;
  int status=0;
  int last;

  strinit(&s);

  last=0;
  for(line=1U;!last;++line) {
    s.s_size=0U;
    switch(readline(filename, file, &s)) {
    case 1:
      last=1;
      status|=!!ferror(file);
      break;

    case 2:
      last=1;
      status|=2;
      break;
    }
    if(!s.s_size) {
      if(c && !strappend(&c->c_file, '\n'))
        dienomem();
      continue;
    }

    switch(s.s_base[0]) {
    case '@':
      c=confnew(filename, line, s.s_base+1, s.s_size-1U);
      if(!c) { status|=2; goto done; }
      if(!dbappend(db, c)) { conffree(c); status|=2; goto done; }
      break;

    default:
      if(c) {
        if(!strexpand(&s))
          dienomem();
        if(!strconcat(&c->c_file, &s))
          dienomem();
        if(!strappend(&c->c_file, '\n'))
          dienomem();
      }
      break;

    case '.':
      c=(struct conf*)0;
      break;
    }
  }

done:
  strdestroy(&s);
  return status;
}

/*static*/
readline(filename, file, s)
  char *filename;
  FILE *file;
  struct string *s;
{
  int ch;

  goto start;

  while('\n'!=ch) {
    if(!strappend(s, ch)) return 2;

start:
    ch=fgetc(file);
    if(EOF==ch) break;
  }

  if(!strappend0(s)) return 2;
  return EOF==ch;
}

/*
 * if the db field is null or empty, it matches any spec
 * field if db field does not match spec field, no match
 * otherwise match.  |w| is the relative weight of the
 * match.  exact matches give more weight.
 */
/*static*/ unsigned
weight(db, s)
  struct conf *db, *s;
{
  unsigned i, w;

  w=1U;
  for(i=0U; i<CSIZE; ++i) {
    if(!db->c_field[i].s_size) continue;
    if(!strequal(&db->c_field[i], &s->c_field[i])) return 0;
    ++w; /* non-blank field match */
  }

  return w;
}
