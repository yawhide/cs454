#import <iostream>
#include <stdlib.h>

using namespace std;

int main(int argc, char const *argv[])
{
  char* SERVER_ADDRESS;
  char* SERVER_PORT;
  SERVER_ADDRESS = getenv("SERVER_ADDRESS");
  SERVER_PORT = getenv("SERVER_PORT");
  if (SERVER_ADDRESS == NULL)
  {
    cout << "SERVER_ADDRESS not provided in environment variables" << endl;
    return 1;
  }
  if (SERVER_PORT == NULL)
  {
    cout << "SERVER_PORT not provided in environment variables" << endl;
    return 1;
  }
  cout << "SERVER_ADDRESS " << SERVER_ADDRESS << endl;
  cout << "SERVER_PORT " << SERVER_PORT << endl;

  return 0;
}