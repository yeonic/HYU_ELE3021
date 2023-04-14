// queue structure consists each level of mlfq
struct levelq{
  struct proc* queue[QSIZE]; 
  int front;
  int rear;
};

// mlfq 
struct mlfq{
  struct procq* levels[NQLEV];

};
