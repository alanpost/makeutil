/*
 * dearmor - convert an ascii file to binary
 *
 * this file is part of the makeutil package:
 *   https://github.com/alanpost/makeutil
 *
 * this file is hereby placed in the public domain.
 * alyn.post@lodockikumazvati.org
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int forarg();

#define d(byte) (((byte)-' ')&0x3f)

static
dearmor(filename, file)
  char *filename;
  FILE *file;
{
  char line[256];

  while(NULL!=fgets(line, sizeof line/sizeof line[0], file)) {
    size_t i, n, size;

    /* remove newline */
    n=strlen(line);
    if(n && '\n'==line[n-1]) line[--n]='\0';

    /* skip line size */
    if(n) --n;

    /*
     * this check is important so we don't overflow in the while loop
     * below.
     */
    if(n%4) {
      perror("error: line length not divisible by 4\n");
      return EXIT_FAILURE;
    }

    i=0;
    size=d(line[i++]);
    while(i<n) {
      int b0, b1, b2, b3;

      b0=d(line[i++]);
      b1=d(line[i++]);
      b2=d(line[i++]);
      b3=d(line[i++]);

      if((size>=1) && (EOF==fputc((b0<<2)|(b1>>4), stdout))) {
        perror("error: write");
        exit(EXIT_FAILURE);
      }
      if((size>=2) && (EOF==fputc((b1<<4)|(b2>>2), stdout))) {
        perror("error: write");
        exit(EXIT_FAILURE);
      }
      if((size>=3) && (EOF==fputc((b2<<6)|b3, stdout))) {
        perror("error: write");
        exit(EXIT_FAILURE);
      }
      size-=3;
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
  exit(forarg(argc, argv, dearmor, "r"));
}

/*                                                              ..__
 *                                                              `' "
 */
