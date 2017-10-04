#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

#include <memory.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <assert.h>

#include <pthread.h>

#include "service_client_socket.h"
#include "service_listen_socket.h"
#include "make_printable_address.h"

typedef struct thread_control_block {
  int client;
  struct sockaddr_in6 their_address;
  socklen_t their_address_size;
} thread_control_block_t;


static void
*client_thread (void *data) {
  thread_control_block_t *tcb_p = (thread_control_block_t *) data;
  char buffer [INET6_ADDRSTRLEN + 32];
  char *printable;

  assert (tcb_p->their_address_size == sizeof (tcb_p->their_address));

  printable = make_printable_address (&(tcb_p->their_address),
				      tcb_p->their_address_size,
				      buffer, sizeof (buffer));
  (void) service_client_socket (tcb_p->client, printable);
  free (printable);		/* this was strdup'd */
  free (data);			/* this was malloc'd */
  pthread_exit (0);
}

int
service_listen_socket (const int s) {
  /* accept takes a socket in the listening state, and waits until a
     connection arrives.  It returns the new connection, and updates the
     address structure and length (if supplied) with the address of the
     caller. */


  while (1) {
    /* create space to put information specific to the thread.  

       a thread can access global variables, its OWN stack (ie,
       automatic variables in functions that have been executed from the
       thread start routine downwards) and malloc'd data.  What a thread
       MUST NOT access is data from another thread's stack, as that can
       disappear, change and generally cause problems.  Any data which
       is not known to belong exclusively to the thread must be locked
       when it is being accessed, which is outside the scope of this
       example.  */
    
    thread_control_block_t *tcb_p = malloc (sizeof (*tcb_p));

    if (tcb_p == 0) {
      perror ("malloc");
      exit (1);
    }

    tcb_p->their_address_size = sizeof (tcb_p->their_address);

    /* we call accept as before, except we now put the data into the
       thread control block, rather than onto our own stack,
       because...[1] */
    if ((tcb_p->client = accept (s, (struct sockaddr *) &(tcb_p->their_address),
				 &(tcb_p->their_address_size))) < 0) {
      perror ("accept");
      /* may as well carry on, this is probably temporary */
    } else {
      pthread_t thread;
      /* [1]...we now create a thread and start it by calling
	 client_thread with the tcb_p pointer.  That data was malloc'd,
	 so is safe to use.  If we just have a structure on our own
	 stack (ie, a variable local to this function) and passed a
	 pointer to that, then there would be some exciting race
	 conditions as it was destroyed (or not) before the created
	 thread finished using it. */
    
      if (pthread_create (&thread, 0, &client_thread, (void *) tcb_p) != 0) {
	perror ("pthread_create");
	goto error_exit;	/* avoid break here in of the later
				   addition of an enclosing loop */
      }
    }
  }
 error_exit:
  return -1;
}
