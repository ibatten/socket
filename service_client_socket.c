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

/* why can I not use const size_t here? */
#define buffer_size 1024

int
service_client_socket (const int s, const char *const tag) {
  char buffer[buffer_size];
  size_t bytes;

  printf ("new connection from %s\n", tag);

  /* repeatedly read a buffer load of bytes, leaving room for the
     terminating NUL we want to add to make using printf() possible */
  while ((bytes = read (s, buffer, buffer_size - 1)) > 0) {
    /* this code is not quite complete: a write can in this context be
       partial and return 0<x<bytes.  realistically you don't need to
       deal with this case unless you are writing multiple megabytes */
    if (write (s, buffer, bytes) != bytes) {
      perror ("write");
      return -1;
    }
    /* NUL-terminal the string */
    buffer[bytes] = '\0';
    /* special case for tidy printing: if the last two characters are
       \r\n or the last character is \n, zap them so that the newline
       following the quotes is the only one. */
    if (bytes >= 1 && buffer[bytes - 1] == '\n') {
      if (bytes >= 2 && buffer[bytes - 2] == '\r') {
	strcpy (buffer + bytes - 2, "..");
      } else {
	strcpy (buffer + bytes - 1, ".");
      }
    }
    
#if (__SIZE_WIDTH__ == 64 || __SIZEOF_POINTER__ == 8)
    printf ("echoed %ld bytes back to %s, \"%s\"\n", bytes, tag, buffer);
#else
    printf ("echoed %d bytes back to %s, \"%s\"\n", bytes, tag, buffer);
#endif
  }
  /* bytes == 0: orderly close; bytes < 0: something went wrong */
  if (bytes != 0) {
    perror ("read");
    return -1;
  }
  printf ("connection from %s closed\n", tag);
  close (s);
  return 0;
}

