#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "fq.h"

#define SEND_COUNT 1000
int send_count = SEND_COUNT;
void logger(const char *);

void logger(const char *s) {
  fprintf(stderr, "fq_logger: %s\n", s);
}
static void
print_rate(fq_client c, hrtime_t s, hrtime_t f, uint64_t cnt, uint64_t icnt) {
  double d;
  if(cnt) {
    d = (double)cnt * 1000000000;
    d /= (double)(f-s);
    printf("[%d backlog] output %0.2f msg/sec\n",
           fq_client_data_backlog(c), d);
  }
  if(icnt) {
    d = (double)icnt * 1000000000;
    d /= (double)(f-s);
    printf("[%d backlog]  input %0.2f msg/sec\n",
           fq_client_data_backlog(c), d);
  }
}
int main(int argc, char **argv) {
  hrtime_t s0, s, f, f0;
  uint64_t cnt = 0, icnt = 0;
  int psize = 0, i = 0, rcvd = 0;
  fq_client c;
  fq_bind_req breq;
  fq_msg *m;
  signal(SIGPIPE, SIG_IGN);
  fq_client_init(&c, 0, logger);
  if(argc < 5) {
    fprintf(stderr, "%s <host> <port> <user> <pass> [size [count]]\n",
            argv[0]);
    exit(-1);
  }
  fq_client_creds(c, argv[1], atoi(argv[2]), argv[3], argv[4]);
  fq_client_heartbeat(c, 1000);
  fq_client_set_backlog(c, 10000, 100);
  fq_client_connect(c);

  memset(&breq, 0, sizeof(breq));
  memcpy(breq.exchange.name, "maryland", 8);
  breq.exchange.len = 8;
  breq.peermode = 0;
  breq.program = (char *)"prefix:\"test.prefix.\"";

  fq_client_bind(c, &breq);
  while(breq.out__route_id == 0) usleep(100);
  printf("route set -> %u\n", breq.out__route_id);
  if(breq.out__route_id == FQ_BIND_ILLEGAL) {
    fprintf(stderr, "Failure to bind...\n");
    exit(-1);
  }

  if(argc > 5) {
     psize = atoi(argv[5]);
  }
  printf("payload size -> %d\n", psize);
  if(argc > 6) {
    send_count = atoi(argv[6]);
  }
  printf("message count -> %d\n", send_count);

  s0 = s = fq_gethrtime();
  while(i < send_count || fq_client_data_backlog(c) > 0) {
    if(i < send_count) {
      m = fq_msg_alloc_BLANK(psize);
      memset(m->payload, 0, psize);
      fq_msg_exchange(m, "maryland", 8);
      fq_msg_route(m, "test.prefix.foo", 15);
      fq_msg_id(m, NULL);
      fq_client_publish(c, m);
      cnt++;
      i++;
      fq_msg_free(m);
    }
    else usleep(100);


    f = fq_gethrtime();
    while(m = fq_client_receive(c)) {
      icnt++;
      rcvd++;
      fq_msg_deref(m);
    }
    if(f-s > 1000000000) {
      print_rate(c, s, f, cnt, icnt);
      icnt = 0;
      cnt = 0;
      s = f;
    }
  }
  f0 = fq_gethrtime();
  print_rate(c, s0, f0, i, 0);
  do {
    icnt=0;
    while(m = fq_client_receive(c)) {
      icnt++;
      rcvd++;
      fq_msg_deref(m);
    }
  } while(rcvd < send_count);
  f0 = fq_gethrtime();
  print_rate(c, s0, f0, 0, rcvd);
  printf("Total received during test: %d\n", rcvd);

  (void) argc;
  return 0;
}
