hosts=mail.batten.eu.org pi-one.home.batten.eu.org gromit.cs.bham.ac.uk offsite8.batten.eu.org offsite9.batten.eu.org

OS=$(shell uname -s)
ZIP=socket

CFLAGS=-Wall -Werror
# needlessly included for single-thread case: hardly a crime
LIBS=-lpthread
ifeq ($(OS), SunOS)
  LIBS+= -lsocket -lnsl
endif

CLANG=$(shell which clang)
ifeq ($(CLANG), /usr/bin/clang)
	CC=clang
else
	CC=cc
endif

BIN=single_thread_server multi_thread_server

common_objs=main.o get_listen_socket.o service_client_socket.o \
	make_printable_address.o

single_objs=service_listen_socket.o

multi_objs=service_listen_socket_multithread.o

all_objs=${common_objs} ${single_objs} ${multi_objs}


all: ${BIN}

single_thread_server: ${common_objs} ${single_objs}
	${CC} -o $@ ${CFLAGS} $+ ${LIBS}

multi_thread_server: ${common_objs} ${multi_objs}
	${CC} -o $@ ${CFLAGS} $+ ${LIBS} 

clean:
	rm -f ${common_objs} ${BIN} ${single_objs} ${multi_objs} $(ZIP) *~

# don't use this rule unless you understand exactly what it is doing

zip: $(ZIP)

$(ZIP):
	zip --must-match $(ZIP) *.c  *.h Makefile README

rsync:
	for host in $(hosts); do rsync -avFF . $$host:socket; done

