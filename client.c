#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

#include <memory.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <assert.h>

#include <netdb.h>

#include <pthread.h>

static char *myname = "unknown";

typedef enum connect_state {
  cs_none = 0,
  cs_socket = 1,
  cs_connect = 2,
  cs_connected = 3 } connect_state_t;

typedef struct fdpair {
  int from;
  int to;
  int shutdown_on_eof;
} fdpair_t;

const char *const cs_name[] = { "none", "socket",
				"connect", "connected" };

const int trace = 0;

/* put a space before the arguments in this and it doesn't work */
#define trace_print(...) if (trace) { fprintf (stderr, __VA_ARGS__); }

static void *
copy_stream (void *data) {
  fdpair_t *fdpair = (fdpair_t *) data;


  char buffer[BUFSIZ];
  int bytes;
  int finished_reading = 0;

  trace_print ("copy_stream %d->%d, shutdown = %d\n", fdpair->from, fdpair->to, fdpair->shutdown_on_eof);
  while (!finished_reading) {
    bytes = read (fdpair->from, buffer, BUFSIZ);
    if (bytes < 0) {
      fprintf (stderr, "%d->%d read %s\n", fdpair->from, fdpair->to, strerror (errno));
      pthread_exit (0);
    } else if (bytes == 0) {
      trace_print ("%d->%d end of file on input\n", fdpair->from, fdpair->to);
      finished_reading++;
      if (fdpair->shutdown_on_eof) {
	trace_print ("shutdown (%d)\n", fdpair->to);
	if (shutdown (fdpair->to, SHUT_WR) != 0) {
	  perror ("shutdown");
	}
      }
    } else {
      assert (bytes > 0);
      if (write (fdpair->to, buffer, bytes) != bytes) {
	fprintf (stderr, "%d->%d write %s\n", fdpair->from, fdpair->to, strerror (errno));
	pthread_exit (0);
      }
    }
  }
  trace_print ("%d->%d exiting\n", fdpair->from, fdpair->to);

  pthread_exit (0);
}


int
main (int argc, char **argv) {
  assert (argv[0] && *argv[0]);
  myname = argv[0];
  struct addrinfo *results, hints;
  int res;

  if (argc != 3) {
    fprintf (stderr, "%s: usage is %s host port\n", myname, myname);
    exit (1);
  }

  assert (argv[1] && argv[2]);

  /* hints as a null pointer works on Linux, but not on OSX or Solaris */

  memset (&hints, '\0', sizeof (hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM; /* very important */
  res = getaddrinfo (argv[1], argv[2], &hints, &results);
  
  if (res != 0) {
    fprintf (stderr, "%s: cannot resolve %s:%s (%s)\n",
	     myname, argv[1], argv[2], gai_strerror (res));
    exit (1);
  }

  connect_state_t phase = cs_none;
  int s;
  
  for (struct addrinfo *scan = results;
       phase != cs_connected && scan != 0;
       scan = scan->ai_next) {
    phase = cs_socket;

    s = socket (results->ai_family, results->ai_socktype,
		    results->ai_protocol);
    if (s >= 0) {
      phase = cs_connect;
      if (connect (s, results->ai_addr, results->ai_addrlen) == 0) {
	phase = cs_connected;
      } else {
	close (s);
      }
    }	
  }

  if (phase != cs_connected) {
    perror (cs_name[phase]);
    exit (1);
  }

  fprintf (stderr, "connected\n");

  pthread_t send_thread, receive_thread;

  /* as it happens, in this particular case, you could get away with not
     malloc-ing these.  The main thread will not exit until the other
     threads have exited, and therefore automatic variables on this
     thread's stack are OK for other threads to access.  But it violates
     the principle of least surprise and looks scary, so malloc () them
     to be sure.  */

  fdpair_t *send = malloc (sizeof (fdpair_t));
  fdpair_t *receive = malloc (sizeof (fdpair_t));

  if (send == 0 || receive == 0) {
    perror ("malloc");
    exit (1);
  }

  send->from = 0;
  send->to = s;
  send->shutdown_on_eof = 1;

  receive->from = s;
  receive->to = 1;
  receive->shutdown_on_eof = 0;
  
  if (pthread_create (&send_thread, 0, &copy_stream, send) != 0 ||
      pthread_create (&receive_thread, 0, &copy_stream, receive) != 0) {
    perror ("pthread_create");
    exit (1);
  }

  (void) pthread_join (send_thread, 0);
  (void) pthread_join (receive_thread, 0);
}
  


  

