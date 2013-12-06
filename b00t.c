/*
 * l1sp - library for basic in-process lisp scripting
 *
 * this file is part of the makeutil package:
 *   http://sourceforge.net/projects/makeutil/
 *   http://www.cybermesa.com/~aisa/makeutil/
 *
 * this file is hereby placed in the public domain.
 * aisa0@users.sourceforge.net, aisa@cybermesa.com
 */

static char rcsid[]="$Id: b00t.c,v 1.2 2006/07/25 02:29:15 aisa0 Exp $";

#define _LARGEFILE_SOURCE 1
#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>
#include <ctype.h>
#include <errno.h>

/*
 * the default size of one semispace, which
 * is the amount of space available for
 * objects.  actual memory used is twice this,
 * as we use a copying garbage collector.
 */
#ifndef L1SP_SEMISPACE
#define L1SP_SEMISPACE 32768
#endif

/* the size of both the input buffer and the
 * output buffer.  is also the maximum size
 * of a symbol.
 */
#ifndef L1SP_IOBUFFER
#define L1SP_IOBUFFER 256
#endif


/*
 * memory alignment of allocated objects.
 * this should work for most platforms.
 */
#ifndef L1SP_ALIGN
#define L1SP_ALIGN 16
#endif

/*
 * number of bytes in a string formated
 * hexadecimal number.
 */
#ifndef L1SP_HEX
#define L1SP_HEX 16
#endif

/*
 * number of bytes in a string formated
 * octal number.
 */
#ifndef L1SP_OCTAL
#define L1SP_OCTAL 3
#endif

/*
 * symbolic expression tags.  this value
 * is stored in |l1_tag| for each object.
 */
#define L1SP_NIL     0x00U
#define L1SP_FORM0   0x01U
#define L1SP_PROC0   0x02U
#define L1SP_FORMN   0x03U
#define L1SP_PROCN   0x04U
#define L1SP_MACRO   0x05U
#define L1SP_LAMBDA  0x06U
#define L1SP_PAIR    0x07U
#define L1SP_SYMBOL  0x08U
#define L1SP_FORWARD 0x09U

/*
 * every type in the interpreter
 */
typedef struct l1sp_pair    l1sp_nil;
typedef struct l1sp_func    l1sp_func;
typedef struct l1sp_pair    l1sp_closure;
typedef struct l1sp_pair    l1sp_pair;
typedef struct l1sp_symbol  l1sp_symbol;
typedef struct l1sp_forward l1sp_forward;
typedef struct l1sp_forward l1sp;

/*
 * forward type.  any type is converted to
 * a forward in |gc|, so it is also the
 * smallest type.
 */
struct l1sp_forward {
  unsigned l1_tag;
  l1sp *l1_forward;
};

struct l1sp_pair {
  unsigned l1_tag;
  l1sp *l1_car;
  l1sp *l1_cdr;
};

struct l1sp_symbol {
  unsigned l1_tag;
  char l1_data[1]; /*room for nil pointer*/
};

struct l1sp_func {
  unsigned l1_tag;
  l1sp *(*l1_func)();
};

typedef struct l1sp_ctx {
  char *l1_semispace[2],              /* copying garbage collector */
       *l1_iobuffer,                  /* i/o buffer */
       *l1_ioname;                    /* i/o filename */
  size_t l1_sscur,                    /* semispace access */
         l1_sssize,
         l1_ssalloc,
         l1_iosize,                   /* i/o buffer access */
         l1_ioalloc;
  int l1_ch,                          /* lookahead character */
      l1_iofd;                        /* i/o file descriptor */
  void (*l1_exit)(struct l1sp_ctx*),  /* non-error exit */
       (*l1_die)(struct l1sp_ctx*,
                 char*,
                 l1sp*),              /* fatal error */
       (*l1_nomem)(struct l1sp_ctx*,
                   size_t);           /* out of memory */
  ssize_t (*l1_read)(int,
                     void*,
                     size_t),         /* read data from fd */
          (*l1_write)(int,
                      const void*,
                      size_t);        /* write data to fd */
  l1sp *l1_nil,                       /* cache frequently accessed variables */
       *l1_t,
       *l1_quote,
       *l1_quasiquote,
       *l1_unquote,
       *l1_unquote_splicing,
       *l1_root,                      /* root pointers */
       *l1_oblist,                    /* object list */
       *l1_evlist;                    /* current environments */
  l1sp_nil _l1_nil;
} l1sp_ctx;

/*
 * static initialization.  only for use
 * in tracking root pointers. 
 */
#define L1SP_CONS(x, y) { L1SP_PAIR, x, y }

/*
 * protect stack variables during garbage collection
 */
#define L1SP_PROT1_DECL(g) \
  l1sp_pair _p0=L1SP_CONS((g)->l1_nil, (g)->l1_root); \
  l1sp *p0=(l1sp*)&_p0
  
#define L1SP_PROT2_DECL(g) \
  l1sp_pair _p0=L1SP_CONS((g)->l1_nil, (g)->l1_root), \
            _p1=L1SP_CONS((g)->l1_nil, (l1sp*)&_p0); \
  l1sp *p0=(l1sp*)&_p0, \
       *p1=(l1sp*)&_p1
   
#define L1SP_PROT3_DECL(g) \
  l1sp_pair _p0=L1SP_CONS((g)->l1_nil, (g)->l1_root), \
            _p1=L1SP_CONS((g)->l1_nil, (l1sp*)&_p0), \
            _p2=L1SP_CONS((g)->l1_nil, (l1sp*)&_p1); \
  l1sp *p0=(l1sp*)&_p0, \
       *p1=(l1sp*)&_p1, \
       *p2=(l1sp*)&_p2
  
#define L1SP_PROT4_DECL(g) \
  l1sp_pair _p0=L1SP_CONS((g)->l1_nil, (g)->l1_root), \
            _p1=L1SP_CONS((g)->l1_nil, (l1sp*)&_p0), \
            _p2=L1SP_CONS((g)->l1_nil, (l1sp*)&_p1), \
            _p3=L1SP_CONS((g)->l1_nil, (l1sp*)&_p2); \
  l1sp *p0=(l1sp*)&_p0, \
       *p1=(l1sp*)&_p1, \
       *p2=(l1sp*)&_p2, \
       *p3=(l1sp*)&_p3

#define l1sp_prot1(g, l0) \
  l1sp_rplaca(p0, (l1sp*)&l0); \
  (g)->l1_root=p0

#define l1sp_prot2(g, l0, l1) \
  l1sp_rplaca(p0, (l1sp*)&l0); \
  l1sp_rplaca(p1, (l1sp*)&l1); \
  (g)->l1_root=p1

#define l1sp_prot3(g, l0, l1, l2) \
  l1sp_rplaca(p0, (l1sp*)&l0); \
  l1sp_rplaca(p1, (l1sp*)&l1); \
  l1sp_rplaca(p2, (l1sp*)&l2); \
  (g)->l1_root=p2

#define l1sp_prot4(g, l0, l1, l2, l3) \
  l1sp_rplaca(p0, (l1sp*)&l0); \
  l1sp_rplaca(p1, (l1sp*)&l1); \
  l1sp_rplaca(p2, (l1sp*)&l2); \
  l1sp_rplaca(p3, (l1sp*)&l3); \
  (g)->l1_root=p3


#define l1sp_unprot1(g) (g)->l1_root=l1sp_cdr((g)->l1_root)
#define l1sp_unprot2(g) (g)->l1_root=l1sp_cddr((g)->l1_root)
#define l1sp_unprot3(g) (g)->l1_root=l1sp_cdddr((g)->l1_root)
#define l1sp_unprot4(g) (g)->l1_root=l1sp_cddddr((g)->l1_root)


/* return the tag of an object */
#define l1sp_tag(s) ((s)->l1_tag)

/* type predicates */
#define l1sp_nilp(s)    (L1SP_NIL==l1sp_tag(s))
#define l1sp_procp(s) \
  (L1SP_PROC0==l1sp_tag(s)||L1SP_PROCN==l1sp_tag(s)||L1SP_LAMBDA==l1sp_tag(s))
#define l1sp_formp(s) \
  (L1SP_FORM0==l1sp_tag(s)||L1SP_FORMN==l1sp_tag(s)||L1SP_MACRO==l1sp_tag(s))
#define l1sp_consp(s)   (L1SP_PAIR==l1sp_tag(s))
#define l1sp_symbolp(s) (L1SP_SYMBOL==l1sp_tag(s))

/* pair access */
#define l1sp_car(s) (((l1sp_pair*)(s))->l1_car)
#define l1sp_cdr(s) (((l1sp_pair*)(s))->l1_cdr)

#define l1sp_caar(s) (l1sp_car(l1sp_car(s)))
#define l1sp_cadr(s) (l1sp_car(l1sp_cdr(s)))
#define l1sp_cdar(s) (l1sp_cdr(l1sp_car(s)))
#define l1sp_cddr(s) (l1sp_cdr(l1sp_cdr(s)))

