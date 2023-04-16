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
    e->mlfq.pqindex = i/2;
    i = i/2;
  }
  return Q_TASK_SUCCEED;
}

void
minheapify(struct levelpq* pq, uint curr)
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
    minheapify(pq, smallest);
  }
}

struct proc*
depq(struct levelpq* pq)
{
  if(pq->size == 0)
    return Q_EMPTY;
  
  struct proc* min = pq->heap[1];
  pq->heap[1] = pq->heap[pq->size--];

  minheapify(pq, 1);
  return min;
}

void
decreasekey(struct levelpq* pq, int idx, uint pri)
{
  struct proc* proc = pq->heap[idx];
  pq->heap[idx]->mlfq.priority = pri;
  while(idx < 2 && comp(pq->heap[idx/2], pq->heap[idx])) {
      swap(pq->heap, idx/2, idx);
      proc->mlfq.pqindex = idx/2;
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
  q->prlevel.heap[0] = (struct proc*)DISABLED;
}


// only called when mlfq.rmtime == 0
// move the process e to lowerly prioritized queue.
void
updatemlfq(struct mlfq* q, struct proc* e)
{
  switch (e->mlfq.level){
    case 0:
      e->mlfq.level++;
      e->mlfq.rmtime = 2*(e->mlfq.level) + 4;
      enprocq(&q->rrlevels[e->mlfq.level], e);
      e->mlfq.queuedtick = sys_uptime();
      break;
    case 1:
      e->mlfq.level++;
      e->mlfq.rmtime = 2*(e->mlfq.level) + 4;
      enpq(&q->prlevel, e);
      e->mlfq.queuedtick = sys_uptime();
      break;
    case 2:
      int newpri = e->mlfq.priority == 0 ? 0 : e->mlfq.priority-1; 
      decreasekey(&q->prlevel, e->mlfq.pqindex, newpri);// setPriority
      e->mlfq.rmtime = 2*(e->mlfq.level) + 4;
      enpq(&q->prlevel, e);
      e->mlfq.queuedtick = sys_uptime();
      break;
  }
}

void
enmlfq(struct mlfq* q, struct proc* e)
{
  // case of proc timeout
  if(e->mlfq.rmtime == 0) {
    updatemlfq(q, e);
    return;
  }
  if(e->mlfq.level == 2) {
    enpq(&q->prlevel, e);
  } else {
    enprocq(&q->rrlevels[e->mlfq.level], e);
  }
  e->mlfq.queuedtick = sys_uptime();
}


struct proc*
demlfq(struct mlfq* q, int level)
{
  struct proc* this;
  if(level == 2) {
    this = depq(&q->prlevel);
  } else {
    this = deprocq(&q->rrlevels[level]);
  }
  this->mlfq.rmtime--;
  return this;
}


// should be called only when ticks == 0
void
boostmlfq(struct mlfq* q, int* ticks)
{
  struct proc* temp;
  // reset ( rmtime ) of proc in L0
  while((temp = deprocq(&q->rrlevels[0])) != 0){
    temp->mlfq.rmtime = 2*(temp->mlfq.level) + 4;
    enprocq(&q->rrlevels[0], temp);
  }

  // reset ( rmtime ) of proc in L1 and enqueue to L0
  while((temp = deprocq(&q->rrlevels[1])) != 0){
    temp->mlfq.level = 0;
    temp->mlfq.rmtime = 2*(temp->mlfq.level) + 4;
    enprocq(&q->rrlevels[0], temp);
  }

  // reset ( rmtime, priority ) of proc in L2 and enqueue to L0
  while((temp=depq(&q->prlevel)) != Q_EMPTY){
    temp->mlfq.level = 0;
    temp->mlfq.rmtime = 2*(temp->mlfq.level) + 4;
    enprocq(&q->rrlevels[0], temp);
  }

  // reset tick to 0
  acquire(&tickslock);
  *ticks = 0;
  release(&tickslock);
}