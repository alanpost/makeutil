/*
 * tree.h - huffman compression data structures
 *
 * this file is part of the makeutil package:
 *   http://sourceforge.net/projects/makeutil/
 *   http://www.cybermesa.com/~aisa/makeutil/
 *
 * this file is hereby placed in the public domain.
 * aisa0@users.sourceforge.net, aisa@cybermesa.com
 *
 * $Id: tree.h,v 1.1 2004/11/28 22:57:53 aisa0 Exp $
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