#define l1sp_caaar(s) (l1sp_car(l1sp_car(l1sp_car(s))))
#define l1sp_caadr(s) (l1sp_car(l1sp_car(l1sp_cdr(s))))
#define l1sp_cadar(s) (l1sp_car(l1sp_cdr(l1sp_car(s))))
#define l1sp_caddr(s) (l1sp_car(l1sp_cdr(l1sp_cdr(s))))
#define l1sp_cdaar(s) (l1sp_cdr(l1sp_car(l1sp_car(s))))
#define l1sp_cdadr(s) (l1sp_cdr(l1sp_car(l1sp_cdr(s))))
#define l1sp_cddar(s) (l1sp_cdr(l1sp_cdr(l1sp_car(s))))
#define l1sp_cdddr(s) (l1sp_cdr(l1sp_cdr(l1sp_cdr(s))))

#define l1sp_caaaar(s) (l1sp_car(l1sp_car(l1sp_car(l1sp_car(s)))))
#define l1sp_caaadr(s) (l1sp_car(l1sp_car(l1sp_car(l1sp_cdr(s)))))
#define l1sp_caadar(s) (l1sp_car(l1sp_car(l1sp_cdr(l1sp_car(s)))))
#define l1sp_caaddr(s) (l1sp_car(l1sp_car(l1sp_cdr(l1sp_cdr(s)))))
#define l1sp_cadaar(s) (l1sp_car(l1sp_cdr(l1sp_car(l1sp_car(s)))))
#define l1sp_cadadr(s) (l1sp_car(l1sp_cdr(l1sp_car(l1sp_cdr(s)))))
#define l1sp_caddar(s) (l1sp_car(l1sp_cdr(l1sp_cdr(l1sp_car(s)))))
#define l1sp_cadddr(s) (l1sp_car(l1sp_cdr(l1sp_cdr(l1sp_cdr(s)))))
#define l1sp_cdaaar(s) (l1sp_cdr(l1sp_car(l1sp_car(l1sp_car(s)))))
#define l1sp_cdaadr(s) (l1sp_cdr(l1sp_car(l1sp_car(l1sp_cdr(s)))))
#define l1sp_cdadar(s) (l1sp_cdr(l1sp_car(l1sp_cdr(l1sp_car(s)))))
#define l1sp_cdaddr(s) (l1sp_cdr(l1sp_car(l1sp_cdr(l1sp_cdr(s)))))
#define l1sp_cddaar(s) (l1sp_cdr(l1sp_cdr(l1sp_car(l1sp_car(s)))))
#define l1sp_cddadr(s) (l1sp_cdr(l1sp_cdr(l1sp_car(l1sp_cdr(s)))))
#define l1sp_cdddar(s) (l1sp_cdr(l1sp_cdr(l1sp_cdr(l1sp_car(s)))))
#define l1sp_cddddr(s) (l1sp_cdr(l1sp_cdr(l1sp_cdr(l1sp_cdr(s)))))

/* replace car, replace cdr */ 
#define l1sp_rplaca(s, x) (((l1sp_pair*)(s))->l1_car=(x))
#define l1sp_rplacd(s, x) (((l1sp_pair*)(s))->l1_cdr=(x))

/* misc */
#define l1sp_eqp(a, b) ((a)==(b))

/*
 * l1sp_def convenience macros
 */
#define l1sp_def(g, n, v) \
  l1sp_rplaca(g->l1_evlist, \
              l1sp_cons(g, l1sp_cons(g, n, v), \
              l1sp_car(g->l1_evlist)))

#define l1sp_def0(g, n, v) \
  l1sp_rplaca(g->l1_evlist, \
              l1sp_cons(g, l1sp_cons(g, l1sp_intern0(g, n), v), \
                        l1sp_car(g->l1_evlist)))

#define l1sp_defform0(g, n, v) \
  l1sp_rplaca(g->l1_evlist, \
              l1sp_cons(g, l1sp_cons(g, l1sp_intern0(g, n), \
                                        l1sp_form0(g, v)), \
                           l1sp_car(g->l1_evlist)))

#define l1sp_defproc0(g, n, v) \
  l1sp_rplaca(g->l1_evlist, \
              l1sp_cons(g, l1sp_cons(g, l1sp_intern0(g, n), \
                                        l1sp_proc0(g, v)), \
                           l1sp_car(g->l1_evlist)))

#define l1sp_defformn(g, n, v) \
  l1sp_rplaca(g->l1_evlist, \
              l1sp_cons(g, l1sp_cons(g, l1sp_intern0(g, n), \
                                        l1sp_formn(g, v)), \
                           l1sp_car(g->l1_evlist)))

#define l1sp_defprocn(g, n, v) \
  l1sp_rplaca(g->l1_evlist, \
              l1sp_cons(g, l1sp_cons(g, l1sp_intern0(g, n), \
                                        l1sp_procn(g, v)), \
                           l1sp_car(g->l1_evlist)))


/* buffer manipulation */
#define l1sp_setbuf_r(g, name, fd, buf, size) \
{ \
  g->l1_ioname=(name); \
  g->l1_iofd=(fd); \
  g->l1_iobuffer=(buf); \
  g->l1_iosize=(size_t)(size); \
  g->l1_ioalloc=(size_t)(size); \
}

#define l1sp_setbuf_w(g, name, fd, buf, size) \
{ \
  g->l1_ioname=(name); \
  g->l1_iofd=(fd); \
  g->l1_iobuffer=(buf); \
  g->l1_iosize=(size_t)0U; \
  g->l1_ioalloc=(size_t)(size); \
}

/* unboxing */
#define l1sp_symbits(s) (&((l1sp_symbol*)(s))->l1_data[(size_t)0U])

#define l1sp_funcalln(g,f,l) ((*((l1sp_func*)(f))->l1_func)(g,l))

#define l1sp_funcall0(g,f,a0,a1,a2,a3) \
  ((*((l1sp_func*)(f))->l1_func)(g,a0,a1,a2,a3))

/* initialize interpreter */
extern void l1sp_init(l1sp_ctx*,char*,char*,size_t),
            l1sp_fini(l1sp_ctx*);

/* error reporting */
extern void _l1sp_exit(l1sp_ctx*),            /* non-error exit callback */
            _l1sp_die(l1sp_ctx*,char*,l1sp*), /* die callback */
            _l1sp_dienomem(l1sp_ctx*,size_t), /* out of memory handler */
            l1sp_diebadarg(l1sp_ctx*,l1sp*),  /* invalid argument type */
            l1sp_diebaddot(l1sp_ctx*,l1sp*),  /* invalid list argument */
            l1sp_dienoread(l1sp_ctx*),        /* read error */
            l1sp_diebadtok(l1sp_ctx*,int),    /* invalid character in read */
            l1sp_dienomatch(l1sp_ctx*,int),   /* unmatched '(' or '"' */
            l1sp_dieunbound(l1sp_ctx*,l1sp*), /* unbound variable */
            l1sp_dienoproc(l1sp_ctx*,l1sp*),  /* unbound procedure */
            l1sp_dienowrite(l1sp_ctx*);       /* write error */

#define l1sp_exit(g) ((g)->l1_exit(g))           /* non-error exit */
#define l1sp_die(g,s,l) ((g)->l1_die(g,s,l))     /* low-level die routine */
#define l1sp_dienomem(g,n) ((g)->l1_nomem(g,n))  /* out of memory handler */

/* format information */
extern size_t l1sp_fmtbase(char*, size_t, unsigned, unsigned, int),
              l1sp_fmthex(char*, size_t),
              l1sp_fmtuint(char*, size_t);

/* miscellaneous routines */
extern l1sp *l1sp_cat(l1sp_ctx*,l1sp*),         /* concatenate strings */
            *l1sp_cat0(l1sp_ctx*,
                       char*,
                       char*,
                       char*,
                       char*),                  /* concatenate strings */
            *l1sp_cond(l1sp_ctx*,l1sp*),        /* conditional evaluation */
            *l1sp_nreverse(l1sp*,
                           l1sp*),              /* destructive list reverse */
            *l1sp_quasiquote(l1sp_ctx* g,
                             l1sp* l,
                             int *splice,
                             int *level),       /* quasiquote */
            *l1sp_set(l1sp_ctx*,l1sp*,l1sp*),   /* set variable */
            *l1sp_map(l1sp_ctx*,l1sp*,l1sp*),   /* map list over procedure */
            *l1sp_find(l1sp_ctx*,l1sp*,l1sp*);  /* find element in list */

/* object construction */
extern void l1sp_gc(l1sp_ctx*,l1sp**,l1sp**);
extern l1sp *l1sp_alloc(l1sp_ctx*,
                        unsigned,
                        size_t,
                        l1sp**,
                        l1sp**),
            *l1sp_form0(l1sp_ctx*,
                        l1sp *(*)()),           /* special form: fixed args */
            *l1sp_proc0(l1sp_ctx*,
                        l1sp *(*)()),           /* procedure: fixed args */
            *l1sp_formn(l1sp_ctx*,
                        l1sp *(*)()),           /* special form: list args */
            *l1sp_procn(l1sp_ctx*,
                        l1sp *(*)()),           /* procedure: list args */
            *l1sp_macro(l1sp_ctx*,
                        l1sp*,
                        l1sp*),                 /* macro expression */
            *l1sp_lambda(l1sp_ctx*,
                         l1sp*,
                         l1sp*),                /* lambda expression */
            *l1sp_cons(l1sp_ctx*,
                       l1sp*, l1sp*),           /* pair */
            *l1sp_intern(l1sp_ctx*,
                         char*,
                         size_t),               /* symbol */
            *l1sp_intern0(l1sp_ctx*,
                          char*);               /* symbol */

