#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <resolv.h>
#include <malloc.h>
#include <cerrno>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <vector>
#include <algorithm>

#include <sys/stat.h>
#include <dirent.h>

#define BUFF_SIZE			1024//velikost bufferu

#define FINE                0

#define ARGERR              1
#define HOSTERR             2
#define SOCKERR             3
#define CONNECTERR          4
#define AUTHERR             5
#define FILEERR             6
#define COMERR              7
#define RESPERR             8
#define RETRERR             9
#define DIRERR              10
#define CERTERR             11

using namespace std;