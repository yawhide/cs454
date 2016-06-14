#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/time.h>

#define MAX_CLIENT 5
#define MAXDATASIZE 256

void sigchld_handler(int s) {
  // waitpid() might overwrite errno, so we save and restore it:
  int saved_errno = errno;

  while(waitpid(-1, NULL, WNOHANG) > 0);

  errno = saved_errno;
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET)
    {
      return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void modifyBufferToUpperCase(char* buf, int nbytes) {
  buf[nbytes] = '\0';
  int multiplier = 1000;
  int counter = 0;
  int prefixBufferLength = 0;
  while (counter < 4) {
    prefixBufferLength += (multiplier * (buf[counter] - '0'));
    multiplier /= 10;
    counter++;
  }
  printf("Raw client msg: %s\n", buf);
  printf("prefixBufferLength: %d\n", prefixBufferLength);

  if (nbytes > 4 && strlen(buf) > 4) {
    if (buf[4] != ' ') {
      buf[4] = toupper(buf[4]);
    }
    int j = 5;
    for (; j < strlen(buf); j++) {
      if (buf[j - 1] == ' ') {
        buf[j] = toupper(buf[j]);
        continue;
      }
      if (buf[j] != ' ') {
        buf[j] = tolower(buf[j]);
      }
    }
  }
  printf("Client response: ");
  int j = 4;
  for (;j < nbytes; j++){
    printf("%c", buf[j]);
  }
  printf("\n");
}

int main(int argc, char const *argv[]) {
  fd_set master;
  fd_set read_fds;
  int fdmax;

  int listener;
  int newfd;
  struct sockaddr_storage remoteaddr;
  socklen_t addrlen;
  char buf[256];
  int nbytes;

  // allows us to figure out our IP
  socklen_t ourIPLen = sizeof(struct sockaddr);
  struct sockaddr_in our_addr;
  char SERVER_ADDRESS[256];

  char remoteIP[INET6_ADDRSTRLEN];
  int yes=1;
  int i, j, rv;
  struct addrinfo hints, *ai, *p;
  FD_ZERO(&master);
  FD_ZERO(&read_fds);

  // get us a socket and bind it
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  if ((rv = getaddrinfo(NULL, "0", &hints, &ai)) != 0) {
    fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
    exit(1);
  }

  for(p = ai; p != NULL; p = p->ai_next) {
    listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (listener < 0) {
      continue;
    }
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
      close(listener);
      continue;
    }
    break;
  }
  if (getsockname(listener, (struct sockaddr *) &our_addr, &ourIPLen) == -1) {
    perror("Cannot figure out which port I am bounded to");
    exit(1);
  }
  int SERVER_PORT = ntohs(our_addr.sin_port);
  if (gethostname(SERVER_ADDRESS, 255) == -1) {
    perror("Cannot figure out which address I am bounded to");
    exit(1);
  }
  // char* SERVER_ADDRESS = inet_ntoa(our_addr.sin_addr);
  FILE* f = fopen(".serverport", "w");
  if (f != NULL) {
    fprintf(f, "%d", SERVER_PORT);
    fclose(f);
  }
  FILE* fAddr = fopen(".serveraddr", "w");
  if (fAddr != NULL) {
    fprintf(fAddr, "%s", SERVER_ADDRESS);
    fclose(fAddr);
  }

  printf("SERVER_ADDRESS %s\nSERVER_PORT %d\n", SERVER_ADDRESS, SERVER_PORT);

  if (p == NULL) {
    fprintf(stderr, "selectserver: failed to bind\n");
    exit(2);
  }
  freeaddrinfo(ai);

  if (listen(listener, MAX_CLIENT) == -1) {
    perror("listen");
    exit(3);
  }
  FD_SET(listener, &master);
  fdmax = listener;


  // main loop
  while (1) {
    read_fds = master;
    if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
      perror("select");
      exit(4);
    }

    // run through the existing connections looking for data to read
    for(i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) {
        if (i == listener) {
          // handle new connections
          addrlen = sizeof remoteaddr;
          newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);

          if (newfd == -1) {
            perror("accept");
          } else {
            FD_SET(newfd, &master);
            if (newfd > fdmax) {
              fdmax = newfd;
            }
            printf("selectserver: new connection from %s on socket %d\n", inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr*)&remoteaddr), remoteIP, INET6_ADDRSTRLEN), newfd);
          }
        } else {
          // handle data from a client
          if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
            if (nbytes == 0) {
              printf("selectserver: socket %d hung up\n", i);
            } else {
              perror("recv");
            }
            close(i);
            FD_CLR(i, &master);
          } else {
            // we got some data from a client
            if (nbytes < 4) {
              perror("ERROR received less then 4 bytes, cant even tell what the length of the proceeding string is..........");
            }

            modifyBufferToUpperCase(buf, nbytes);

            // doesnt send the whole string potentially, send returns the number of chars it does
            if (write(i, buf, strlen(buf)) == -1) {
              perror("ERROR writing to socket");
            }
          }

        }
      }
    }
  }

  return 0;
}


