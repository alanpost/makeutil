/*
 * textpack - compress input using builtin frequency table
 *
 * this file is part of the makeutil package:
 *   http://sourceforge.net/projects/makeutil/
 *   http://www.cybermesa.com/~aisa/makeutil/
 *
 * this file is hereby placed in the public domain.
 * aisa0@users.sourceforge.net, aisa@cybermesa.com
 */

static char rcsid[]="$Id: textpack.c,v 1.1 2004/11/28 22:57:53 aisa0 Exp $";

#include "tree.h"
#include <stdio.h>
#include <stdlib.h>

struct table
{
  unsigned size;
  int bit[0xff+2];
};

extern char freq[];
extern size_t _freq_size;

extern int forarg();

static void
traverse(tree, table, bit, count, direction)
  struct tree *tree;
  struct table *table;
  int *bit;
  unsigned count;
  int direction;
{
  if(tree) {
    bit[count++]=direction;

    if(leaf(tree)) {
      unsigned i;

      for(i=0U; i<count; ++i) (table+1)[tree->ch].bit[i]=bit[i];
      (table+1)[tree->ch].size=count;
    } else {
      traverse(tree->node[0], table, bit, count, 0);
      traverse(tree->node[1], table, bit, count, 1);
    }
  }
}

static void
huffman_table(tree, table)
  struct tree *tree;
  struct table *table;
{
  int bit[0xff+2];

  traverse(tree->node[0], table, bit, 0U, 0);
  traverse(tree->node[1], table, bit, 0U, 1);
}

static
textpack(filename, file)
  char *filename;
  FILE *file;
{
  struct table table[0xff+2];
  int ch, wbit=0x0;
  unsigned i=0U;
  struct forest forest;
  struct tree *tree;

  tree=huffman_tree(&forest, &freq[0], _freq_size);
  huffman_table(tree, table);

  do {
    struct table *t;
    int rbit=0;

    ch=fgetc(file);
    t=&(table+1)[ch];

    while(rbit<t->size) {
      wbit|=(t->bit[rbit++]<<i++);
      if(8U==i) {
        if(EOF==fputc(wbit, stdout)) {
          perror("error: write");
          exit(EXIT_FAILURE);
        }
        wbit=0x0;
        i=0U;
      }
    }
  } while(EOF!=ch);
  if(i) {
    if(EOF==fputc(wbit, stdout)) {
      perror("error: write");
      exit(EXIT_FAILURE);
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
  exit(forarg(argc, argv, textpack, "rb"));
}

/*                                                              ..__
 *                                                              `' "
 */
