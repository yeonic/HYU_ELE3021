#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "mlfq.h"

// functions for rrlevel
int 
isempty(struct levelq* q, int level)
{
  if(q[level].rear == q[level].front)
    return Q_EMPTY;
  return Q_TASK_SUCCEED;
}

int 
isfull(struct levelq* q, int level)
{
  if((q[level].rear+1)%(NPROC+1) == q[level].front)
    // max size is NPROC + 1
    return Q_FULL;
  return Q_TASK_SUCCEED;
}

void
enprocq(struct levelq* q, int level, struct proc* e)
{
  if(isfull(q, level) == Q_FULL)
    return;
  
  q[level].rear = (q[level].rear+1) % QSIZE;
  q[level].queue[q[level].rear] = e;
}

// enqueue to front of the L0 queue.
// only used in schedulerunlock.
void
fenprocq(struct levelq* q, struct proc* e)
{
  if(isempty(q, 0) == Q_EMPTY)
    return enprocq(q, 0, e);

  // it is make sense that least prioritized process need to be kicked.
  if(isfull(q, 0) == Q_FULL) {
    q[0].rear = q[0].rear == 0 ? QSIZE-1 : q[0].rear - 1;
  }
  q[0].front = q[0].front == 0 ? QSIZE-1 : q[0].front - 1;
  q[0].queue[q[0].front] = e;  
}


// return null if queue is empty
// return proc* if dequeue succeed
struct proc*
deprocq(struct levelq* q, int level)
{
  if(isempty(q, level) == Q_EMPTY)
    return (struct proc*)0;

  q[level].front = (q[level].front+1) % QSIZE;
  return q[level].queue[q[level].front];
}


// functions for prlevel
// compare function for priority->elapsed time comparison.
// fit to while loop.
// FALSE -> stop loop.
int 
comp(struct proc* parent, struct proc* child)
{
  if(parent->mlfq.priority < child->mlfq.priority)
    return FALSE;
  else if(parent->mlfq.priority == child->mlfq.priority)
    return parent->mlfq.elapsed >= child->mlfq.elapsed ? FALSE : TRUE;
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
  e->mlfq.pqindex = i;
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

  if(left<pq->size && !comp(pq->heap[curr], pq->heap[left])){
    smallest = left;
  }
  if(right<pq->size && !comp(pq->heap[curr], pq->heap[right])){
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
  // cprintf("(pq, idx, pri): (%d, %d, %d)\n", pq, idx, pri);
  struct proc* proc = pq->heap[idx];
  proc->mlfq.priority = pri;

  while(idx > 2 && comp(pq->heap[idx/2], pq->heap[idx])) {
      swap(pq->heap, idx/2, idx);
      idx = idx/2;
      proc->mlfq.pqindex = idx;
  }
}

// functions for mlfq
void
mlfqinit(struct mlfq* q)
{
  int i;
  // round-robin level
  for(i=0; i<NQLEV; i++){
    q->rrlevels[i].front = 0;
    q->rrlevels[i].rear = 0;
  }

  // priority queue level
  q->prlevel.size = 0;
  q->prlevel.heap[0] = (struct proc*)DISABLED;

  // lockflag is set to 1 when schedulerLock is called successfully.
  // initlock to slock
  q->locked = 0;
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
      enprocq(q->rrlevels, e->mlfq.level, e);
      e->mlfq.elapsed = 0;
      break;
    case 1:
      e->mlfq.level++;
      e->mlfq.rmtime = 2*(e->mlfq.level) + 4;
      enpq(&q->prlevel, e);
      e->mlfq.elapsed = 0;
      minheapify(&q->prlevel, 1);
      break;
    case 2:
      if(e->mlfq.priority != 0) {
        e->mlfq.priority--;
        decreasekey(&q->prlevel, e->mlfq.pqindex, e->mlfq.priority);// setPriority
      }

      e->mlfq.rmtime = 2*(e->mlfq.level) + 4;
      enpq(&q->prlevel, e);
      e->mlfq.elapsed = 0;
      break;
  }
}

void
enmlfq(struct mlfq* q, struct proc* e)
{
  if(e->state != RUNNABLE) {
    return;
  }

  // case of proc timeout
  if(e->mlfq.rmtime == 0) {
    updatemlfq(q, e);
    return;
  }
  if(e->mlfq.level == 2) {
    enpq(&q->prlevel, e);
    // cprintf("enmlfq: pid%d to L2.(%d ticks left.)\n", e->pid, e->mlfq.rmtime);
  } else {
    enprocq(q->rrlevels, e->mlfq.level, e);
    // cprintf("enmlfq: pid%d to L%d(%d ticks left.)\n", e->pid, e->mlfq.level, e->mlfq.rmtime);
  }
  e->mlfq.elapsed++;
}


struct proc*
demlfq(struct mlfq* q, int level)
{
  struct proc* this;
  if(level == 2) {
    this = depq(&q->prlevel);
    // cprintf("demlfq: pid%d from L2.(%d ticks left.)\n", this->pid, this->mlfq.rmtime);
  } else {
    this = deprocq(q->rrlevels, level);
    // cprintf("demlfq: pid%d from L%d.(%d ticks left.)\n", this->pid, level, this->mlfq.rmtime);
  }
  this->mlfq.rmtime--;
  return this;
}


// should be called only when ticks == 100
void
boostmlfq(struct mlfq* q)
{
  struct proc* temp;
  // reset ( rmtime ) of proc in L0
  while((temp = deprocq(q->rrlevels, 0)) != 0){
    temp->mlfq.rmtime = 2*(temp->mlfq.level) + 4;
    enprocq(q->rrlevels, 0, temp);
  }

  // reset ( rmtime ) of proc in L1 and enqueue to L0
  while((temp = deprocq(q->rrlevels, 1)) != 0){
    temp->mlfq.level = 0;
    temp->mlfq.rmtime = 2*(temp->mlfq.level) + 4;
    enprocq(q->rrlevels, 0, temp);
  }

  // reset ( rmtime, priority ) of proc in L2 and enqueue to L0
  while((temp=depq(&q->prlevel)) != Q_EMPTY){
    temp->mlfq.level = 0;
    temp->mlfq.rmtime = 2*(temp->mlfq.level) + 4;
    enprocq(q->rrlevels, 0, temp);
  }
}