/* reader interface */
extern l1sp *l1sp_rdsexp(l1sp_ctx*);

/* eval interface */
extern l1sp *l1sp_eval(l1sp_ctx*,l1sp*),
            *l1sp_apply(l1sp_ctx*,l1sp*,l1sp*);

/* printer interface */
extern void l1sp_printc(l1sp_ctx*,int),     /* print character */
            l1sp_prints(l1sp_ctx*,char*),   /* print string */
            l1sp_printo(l1sp_ctx*,int),     /* print character in octal */
            l1sp_printn(l1sp_ctx*,size_t);  /* print number in hex */
extern l1sp *l1sp_print(l1sp_ctx*,l1sp*),   /* print atom */
            *l1sp_grindef(l1sp_ctx*,l1sp*), /* print symbolic expression */
            *l1sp_flush(l1sp_ctx*),         /* flush output stream */
            *l1sp_terpri(l1sp_ctx*);        /* terminate print line */

/* high level interface */
extern l1sp *l1sp_load(l1sp_ctx*,l1sp*),    /* load file (read/eval) */
            *l1sp_load0(l1sp_ctx*,char*);   /* load file (read/eval) */


void
_l1sp_exit(g)
  l1sp_ctx *g;
{
  exit(0);
}

void
_l1sp_die(g, syscall, resource)
  l1sp_ctx *g;
  char *syscall;
  l1sp *resource;
{
  extern char *__progname;
  char b[L1SP_IOBUFFER];
  l1sp_setbuf_w(g,
                "/dev/fd/2",
                2,
                &b[(size_t)0U],
                sizeof b/sizeof b[(size_t)0U]);

  l1sp_prints(g, __progname);
  l1sp_prints(g, ": error: ");
  l1sp_prints(g, syscall);
  if(g->l1_nil!=resource) {
    l1sp_prints(g, ": ");
    l1sp_grindef(g, resource);
  }
  if(errno) {
    l1sp_prints(g, ": ");
    l1sp_prints(g, strerror(errno));
  }
  l1sp_terpri(g);
  exit(0x80);
}

void
_l1sp_dienomem(g, n)
  l1sp_ctx *g;
  size_t n;
{
  extern char *__progname;
  char b[L1SP_IOBUFFER];
  l1sp_setbuf_w(g,
                "/dev/fd/2",
                2,
                &b[(size_t)0U],
                sizeof b/sizeof b[(size_t)0U]);

  l1sp_prints(g, __progname);
  l1sp_prints(g, ": error: brk: out of memory allocating ");
  l1sp_printn(g, n);
  l1sp_prints(g, " bytes");
  l1sp_terpri(g);
  exit(0x80);
}

void
l1sp_diebadarg(g, l)
  l1sp_ctx *g;
  l1sp *l;
{
  l1sp_die(g, "invalid argument", l);
}

void
l1sp_diebaddot(g, l)
  l1sp_ctx *g;
  l1sp *l;
{
  l1sp_die(g, "invalid dot expression", l);
}

void
l1sp_dienoread(g)
  l1sp_ctx *g;
{
  l1sp_die(g, "read", l1sp_intern0(g, g->l1_ioname));
}

void
l1sp_diebadtok(g, ch)
  l1sp_ctx *g;
{
  char p[2];
  p[0]=(char)ch;
  p[1]='\0';
  l1sp_die(g, "unrecognized character", l1sp_intern0(g, &p[(size_t)0U]));
}

void
l1sp_dienomatch(g, ch)
  l1sp_ctx *g;
{
  char p[2];
  p[0]=(char)ch;
  p[1]='\0';
  l1sp_die(g, "unmatched character", l1sp_intern0(g, &p[(size_t)0U]));
}

void
l1sp_dieunbound(g, s)
  l1sp_ctx *g;
  l1sp *s;
{
  l1sp_die(g, "unbound variable", s);
}

void
l1sp_dienoproc(g, s)
  l1sp_ctx *g;
  l1sp *s;
{
  l1sp_die(g, "no procedure", s);
}

void
l1sp_dienowrite(g)
  l1sp_ctx *g;
{
  l1sp_die(g, "write", l1sp_intern0(g, g->l1_ioname));
}


#define align(a) \
  (((a)+((size_t)L1SP_ALIGN)-(size_t)1U)&~(((size_t)L1SP_ALIGN)-(size_t)1U))

#define ssallocp(g, n) \
  ((g->l1_sssize+(n))<=g->l1_ssalloc)

#define forward(s, x) \
{ \
  (s)->l1_tag=L1SP_FORWARD; \
  (s)->l1_forward=(x); \
}

l1sp*
l1sp_alloc(g, t, n, a, b)
  l1sp_ctx *g;
  unsigned t;
  size_t n;
  l1sp **a, **b;
{
  l1sp *s;
  size_t alloc=0U;

  switch(t) {
  case L1SP_FORM0:
  case L1SP_PROC0:
  case L1SP_FORMN:
  case L1SP_PROCN:
    alloc=sizeof(l1sp_func);
    break;

  case L1SP_MACRO:
  case L1SP_LAMBDA:
  case L1SP_PAIR:
    alloc=sizeof(l1sp_pair);
    break;

  case L1SP_SYMBOL:
    alloc=sizeof(l1sp_symbol)+n;
    break;
  }

  alloc=align(alloc);

  if(!ssallocp(g, alloc)) {
    l1sp_gc(g, a, b);
    if(!ssallocp(g, alloc)) l1sp_dienomem(g, alloc);
  }

  s=(l1sp*)(g->l1_semispace[g->l1_sscur]+g->l1_sssize);
  s->l1_tag=t;
  g->l1_sssize+=alloc;

  return s;
}

static l1sp*
move(g, s)
  l1sp_ctx *g;
  l1sp *s;
{
  l1sp *x=g->l1_nil;
  size_t n;

  switch(l1sp_tag(s)) {
  case L1SP_NIL:
    x=s;
    break;

  case L1SP_FORM0:
  case L1SP_PROC0:
  case L1SP_FORMN:
  case L1SP_PROCN:
    x=l1sp_alloc(g, l1sp_tag(s), (size_t)0U, &g->l1_nil, &g->l1_nil);
    ((l1sp_func*)x)->l1_func=((l1sp_func*)s)->l1_func;
    forward(s, x);
    break;

  case L1SP_MACRO:
  case L1SP_LAMBDA:
  case L1SP_PAIR:
    x=l1sp_alloc(g, l1sp_tag(s), (size_t)0U, &g->l1_nil, &g->l1_nil);
    l1sp_rplaca(x, l1sp_car(s));
    l1sp_rplacd(x, l1sp_cdr(s));
    forward(s, x);
    break;

  case L1SP_SYMBOL:
    x=l1sp_alloc(g,
                 l1sp_tag(s),
                 n=strlen(l1sp_symbits(s)),
                 &g->l1_nil,
                 &g->l1_nil);
    memcpy(l1sp_symbits(x), l1sp_symbits(s), n+(size_t)1U);
    g->l1_oblist=l1sp_cons(g, x, g->l1_oblist);
    forward(s, x);
    break;

  case L1SP_FORWARD:
    x=s->l1_forward;
    break;
  }

  return x;
}

void
l1sp_gc(g, a, b)
  l1sp_ctx *g;
  l1sp **a, **b;
{
  l1sp *x, **c;
  char *space;
  size_t alloc, scan;

  g->l1_sscur=!g->l1_sscur;
  g->l1_sssize=(size_t)0U;

  space=g->l1_semispace[g->l1_sscur];
  scan=(size_t)0U;

  /* clear the oblist, we will recreate as we gc */
  g->l1_oblist=g->l1_nil;

  g->l1_nil=move(g->l1_nil);

  for(x=g->l1_root; l1sp_consp(x); x=l1sp_cdr(x)) {
    c=(l1sp**)l1sp_car(x); *c=move(*c);
  }

  g->l1_t=move(g, g->l1_t);
  g->l1_quote=move(g, g->l1_quote);
  g->l1_quasiquote=move(g, g->l1_quasiquote);
  g->l1_unquote=move(g, g->l1_unquote);
  g->l1_unquote_splicing=move(g, g->l1_unquote_splicing);

  g->l1_evlist=move(g, g->l1_evlist);

  *a=move(g, *a);
  *b=move(g, *b);

  while(scan<g->l1_sssize) {
    x=(l1sp*)(space+scan);
    switch(l1sp_tag(x)) {
    case L1SP_FORM0:
    case L1SP_PROC0:
    case L1SP_FORMN:
    case L1SP_PROCN:
      scan+=align(sizeof(l1sp_func));
      break;

    case L1SP_MACRO:
    case L1SP_LAMBDA:
    case L1SP_PAIR:
      scan+=align(sizeof(l1sp_pair));
      l1sp_rplaca(x, move(l1sp_car(x)));
      l1sp_rplacd(x, move(l1sp_cdr(x)));
      break;

    case L1SP_SYMBOL:
      alloc=sizeof(l1sp_symbol)+strlen(l1sp_symbits(x));
      scan+=align(alloc);
      scan+=align(sizeof(l1sp_pair));
      break;
    }
  }
}

