/*
 * tree - shared compression routines
 *
 * this file is part of the makeutil package:
 *   https://github.com/alanpost/makeutil
 *
 * this file is hereby placed in the public domain.
 * alyn.post@lodockikumazvati.org
 */

#include "tree.h"
#include <stdio.h>

struct list
{
  struct tree *tree;
  struct list *next;
};

struct queue
{
  int n;
  struct list *list;
  struct list head;
  struct list node[0xff+2];
};

static size_t
unber(b, o)
   char *b;
   unsigned long *o;
{
  char *s=b;
  unsigned long n=0U;

  do {
     n=n*128UL+(unsigned long)(*s&127);
  } while(*s++&128);

  *o=n;
  return s-b;
}

static void
queue_init(queue)
  struct queue *queue;
{
  int i;

  queue->n=-1;
  for(i=-1; i<=0xff; ++i) {
    (queue->node+1)[i].tree=0;
    (queue->node+1)[i].next=0;
  }
  queue->head.tree=0;
  queue->head.next=0;
  queue->list=&queue->head;
}

static void
enqueue(queue, tree)
  struct queue *queue;
  struct tree *tree;
{
  struct list *list, *node;

  list=queue->list;
  while(list->next && list->next->tree->freq<tree->freq) {
    list=list->next;
  }
  while((queue->node+1)[queue->n].tree) {
    if(0xff+1==queue->n++) queue->n=-1;
  }
  node=&(queue->node+1)[queue->n];

  node->tree=tree;
  node->next=list->next;
  list->next=node;
}

static struct tree*
dequeue(queue)
  struct queue *queue;
{
  struct list *node;
  struct tree *tree;

  if(node=queue->list->next) {
    queue->list->next=node->next;
    tree=node->tree;
    node->tree=0;
    return tree;
  } else {
    return 0;
  }
}

struct tree*
huffman_tree(forest, freq, freq_size)
  struct forest *forest;
  char *freq;
  size_t freq_size;
{
  struct queue queue;
  struct tree *tree0, *tree1;
  char *s, *e;
  int i;

  (forest->freq+1)[EOF]=1UL;
  for(i=0 /*skip eof*/; i<=0xff; ++i)
    (forest->freq+1)[i]=0UL;

  s=freq;
  e=s+freq_size;
  while(s<e) {
	unsigned long n, v;
	s+=unber(s, &n);
	s+=unber(s, &v);
	(forest->freq+1)[n]=v;
  }

  for(i=-1; i<=0xff; ++i) {
    tree0=&(forest->leaf+1)[i];
    tree0->ch=i;
    tree0->freq=(forest->freq+1)[i];
    tree0->node[0]=(struct tree*)0;
    tree0->node[1]=(struct tree*)0;

    tree0=&(forest->branch+1)[i];
    tree0->ch=-1;
    tree0->freq=0UL;
    tree0->node[0]=(struct tree*)0;
    tree0->node[1]=(struct tree*)0;
  }

  queue_init(&queue);
  for(i=-1; i<=0xff; ++i)
    enqueue(&queue, &(forest->leaf+1)[i]);

  i=0;
  while((tree0=dequeue(&queue)) && (tree1=dequeue(&queue))) {
    struct tree *branch=&forest->branch[i++];

    branch->freq=tree0->freq+tree1->freq;
    branch->node[0]=tree0;
    branch->node[1]=tree1;
    enqueue(&queue, branch);
  }

  return tree0;
}

/*                                                              ..__
 *                                                              `' "
 */
