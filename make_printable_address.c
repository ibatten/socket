#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <netinet/in.h>
#include <assert.h>

char *
make_printable_address (const struct sockaddr_in6 *const addr,
			const socklen_t addr_len,
			char *const buffer,
			const size_t buffer_size) {
  char printable[INET6_ADDRSTRLEN];

  assert (addr_len == sizeof (*addr));

  /* inet_ntop is the modern way to convert address structures into
     printable strings.  DO NOT USE inet_ntoa and similar as they only
     handle IPv4.  Note that IPv4 connections will return an IPv6 address
     in a special format.  Note also (because I wasted half an hour until
     I found the bug) that there is no type checking on the second argument
     and you pass in the address, not the sockaddr *. */
  if (inet_ntop (addr->sin6_family, &(addr->sin6_addr),
		 printable, sizeof (printable)) == printable) {
    snprintf (buffer, buffer_size, "%s port %d",
	      printable, ntohs (addr->sin6_port));
  } else {
    perror ("inet_ntop");
    snprintf (buffer, buffer_size, "unparseable address");
  }

  return strdup (buffer);
}

  