#define FUNC(name, type) \
l1sp* \
name(g, f) \
  l1sp_ctx *g; \
  l1sp *(*f)(l1sp*); \
{ \
  l1sp_func *x; \
  x=(l1sp_func*)l1sp_alloc(g, type, (size_t)0U, &g->l1_nil, &g->l1_nil); \
  x->l1_func=f; \
  return (l1sp*)x; \
}

FUNC(l1sp_form0, L1SP_FORM0)
FUNC(l1sp_proc0, L1SP_PROC0)
FUNC(l1sp_formn, L1SP_FORMN)
FUNC(l1sp_procn, L1SP_PROCN)

#define PAIR(name, type) \
l1sp* \
name(g, x, y) \
  l1sp_ctx *g; \
  l1sp *x, *y; \
{ \
  l1sp *s; \
  s=l1sp_alloc(g, type, (size_t)0U, &x, &y); \
  l1sp_rplaca(s, x); \
  l1sp_rplacd(s, y); \
  return s; \
}

PAIR(l1sp_macro,  L1SP_MACRO)
PAIR(l1sp_lambda, L1SP_LAMBDA)
PAIR(l1sp_cons,   L1SP_PAIR)

l1sp*
l1sp_intern(g, p, n)
  l1sp_ctx *g;
  char *p;
  size_t n;
{
  l1sp *x;
  char *s;
  size_t m;

  for(x=g->l1_oblist; l1sp_consp(x); x=l1sp_cdr(x)) {
    s=l1sp_symbits(l1sp_car(x)); m=strlen(s);
    if(n==m && 0==memcmp(p, s, n)) return l1sp_car(x);
  }

  x=l1sp_alloc(g, L1SP_SYMBOL, n, &g->l1_nil, &g->l1_nil);
  memcpy(l1sp_symbits(x), p, n); l1sp_symbits(x)[n]='\0';
  g->l1_oblist=l1sp_cons(g, x, g->l1_oblist);
  return x;
}

l1sp*
l1sp_intern0(g, p)
  l1sp_ctx *g;
  char *p;
{
  l1sp *x;
  size_t n;

  for(x=g->l1_oblist; l1sp_consp(x); x=l1sp_cdr(x)) {
    if(0==strcmp(l1sp_symbits(l1sp_car(x)), p)) return l1sp_car(x);
  }

  x=l1sp_alloc(g, L1SP_SYMBOL, n=strlen(p), &g->l1_nil, &g->l1_nil);
  memcpy(l1sp_symbits(x), p, n+(size_t)1U);
  g->l1_oblist=l1sp_cons(g, x, g->l1_oblist);
  return x;
}


/*
 * l1sp_cat - concatenate symbols
 *
 * this routine is like |l1sp_intern0| except
 * the memory for the symbol bits is not
 * contiguous.
 */
l1sp*
l1sp_cat(g, l)
  l1sp_ctx *g;
  l1sp *l;
{
  L1SP_PROT1_DECL(g);
  l1sp *x, *y, *r;
  char *d, *s;
  size_t n;

  /* determine the length of the new symbol */
  n=(size_t)0U;
  for(x=l; l1sp_consp(x); x=l1sp_cdr(x)) {
    y=l1sp_car(x);
    if(!l1sp_symbolp(y)) l1sp_diebadarg(g, y);
    n+=strlen(l1sp_symbits(y));
  }

  /* allocate the object */
  l1sp_prot1(g, l);
  r=l1sp_alloc(g, L1SP_SYMBOL, n, &g->l1_nil, &g->l1_nil);
  l1sp_unprot1(g);

  /* copy bits for each input */
  d=l1sp_symbits(r);
  for(x=l; l1sp_consp(x); x=l1sp_cdr(x)) {
    s=l1sp_symbits(l1sp_car(x));
    memcpy(d, s, n=strlen(s));
    d+=n;
  }
  *d='\0';

  /* search for an existing symbol */
  for(x=g->l1_oblist; l1sp_consp(x); x=l1sp_cdr(x)) {
    if(0==strcmp(l1sp_symbits(l1sp_car(x)), l1sp_symbits(r)))
      return l1sp_car(x);
  }

  return r;
}

l1sp*
l1sp_cat0(g, a0, a1, a2, a3)
  l1sp_ctx *g;
  char *a0, *a1, *a2, *a3;
{
  l1sp *x, *r;
  char *b;
  size_t n;

  /* determine the length of the new symbol */
  n=(size_t)0U;
  if(a0) n+=strlen(a0);
  if(a1) n+=strlen(a1);
  if(a2) n+=strlen(a2);
  if(a3) n+=strlen(a3);

  /* allocate the object */
  r=l1sp_alloc(g, L1SP_SYMBOL, n, &g->l1_nil, &g->l1_nil);

  /* copy bits for each input */
  b=l1sp_symbits(r);
  if(a0) { memcpy(b, a0, n=strlen(a0)); b+=n; }
  if(a1) { memcpy(b, a1, n=strlen(a1)); b+=n; }
  if(a2) { memcpy(b, a2, n=strlen(a2)); b+=n; }
  if(a3) { memcpy(b, a3, n=strlen(a3)); b+=n; }
  *b='\0';

  /* search for an existing symbol */
  for(x=g->l1_oblist; l1sp_consp(x); x=l1sp_cdr(x)) {
    if(0==strcmp(l1sp_symbits(l1sp_car(x)), l1sp_symbits(r)))
      return l1sp_car(x);
  }

  return r;
}

l1sp*
l1sp_cond(g, l)
  l1sp_ctx *g;
  l1sp *l;
{
  L1SP_PROT3_DECL(g);
  l1sp *r, *x, *y;

  r=x=y=g->l1_nil;
  l1sp_prot3(g, r, x, y);
  
  /* the argument list is a list of lists */
  for(x=l; l1sp_consp(x); x=l1sp_cdr(x)) {
    y=l1sp_car(x);

    /*
     * if the element is not a list,
     * we are not a proper cond
     */
    if(!l1sp_consp(y)) l1sp_diebadarg(g, y);

    /*
     * if the first element evaluates to
     * |nil|, try the next list
     */
    r=l1sp_eval(g, l1sp_car(y));
    if(l1sp_nilp(r)) continue;

    /*
     * otherwise, evaluate every other
     * element in the list, returning
     * the result of the last
     */
    for(y=l1sp_cdr(y); l1sp_consp(y); y=l1sp_cdr(y)) {
      r=l1sp_eval(g, l1sp_car(y));
    }
    if(!l1sp_nilp(y)) l1sp_diebaddot(g, y);
    break;
  }

  l1sp_unprot3(g);
  return r;
}

l1sp*
l1sp_nreverse(t, l)
  l1sp *t, *l;
{
  l1sp *p;
  while(l1sp_consp(l)) {
    p=l1sp_cdr(l);
    l1sp_rplacd(l, t);
    t=l;
    l=p;
  }
  return t;
}

l1sp*
l1sp_quasiquote(g, l, set_splice, set_level)
  l1sp_ctx *g;
  l1sp *l;
  int *set_splice, *set_level;
{
  L1SP_PROT2_DECL(g);
  l1sp *r, *x, *y;
  int do_splice=0,
      do_level=0;

  switch(l1sp_tag(l)) {
  case L1SP_PAIR:
    r=x=g->l1_nil;
    l1sp_prot2(g, l, x);

    for(x=l; l1sp_consp(x); x=l1sp_cdr(x)) {
      y=l1sp_quasiquote(g, l1sp_car(x), &do_splice, &do_level);
      if(do_splice) { /* unquote-splicing */
        do_splice=0;
        r=l1sp_nreverse(r, y);
      } else {
        r=l1sp_cons(g, y, r);
      }
    }

    r=l1sp_nreverse(g->l1_nil, r);

    if(!do_level) {
      if(l1sp_eqp(g->l1_unquote, l1sp_car(r))) {
        if(!l1sp_nilp(x)) l1sp_diebaddot(g, l);
        l1sp_unprot2(g);
        return l1sp_eval(g, l1sp_cadr(r));
      } else if(l1sp_eqp(g->l1_unquote_splicing, l1sp_car(r))) {
        if(!l1sp_nilp(x)) l1sp_diebaddot(g, l);
        l1sp_unprot2(g);
        *set_splice=1;
        return l1sp_eval(g, l1sp_cadr(r));
      }
    }
    l1sp_unprot2(g);
    return r;

  default:
    return l;
  }
}

l1sp*
l1sp_set(g, n, v)
  l1sp_ctx *g;
  l1sp *n, *v;
{
  l1sp_rplaca(g->l1_evlist,
              l1sp_cons(g, l1sp_cons(g, n, v),
              l1sp_car(g->l1_evlist)));
  return l1sp_caar(g->l1_evlist);
}

l1sp*
l1sp_map(g, p, l)
  l1sp_ctx *g;
  l1sp *p, *l;
{
  L1SP_PROT3_DECL(g);
  l1sp *a, *r, *x;

  a=r=g->l1_nil;
  l1sp_prot3(g, a, p, r);

  /* preallocate the list for apply */
  a=l1sp_cons(g, g->l1_nil, g->l1_nil);

  for(x=l; l1sp_consp(x); x=l1sp_cdr(x)) {
    l1sp_rplaca(a, l1sp_car(x));
    l1sp_rplacd(a, g->l1_nil);
    r=l1sp_cons(g, l1sp_apply(g, p, a), r);
  }
  if(!l1sp_nilp(x)) l1sp_diebaddot(g, l);

  l1sp_unprot3(g);
  return l1sp_nreverse(g->l1_nil, r);
}

