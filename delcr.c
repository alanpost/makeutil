/*
 * delcr - delete a carriage return before each line feed in input file.
 *
 * this file is part of the makeutil package:
 *   https://github.com/alanpost/makeutil
 *
 * this file is hereby placed in the public domain.
 * alyn.post@lodockikumazvati.org
 */

#include <stdio.h>
#include <stdlib.h>

extern int forarg();

static
delcr(filename, file)
  char *filename;
  FILE *file;
{
  char _p[BUFSIZ], *p;
  int flag=0;
  size_t n;
  char ch;

  while(n=fread(&_p[0], sizeof(char), sizeof _p/sizeof _p[0], file)) {
    p=&_p[0];
    while(n--) {
      ch=*p++;
      if('\r'==ch) {
        if(flag && (EOF==fputc('\r', stdout))) {
          perror("error: write");
          exit(EXIT_FAILURE);
        }
        flag=1;
        continue;
      }

      if(flag && ('\n'!=ch) && (EOF==fputc('\r', stdout))) {
        perror("error: write");
        exit(EXIT_FAILURE);
      }
      flag=0;

      if(EOF==fputc(ch, stdout)) {
        perror("error: write");
        exit(EXIT_FAILURE);
      }
    }
  }
  if(flag && (EOF==fputc('\r', stdout))) {
    perror("error: write");
    exit(EXIT_FAILURE);
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
  exit(forarg(argc, argv, delcr, "rb"));
}

/*                                                              ..__
 *                                                              `' "
 */
