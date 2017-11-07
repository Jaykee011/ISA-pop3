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

#define ARGERR              1   //  Command-line argument error
#define HOSTERR             2   //  Invalid hostname or address
#define SOCKERR             3   //  Error while performing socket operation
#define CONNECTERR          4   //  Error while connecting to host
#define AUTHERR             5   //  Error during authorization or invalid auth file
#define COMERR              7   //  Send Error
#define RESPERR             8   //  Recv or Read Error
#define RETRERR             9   //  Error while retrieving messages
#define DIRERR              10  //  Error while creating directory
#define CERTERR             11  //  Error while validating certificates
#define INITERR             12  //  SSL initialization error

using namespace std;