l1sp*
l1sp_find(g, p, l)
  l1sp_ctx *g;
  l1sp *p, *l;
{
  L1SP_PROT2_DECL(g);
  l1sp *a, *r, *x;

  /* mark |p| and |r| as root pointers */
  a=r=g->l1_nil;
  l1sp_prot2(g, a, p);

  /* preallocate the list for apply */
  a=l1sp_cons(g, g->l1_nil, g->l1_nil);

  for(x=l; l1sp_consp(x); x=l1sp_cdr(x)) {
    l1sp_rplaca(a, l1sp_car(x));
    l1sp_rplacd(a, g->l1_nil);
    r=l1sp_apply(g, p, a);
    if(!l1sp_nilp(r)) break;
  }

  l1sp_unprot2(g);
  return l1sp_car(x);
}


/*
 * builtin procedures
 */
static l1sp*
p_atomp(g, l)
  l1sp_ctx *g;
  l1sp *l;
{
  if(l1sp_consp(l)) return g->l1_nil;
  return g->l1_t;
}

static l1sp*
p_car(g, l)
  l1sp_ctx *g;
  l1sp *l;
{
  switch(l1sp_tag(l)) {
  case L1SP_NIL:
  case L1SP_PAIR: return l1sp_car(l);
  default:        l1sp_diebadarg(g, l);
  }
}

static l1sp*
p_cdr(g, l)
  l1sp_ctx *g;
  l1sp *l;
{
  switch(l1sp_tag(l)) {
  case L1SP_NIL:
  case L1SP_PAIR: return l1sp_cdr(l);
  default:        l1sp_diebadarg(g, l);
  }
}

static l1sp*
p_consp(g, l)
  l1sp_ctx *g;
  l1sp *l;
{
  if(l1sp_consp(l)) return g->l1_t;
  return g->l1_nil;
}

static l1sp*
p_def(g, n, v)
  l1sp_ctx *g;
  l1sp *n, *v;
{
  l1sp_def(g, n, v);
  return l1sp_caar(g->l1_evlist);
}

static l1sp*
p_exit(g)
  l1sp_ctx *g;
{
  l1sp_exit(g);
}

static l1sp*
p_die(g, l)
  l1sp_ctx *g;
  l1sp *l;
{
  l1sp_die(g, "runtime error", l);
}

static l1sp*
p_evlist(g)
  l1sp_ctx *g;
{
  return g->l1_evlist;
}

/*
 * the eqp available from the interpreter also does
 * structure equivalence.
 *
 * note we waste a lot of time here if the structures
 * are pointer-eq cons cells.  i'm not that worried
 * about it.
 */
static l1sp*
p_eqp(g, a, b)
  l1sp_ctx *g;
  l1sp *a, *b;
{
  for(; l1sp_consp(a) && l1sp_consp(b); a=l1sp_cdr(a), b=l1sp_cdr(b)) {
    if(g->l1_nil==p_eqp(g, l1sp_car(a), l1sp_car(b))) return g->l1_nil;
  }
  if(l1sp_eqp(a, b)) return g->l1_t;
  return g->l1_nil;
}

static l1sp*
p_gc(g)
  l1sp_ctx *g;
{
  l1sp_gc(g, &g->l1_nil, &g->l1_nil);
  return g->l1_nil;
}

static l1sp*
p_lambda(g, l)
  l1sp_ctx *g;
  l1sp *l;
{
  return l1sp_lambda(g, l, g->l1_evlist);
}

static l1sp*
p_list(g, l)
  l1sp_ctx *g;
  l1sp *l;
{
  return l;
}

static l1sp*
p_macro(g, l)
  l1sp_ctx *g;
  l1sp *l;
{
  return l1sp_macro(g, l, g->l1_evlist);
}

static l1sp*
p_nilp(g, l)
  l1sp_ctx *g;
  l1sp *l;
{
  if(l1sp_nilp(l)) return g->l1_t;
  return g->l1_nil;
}

static l1sp*
p_oblist(g)
  l1sp_ctx *g;
{
  return g->l1_oblist;
}

static l1sp*
p_quasiquote(g, l)
  l1sp_ctx *g;
  l1sp *l;
{
  int splice=0, level=0;
  return l1sp_quasiquote(g, l, &splice, &level);
}

static l1sp*
p_rplaca(g, l, a)
  l1sp_ctx *g;
  l1sp *l, *a;
{
  switch(l1sp_tag(l)) {
  case L1SP_PAIR: l1sp_rplaca(l, a);
  case L1SP_NIL:  return l;
  default:        l1sp_diebadarg(g, l);
  }
}

static l1sp*
p_rplacd(g, l, d)
  l1sp_ctx *g;
  l1sp *l, *d;
{
  switch(l1sp_tag(l)) {
  case L1SP_PAIR: l1sp_rplacd(l, d);
  case L1SP_NIL:  return l;
  default:        l1sp_diebadarg(g, l);
  }
}

void
l1sp_init(g, a, b, n)
  l1sp_ctx *g;
  char *a, *b;
  size_t n;
{
  extern char *__progname;
  g->l1_semispace[(size_t)0U]=a;
  g->l1_semispace[(size_t)1U]=b;

  g->l1_iobuffer=(char*)0;

  g->l1_sscur=(size_t)0U;
  g->l1_sssize=(size_t)0U;
  g->l1_ssalloc=n;

  g->l1_iosize=(size_t)0U;
  g->l1_ioalloc=(size_t)0U;

  g->l1_iofd=-1;

  g->l1_exit=&_l1sp_exit;
  g->l1_die=&_l1sp_die;
  g->l1_nomem=&_l1sp_dienomem;
  g->l1_read=&read;
  g->l1_write=&write;

  g->_l1_nil.l1_tag=L1SP_NIL;
  g->_l1_nil.l1_car=(l1sp*)&g->_l1_nil;
  g->_l1_nil.l1_cdr=(l1sp*)&g->_l1_nil;
  g->l1_nil=(l1sp*)&g->_l1_nil;

  g->l1_root=g->l1_nil;

  g->l1_oblist=g->l1_nil;
  g->l1_evlist=l1sp_cons(g, g->l1_nil, g->l1_nil);

  g->l1_t=l1sp_intern0(g, "t");
  g->l1_quote=l1sp_intern0(g, "quote");
  g->l1_quasiquote=l1sp_intern0(g, "quasiquote");
  g->l1_unquote=l1sp_intern0(g, "unquote");
  g->l1_unquote_splicing=l1sp_intern0(g, "unquote-splicing");

  l1sp_defproc0(g, "terpri",          l1sp_terpri);
  l1sp_def(g, g->l1_t,                g->l1_t);
  l1sp_defproc0(g, "set",             l1sp_set);
  l1sp_defproc0(g, "rplacd",          p_rplacd);
  l1sp_defproc0(g, "rplaca",          p_rplaca);
  l1sp_defproc0(g, "rdsexp",          l1sp_rdsexp);
  l1sp_def0(g, "prognam",             l1sp_intern0(g, __progname));
  l1sp_defproc0(g, "print",           l1sp_print);
  l1sp_def(g, g->l1_quote,            l1sp_form0(g, p_list));
  l1sp_def(g, g->l1_quasiquote,       l1sp_form0(g, p_quasiquote));
  l1sp_defprocn(g, "oblist",          p_oblist);
  l1sp_defproc0(g, "nilp",            p_nilp);
  l1sp_def0(g, "nil",                 g->l1_nil);
  l1sp_defproc0(g, "map",             l1sp_map);
  l1sp_defformn(g, "macro",           p_macro);
  l1sp_defprocn(g, "list",            p_list);
  l1sp_defformn(g, "lambda",          p_lambda);
  l1sp_defproc0(g, "grindef",         l1sp_grindef);
  l1sp_defprocn(g, "gc",              p_gc);
  l1sp_defproc0(g, "find",            l1sp_find);
  l1sp_defproc0(g, "flush",           l1sp_flush);
  l1sp_defprocn(g, "exit",            p_exit);
  l1sp_defproc0(g, "eval",            l1sp_eval);
  l1sp_defproc0(g, "eqp",             p_eqp);
  l1sp_defprocn(g, "evlist",          p_evlist);
  l1sp_defproc0(g, "die",             p_die);
  l1sp_defproc0(g, "def",             p_def);
  l1sp_defproc0(g, "consp",           p_consp);
  l1sp_defproc0(g, "cons",            l1sp_cons);
  l1sp_defformn(g, "cond",            l1sp_cond);
  l1sp_defproc0(g, "cdr",             p_cdr);
  l1sp_defprocn(g, "cat",             l1sp_cat);
  l1sp_defproc0(g, "car",             p_car);
  l1sp_defproc0(g, "atomp",           p_atomp);
  l1sp_defproc0(g, "apply",           l1sp_apply);
}

void
l1sp_fini(g)
  l1sp_ctx *g;
{
  ;
}


