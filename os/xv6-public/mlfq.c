#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "mlfq.h"

extern int sys_uptime(void);

// functions for rrlevel
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
enprocq(struct levelq* q, struct proc* e)
{
  if(isfull(q))
    return Q_FULL;
  
  q->rear = (q->rear+1) % QSIZE;
  q->queue[q->rear] = e;
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


// functions for prlevel
// compare function for priority->queuedtime comparison.
// fit to while loop.
int 
comp(struct proc* a, struct proc* b)
{
  if(a->mlfq.priority < b->mlfq.priority)
    return FALSE;
  else if(a->mlfq.priority == b->mlfq.priority)
    return a->mlfq.queuedtick <= b->mlfq.queuedtick ? FALSE : TRUE;
  return TRUE;
}

void 
swap(struct proc* h[], int idx1, int idx2)
{
  struct proc* temp;
  temp = h[idx1];
  h[idx1] = h[idx2];
  h[idx2] = temp;
}

// put item into priority queue
int
enpq(struct levelpq* pq, struct proc* e)
{
  int i;
  if(pq->size == QSIZE - 1) {
    return Q_FULL;
  }
  i = ++pq->size;
  pq->heap[i] = e;
  while(i>1 && comp(pq->heap[i/2], pq->heap[i])){
    swap(pq->heap, i/2, i);
    i = i/2;
  }
  return Q_TASK_SUCCEED;
}

void
min_heapify(struct levelpq* pq, uint curr)
{
  int smallest = curr;
  int left = 2*curr;
  int right = 2*curr + 1;

  if(left<QSIZE && comp(pq->heap[left], pq->heap[curr])){
    smallest = left;
  }
  if(right<QSIZE && comp(pq->heap[right], pq->heap[curr])){
    smallest = right;
  }
  
  if(smallest != curr) {
    swap(pq->heap, curr, smallest);
    min_heapify(pq, smallest);
  }
}

struct proc*
depq(struct levelpq* pq)
{
  if(pq->size == 0)
    return Q_EMPTY;
  
  struct proc* min = pq->heap[1];
  pq->heap[1] = pq->heap[pq->size];

  min_heapify(pq, 1);
  return min;
}

void
decreasekey(struct levelpq* pq, int idx, uint key)
{
  pq->heap[idx] = key;
  while(idx < 2 && pq->heap[idx/2] < pq->heap[idx]) {
      swap(pq->heap, idx/2, idx);
      idx = idx/2;
  }
}

// functions for mlfq
void
mlfqinit(struct mlfq* q)
{
  int i;
  // round-robin level
  for(i=0; i<NQLEV; i++) {
    q->rrlevels[i].level = i;
    q->rrlevels[i].front = 0;
    q->rrlevels[i].rear = 0;
  }

  // priority queue level
  q->prlevel.level = 2;
  q->prlevel.size = 0;
  q->prlevel.heap[0] = DISABLED;
}

int
enmlfq(struct mlfq* q, struct proc* e)
{
  int clevel = e->mlfq.level;
  int rmtime = e->mlfq.rmtime;

  // when remaining time(rmtime) is 0
  // enqueue process into new level.
  if(rmtime==0 && clevel==2) {
    if(enprocq(&q->rrlevels[clevel], e) == Q_FULL)
      return Q_FULL;
    e->mlfq.priority--;
    e->mlfq.rmtime = 2*clevel + 4;
    e->mlfq.queuedtick = sys_uptime();
    return Q_TASK_SUCCEED;
  }

  if(rmtime==0 && clevel<2){
    if(enprocq(&q->rrlevels[clevel+1], e) == Q_FULL)
      return Q_FULL;
    e->mlfq.level++;
    e->mlfq.rmtime = 2*e->mlfq.level + 4;
    e->mlfq.queuedtick = sys_uptime();
    return Q_TASK_SUCCEED;
  }

  // usual case
  if(enprocq(&q->rrlevels[clevel], e) == Q_FULL)
    return Q_FULL;
  e->mlfq.queuedtick = sys_uptime();

  return Q_TASK_SUCCEED;
}

int
demlfq(struct mlfq* q, struct proc* e)
{

}