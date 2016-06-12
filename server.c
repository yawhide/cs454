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

#define BACKLOG 20

#define MAXDATASIZE 256

#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT "8000"

void sigchld_handler(int s)
{
  // waitpid() might overwrite errno, so we save and restore it:
  int saved_errno = errno;

  while(waitpid(-1, NULL, WNOHANG) > 0);

  errno = saved_errno;
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
      return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// string upperCase(string input)
// {
//   stringstream ss;
//   ss << input;
//   int length;
//   string word, output;
//   ss >> length;
//   cout << "length of proceding string is: " << length << endl;
//   while (ss >> word)
//   {
//     //output += word.substring(0, 1);
//     cout << word << endl;
//     word[0] = toupper(word[0]);
//     output += word;
//     output += " ";
//   }
//   int outputLength = output.size();
//   if (outputLength > 0 && output[outputLength - 1] == ' ')
//   {
//     output.resize(outputLength - 1);
//   }
//   return output;
// }

int main(int argc, char const *argv[])
{
  printf("SERVER_ADDRESS %s\nSERVER_PORT %s\n", SERVER_ADDRESS, SERVER_PORT);

  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; // connector's address information
  socklen_t sin_size;
  struct sigaction sa;
  char ipstr[INET6_ADDRSTRLEN];
  int status;
  int yes = 1;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((status = getaddrinfo(SERVER_ADDRESS, SERVER_PORT, &hints, &servinfo)) != 0)
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return 1;
  }

  for(p = servinfo;p != NULL; p = p->ai_next)
  {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
    {
      perror("server: socket");
      continue;
    }

    // reuse port if I was using it
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
      perror("setsockopt");
      exit(1);
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
    {
      close(sockfd);
      perror("server: bind");
      continue;
    }

    break;
  }

  freeaddrinfo(servinfo);

  if (p == NULL)
  {
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }

  if (listen(sockfd, BACKLOG) == -1)
  {
    perror("listen");
    exit(1);
  }

  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1)
  {
    perror("sigaction");
    exit(1);
  }

  printf("server: waiting for connections...\n");

  while(1) {  // main accept() loop
    sin_size = sizeof their_addr;
    int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1)
    {
      perror("accept");
      continue;
    }

    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), ipstr, sizeof ipstr);
    printf("server: got connection from %s\n", ipstr);

    if (!fork())
    { // this is the child process
      close(sockfd); // child doesn't need the listener
      char buf[MAXDATASIZE];
      bzero(buf, MAXDATASIZE);

      int numbytes = read(new_fd, buf, MAXDATASIZE);
      if (numbytes == -1)
      {
        perror("ERROR reading from socket");
      }

      buf[numbytes] = '\0';
      printf("client: received '%s'\n", buf);

      if (numbytes > 0 && strlen(buf) > 0)
      {
        if (buf[0] != ' ')
        {
          buf[0] = toupper(buf[0]);
        }
        for (int i = 1; i < strlen(buf); i++)
        {
          if (buf[i - 1] == ' ')
          {
            buf[i] = toupper(buf[i]);
            continue;
          }
          if (buf[i] != ' ')
          {
            buf[i] = tolower(buf[i]);
          }
        }
      }

      if (write(new_fd, buf, strlen(buf)) == -1) // doesnt send the whole string potentially, send returns the number of chars it does
      {
        perror("ERROR writing to socket");
      }
      close(new_fd);
      exit(0);
    }
    close(new_fd);  // parent doesn't need this
  }

  // string input;
  // while(true)
  // {
  //   getline(cin, input);
  //   cout << "transformed:" << upperCase(input) << endl;
  // }

  return 0;
}