static int
readc(g)
  l1sp_ctx *g;
{
  ssize_t r;
  size_t i, n;

  if(g->l1_iosize<g->l1_ioalloc) return g->l1_iobuffer[g->l1_iosize++];
  if(-1==g->l1_iofd) return -1;

  g->l1_iosize=(size_t)0U;

  i=(size_t)0U;
  n=g->l1_ioalloc;
  do {
retry:
    r=(*g->l1_read)(g->l1_iofd, g->l1_iobuffer+i, n);
    switch(r) {
    case -1:
      if(EINTR==errno) goto retry;
      l1sp_dienoread(g);

    case 0:
      g->l1_iofd=-1;
      g->l1_iosize=g->l1_ioalloc-i;
      memmove(g->l1_iobuffer+g->l1_iosize, g->l1_iobuffer, i);
      if(g->l1_iosize<g->l1_ioalloc) return g->l1_iobuffer[g->l1_iosize++];
      return -1;

    default:
      i+=(size_t)r;
      n-=(size_t)r;
      break;
    }
  } while(n);

  return g->l1_iobuffer[g->l1_iosize++];
}

static
next(g, l)
  l1sp_ctx *g;
  l1sp **l;
{
  char b[L1SP_IOBUFFER], *p, *e;
  int ch;

  ch=g->l1_ch;
  *l=g->l1_nil;

restart:
  switch(ch) {
  case '(':
  case ')':
  case '\'':
  case '`':
  case '.':
    g->l1_ch=readc(g);

    /* down seems more likely */

  case -1:
    return ch;

  case ',':
    ch=readc(g);
    if('@'==ch) { g->l1_ch=readc(g); return '@'; }
    else        { g->l1_ch=ch;       return ','; }

  case ';':
    do {
      ch=readc(g);
      if(-1==ch) goto restart;
    } while('\n'!=ch);
    ch=readc(g);
    goto restart;

  case '"':
    p=&b[(size_t)0U];
    e=p+sizeof b/sizeof b[(size_t)0U];

    for(;;) {
      ch=readc(g);

      switch(ch) {
      case -1:
        l1sp_dienomatch(g, '"');

      case '"':
        *p='\0';
        g->l1_ch=readc(g);
        *l=l1sp_intern0(g, &b[(size_t)0U]);
        return '"';

      case '\\':
        ch=readc(g);

        switch(ch) {
        case -1:
          l1sp_dienomatch(g, '"');

        case 'n':
          ch='\n';
          break;

        case 't':
          ch='\t';
          break;
        }

        /*down seems more likely*/

      default:
        *p++=ch; if(p>=e) l1sp_dienomem(g, (size_t)1U);
        break;
      }
    }
    break;

  default:
    if(isspace(ch)) {
      do {
        ch=readc(g);
        if(-1==ch) goto restart;
      } while(isspace(ch));
      goto restart;
    }

    if(isalnum(ch) || ispunct(ch)) {
      p=&b[(size_t)0U];
      e=p+sizeof b/sizeof b[(size_t)0U];

      do {
        switch(ch) {
        case '"':
        case '\'':
        case '(':
        case ')':
        case ',':
        case ';':
        case '`':
          goto endsym; /*will never trigger on first pass*/

        default:
          *p++=ch; if(p>=e) l1sp_dienomem(g, (size_t)1U);
          ch=readc(g);
        }
      } while(isalnum(ch) || ispunct(ch));

endsym:
      *p='\0';
      *l=l1sp_intern0(g, &b[(size_t)0U]);
      g->l1_ch=ch;
      return 's';
    }

    l1sp_diebadtok(g, ch);
  }
}

static l1sp *form(),
            *quote(),
            *quasiquote(),
            *unquote(),
            *unquote_splicing();

static l1sp*
object(g, la, ch)
  l1sp_ctx *g;
  l1sp **la;
{
  switch(ch) {
  case -1:   l1sp_dienomatch(g, ')');
  case '(':  return form(g, la);
  case '\'': return quote(g, la);
  case '`':  return quasiquote(g, la);
  case ',':  return unquote(g, la);
  case '@':  return unquote_splicing(g, la);
  case '"':  return l1sp_cons(g, g->l1_quote, l1sp_cons(g, *la, g->l1_nil));
  case 's':  return *la;
  default:   l1sp_dienomatch(g, ch);
  }
}

static l1sp*
form(g, la)
  l1sp_ctx *g;
  l1sp **la;
{
  L1SP_PROT1_DECL(g);
  l1sp *r;
  int ch;

  r=g->l1_nil;
  l1sp_prot1(g, r);

  for(;;) {
    switch(ch=next(g, la)) {
    case -1:
      l1sp_dienomatch(g, '(');

    case ')':
      l1sp_unprot1(g);
      return l1sp_nreverse(g->l1_nil, r);

    case '.':
      if(l1sp_nilp(r)) l1sp_diebaddot(g, r);
      r=l1sp_nreverse(object(g, la, next(g, la)), r);
      if(')'!=next(g, la)) l1sp_diebaddot(g, r);
      l1sp_unprot1(g);
      return r;

    default:
      r=l1sp_cons(g, object(g, la, ch), r);
      break;
    }
  }
}

#define _quote(type) \
static l1sp* \
type(g, la) \
  l1sp_ctx *g; \
  l1sp **la; \
{ \
  int ch; \
  switch(ch=next(g, la)) { \
  case -1: l1sp_dienomatch(g, '('); \
  default: return l1sp_cons(g, \
                            g->l1_##type, \
                            l1sp_cons(g, object(g, la, ch), g->l1_nil)); \
  } \
}

_quote(quote)
_quote(quasiquote)
_quote(unquote)
_quote(unquote_splicing)

l1sp*
l1sp_rdsexp(g)
  l1sp_ctx *g;
{
  L1SP_PROT2_DECL(g);
  l1sp *la, *r;
  int ch;

  g->l1_ch=readc(g);

  la=r=g->l1_nil;
  l1sp_prot2(g, la, r);

  for(;;) {
    switch(ch=next(g, &la)) {
    case -1:
      l1sp_unprot2(g);
      return l1sp_nreverse(g->l1_nil, r);

    case '(':
      r=l1sp_cons(g, form(g, &la), r);
      break;

    case '\'':
      r=l1sp_cons(g, quote(g, &la), r);
      break;

    case '`':
      r=l1sp_cons(g, quasiquote(g, &la), r);
      break;

    case ',':
      r=l1sp_cons(g, unquote(g, &la), r);
      break;

    case '@':
      r=l1sp_cons(g, unquote_splicing(g, &la), r);
      break;

    case '"':
      r=l1sp_cons(g,
                  l1sp_cons(g, g->l1_quote, l1sp_cons(g, la, g->l1_nil)),
                  r);
      break;

    case 's':
      r=l1sp_cons(g, la, r);
      break;

    default:
      l1sp_dienomatch(g, ch);
    }
  }
}


/*
 * execute user-defined function
 */
static l1sp*
funcall(g, p, l)
  l1sp_ctx *g;
  l1sp *p, *l;
{
  L1SP_PROT4_DECL(g);
  l1sp *e, *r, *x, *y;

  e=g->l1_evlist;
  r=x=g->l1_nil;
  l1sp_prot4(g, e, p, l, x);

  /* set up a new environment */
  g->l1_evlist=l1sp_cons(g, g->l1_nil, l1sp_cdr(p));

  x=l1sp_caar(p);
  switch(l1sp_tag(x)) {
  /*
   * the first element is a symbol that all
   * of the arguments are bound to.
   */
  case L1SP_SYMBOL:
    y=l1sp_cons(g, l1sp_cons(g, x, l), l1sp_car(g->l1_evlist));
    l1sp_rplaca(g->l1_evlist, y);
    break;

  /*
   * the first element is a list of parameters
   * bind them to the argument list in the new
   * environment.
   */
  case L1SP_PAIR:
    for(; l1sp_consp(x); x=l1sp_cdr(x), l=l1sp_cdr(l)) {
      y=l1sp_cons(g, l1sp_cons(g, l1sp_car(x), l1sp_car(l)),
                     l1sp_car(g->l1_evlist));
      l1sp_rplaca(g->l1_evlist, y);
    }
    if(!l1sp_nilp(x)) {
      y=l1sp_cons(g, l1sp_cons(g, x, l), l1sp_car(g->l1_evlist));
      l1sp_rplaca(g->l1_evlist, y);
    }
    break;

  default:
    l1sp_diebadarg(g, x);
  }

  /*
   * the remaining elements are the body of the lambda.
   * execute them and return the result of the last form.
   */
  for(x=l1sp_cdar(p); l1sp_consp(x); x=l1sp_cdr(x)) {
    r=l1sp_eval(g, l1sp_car(x));
  }

  /* tear down the new environment */
  g->l1_evlist=e;

  /* return the last form */
  l1sp_unprot4(g);
  return r;
}

l1sp*
l1sp_apply(g, p, l)
  l1sp_ctx *g;
  l1sp *p, *l;
{
  l1sp *a0, *a1, *a2, *a3;

  switch(l1sp_tag(p)) {
  case L1SP_PROC0:
  case L1SP_FORM0:
    a0=l1sp_car(l); l=l1sp_cdr(l);
    a1=l1sp_car(l); l=l1sp_cdr(l);
    a2=l1sp_car(l); l=l1sp_cdr(l);
    a3=l1sp_car(l);
    return l1sp_funcall0(g, p, a0, a1, a2, a3);

  case L1SP_PROCN:
  case L1SP_FORMN:
    return l1sp_funcalln(g, p, l);

  case L1SP_MACRO:
    return l1sp_eval(g, funcall(g, p, l));

  case L1SP_LAMBDA:
    return funcall(g, p, l);

  default:
    l1sp_dienoproc(g, p);
  }
}

