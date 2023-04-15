// queue structure consists each level of mlfq
struct levelq{
  struct proc* queue[QSIZE]; 
  int front;
  int rear;
  int level;
};

struct levelpq{
  struct proc* heap[QSIZE];
  int level;
  int size;
};

struct mlfq{
  struct levelq rrlevels[NQLEV];
  struct levelpq prlevel;
};

enum qstate{
  Q_EMPTY = 0,
  Q_FULL = 1,
  Q_TASK_SUCCEED = 2,
};