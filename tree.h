/*
 * tree.h - huffman compression data structures
 *
 * this file is part of the makeutil package:
 *   https://github.com/alanpost/makeutil
 *
 * this file is hereby placed in the public domain.
 * alyn.post@lodockikumazvati.org
 */

struct tree
{
  int ch;
  unsigned long freq;
  struct tree *node[2];
};

struct forest
{
  unsigned long freq[0xff+2];
  struct tree leaf[0xff+2];
  struct tree branch[0xff+2];
};

#define leaf(tree) (!((tree)->node[0] || (tree)->node[1]))

extern struct tree* huffman_tree();

/*                                                              ..__
 *                                                              `' "
 */
