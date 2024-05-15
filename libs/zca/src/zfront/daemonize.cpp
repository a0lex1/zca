#include "zbackfront/daemonize.h"

#ifndef _WIN32
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

bool Daemonize() {
  pid_t pid;
  if (pid = fork()) {
    if (pid > 0) {
      exit(0);
    } else {
      syslog(LOG_ERR | LOG_USER, "First fork failed: %m");
      return 1;
    }
  }
  setsid();
  chdir("/");
  umask(0);
  if (pid = fork()) {
    if (pid > 0) {
      exit(0);
    } else {
      syslog(LOG_ERR | LOG_USER, "Second fork failed: %m");
      return 1;
    }
  }
  close(0);
  close(1);
  close(2);
  // We don't want the daemon to have any standard input.
  if (open("/dev/null", O_RDONLY) < 0)
  {
    syslog(LOG_ERR | LOG_USER, "Unable to open /dev/null: %m");
    return 1;
  }
  return true;
}
#endif
