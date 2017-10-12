#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <iostream>

#include <sys/stat.h>
#include <dirent.h>

#define BUFF_SIZE			8192 //velikost bufferu

#define ARGERR              1
#define HOSTERR             2
#define SOCKERR             3

using namespace std;