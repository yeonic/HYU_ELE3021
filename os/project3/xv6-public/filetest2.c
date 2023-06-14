#include "param.h"
#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "user.h"

static int failed = 0;

static void testsymlink(void);
void fail(char* msg);

int
main(int argc, char *argv[])
{
  testsymlink();
  exit();
}

void fail(char* msg) {
    printf(2, "FAILURE: %s\n", msg);
    failed = 1;
}

static void
cleanup(void)
{
  unlink("/testsymlink/a");
  unlink("/testsymlink/b");
  unlink("/testsymlink/c");
  unlink("/testsymlink/1");
  unlink("/testsymlink/2");
  unlink("/testsymlink/3");
  unlink("/testsymlink/4");
  unlink("/testsymlink");
}

static void
testsymlink(void)
{
  int r, fd1 = -1, fd2 = -1;
  char buf[4] = {'a', 'b', 'c', 'd'};
  char c = 0, c2 = 0;

  cleanup();

  printf(2, "\nSTART: test symlinks\n");

  mkdir("/testsymlink");

  printf(2, "Creating a\n");
  fd1 = open("/testsymlink/a", O_CREATE | O_RDWR);
  if(fd1 < 0){
    fail("failed to open a");
    goto done;
  }

  printf(2, "Linking b -> a\n");
  r = symlink("/testsymlink/a", "/testsymlink/b");
  if(r < 0){
    fail("symlink b -> a failed");
    goto done;
  }

  printf(2, "Writing to a\n");
  if(write(fd1, buf, sizeof(buf)) != 4){
    fail("failed to write to a");
    goto done;
  }

  printf(2, "Reading from b\n");
  fd2 = open("/testsymlink/b", O_RDWR);
  if(fd2 < 0)
    {fail("failed to open b"); goto done;}
  read(fd2, &c, 1);
  if (c != 'a')
    {fail("failed to read bytes from b"); goto done;}

  printf(2, "Removing a\n");
  unlink("/testsymlink/a");
  if(open("/testsymlink/b", O_RDWR) >= 0)
    {fail("Should not be able to open b after deleting a"); goto done;}

  printf(2, "Linking a -> b\n");
  r = symlink("/testsymlink/b", "/testsymlink/a");
  if(r < 0)
    {fail("symlink a -> b failed"); goto done;}

  printf(2, "Attempting to open b (cycle)\n");
  r = open("/testsymlink/b", O_RDWR);
  if(r >= 0)
    {fail("Should not be able to open b (cycle b->a->b->..)\n"); goto done;}
  
  printf(2, "Symlinking c to nonexistent file\n");
  r = symlink("/testsymlink/nonexistent", "/testsymlink/c");
  if(r != 0)
    {fail("Symlinking to nonexistent file should succeed\n"); goto done;}

  printf(2, "Creating symlink chain 1->2->3->4\n");
  r = symlink("/testsymlink/2", "/testsymlink/1");
  if(r) {fail("Failed to link 1->2"); goto done;}
  r = symlink("/testsymlink/3", "/testsymlink/2");
  if(r) {fail("Failed to link 2->3"); goto done;}
  r = symlink("/testsymlink/4", "/testsymlink/3");
  if(r) {fail("Failed to link 3->4"); goto done;}

  close(fd1);
  close(fd2);

  fd1 = open("/testsymlink/4", O_CREATE | O_RDWR);
  if(fd1<0) {fail("Failed to create 4\n"); goto done;}
  fd2 = open("/testsymlink/1", O_RDWR);
  if(fd2<0) {fail("Failed to open 1\n"); goto done;}

  c = '#';
  r = write(fd2, &c, 1);
  if(r!=1) {fail("Failed to write to 1\n"); goto done;}
  r = read(fd1, &c2, 1);
  if(r!=1) {fail("Failed to read from 4\n"); goto done;}
  if(c!=c2)
    {fail("Value read from 4 differed from value written to 1\n"); goto done;}

  printf(2, "SUCCESS: test symlinks\n");
done:
  close(fd1);
  close(fd2);
  cleanup();
}