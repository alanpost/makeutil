/*
 * addcr - add a carriage return before each line feed in input file.
 *
 * this file is part of the makeutil package:
 *   http://sourceforge.net/projects/makeutil/
 *   http://www.cybermesa.com/~aisa/makeutil/
 *
 * this file is hereby placed in the public domain.
 * aisa0@users.sourceforge.net, aisa@cybermesa.com
 */

static char rcsid[]="$Id: addcr.c,v 1.1 2004/11/28 22:57:53 aisa0 Exp $";

#include <stdio.h>
#include <stdlib.h>

extern int forarg();

static
addcr(filename, file)
  char *filename;
  FILE *file;
{
  char _p[BUFSIZ], *p;
  size_t n;
  char ch;

  while(n=fread(&_p[0], sizeof(char), sizeof _p/sizeof _p[0], file)) {
    p=&_p[0];
    while(n--) {
      ch=*p++;

      if(('\n'==ch) && (EOF==fputc('\r', stdout))) {
        perror("error: write");
        exit(EXIT_FAILURE);
      }

      if(EOF==fputc(ch, stdout)) {
        perror("error: write");
        exit(EXIT_FAILURE);
      }
    }
  }
  if(ferror(file)) {
    perror("error: read");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

main(argc, argv)
  char *argv[];
{
  exit(forarg(argc, argv, addcr, "rb"));
}

/*                                                              ..__
 *                                                              `' "
 */
