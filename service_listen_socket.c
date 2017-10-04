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

#include "service_client_socket.h"
#include "service_listen_socket.h"
#include "make_printable_address.h"

int
service_listen_socket (const int s) {
  int client;
  struct sockaddr_in6 their_address;
  socklen_t their_address_size = sizeof (their_address);
  char buffer[INET6_ADDRSTRLEN + 32];
  char *printable;
  
  /* accept takes a socket in the listening state, and waits until a
     connection arrives.  It returns the new connection, and updates the
     address structure and length (if supplied) with the address of the
     caller. */
  while ((client = accept (s, (struct sockaddr *) &their_address,
			   &their_address_size)) >= 0) {
    assert (their_address_size == sizeof (their_address));

    printable = make_printable_address (&their_address,
					their_address_size,
					buffer, sizeof (buffer));
    /* now go off and deal with the new socket */
    (void) service_client_socket (client, printable);
    free (printable);		/* strdup'd */
  }
  if (client < 0) {
    perror ("accept");
    return -1;
  }
  return 0;
}

  
