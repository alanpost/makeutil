/*
 * textpand - uncompress input using builtin frequency table
 *
 * this file is part of the makeutil package:
 *   https://github.com/alanpost/makeutil
 *
 * this file is hereby placed in the public domain.
 * alyn.post@lodockikumazvati.org
 */

#include "tree.h"
#include <stdio.h>
#include <stdlib.h>

extern char freq[];
extern size_t _freq_size;

extern int forarg();

static
textpand(filename, file)
  char *filename;
  FILE *file;
{
  struct forest forest;
  struct tree *head, *tree;
  int ch;

  tree=head=huffman_tree(&forest, &freq[0], _freq_size);

  while(EOF!=(ch=fgetc(file))) {
    unsigned i=0U;
    while(i<8U) {
      if(NULL==(tree=tree->node[(ch>>i++)&0x1U])) {
        fputs("error: fell off tree\n", stderr);
        exit(EXIT_FAILURE);
      }

      if(leaf(tree)) {
        if(EOF==tree->ch) break;

        if(EOF==fputc(tree->ch, stdout)) {
          perror("error: write");
          exit(EXIT_FAILURE);
        }
        tree=head;
      }
    }
  }
  if(ferror(file)) {
    perror("error: read");
    exit(EXIT_FAILURE);
  }
  if(EOF==fflush(stdout)) {
    perror("error: write");
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}

main(argc, argv)
  char *argv[];
{
  exit(forarg(argc, argv, textpand, "rb"));
}

/*                                                              ..__
 *                                                              `' "
 */
