/*
 * mkdep - return all files that input file #includes
 *
 * this file is part of the makeutil package:
 *   http://sourceforge.net/projects/makeutil/
 *   http://www.cybermesa.com/~aisa/makeutil/
 *
 * this file is hereby placed in the public domain.
 * aisa0@users.sourceforge.net, aisa@cybermesa.com
 */

static char rcsid[]="$Id: mkdep.c,v 1.2 2005/03/25 19:42:30 aisa0 Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/*
 * very quick, very dirty, very ugly parser
 */
static size_t
match(p, n, m)
  char *p;
  size_t n;
  int *m;
{
  char *b;

  switch(*m) {
  /* default mode, check for mode changes */
  case '\0':
    switch(*p) {
    case '"':
    case '/':
      *m=*p;
      break;

    case '#':
      *m='^';
      break;
    }
    return 1;

  case '^':
    b=p;
    while(n && (' '==*p || '\t'==*p)) {
      --n; ++p;
    }
    if(n) *m='i';
    return p-b;

  case 'i':
    if('i'==*p) {
      *m='n';
      return 1;
    } else {
      *m='\0';
      return 0;
    }

  case 'n':
    if('n'==*p) {
      *m='c';
      return 1;
    } else {
      *m='\0';
      return 0;
    }

  case 'c':
    if('c'==*p) {
      *m='l';
      return 1;
    } else {
      *m='\0';
      return 0;
    }

  case 'l':
    if('l'==*p) {
      *m='u';
      return 1;
    } else {
      *m='\0';
      return 0;
    }

  case 'u':
    if('u'==*p) {
      *m='d';
      return 1;
    } else {
      *m='\0';
      return 0;
    }

  case 'd':
    if('d'==*p) {
      *m='e';
      return 1;
    } else {
      *m='\0';
      return 0;
    }

  case 'e':
    if('e'==*p) {
      *m=' ';
      return 1;
    } else {
      *m='\0';
      return 0;
    }

  case ' ':
    b=p;
    while(n && (' '==*p || '\t'==*p)) {
      --n; ++p;
    }
    if(n) *m='<';
    return p-b;

  case '<':
    if('"'==*p || '<'==*p) {
      *m='F';
      return 1;
    } else {
      *m='\0';
      return 0;
    }

  case 'F':
    b=p;
    while(n && (isalnum(*p) || '/'==*p || '_'==*p || '.'==*p)) {
      --n; ++p;
    }
    fwrite(b, p-b, (size_t)1U, stdout);
    if(n) *m='>';
    return p-b;

  case '>':
    /*
     * too late to check for bad syntax.
     */
    /*if('"'==*p || '>'==*p) ...*/
    fwrite("\n", (size_t)1U, (size_t)1U, stdout);
    *m='\0';
    return 1;

  /* process quoted string */
  case '"':
    b=p;
    while(n && ('\\'!=*p && '"'!=*p)) {
      --n; ++p;
    }

    if(n) {
      switch(*p) {
      case '"':
        *m='\0';
        break;

      case '\\':
        *m='\\';
        break;
      }
      --n; ++p;
    }
    return p-b;

  /* continue at a string escape */
  case '\\':
    return 1; /* skip the escaped char */

  /* process comments */
  case '/':
    /*
     * if we are soc, set in-comment
     * mode, otherwise reset mode.
     */
    if('*'==*p) {
      *m='C';
      return 1;
    } else {
      *m='\0';
      return 0;
    }

  case 'C': /* in-comment continuation */
    b=p;
    while(n && '*'!=*p) {
      --n; ++p;
    }
    if(n) {
      --n; ++p; /* advance past '*' */
      *m='*';
    }
    return p-b;

  /* continue at a comment ending */
  case '*':
    /*
     * if we are eoc, reset the mode
     * otherwise continue reading a
     * comment
     */
    if('/'==*p) {
      *m='\0';
      return 1;
    } else {
      *m='C';
      return 0;
    }
  }

  fprintf(stderr, "internal error: invalid mode: '%c'\n", *m);
  exit(1);
}

static
mkdep(filename, file)
  char *filename;
  FILE *file;
{
  char _b[4096], *p=&_b[0];
  int m='\0', status=0;
  size_t n, i;
  while(n=fread(p=&_b[0], (size_t)1U, sizeof _b/sizeof _b[0], file)) {
    do {
      i=match(p, n, &m);
      p+=i; n-=i;
    } while(n);
  }
  if(ferror(file)) {
    fprintf(stderr, "error: read: %s: ", filename);
    perror("");
    status|=EXIT_FAILURE;
  }
  return status;
}

main(argc, argv)
  char *argv[];
{
  int status=EXIT_SUCCESS;
  char *filename;
  FILE *file;

  --argc;
  ++argv;

  if(*argv) {
    while(filename=*argv++) {
      file=fopen(filename, "r");
      if(!file) {
        fprintf(stderr, "error: open: %s: ", filename);
        perror("");
        status|=EXIT_FAILURE;
        continue;
      }
      status|=mkdep(filename, file);
      fclose(file);
    }
  } else {
    status|=mkdep("-", stdin);
  }

  fflush(stdout);
  exit(status);
}
