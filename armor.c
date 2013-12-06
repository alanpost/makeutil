/*
 * armor - convert a binary file to ascii
 *
 * this file is part of the makeutil package:
 *   http://sourceforge.net/projects/makeutil/
 *   http://www.cybermesa.com/~aisa/makeutil/
 *
 * this file is hereby placed in the public domain.
 * aisa0@users.sourceforge.net, aisa@cybermesa.com
 */

static char rcsid[]="$Id: armor.c,v 1.1 2004/11/28 22:57:53 aisa0 Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int forarg();

/*
 * read up to the size of our block or to end of file.
 */
static size_t
block_read(block, size, file)
  char *block;
  size_t size;
  FILE *file;
{
  char *p;
  size_t n, r;

  p=block;
  n=size;
  while(n && (r=fread(p, sizeof(char), n, file))) {
    p+=r;
    n-=r;
  }

  /*set any trailing bits to zero.*/
  memset(p, '\0', n);

  return p-block;
}

static void
fputa(ch, file)
  FILE *file;
{
  static int t[64]={
    '`', '!', '"', '#', '$', '%', '&', '\'',
    '(', ')', '*', '+', ',', '-', '.', '/',
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', ':', ';', '<', '=', '>', '?',
    '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
    'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
    'X', 'Y', 'Z', '[', '\\', ']', '^', '_'
  };

  if(EOF==fputc(t[ch&0x3f], file)) {
    perror("error: write");
    exit(EXIT_FAILURE);
  }
}

static
armor(filename, file)
  char *filename;
  FILE *file;
{
  char p[54];
  size_t i, n;

  while(n=block_read(p, sizeof p/sizeof p[0], file)) {
    fputa(n, stdout);

    i=0;
    while(i<n) {
      int b0, b1, b2;

      b0=p[i++];
      b1=p[i++];
      b2=p[i++];

      fputa(b0>>2, stdout);
      fputa(((b0<<4)&0x30)|(b1>>4), stdout);
      fputa(((b1<<2)&0x3c)|(b2>>6), stdout);
      fputa(b2&0x3f, stdout);
    }

    if(EOF==fputc('\n', stdout)) {
      perror("error: write");
      exit(EXIT_FAILURE);
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
  exit(forarg(argc, argv, armor, "rb"));
}

/*                                                              ..__
 *                                                              `' "
 */
