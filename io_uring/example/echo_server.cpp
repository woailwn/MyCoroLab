// echo_server.cpp
#include <liburing.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define EVENT_ACCEPT 0
#define EVENT_READ 1
#define EVENT_WRITE 2

struct conn_info
{
  int fd;
  int event;
};

int init_server(unsigned short port)
{
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in serveraddr;
  memset(&serveraddr, 0, sizeof(struct sockaddr_in));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons(port);

  if (-1 == bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr)))
  {
    perror("bind");
    return -1;
  }

  listen(sockfd, 10);

  return sockfd;
}

#define ENTRIES_LENGTH 1024
#define BUFFER_LENGTH 1024

void set_event_recv(struct io_uring *ring, int sockfd, void *buf, size_t len, int flags)
{
  struct io_uring_sqe *sqe = io_uring_get_sqe(ring);

  struct conn_info accept_info = {    //conn_info结构体记录I/O操作属于哪个FD
      .fd = sockfd,
      .event = EVENT_READ,
  };

  io_uring_prep_recv(sqe, sockfd, buf, len, flags);  // 为 sqe entry 填充读事件所需的相关数据
  memcpy(&sqe->user_data, &accept_info, sizeof(struct conn_info));
}

void set_event_send(struct io_uring *ring, int sockfd, void *buf, size_t len, int flags)
{
  struct io_uring_sqe *sqe = io_uring_get_sqe(ring);

  struct conn_info accept_info = {
      .fd = sockfd,
      .event = EVENT_WRITE,
  };

  io_uring_prep_send(sqe, sockfd, buf, len, flags); // 为 sqe entry 填充写事件所需的相关数据
  memcpy(&sqe->user_data, &accept_info, sizeof(struct conn_info));
}

void set_event_accept(struct io_uring *ring, int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags)
{
  struct io_uring_sqe *sqe = io_uring_get_sqe(ring);

  struct conn_info accept_info = {
      .fd = sockfd,
      .event = EVENT_ACCEPT,
  };

  io_uring_prep_accept(sqe, sockfd, (struct sockaddr *)addr, addrlen, flags); // 为 sqe entry 填充连接事件所需的相关数据
  memcpy(&sqe->user_data, &accept_info, sizeof(struct conn_info));
}

int main(int argc, char *argv[])
{
  unsigned short port = 8000; 
  int sockfd = init_server(port);

  struct io_uring_params params;
  memset(&params, 0, sizeof(params));

  struct io_uring ring;
  io_uring_queue_init_params(ENTRIES_LENGTH, &ring, &params); // 根据参数初始化 io_uring 实例

  struct sockaddr_in clientaddr;
  socklen_t len = sizeof(clientaddr);
  set_event_accept(&ring, sockfd, (struct sockaddr *)&clientaddr, &len, 0);

  char buffer[BUFFER_LENGTH] = {0};

  while (1)
  {
    io_uring_submit(&ring); // 提交 I/O 请求

    struct io_uring_cqe *cqe;
    io_uring_wait_cqe(&ring, &cqe); // 等待至少一个完成事件

    struct io_uring_cqe *cqes[128];
    int nready = io_uring_peek_batch_cqe(&ring, cqes, 128); // 批量获取完成事件

    int i = 0;
    for (i = 0; i < nready; i++)
    {
      struct io_uring_cqe *entries = cqes[i];
      struct conn_info result;
      memcpy(&result, &entries->user_data, sizeof(struct conn_info));
      if (result.event == EVENT_ACCEPT)
      {
        set_event_accept(&ring, sockfd, (struct sockaddr *)&clientaddr, &len, 0);
        int connfd = entries->res;
        set_event_recv(&ring, connfd, buffer, BUFFER_LENGTH, 0);
      }
      else if (result.event == EVENT_READ)
      {
        int ret = entries->res;
        if (ret == 0)
        {
          close(result.fd);
        }
        else if (ret > 0)
        {
          set_event_send(&ring, result.fd, buffer, ret, 0);
        }
      }
      else if (result.event == EVENT_WRITE)
      {
        int ret = entries->res;
        set_event_recv(&ring, result.fd, buffer, BUFFER_LENGTH, 0);
      }
    }
    io_uring_cq_advance(&ring, nready); // 批量标记完成事件，使得关联的 sqe 可以被重新使用
  }
  return 0;
}