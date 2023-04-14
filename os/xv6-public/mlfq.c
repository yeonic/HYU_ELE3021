#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "mlfq.h"

struct levelq mlfq[NQLEV];

// functions for struct procq
int 
isempty(struct levelq* q)
{
  if(q->rear == q->front)
    return 1;
  return 0;
}

int 
isfull(struct levelq* q)
{
  if((q->rear)%(NPROC+1) == q->front)
    // max size is NPROC + 1
    return 1;
  return 0;
}


// return -1 if queue is full.
// return 0 if enqueue succeed.
int
enprocq(struct levelq* q, struct proc* entry)
{
  if(isfull(q))
    return -1;
  
  q->rear = (q->rear+1) % QSIZE;
  q->queue[q->rear] = entry;
  return 0;
}


// return null if queue is empty
// return proc* if dequeue succeed
struct proc*
deprocq(struct levelq* q)
{
  if(isempty(q))
    return (struct proc*)0;

  q->front = (q->front+1) % QSIZE;
  return q->queue[q->front];
}

// functions for struct mlfq
void
mlfqinit()
{
  int i;
  for(i=0; i<NQLEV; i++) {
    mlfq[i].level = i;
    mlfq[i].front = 0;
    mlfq[i].rear = 0;
    mlfq[i].hstpri = DISABLED;
  }
  mlfq[i].hstpri = INITPRI;
}