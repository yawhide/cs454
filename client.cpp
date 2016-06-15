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

#include <mutex>
#include <queue>
#include <thread>

#define MAX_MSG_LENGTH (256 - 4 - 1) // max number of bytes we can get at once from stdin

struct I {
  char input[MAX_MSG_LENGTH];
};

std::queue<I> buffer;
std::mutex bufferMutex;

void sendMsgToServer(int sock, char* buffer) {
  unsigned int inputLen = strlen(buffer);
  int nBytes;
  char msg[256];
  bzero(msg, 256);
  unsigned int msgLength = htonl(inputLen);
  // printf("input len: %d, msgLength: %d\n", inputLen, msgLength);
  memcpy(msg, &msgLength, 4);
  memcpy(msg+4, buffer, inputLen);
  // printf("msg: '%s'\n", msg+4);

  // printf("final formatted msg: '%s', last char is '\\0': %d\n", buf, buf[tmpBufLen] == '\0');

  if (write(sock, msg, inputLen + 4) == -1) {
    perror("ERROR writing to socket");
  }

  char response[256];
  if ((nBytes = read(sock, response, 255)) == -1) {
    perror("recv");
    exit(1);
  }

  response[nBytes] = '\0';
  printf("Server: %s", response+4);
  // int i = 4;
  // for (; i < nBytes; ++i) {
  //   printf("%c", response[i]);
  // }
}

void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void readInput() {
  // char input[MAX_MSG_LENGTH]; // +1 for null terminator
  // bzero(input, MAX_MSG_LENGTH);
  I i;
  while (fgets(i.input, MAX_MSG_LENGTH, stdin) != NULL) {
    // printf("--- read from stdin: %s ---\n", i.input);
    bufferMutex.lock();
    buffer.push(i);
    bufferMutex.unlock();
  }
  // printf("%s\n", input);
  perror("Failed to get input from user");
}

void sendRequests(int sock) {
  while (1) {
    bufferMutex.lock();
    if (!buffer.empty()) {
      I i = buffer.front();
      // printf("--- first thing in buffer: %s ---\n", i.input);
      buffer.pop();
      bufferMutex.unlock();
      sendMsgToServer(sock, i.input);
      sleep(2);
    } else {
      bufferMutex.unlock();
    }
  }
}

int main(int argc, char *argv[]) {
  char* SERVER_ADDRESS;
  char* SERVER_PORT;
  SERVER_ADDRESS = getenv("SERVER_ADDRESS");
  SERVER_PORT = getenv("SERVER_PORT");

  // printf("%s:%s\n", SERVER_ADDRESS, SERVER_PORT);

  int sockfd;
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

  std::thread t1(readInput);
  std::thread t2(sendRequests, sockfd);

  t1.join();
  t2.join();

  close(sockfd);

  return 0;
}


//TODO
/*
- sleep for 2 seconds before sending each subsequent msg
- allow multiplexing (aka you can get input from user AND send/receive msgs)
- on eof, wait until you receive all the sent requests before exiting
- must support over 256 character strings from stdin
*/

/* done
- 4 byte value of strlen(text) + 1, prepend that to the message we send along

*/
