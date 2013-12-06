/*
 * ckey - generate frequency table (compression key) from input
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

static unsigned long freq[0xff+2]={0};

static size_t
ber(s, n0)
  char *s;
  unsigned long n0;
{
  size_t l;
  unsigned long n;

  l=0U;
  n=n0;
  do {
    n/=128UL;
    ++l;
  } while(n);

  if(s) {
    s+=l;
    n=n0;
    do {
      *--s=(char)(unsigned char)(n%128|128);
       n/=128UL;
    } while(n);

    s[l-1U]&=(char)(unsigned char)127;
  }

  return l;
}

static
ckey(filename, file)
  char *filename;
  FILE *file;
{
  int ch;

  do {
    ch=fgetc(file);
    ++(freq+1)[ch];
  } while(EOF!=ch);

  if(ferror(file)) {
    perror("error: read");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

main(argc, argv)
  char *argv[];
{
  char fmt[sizeof(unsigned long)+1];
  int i, status=0;
  size_t r;

  status|=forarg(argc, argv, ckey, "rb");

  for(i=0 /*skip eof*/; i<=0xff; ++i) {
    if((freq+1)[i]) {
      r=fwrite(fmt, ber(fmt, (unsigned long)i), sizeof(char), stdout);
      if(sizeof(char)!=r) {
		perror("error: write");
		exit(EXIT_FAILURE|status);
	  }
      r=fwrite(fmt, ber(fmt, (freq+1)[i]), sizeof(char), stdout);
      if(sizeof(char)!=r) {
		perror("error: write");
		exit(EXIT_FAILURE|status);
	  }
    }
  }
  if(EOF==fflush(stdout)) {
    perror("error: write");
    exit(EXIT_FAILURE|status);
  }

  exit(status);
}

/*                                                              ..__
 *                                                              `' "
 */
