/**
 * Auxiliary caching HTTPD for ODROID GO sketch PIMwatch
 * Compile it with: [diet] gcc -O2 -Wall ... ; sstrip ...
 * License is BSD 2-Clause.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
//#include <sys/syscall.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <sched.h>
#include <pthread.h>
#include <signal.h>
#define _XOPEN_SOURCE
#include <time.h>

/**
 * Script executed on start up to register on DYNDNS
 */
#define COMMAND_DYNDNS_REGISTER "/Users/demo/PIMwatch/PIMwatch-caching-httpd-dyndns-register.sh"

/**
 * Script to get the data from the remote server.
 *
 * Exampled data (first line is request time, second line is logged time):
 *
 * 2019-10-14T01:56:26
 * 2019-10-14T01:56:01
 * 0 27% BESClient
 * 0 76% BESClient
 * 0 17% BESClient
 * 0 38% BESClient
 * 1 93% ConcurrentMarkS
 * 0 13% BESClient
 * 1 58% SyncJob::de
 * 0 24% BESClient
 */
#define COMMAND_LOADLOG_LINE "/Users/demo/PIMwatch/get-last-loadlog-lines.sh"

#define VERSION "v.0.1"

/**
 * This is the default type 200 http header to be returned.
 */
const char str200[]="HTTP/1.0 200 OK\r\nServer: PIMwatch-caching-httpd/"VERSION"\r\nContent-Type: text/plain\r\n\r\n";

//#define write1(b,c) (syscall(__NR_write, 1, b, c))
//#define write2(b,c) (syscall(__NR_write, 2, b, c))
#define write1(b,c) (write(1, b, c))
#define write2(b,c) (write(2, b, c))

/* gcc >= 3 required */
#ifndef __expect
#define __expect(foo,bar) __builtin_expect((long)(foo),bar)
#endif
#ifndef __likely
/* macros borrowed from Linux kernel */
#define __likely(foo) __expect((foo),1)
#define __unlikely(foo) __expect((foo),0)
#endif

#define DAEMON_NET_BUFSIZE 1448
#define MAX_HTTP_REQUEST_SIZE 512
#define PIPE_BUF_SIZE 512
#define TIMESTR_SIZE 22
#define LOADSTRS_SIZE 254

#ifdef USE_CLONE
#include <sched.h>
#ifndef CLONE_SYSVSEM
#define CLONE_SYSVSEM 0x00040000
#endif
#ifndef CLONE_IO
#define CLONE_IO 0x80000000
#endif
/**
 * thread's stack
 */
#define STACKSIZE 262144
char stack[STACKSIZE];
// end of ifdef USE_CLONE
#endif

/**
 * cached values, written by thread,
 * 2 values for each type
 */
char cached_timestr1[2][TIMESTR_SIZE+1];
char cached_timestr2[2][TIMESTR_SIZE+1];
char cached_loadsstr[2][LOADSTRS_SIZE+1];

/**
 * generic cursor pointing to current cached values,
 * written by thread, read by main
 */
int cached_cursor;

/**
 * trigger a cache refresh,
 * set by main to 1, read by thread and reset to 0
 */
int refresh_cache;

/**
 * get values from external command
 * raw method without FILE*
 */
static int execute_external_command(register char *buf, const uint16_t bufsize, char *path) {
  char *p;
  int i;
  int fd[2];
  int status_child;
  register char *u;
  pid_t command_child;

  *buf = 0;
  if (pipe(fd)<0) return 0;
  if ((command_child=fork())<0) {
    close(fd[0]); close(fd[1]);
    return 0;
  }
  if (!command_child) {
    const char *argv[]={0,0};
    close(fd[0]); close(0);
    dup2(fd[1],1); close(fd[1]); 
    argv[0]=path;
    execve(argv[0],(char*const*)argv,NULL);
    close(1);
    _exit(127);
  }
  close(fd[1]);

  p = buf;
  u = p + bufsize;
  i = read(fd[0], p, bufsize);
  if (i <= 0) goto out;
  for (;;) {
    p += i;
    *p = 0;
    if (p >= u) break;
    i = read(fd[0], p, u-p);
    if (i <= 0) break;
  }
out:
  close(fd[0]);
  /* TODO: check on macOS (BSD) for better handling */
  waitpid(command_child, &status_child, 0);
  if (!WIFEXITED(status_child)) {
    kill(command_child,SIGKILL);
  }
  return p - buf;
}