l1sp*
l1sp_eval(g, s)
  l1sp_ctx *g;
  l1sp *s;
{
  L1SP_PROT3_DECL(g);
  l1sp *l, *x, *y;

  switch(l1sp_tag(s)) {
  /*
   * evaluate the list only if the first argument is not
   * a special form.
   */
  case L1SP_PAIR:
    /*
     * make |s| and |x| visible to the garbage collector
     */
    l=x=g->l1_nil;
    l1sp_prot3(g, l, s, x);

    /* create a new list and evaluate the first element */
    l=l1sp_cons(g, l1sp_eval(g, l1sp_car(s)), g->l1_nil);

    /*
     * if we have a special form, we don't evaluate the
     * argument list.  otherwise evaluate each element in
     * the list.  in both cases we copy the input, in case
     * the procedure modifies it's arguments.  pass the
     * procedure and the argument list to apply.
     */
    if(l1sp_formp(l1sp_car(l))) {
      for(x=l1sp_cdr(s); l1sp_consp(x); x=l1sp_cdr(x)) {
        l=l1sp_cons(g, l1sp_car(x), l);
      }
      if(!l1sp_nilp(x)) l1sp_diebaddot(g, s);
    } else {
      for(x=l1sp_cdr(s); l1sp_consp(x); x=l1sp_cdr(x)) {
        l=l1sp_cons(g, l1sp_eval(g, l1sp_car(x)), l);
      }
      if(!l1sp_nilp(x)) l1sp_diebaddot(g, s);
    }

    l1sp_unprot3(g);

    l=l1sp_nreverse(g->l1_nil, l);
    return l1sp_apply(g, l1sp_car(l), l1sp_cdr(l));

  case L1SP_SYMBOL:
    for(x=g->l1_evlist; l1sp_consp(x); x=l1sp_cdr(x)) {
      for(y=l1sp_car(x); l1sp_consp(y); y=l1sp_cdr(y)) {
        if(l1sp_eqp(l1sp_caar(y), s)) return l1sp_cdar(y);
      }
    }
    l1sp_dieunbound(g, s);

  default:
    return s;
  }
}


static
l1sp_openread0(f)
  char *f;
{
  int fd;

reopen:
  fd=open(f, O_RDONLY|O_NONBLOCK|O_LARGEFILE);
  if(-1==fd && EINTR==errno) goto reopen;
  return fd;
}

static
l1sp_close(fd)
{
  int r;
  if(-1==fd) return 0;
restart:
  if(-1==(r=close(fd)) && EINTR==errno) goto restart;
  return r;
}

l1sp*
l1sp_load(g, l)
  l1sp_ctx *g;
  l1sp *l;
{
  if(!l1sp_symbolp(l)) l1sp_diebadarg(g, l);
  return l1sp_load0(g, l1sp_symbits(l));
}

l1sp*
l1sp_load0(g, f)
  l1sp_ctx *g;
  char *f;
{
  L1SP_PROT1_DECL(g);
  int saved_errno=errno;
  char b[L1SP_IOBUFFER];
  l1sp *l;
  int fd;

  fd=l1sp_openread0(f);
  if(-1==fd) return g->l1_nil;

  l=g->l1_nil;
  l1sp_prot1(g, l);

  l1sp_setbuf_r(g,
                f,
                fd,
                &b[(size_t)0U],
                sizeof b/sizeof b[(size_t)0U]);
  l=l1sp_rdsexp(g);

  l1sp_setbuf_w(g,
                "/dev/fd/1",
                1,
                &b[(size_t)0U],
                sizeof b/sizeof b[(size_t)0U]);
  for(; l1sp_consp(l); l=l1sp_cdr(l)) l1sp_eval(g, l1sp_car(l));

  l1sp_unprot1(g);
  l1sp_close(fd);
  errno=saved_errno;
  return g->l1_t;
}


/*
 * fmt |n| into |p| in radix |r|.
 * if |w| is greater than the number
 * of characters written to |s|,
 * fill the remaining space in |s|
 * with |ch|.
 *
 * return the number of characters
 * written to |s|.
 *
 * if |s| is null, return the number
 * of characters required to fmt |n|.
 */
size_t
l1sp_fmtbase(p, n, r, w, ch)
    char *p;
    size_t n;
    unsigned r, w;
{
  size_t l, d, n0=n, r0=r, w0=w;
  int i;

  l=(size_t)1U;
  while(n0>=r0) {
    ++l;
    n0/=r0;
  }

  d=(size_t)0U;
  if(l<w0) { d=w0-l; l=w0; }

  if(p) {
    p+=l;

    do {
      i=(n%r0);
      if(i<10) *--p='0'+i; else *--p='a'+i-10;
      n/=r0;
    } while(n);

    while(d) {
      --d;
      *--p=ch;
    }
  }

  return l;
}

size_t
l1sp_fmthex(p, n)
  char *p;
  size_t n;
{
  return l1sp_fmtbase(p, n, 16U, (unsigned)L1SP_HEX, '0');
}

size_t
l1sp_fmtoctal(p, n)
  char *p;
  size_t n;
{
  return l1sp_fmtbase(p, n, 8U, (unsigned)L1SP_OCTAL, '0');
}


#define ioallocp(g, n) \
  ((g->l1_iosize+(n))<=g->l1_ioalloc)

void
l1sp_printc(g, ch)
  l1sp_ctx *g;
{
  if(!ioallocp(g, (size_t)1U)) l1sp_flush(g);
  g->l1_iobuffer[g->l1_iosize++]=ch;
}

void
l1sp_prints(g, p)
  l1sp_ctx *g;
  char *p;
{
  size_t n;
  n=strlen(p);
  if(!ioallocp(g, n)) l1sp_flush(g);
  if(!ioallocp(g, n)) l1sp_dienomem(g, n);
  memcpy(g->l1_iobuffer+g->l1_iosize, p, n);
  g->l1_iosize+=n;
}

void
l1sp_printo(g, ch)
  l1sp_ctx *g;
{
  if(!ioallocp(g, (size_t)L1SP_OCTAL)) l1sp_flush(g);
  g->l1_iosize+=l1sp_fmtoctal(g->l1_iobuffer+g->l1_iosize,
                               (size_t)(ssize_t)ch);
}

void
l1sp_printn(g, n)
  l1sp_ctx *g;
  size_t n;
{
  if(!ioallocp(g, (size_t)L1SP_HEX)) l1sp_flush(g);
  g->l1_iosize+=l1sp_fmthex(g->l1_iobuffer+g->l1_iosize, n);
}

l1sp*
l1sp_flush(g)
  l1sp_ctx *g;
{
  ssize_t r;
  size_t i=(size_t)0U;
  while(g->l1_iosize) {
    switch(r=(*g->l1_write)(g->l1_iofd, g->l1_iobuffer+i, g->l1_iosize)) {
    case -1:
    case 0:
      l1sp_dienowrite(g);

    default:
      i+=(size_t)r;
      g->l1_iosize-=(size_t)r;
      break;
    }
  }
  return g->l1_nil;
}

l1sp*
l1sp_terpri(g)
  l1sp_ctx *g;
{
  l1sp_printc(g, '\n');
  l1sp_flush(g);
  return g->l1_nil;
}

l1sp*
l1sp_print(g, s)
  l1sp_ctx *g;
  l1sp *s;
{
  if(!l1sp_symbolp(s)) l1sp_diebadarg(g, s);
  l1sp_prints(g, l1sp_symbits(s));
  return s;
}

