#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "mlfq.h"

extern int sys_uptime(void);

// functions for struct procq
int 
isempty(struct levelq* q)
{
  if(q->rear == q->front)
    return Q_EMPTY;
  return Q_TASK_SUCCEED;
}

int 
isfull(struct levelq* q)
{
  if((q->rear)%(NPROC+1) == q->front)
    // max size is NPROC + 1
    return Q_FULL;
  return Q_TASK_SUCCEED;
}

int
enprocq(struct levelq* q, struct proc* entry)
{
  if(isfull(q))
    return Q_FULL;
  
  q->rear = (q->rear+1) % QSIZE;
  q->queue[q->rear] = entry;
  return Q_TASK_SUCCEED;
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

// functions for mlfq
void
mlfqinit(struct mlfq* q)
{
  int i;
  for(i=0; i<NQLEV; i++) {
    q->levels[i].level = i;
    q->levels[i].front = 0;
    q->levels[i].rear = 0;
    q->levels[i].hstpri = DISABLED;
  }
  q->levels[i].hstpri = INITPRI;
}

int
enmlfq(struct mlfq* q, struct proc* entry)
{
  int clevel = entry->mlfq.level;
  int rmtime = entry->mlfq.rmtime;

  // when remaining time(rmtime) goes 0
  // enqueue process into new level.
  if(rmtime==0 && clevel==2) {
    if(enprocq(&q->levels[clevel], entry) == Q_FULL)
      return Q_FULL;
    entry->mlfq.priority--;
    entry->mlfq.rmtime = 2*clevel + 4;
    entry->mlfq.queuedtick = sys_uptime();
    return Q_TASK_SUCCEED;
  }

  if(rmtime==0 && clevel<2){
    if(enprocq(&q->levels[clevel+1], entry) == Q_FULL)
      return Q_FULL;
    entry->mlfq.level++;
    entry->mlfq.rmtime = 2*entry->mlfq.level + 4;
    entry->mlfq.queuedtick = sys_uptime();
    return Q_TASK_SUCCEED;
  }

  // usual case
  if(enprocq(&q->levels[clevel], entry) == Q_FULL)
    return Q_FULL;
  entry->mlfq.queuedtick = sys_uptime();
  
  return Q_TASK_SUCCEED;
}

int
demlfq(struct mlfq* q, struct proc* entry)
{

}