static char *stpcpyt(register char *dest, register const char *src, register char *t) {
  if (dest == 0) return dest;
  if (src == 0) return dest;
  if (t == 0) return dest;
  for (;;) {
    *dest++ = *src++; if (dest == t) break; if (*src == 0) break;
    *dest++ = *src++; if (dest == t) break; if (*src == 0) break;
    *dest++ = *src++; if (dest == t) break; if (*src == 0) break;
    *dest++ = *src++; if (dest == t) break; if (*src == 0) break;
  }
  *dest = 0;
  return dest;
}

/**
 * the thread that gets and caches the values
 */
//int interval_caching(void *notused) {
static void *interval_caching(void *notused) {
  int tmpint;
  char buf[PIPE_BUF_SIZE+1];
  char *p;
  char *r;
  char *s;
  char *t;
  int host;

  /* init all values */
  refresh_cache = 0;
  cached_cursor = 0;
  for (cached_cursor=0;;) {
    cached_timestr1[cached_cursor][0] = 0;
    cached_timestr2[cached_cursor][0] = 0;
    cached_loadsstr[cached_cursor][0] = 0;
    if (cached_cursor == 1) break;
    ++cached_cursor;
  }

  for (;;) {
    /* let writing cursor point to the alternate values */
    tmpint = !cached_cursor;

    next_loop:
    usleep(200000);

    /* get data */
    if (execute_external_command(buf, PIPE_BUF_SIZE, COMMAND_LOADLOG_LINE) == 0) {
      tmpint = 5;
      goto wait_for_update;
    }

    /* copy data */
    r = buf;
    p = strtok_r(0, " \n", &r); if (!p) continue;
    strncpy(cached_timestr1[tmpint], p, TIMESTR_SIZE);
    p = strtok_r(0, " \n", &r); if (!p) continue;
    strncpy(cached_timestr2[tmpint], p, TIMESTR_SIZE);
    s = cached_loadsstr[tmpint];
    t = s + LOADSTRS_SIZE - 1;
    for (host = 32; host <= 39; host++) {
      p = strtok_r(0, "\n", &r); if (!p) goto next_loop;
      s = stpcpyt(s, p, t);
      *s++ = '\n';
      if (host == 39) {
        *s = 0;
        goto data_copied;
      }
    }

    data_copied:
    /* update the cursor to point to the latest values */
    cached_cursor = tmpint;

    /* calculate time diff */
    struct tm tm_time1, tm_time2;
    time_t time1, time2;
    tm_time1 = (struct tm) { 0 };
    tm_time2 = (struct tm) { 0 };
    strptime(cached_timestr1[cached_cursor], "%Y-%m-%dT%H:%M:%S", &tm_time1);
    strptime(cached_timestr2[cached_cursor], "%Y-%m-%dT%H:%M:%S", &tm_time2);
    time1 = mktime(&tm_time1);
    time2 = mktime(&tm_time2);
    //printf("DEBUG: %s / %s time1=%ld time2=%ld time1-time2=%ld\n", cached_timestr1[tmpint], cached_timestr2[tmpint], time1, time2, time1-time2);

    /* wait */
    tmpint = (64 - (time1 - time2));
    if ((tmpint > 69) || (tmpint <=9)) tmpint = 2;
wait_for_update:
    //printf("DEBUG: wait %d\n", tmpint);
    for (;;) {
      if (refresh_cache == 1) {
        refresh_cache = 0;
        break;
      }
      --tmpint;
      if (tmpint == 0) break;
      sleep(1);
    }
  }
  return 0;
}

static int http_to_key(int readsock, char key[], int keysize) {
  register char *p;
  char *u;
  char http_buf[MAX_HTTP_REQUEST_SIZE+1];
  int i;
  char *request;
  char c;

  *key = 0;
  p = http_buf;
  i = (int)read(readsock, p, MAX_HTTP_REQUEST_SIZE);
  if (__unlikely(i<15))
    return 0; /* data too short */

  if (__unlikely(strncmp(p, "GET /", 5))) return 0;
  p += 5;

  request = p;
  if (__unlikely(*p == ' ')) {
    u = p++;
    goto check_proto;
  }
  ++p;
  for (c=' ';;) {
    if (!*p) return -1;
    if (*p == ' ') { u = p++; break; };
    if ((c == '.') && (*p == '.')) return -1;
    c = *p++;
  }
  check_proto:
  if (__unlikely(strncmp(p, "HTTP/1.", 7))) return -1;
  p += 7;
  if (__unlikely(*p != '1')) {
    if (*p != '0') return -1;
  }
  *u = '\0';
  i = u - request;
  if (__unlikely(i >= keysize)) return -1;
  strcpy(key, request);
  return i;
}

