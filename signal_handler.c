#include "signal_handler.h"
#include "bitfield.h"
#include "torrentfile.h"
#include "tracker.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

void do_clean_work() {
  // 关闭所有 peer 的 socket
  // 保存位图
  // 关闭所有 文件描述符
  // 释放动态分配的资源
  releasae_memory_in_bitfield();
  release_torrentfile_memory();
  release_tracker_memory();
  exit(0);
}

void process_singal(int signo) {
  printf("Please wait for clean operations\n");
  do_clean_work();
}

int set_signal_handler() {
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
    perror("can not catch signal:sigpipe\n");
    return -1;
  }

  if (signal(SIGINT, process_singal) == SIG_ERR) {
    perror("can not catch signal:sigint\n");
    return -1;
  }

  if (signal(SIGTERM, process_singal) == SIG_ERR) {
    perror("can not catch signal:sigterm\n");
    return -1;
  }
  return 0;
}
