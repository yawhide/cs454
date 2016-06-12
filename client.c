#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAXDATASIZE 255 // max number of bytes we can get at once

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
  char* SERVER_ADDRESS;
  char* SERVER_PORT;
  SERVER_ADDRESS = getenv("SERVER_ADDRESS");
  SERVER_PORT = getenv("SERVER_PORT");

  int sockfd, numbytes;
  struct addrinfo hints, *servinfo, *p;
  int rv;
  char ipstr[INET6_ADDRSTRLEN];

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(SERVER_ADDRESS, SERVER_PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // loop through all the results and connect to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("client: socket");
      continue;
    }

    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("client: connect");
      exit(1);
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "client: failed to connect\n");
    return 2;
  }

  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), ipstr, sizeof ipstr);
  // printf("client: connecting to %s\n", ipstr);

  freeaddrinfo(servinfo); // all done with this structure

  while (1) {
    char buf[MAXDATASIZE];
    bzero(buf, MAXDATASIZE);
    if (fgets(buf,255,stdin) != NULL) {
      int tmpBufLen = strlen(buf);
      // gets rid of the newline char
      if (buf[tmpBufLen - 1] == '\n') {
        buf[tmpBufLen - 1] = '\0';
      }
      tmpBufLen = strlen(buf) + 1;
      // printf("tmp length of buf w/o newline: %d\n", tmpBufLen);
      char msgPrefix[4];
      // convert strlen(buf) + 1 to string
      int len = tmpBufLen;
      int divider = 1000;
      int counter = 0;
      while (counter < 4) {
        int tmpNum = len / divider;
        if (tmpNum != 0) {
          len -= (tmpNum * divider);
        }
        msgPrefix[counter] = tmpNum + '0';
        divider /= 10;
        counter++;
      }

      // shift buf to the right 4 indicies to make room for size of msg
      for (int i = tmpBufLen - 1; i >= 0; i--) {
        buf[i+4] = buf[i];
      }
      for (int i = 0; i < 4; i++) {
        buf[i] = msgPrefix[i];
      }
      tmpBufLen += 4;
      // printf("final formatted msg: '%s', last char is '\\0': %d\n", buf, buf[tmpBufLen] == '\0');

      if (write(sockfd, buf, tmpBufLen) == -1) {
        perror("ERROR writing to socket");
      }

      if ((numbytes = read(sockfd, buf, MAXDATASIZE-1)) == -1) {
        perror("recv");
        exit(1);
      }

      buf[numbytes] = '\0';
      printf("Server: ");
      for (int i = 4; i < numbytes; ++i) {
        printf("%c", buf[i]);
      }
      printf("\n");
      sleep(2);
    }
  }

  close(sockfd);

  return 0;
}


//TODO
/*
- sleep for 2 seconds before sending each subsequent msg
- allow multiplexing (aka you can get input from user AND send/receive msgs)
- on eof, wait until you receive all the sent requests before exiting
- ?????? unsure what to do if message is > 251 chars ???????
- what is the point of having the string length at the beginning of the message
*/

/* done
- 4 byte value of strlen(text) + 1, prepend that to the message we send along

*/