int main(int argc, char *argv[]) {

  register char *p;
  int tmpint;
  int sockfd;
  int connsock;
  struct sockaddr_in servin;
  struct sockaddr_in peerin;
  socklen_t peerin_len;
  char net_buf_rcv[DAEMON_NET_BUFSIZE+1];
  char net_buf_send[DAEMON_NET_BUFSIZE+1];
#ifdef USE_CLONE
  char *child_stack;
  uint32_t flags;
#endif
  uint16_t net_servport = 3737;

#ifdef USE_CLONE
  child_stack = stack + STACKSIZE;
  flags =
    CLONE_FILES |
    CLONE_FS |
    CLONE_IO |
    CLONE_SIGHAND |
    CLONE_SYSVSEM |
    CLONE_THREAD |
    CLONE_VM;
  clone(interval_caching, child_stack, flags, 0);
  /* NOTE: dietlibc defines 'void *(*)(void *)' but glibc defines 'int (*)(void *)' as first arg */
#else
  /* BSD/macOS compatible */
  {
    pthread_t caching_thread;
    if (pthread_create(&caching_thread, 0, interval_caching, 0) != 0) {
      exit(100);
    }
  }
#endif

  execute_external_command(net_buf_rcv, 18, COMMAND_DYNDNS_REGISTER);

  connsock = -1;
  tmpint = 99;
  sockfd = socket(PF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) goto err_exit;

  tmpint = fcntl(sockfd, F_GETFL);
  tmpint |= O_NONBLOCK;
  tmpint = fcntl(sockfd, F_SETFL, tmpint);
  if (tmpint) goto err_close_exit;

  tmpint = 1;
  tmpint = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &tmpint, sizeof tmpint);
  if (tmpint) goto err_close_exit;

  /* bind */
  memset(&servin, 0, sizeof servin);
  servin.sin_family = AF_INET;
  servin.sin_port = htons(net_servport);
  tmpint = bind(sockfd, (struct sockaddr *) &servin, sizeof servin);
  if (tmpint) goto err_close_exit;

  /* we don't fork/clone for each connection, try backlog of 16 */
  tmpint = listen(sockfd, 16);
  if (tmpint) goto err_close_exit;

  tmpint = fcntl(sockfd, F_GETFL);
  tmpint ^= O_NONBLOCK;
  tmpint = fcntl(sockfd, F_SETFL, tmpint);
  if (tmpint) goto err_close_exit;
  
  tmpint = 65536;
  tmpint = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &tmpint, sizeof tmpint);
  if (tmpint) goto err_close_exit;

  peerin_len = sizeof peerin;
  memset(&peerin, 0, peerin_len);

  goto next_request;


  close_conn:
  (void) close(connsock);

  /* read loop */
  next_request:

  while ((connsock = accept(sockfd, (struct sockaddr *) &peerin, &peerin_len)) == -1);
  if (http_to_key(connsock, net_buf_rcv, DAEMON_NET_BUFSIZE) < 0) goto close_conn;
  //printf("DEBUG: main(), net_buf_rcv=XXX%sXXX cursor=%d\n", net_buf_rcv, cached_cursor);

  /* sendto */

  p = stpcpy(net_buf_send, str200);

  if (*net_buf_rcv == 0) {
    p = stpcpy(p, "OK\n");
    goto send_answer;
  }
  if (!strcmp(net_buf_rcv, "load")) {
    struct tm tm_time1, tm_time2;
    time_t time1, time2;
    p = stpcpy(p, cached_timestr1[cached_cursor]);
    *p++ = '\n';
    p = stpcpy(p, cached_timestr2[cached_cursor]);
    *p++ = '\n';
    p = stpcpy(p, cached_loadsstr[cached_cursor]);
    tm_time1 = (struct tm) { 0 };
    tm_time2 = (struct tm) { 0 };
    strptime(cached_timestr1[cached_cursor], "%Y-%m-%dT%H:%M:%S", &tm_time1);
    strptime(cached_timestr2[cached_cursor], "%Y-%m-%dT%H:%M:%S", &tm_time2);
    time1 = mktime(&tm_time1);
    time2 = mktime(&tm_time2);
    if ((time1 != (time_t) -1) && (time2 != (time_t) -1)) {
    }
    goto send_answer;
  }

  goto close_conn;

  send_answer:
  /* send answer */
  tmpint = p - net_buf_send;
  tmpint = write(connsock, net_buf_send, tmpint);
  goto close_conn;

  err_close_exit:
  close(connsock);

  err_exit:
  write2("error\n", 6);
  exit(tmpint);
}

/* use modeline modelines=1 in vimrc */
/* vim: set ft=c sts=2 sw=2 ts=2 ai et: */
