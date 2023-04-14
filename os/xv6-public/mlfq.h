// queue structure consists each level of mlfq
struct levelq{
  struct proc* queue[QSIZE]; 
  int front;
  int rear;
  int level;
  uint hstpri;    // highest priority of the levelq. ONLY used when level=2, in case of others, this is set to
};

