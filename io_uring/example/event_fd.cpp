// main.cpp
// g++ main.cpp -o main -luring
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <liburing.h>
#include <sys/eventfd.h>

int main()
{
  struct io_uring ring;
  int m_efd = eventfd(0, 0);

  if (io_uring_queue_init(16, &ring, 0) != 0)
  {
    perror("io_uring_queue_init");
    return 1;
  }

  if (io_uring_register_eventfd(&ring, m_efd) != 0)
  {
    perror("io_uring_register_eventfd");
    return 1;
  }

  // 提交多个 NOP 操作，NOP 会在提交后立刻产生 cqe
  for (int i = 0; i < 10; i++)
  {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    io_uring_prep_nop(sqe);
    io_uring_submit(&ring);
  }

  // 等待 eventfd 通知
  uint64_t events;
  if (read(m_efd, &events, sizeof(events)) < 0)
  {
    perror("read eventfd");
    return 1;
  }
  printf("Received %llu events\n", events);

  // 清理
  io_uring_queue_exit(&ring);
  close(m_efd);
  return 0;
}