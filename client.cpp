/*
* Name: Wesley Tu
* Date: 4/14/24 
* Title: Programming Assignment 1
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <string>
#include <string.h>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <boost/filesystem.hpp>
#include <unistd.h>
#include <limits.h>

void downloadCommand(std::string userObject, int sock_fd, std::string serverName) {
  size_t slashPos = userObject.find("/");
  if (slashPos == std::string::npos) {
    printf("user/object not in correct format!\n");
    return;
  }
  std::string user = userObject.substr(0, slashPos);
  std::string object = userObject.substr(slashPos+1);

  std::string currentDir = boost::filesystem::current_path().string();
  char username[LOGIN_NAME_MAX];
  getlogin_r(username, LOGIN_NAME_MAX);

  char buf[100];
  std::fill(buf, buf+100, 0);
  std::string message;
  recv(sock_fd, buf, 100, 0);
  for (char ch: buf) {
    if (ch != '\0')
      message += ch;
  }

  if (message == "The file does not exist in any hard drive!") {
    std::cout << message << std::endl;
    return;
  }
  else 
    std::cout << message << std::endl;
  
  recv(sock_fd, buf, 100, 0);
  std::fill(buf, buf+100, 0);
  printf(buf);
  printf("\n");

  system(("scp -o StrictHostKeyChecking=no " + std::string(username) + "@" + serverName + ":/tmp/tmpfiles/" + object + " " + currentDir).c_str());
  message = "Download file tranfered over to client!";
  send(sock_fd, message.c_str(), message.length(), 0);
  std::cout << message << std::endl;
  std::cout << "\nFile Contents: " << std::endl;
  system(("cat " + object).c_str());
  std::cout << std::endl;
}

void listCommand(std::string user, int sock_fd, std::string serverName) {
  char buf[100];
  std::fill(buf, buf+100, 0);
  recv(sock_fd, buf, 100, 0);
  printf(buf);
  printf("\n");

  std::string currentDir = boost::filesystem::current_path().string();
  //std::cout << "Current directory is: " << currentDir << "\n";

  char username[LOGIN_NAME_MAX];
  getlogin_r(username, LOGIN_NAME_MAX);

  system(("scp -o StrictHostKeyChecking=no " + std::string(username) + "@" + serverName + ":/tmp/tmpfiles/output.txt " + currentDir).c_str());

  std::string input = "Output transfered over to client!";
  std::cout << input << std::endl;
  send(sock_fd, input.c_str(), input.length(), 0);

  std::cout << "\nOutput of ls -lrt: " << std::endl;
  system(("cat " + currentDir + "/output.txt").c_str());
  system(("rm -f " + currentDir + "/output.txt").c_str());
}

void uploadCommand(std::string userObject, int sock_fd, std::string serverName) {
  size_t slashPos = userObject.find("/");
  if (slashPos == std::string::npos) {
    printf("user/object not in correct format!\n");
    return;
  }
  std::string user = userObject.substr(0, slashPos);
  std::string object = userObject.substr(slashPos+1);

  std::string currentDir = boost::filesystem::current_path().string();
  //std::cout << "Current directory is: " << currentDir << "\n";

  char username[LOGIN_NAME_MAX];
  getlogin_r(username, LOGIN_NAME_MAX);

  system(("scp -o StrictHostKeyChecking=no " + currentDir + "/" + object + " " + username + "@" + serverName + ":/tmp/tmpfiles").c_str());

  std::string input = "Upload file transfered over!";
  std::cout << input << std::endl;
  send(sock_fd, input.c_str(), input.length(), 0);

  char buf[100];
  std::fill(buf, buf+100, 0);
  recv(sock_fd, buf, 100, 0);
  printf(buf);
  printf("\n");
}

void deleteCommand(std::string userObject, int sock_fd) {
  char buf[100];
  std::fill(buf, buf+100, 0);
  std::string message;
  recv(sock_fd, buf, 100, 0);
  for (char ch: buf) {
    if (ch != '\0')
      message += ch;
  }

  if (message == "The file does not exist in any hard drive!") {
    std::cout << message << std::endl;
    return;
  }
  else 
    std::cout << message << std::endl;

  std::fill(buf, buf+100, 0);
  recv(sock_fd, buf, 100, 0);
  printf(buf);
  printf("\n");
}

void addCommand(std::string disk, int sock_fd) {
  char buf[1000];
  std::fill(buf, buf+1000, 0);
  recv(sock_fd, buf, 1000, 0);
  printf(buf);
  printf("\n");
}

void removeCommand(std::string disk, int sock_fd) {
  char buf[1000];
  std::fill(buf, buf+1000, 0);
  recv(sock_fd, buf, 1000, 0);
  printf(buf);
  printf("\n");
}

int main(int argc, char *argv[]){
  //Get from the command line, server IP, src and dst files.
  if (argc != 3){
		printf ("Usage: %s <ip of server> <port #>\n",argv[0]);
		exit(0);
  } 
  //Declare socket file descriptor and buffer
  int sockfd;

  //Declare server address to accept
  struct sockaddr_in serveraddr;

  //Declare host
  struct hostent *host;

  //get hostname
  host = (struct hostent *)gethostbyname(argv[1]);

  std::string serverName(argv[1]);

  //Open a socket, if successful, returns
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("Error: Cannot create socket.\n");
    return 0;
  }
  /*
  const char* directoryPath = "/tmp/wtu/user";
  struct stat sb;

  if (stat(directoryPath, &sb) != 0) {
    std::cout << "Directory for hard drive does not exist!" << std::endl;
    system("mkdir -p /tmp/wtu/user");
  } 
  else {
    std::cout << "Directory for hard drive does exist!" << std::endl;
  }
  */

  //Set the server address to send using socket addressing structure
  serveraddr.sin_addr = *((struct in_addr *)host->h_addr);
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_port = htons(atoi(argv[2]));
  
  printf("Connecting to server...\n");
  //Connect to the server
  if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr)) < 0) {
      printf("Cannot connect.\n");
      exit(0);
    }
  printf("Connected.\n");
  //Client begins to write and read from the server
  std::string input;
  std::cout << "Please input the command you want. >> ";
  getline(std::cin, input);

  //std::cout << input << std::endl;

  std::string token;
  size_t spacePos;
  std::vector<std::string> tokens;

  send(sockfd, input.c_str(), input.length(), 0);
  
  while(input.size() != 0)
  {
      spacePos = input.find(" ");
      if(spacePos != std::string::npos)
      {
          token = input.substr(0, spacePos );
          input = input.substr(spacePos  + 1);
      }
      else
      {
          token = input;
          input = "";
      }

      // do token stuff
      tokens.push_back(token);
  }

  std::string cmd = tokens[0];
  try {
    if (cmd == "download") {
      downloadCommand(tokens[1], sockfd, serverName);
    }
    else if (cmd == "list") {
      listCommand(tokens[1], sockfd, serverName);
    }
    else if (cmd == "upload") {
      uploadCommand(tokens[1], sockfd, serverName);
    }
    else if (cmd == "delete") {
      deleteCommand(tokens[1], sockfd);
    }
    else if (cmd == "add") {
      addCommand(tokens[1], sockfd);
    }
    else if (cmd == "remove") {
      removeCommand(tokens[1], sockfd);
    }
    else {
      printf("This is not a valid command. Please try again.\n");
    }
  }
  catch (...) {
    std::cout << "Error! Please try again." << std::endl;
  }

  //printf("Client finished.");
  //Close socket descriptor
  close(sockfd);
  return 0;

}