l1sp*
l1sp_grindef(g, s)
  l1sp_ctx *g;
  l1sp *s;
{
  char *p, ch;

  switch(l1sp_tag(s)) {
  case L1SP_NIL:
    l1sp_prints(g, "()");
    break;

  case L1SP_FORM0:
  case L1SP_FORMN:
  case L1SP_MACRO:
    l1sp_prints(g, "#form[");
    l1sp_printn(g, (size_t)s);
    l1sp_prints(g, "]");
    break;

  case L1SP_PROC0:
  case L1SP_PROCN:
  case L1SP_LAMBDA:
    l1sp_prints(g, "#proc[");
    l1sp_printn(g, (size_t)s);
    l1sp_prints(g, "]");
    break;

  case L1SP_PAIR:
    if(g->l1_quote==l1sp_car(s) &&
       l1sp_consp(l1sp_cdr(s)) &&
       l1sp_nilp(l1sp_cddr(s))) {
      l1sp_printc(g, '\'');
      l1sp_grindef(g, l1sp_cadr(s));
    } else if(g->l1_quasiquote==l1sp_car(s) &&
              l1sp_consp(l1sp_cdr(s)) &&
              l1sp_nilp(l1sp_cddr(s))) {
      l1sp_printc(g, '`');
      l1sp_grindef(g, l1sp_cadr(s));
    } else if(g->l1_unquote==l1sp_car(s) &&
              l1sp_consp(l1sp_cdr(s)) &&
              l1sp_nilp(l1sp_cddr(s))) {
      l1sp_printc(g, ',');
      l1sp_grindef(g, l1sp_cadr(s));
    } else if(g->l1_unquote_splicing==l1sp_car(s) &&
              l1sp_consp(l1sp_cdr(s)) &&
              l1sp_nilp(l1sp_cddr(s))) {
      l1sp_prints(g, ",@");
      l1sp_grindef(g, l1sp_cadr(s));
    } else {
      l1sp_printc(g, '(');
      goto first;
      while(l1sp_consp(s)) {
        l1sp_printc(g, ' ');
first:
        l1sp_grindef(g, l1sp_car(s));
        s=l1sp_cdr(s);
      }
      if(!l1sp_nilp(s)) {
        l1sp_prints(g, " . ");
        l1sp_grindef(g, s);
      }
      l1sp_printc(g, ')');
    }
    break;

  case L1SP_SYMBOL:
    p=l1sp_symbits(s);
    /*
     * to avoid quoting, the first character must be
     * alpha numeric.
     */
    if(!isalnum(ch=*p++)) goto escape;

    /*
     * the rest must be printable and not contain
     * syntax.
     */
    while(ch=*p++) {
      if(isalnum(ch)) break;
      switch(ch) {
      case '"':
      case '\'':
      case '(':
      case ')':
      case ',':
      case '.':
      case ';':
      case '`':
        goto escape;
      }
      if(ispunct(ch)) break;
      goto escape;
    }

    p=l1sp_symbits(s);
    l1sp_prints(g, p);
    break;

escape:
    p=l1sp_symbits(s);
    l1sp_printc(g, '"');
    while(ch=*p++) {
      switch(ch) {
      case '\n':
        l1sp_prints(g, "\\n");
        break;

      case '\t':
        l1sp_prints(g, "\\t");
        break;

      case '\\':
      case '"':
        l1sp_printc(g, '\\');

        /* down seems more likely */

      default:
        if(isprint(ch)) {
          l1sp_printc(g, ch);
        } else {
          l1sp_printc(g, '\\');
          l1sp_printo(g, ch);
        }
        break;
      }
    }
    l1sp_printc(g, '"');
    break;

  case L1SP_FORWARD:
    l1sp_prints(g, "#forward[");
    l1sp_printn(g, (size_t)s);
    l1sp_prints(g, "]");
    break;
  }

  return s;
}


/*
 * all of the code below this point is for b00t.c only.
 */

#include <stdio.h>

#ifndef B00T
#define B00T "."
#endif

static l1sp*
b00t_dirname(g, l)
  l1sp_ctx *g;
  l1sp *l;
{
  char *p, *s;
  size_t n;

  if(!l1sp_symbolp(l)) l1sp_diebadarg(g, l);

  p=s=l1sp_symbits(l);
  n=strlen(s);
  s+=n-(!!n); /* go to end of (possibly empty) string */

  while(n--) {
    if('/'==*s) {
      if(p==s) {
        return l1sp_intern0(g, "/");
      } else {
        return l1sp_intern(g, p, n);
      }
    }
    --s;
  }

  return l1sp_intern0(g, ".");
}

static l1sp*
b00t_exec(g, l)
  l1sp_ctx *g;
  l1sp *l;
{
  char *_argv[(size_t)128U], **argv=&_argv[(size_t)0U];
  l1sp *x, *y;

  for(;l1sp_consp(l); l=l1sp_cdr(l)) {
    x=l1sp_car(l);
    switch(l1sp_tag(x)) {
    case L1SP_SYMBOL:
      *argv++=l1sp_symbits(x);
      break;

    case L1SP_PAIR:
      for(;l1sp_consp(x); x=l1sp_cdr(x)) {
        y=l1sp_car(x);
        if(!l1sp_symbolp(y)) l1sp_diebadarg(g, x);
        *argv++=l1sp_symbits(y);
      }
      if(!l1sp_nilp(x)) l1sp_diebadarg(g, l);
      break;

    default:
      l1sp_diebadarg(g, x);
    }
  }
  if(!l1sp_nilp(l)) l1sp_diebadarg(g, l);

  *argv='\0';
  execvp(_argv[0], _argv);
}

static l1sp*
b00t_objname(g, l)
  l1sp_ctx *g;
  l1sp *l;
{
  char *p, *s;
  size_t n;
  l1sp *x;

  if(!l1sp_symbolp(l)) l1sp_diebadarg(g, l);

  p=s=l1sp_symbits(l);
  n=strlen(s);
  s+=n-(!!n); /* to to end of (possibly empty) string */

  if(!n || 'c'!=*s) l1sp_diebadarg(g, l);
  --n; --s;
  if(!n || '.'!=*s) l1sp_diebadarg(g, l);
  --n; --s;

  x=l1sp_intern(g, p, n);
  return l1sp_cat0(g, l1sp_symbits(x), ".o", (char*)0, (char*)0);
}

static l1sp*
b00t_exist(g, l)
  l1sp_ctx *g;
  l1sp *l;
{
  FILE *f;
  if(!l1sp_symbolp(l)) l1sp_diebadarg(g, l);
  if(!(f=fopen(l1sp_symbits(l), "r"))) return g->l1_nil;
  fclose(f);
  return g->l1_t;
}

static l1sp*
b00t_read1(g, l)
  l1sp_ctx *g;
  l1sp *l;
{
  char _b[L1SP_IOBUFFER], *b=&_b[(size_t)0U];
  size_t i;
  FILE *f;
  if(!l1sp_symbolp(l)) l1sp_diebadarg(g, l);

  if(!(f=fopen(l1sp_symbits(l), "r")))
    return g->l1_nil;
  if(!fgets(b, sizeof _b/sizeof _b[(size_t)0U], f))
    l1sp_die(g, "read", l);
  fclose(f);

  for(i=(size_t)0U; i<sizeof _b/sizeof _b[(size_t)0U]; ++i) {
    if('\n'==*b) {
      *b='\0';
      return l1sp_intern0(g, &_b[(size_t)0U]);
    }
    ++b;
  }

  return g->l1_nil;
}

static l1sp*
b00t_split(g, l, c)
  l1sp_ctx *g;
  l1sp *l, *c;
{
  L1SP_PROT1_DECL(g);
  char *p, *s, ch;
  l1sp *r;

  if(l1sp_nilp(l)) return g->l1_nil;

  if(!l1sp_symbolp(l)) l1sp_diebadarg(g, l);
  if(!l1sp_symbolp(c)) l1sp_diebadarg(g, c);

  r=g->l1_nil;
  l1sp_prot1(g, r);

  p=s=l1sp_symbits(l);
  ch=l1sp_symbits(c)[(size_t)0U];

  if(*s) {
    do {
      if(ch==*s) {
        r=l1sp_cons(g, l1sp_intern(g, p, s-p), r);
        p=++s;
      } else {
        ++s;
      }
    } while(*s);
    r=l1sp_cons(g, l1sp_intern0(g, p), r);
  }

  l1sp_unprot1(g);
  return l1sp_nreverse(g->l1_nil, r);
}

static l1sp*
b00t_repl(g)
  l1sp_ctx *g;
{
  L1SP_PROT1_DECL(g);
  char b[L1SP_IOBUFFER];
  l1sp *l, *r;

  l=g->l1_nil;
  l1sp_prot1(g, l);

  l1sp_setbuf_r(g, "/dev/fd/0", 0, &b[0U], sizeof b/sizeof b[0U]);
  l=l1sp_rdsexp(g);
  l1sp_setbuf_w(g, "/dev/fd/1", 1, &b[0U], sizeof b/sizeof b[0U]);

  for(; l1sp_consp(l); l=l1sp_cdr(l)) {
    r=l1sp_eval(g, l1sp_car(l));
    l1sp_grindef(g, r);
    l1sp_terpri(g);
  }

  l1sp_unprot1(g);
  return g->l1_nil;
}

static void
b00t_init(g, cmd, argc, argv)
  l1sp_ctx *g;
  char *cmd, *argv[];
{
  L1SP_PROT1_DECL(g);
  l1sp *r;
  char *a;

  r=g->l1_nil;
  l1sp_prot1(g, r);
  while(a=*argv++) {
    r=l1sp_cons(g, l1sp_intern0(g, a), r);
  }
  l1sp_unprot1(g);

  l1sp_def0(g, "argv", l1sp_nreverse(g->l1_nil, r));

  l1sp_defproc0(g, "split",   b00t_split);
  l1sp_defproc0(g, "repl",    b00t_repl);
  l1sp_defproc0(g, "read1",   b00t_read1);
  l1sp_defproc0(g, "objname", b00t_objname);
  l1sp_defproc0(g, "exist",   b00t_exist);
  l1sp_defprocn(g, "exec",    b00t_exec);
  l1sp_defproc0(g, "dirname", b00t_dirname);

  l1sp_load(g, l1sp_cat0(g, B00T, "/init.l1", (char*)0, (char*)0));
  l1sp_load(g, l1sp_cat0(g, B00T, "/", cmd, ".l1"));
}

main(argc, argv)
  char *argv[];
{
  l1sp_ctx _g, *g=&_g;
  char semispace[2][L1SP_SEMISPACE],
       *cmd;

  --argc;
  ++argv;

  if(argc) {
    --argc;
    cmd=*argv++;
  } else {
    cmd="repl";
  }

  l1sp_init(g, &semispace[0][0], &semispace[1][0], (size_t)L1SP_SEMISPACE);
  b00t_init(g, cmd, argc, argv);

  l1sp_fini(g);
  l1sp_exit(g);
}
