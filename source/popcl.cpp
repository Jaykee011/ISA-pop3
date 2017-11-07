#include "popcl.h"

/*
*   Error handle function - writes error to stderr and exits with appropriate error code. (codes define in header)
*/
void errorHandle (int type){
    switch (type) {
        case ARGERR:
            cerr << "Error: PARAMETER_ERROR\n";
            cout << "\nUsage: popcl <server> [-p <port>] [-T|-S [-c <certfile>] [-C <certaddr>]] [-d] [-n] -a <auth_file> -o <out_dir>\n";
            exit(ARGERR);
            break;
        case HOSTERR:
            cerr << "Error: HOSTNAME_ERROR - INVALID ADDRESS OR HOSTNAME\n";
            exit(HOSTERR);
        case AUTHERR:
            cerr << "Error: AUTHORIZATION_ERROR - COULD NOT AUTHORIZE EITHER BECAUSE OF INVALID AUTH FILE OR WRONG AUTH INFO\n";
            exit(AUTHERR);
        case SOCKERR:
            cerr << "Error: SOCKET_ERROR - COULD NOT CREATE SOCKET\n";
            exit(SOCKERR);
        case CONNECTERR:
            cerr << "Error: CONNECT_ERROR - ERROR CONNECTING TO SERVER\n";
            exit(CONNECTERR);
        case COMERR:
            cerr << "Error: COMMUNICATION_ERROR - ERROR SENDING MESSAGE\n";
            exit(COMERR);
        case RESPERR:
            cerr << "Error: RESPONSE_ERROR - SERVER RESPONSE ERROR\n";
            exit(errno);
        case RETRERR:
            cerr << "Error: RETRIEVE_ERROR - MAIL RETRIEVE ERROR\n";
            exit(RETRERR);
        case DIRERR:
            cerr << "Error: DIRECTORY_ERROR - COULD NOT CREATE OUTPUT DIRECTORY\n";
            exit(errno);            
        case CERTERR:
            cerr << "Error: CERTIFICATE ERROR - CERTIFICATE NOT VERIFIED, NOT PROVIDED OR INVALID CERTIFICATE PROVIDED\n";
            exit (CERTERR);
        case INITERR:
            cerr << "Error: INITIALIZATION ERROR - ERROR HAPPENNED DURING SSL INITIALIZATION\n";
            exit(INITERR);
        case STLSERR:
            cerr << "Error: STLS ERROR - THERE WAS AN ERROR WHILE ESTABLISHING SSL USING STLS\n";
            exit(STLSERR);
        default:
            exit(-1);
            break;
    }
}

/*
*   Initializing socket and setting timeout
*/
int socketInit(int IPv){
    
    int s;

    if (IPv == AF_INET){
        if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            errorHandle(SOCKERR);
    }
    else{
        if ((s = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
            errorHandle(SOCKERR);
    }

    /*
    *   Setting timeout for recv - platform specific
    *   Source:
    *   https://stackoverflow.com/questions/2876024/linux-is-there-a-read-or-recv-from-socket-with-timeout
    *   author: Robert S. Barnes, further edited by  jjxtra
    */
    #if defined(_WIN32) || defined(_WIN64)
        DWORD timeout = 200;
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    #else
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 200000;
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));
    #endif

    return s;
}

/*
*   Creating output directory or seeing if it exists
*/
void createDir(string out){
    errno = 0;
    if (mkdir(out.c_str(), ACCESSPERMS)){
        if (errno == EEXIST){
            struct stat s;
            if (stat(out.c_str(), &s) == 0){
                if (s.st_mode & S_IFDIR){
                    return;
                }
            }
        }
        errorHandle(DIRERR);
    }
}

/*
*   Function gets and returns server response to command
*/
string getResponse(int socket, bool secure, SSL *ssl){

    char buffer[BUFF_SIZE];
    int bytes;
    string response;

    if (secure) {
        do{
            memset(buffer, '\0', BUFF_SIZE);
            bytes = SSL_read(ssl, buffer, BUFF_SIZE); /* get reply & decrypt */
            response += buffer;
        }while(SSL_get_error(ssl, bytes) == SSL_ERROR_WANT_READ);
    }
    else{
        do {
            memset(buffer, '\0', BUFF_SIZE);
            errno = 0;
            if( bytes = recv(socket , buffer , BUFF_SIZE , 0) < 0){
                if (errno = EAGAIN)
                    continue;
                errorHandle(RESPERR);        
            }
            response += buffer;
        }while(bytes > 0);
    }

    return response;
}

/*
*   Retrieves multi-line response from server and returns it
*/
string retrieveMessage(int socket, bool secure, SSL *ssl){
    char buffer[BUFF_SIZE];
    int bytes;
    bool found = false;
    int result;
    string s = "";
    smatch match;
    regex rgx("\r\n\\.\r\n");

    do{
        memset(buffer, '\0', BUFF_SIZE);
        errno = 0;
        if (secure){
            bytes = SSL_read(ssl, buffer, BUFF_SIZE);
            result = SSL_get_error(ssl, bytes);
        }
        else{
            result = recv(socket , buffer , BUFF_SIZE , 0);
            if(result < 0){
                if (errno == EAGAIN)
                break;
                errorHandle(RESPERR);
            }
        }
        s.append(buffer);
        if (!found){
            if (regex_search(buffer, rgx))
                found = true;
        }
    }while(!found || result > 0);

    return s;
}

/*
*   Retrieves unique message-id from downloaded message.
*/
string getMsgID(string msg){
    int i_begin;
    int i_end;
    bool in = false;
    string line;
    string id = "";
    regex rgx("message-?id: ?<", regex_constants::icase);

    std::istringstream message(msg);

    while (getline(message, line)){
        if (in || regex_search(line, rgx)){
            if (!in){
                i_begin = line.find('<');
                i_end = line.find('>');

                if (i_end == -1){
                    i_end = line.find("\r\n");
                    in = true;
                }

                id = line.substr(i_begin+1, i_end-(i_begin+1));
                if (!in)
                    break;
            }
            else{
                i_end = line.find('>');
                if (i_end == -1){
                    i_end = line.find("\r\n");
                }

                id += line.substr(0, i_end);
            }
        }
    }

    return id;
}

/*
*   Sees if provided message-id is already in the downloaded message database. 
*   If it is a new message writes it's ID to the database.
*   Returns if the message was already downloaded or not.
*/
bool idHandle(string id){
    fstream dbFile;
    bool found = false;
    string line;

    dbFile.open("db.iddb", fstream::in | fstream::out);

    while(getline(dbFile, line)){
        if (line.find(id) != -1){
            found = true;
            break;
        }
    }
    dbFile.close();

    dbFile.open("db.iddb", ios::out | ios::app);

    if (!found){
        dbFile << id << endl;
    }

    dbFile.close();

    return !found;
}

/*
*   Generates a message file and writes the provided message into it
*/
void writeMessage(string out, string msg){
    ofstream output;
    struct stat s;
    int index = 1;
    string file;

    while (1) {
        file = out + to_string(index);
        if (stat (file.c_str(), &s) == 0){
            index++;
            continue;
        }

        output.open(out+to_string(index));
        output << msg;
        output.close();

        break;
    }
}

/*
*   Sends a delete command for provided message UID.
*/
void deleteMessage(int i, int socket, bool secure, SSL *ssl){
    string message = "DELE "+to_string(i)+"\r\n";

    if (secure){
        if (SSL_write(ssl, message.c_str(), message.length()) < 0)
            errorHandle(COMERR);
    }
    else{
        if (send(socket, message.c_str(), 8, 0) < 0)
        errorHandle(COMERR);
    }
        
    getResponse(socket, secure, ssl);
}

/*
*   Retrieves all/new messages from the mail server and writes them to file.
*   ID handling, deleting is handled from this function. 
*/
void getMessages(int socket, int num, bool del, string out, bool newOnly, bool deleteRead, bool secure, SSL *ssl){
    vector<string>  messages;
    string message;
    int result = 0;
    int written = 0;
    string id;
    bool newMsg = false;

    // Retrieving and handling messages
    for (int i=1; i <= num; i++){
        message = "RETR "+to_string(i)+"\r\n";

        if (secure){
            if (SSL_write(ssl, message.c_str(), message.length()) < 0)
                errorHandle(COMERR);
        }
        else{
            if (send(socket, message.c_str(), message.length(), 0) < 0)
                errorHandle(COMERR);
        }

        message = retrieveMessage(socket, secure, ssl);
        if (message[0] == '-'){
            i--;
            continue;
        }

        // erasing the +OK ... line
        message.erase(0, message.find("\r\n") + 2);
        // erasing the finishing .\r\n
        int index = message.find("\r\n.\r\n");
        message.erase(index+2, index+5);
        
        //handling ID
        id = getMsgID(message);
        if (id.empty())
            newMsg = true;
        else
            newMsg = idHandle(id);

        //writing to file and sending delete commands if requested
        if (newOnly){
            if (newMsg){
                written++;
                writeMessage(out, message);
                if (deleteRead)
                    deleteMessage(i, socket, secure, ssl);
            }
        }
        else{
            writeMessage(out, message);
            written++;
            if (deleteRead)
                deleteMessage(i, socket, secure, ssl);
        }
    }

    if (newOnly && !written)
        cout << "Nejsou k dispozici žádné nové zprávy ke stažení." << endl;
    else
        cout << "Staženo " << written << " zpráv." << endl;
}

/*
*   Handles authentication process
*/
void authenticate(int socket, bool secure, SSL *ssl, string username, string password){

    string resp;

    if (secure){
        if (SSL_write(ssl, username.c_str(), username.length()) < 0)
            errorHandle(COMERR);   /* encrypt & send message */
        resp = getResponse(socket, secure, ssl);

        if (SSL_write(ssl, password.c_str(), password.length()) < 0)   /* encrypt & send message */
            errorHandle(COMERR);
        resp = getResponse(socket, secure, ssl);
    }
    else{
        if (send(socket, username.c_str(), username.length(), 0) < 0)
            errorHandle(COMERR);
        resp = getResponse(socket, secure, ssl);
    
        if (send(socket, password.c_str(), password.length(), 0) < 0)
            errorHandle(COMERR);
        resp = getResponse(socket, secure, ssl);
    }

    if (resp[0] == '-')
        errorHandle(AUTHERR);
}

/*
*   Returns number of messages at the server
*/
int countMessages(int socket, bool secure, SSL *ssl){
    string buffer;
    int count;

    string command = "STAT\r\n";

    if (secure){
        if (SSL_write(ssl, command.c_str(), command.length()) < 0)
            errorHandle(COMERR);
    }
    else{
        if (send(socket, command.c_str(), command.length(), 0) < 0)
            errorHandle(COMERR);
    }
    
    buffer = getResponse(socket, secure, ssl);

    smatch match;
    regex rgx("\\+OK ([0-9]*) .*");
    if (regex_search(buffer, match, rgx))
        count=stoi(match[1]);
    else 
        count=0; 

    return count;
}

/*
*   Sends QUIT command
*/
void endCommunication(int socket, bool secure, SSL *ssl){
    string message = "QUIT\r\n";
    
    if (secure){
        if (SSL_write(ssl, message.c_str(), message.length()) < 0)
            errorHandle(COMERR);
    }
    else{
        if (send(socket, message.c_str(), message.length(), 0) < 0)
            errorHandle(COMERR);
    }
}
/*
*   Initiating SSL context
*   Source:
*   http://www.cs.utah.edu/~swalton/listings/sockets/programs/part4/chap16/ssl_client.c
*   author: Sean Walton, School of Computing, university of Utah
*/
SSL_CTX* InitCTX(void)
{   SSL_METHOD *method;
    SSL_CTX *ctx;
 
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */
    ctx = SSL_CTX_new(TLSv1_2_client_method());   /* Create new context */

    if ( ctx == NULL )
        errorHandle(INITERR);

    return ctx;
}

/*
*   Initiating SSL communication.
*   Loading certificates and verifing certificates provided by server.
*/
void sslInitiate(SSL* ssl, SSL_CTX *ctx, bool certProvided, string certDir, string certFile)
{   X509 *cert;
    int ret;

    if (!certProvided)
        SSL_CTX_set_default_verify_paths(ctx);
    else{
        const char *cd = NULL;
        const char *cf = NULL;
        if (!certDir.empty())
            cd = certDir.c_str();
        if (!certFile.empty())
            cf = certFile.c_str();

        SSL_CTX_load_verify_locations(ctx, cf, cd);
    }

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL); 

    do{
        if ( SSL_connect(ssl) == -1 )   /* perform the connection */
            if (errno != EAGAIN)
                errorHandle(CONNECTERR);
    }while(errno == EAGAIN);
 
    cert = SSL_get_peer_certificate(ssl); /* get the server's certificate */
    if ( cert != NULL )
    {
        if ((ret = SSL_get_verify_result(ssl)) != X509_V_OK){
            X509_free(cert);
            errorHandle(CERTERR);
        }

        X509_free(cert);
    }
    else{
        X509_free(cert);
        errorHandle(CERTERR);
    }
}       

int main(int argc, char* argv[])
{
    bool pop3s          = false;
    bool stls           = false;
    bool deleteRead     = false;
    bool newOnly        = false;
    bool secure         = false;
    bool certProvided   = false;
    int port            = 0;
    int popSocket;
    SSL_CTX *ctx        = NULL;
    SSL *ssl            = NULL;

    string certFile = "";
    string certDir = "";
    string authFile;
    string outDir;
	string host;
	

    /*
    *   Command-line argument handling using getopt
    */
	if (argc < 4) errorHandle(ARGERR);
    
    int c;
    opterr = 0;

    while ((c = getopt(argc, argv, "p:TSc:C:dna:o:")) != -1){
        switch (c) {
            case 'p':
                // port = atoi(optarg);
                char *err;
                port = strtol(optarg, &err, 10);
                if (*err) errorHandle(ARGERR);
                break;
            case 'T':
                pop3s = true;
                secure = true;
                break;
            case 'S':
                stls = true;
                break;
            case 'c':
                certFile = optarg;
                certProvided = true;
                break;
            case 'C':
                certDir = optarg;
                certProvided = true;
                break;
            case 'd':
                deleteRead = true;
                break;
            case 'n':
                newOnly = true;
                break;
            case 'a':
                authFile = optarg;
                break;
            case 'o':
                outDir = optarg;
                break;   
            case '?':
                errorHandle(ARGERR);
                break;                      
        }
    }

    if (!port) 
        pop3s ? port=995 : port=110;

    if (outDir.empty() || authFile.empty())
        errorHandle(ARGERR);

    if (argc - optind != 1) 
        errorHandle(ARGERR);
    else 
        host = argv[optind];
    
    if (outDir.back() != '/')
        outDir.push_back('/');

    if (stls && pop3s) 
        errorHandle(ARGERR);

    // preparing the output directory
    createDir(outDir);

    /*
    *   Preparing authorization strings
    */
    ifstream authInput;
    string username;
    string password;

    authInput.open(authFile);
    getline(authInput, username);
    getline(authInput, password);
    authInput.close();

    smatch match;
    regex rgx("(?:username|password) = (.*)");
    if (!regex_search(username, match, rgx))
        errorHandle(AUTHERR);
    username = match[1];
    if (!regex_search(password, match, rgx))
        errorHandle(AUTHERR);
    password = match[1];

    /*
    *   Resolving Hostname or IP address
    */    
    sockaddr_in sa;
    sockaddr_in6 sa6;
    int IPv;
    if (inet_pton(AF_INET, host.c_str(), &(sa.sin_addr)) == 1 )
        IPv = AF_INET;
    else if (inet_pton(AF_INET6, host.c_str(), &(sa6.sin6_addr)) == 1)
        IPv = AF_INET6;
    else {
        hostent *record = gethostbyname(host.c_str());
        if (!record)
            errorHandle(HOSTERR);
        in_addr * address = (in_addr * )record->h_addr;
        sa.sin_addr = *address;
            // sa.sin_addr = record->h_addr;
        IPv = AF_INET;
    }   

    if (IPv == AF_INET){ 
        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);
    }
    else {
        sa6.sin6_family = AF_INET6;
        sa6.sin6_port = htons(port);
    }

    /*
    *   Connecting to remote host
    */
    popSocket = socketInit(IPv);
    string response;

    errno = 0;
    if (IPv == AF_INET){ 
        do{
            if(connect(popSocket, (struct sockaddr *)&sa, sizeof(sa)) < 0 )
                if (errno != EAGAIN)    
                    errorHandle(CONNECTERR);
        }while(errno == EAGAIN);
    }
    else {    
        do{
            if(connect(popSocket, (struct sockaddr *)&sa6, sizeof(sa6)) < 0 )
                if (errno != EAGAIN)
                    errorHandle(CONNECTERR);
        }while(errno == EAGAIN);
    }

    /*
    *   Initiating TLS
    */
    if (stls){
        getResponse(popSocket, secure, ssl);
        string initiation = "STLS\r\n";

        if (send(popSocket, initiation.c_str(), initiation.length(), 0) < 0)
            errorHandle(COMERR);
        response = getResponse(popSocket, secure, ssl);
        if (response[0] == '-')
            errorHandle(STLSERR);
        secure = true;
    }

    /*
    *   Initializing and initiating SSL
    */
    if (secure){
        SSL_library_init();
        ctx = InitCTX();
        ssl = SSL_new(ctx);      /* create new SSL connection state */
        SSL_set_fd(ssl, popSocket);    /* attach the socket descriptor */
        sslInitiate(ssl, ctx, certProvided, certDir, certFile); /* initiate connection and check certificates */
    }
    
    if (!stls)
        response = getResponse(popSocket, secure, ssl);

    /*
    *   AUTHORIZATION state
    */
    string authString = "USER " + username + "\r\n";
    string passString = "PASS " + password + "\r\n";

    authenticate(popSocket, secure, ssl, authString, passString);

    /*
    *   TRANSACTION state
    */

    int messageAmount;
    messageAmount = countMessages(popSocket, secure, ssl);

    if (messageAmount){
        getMessages(popSocket, messageAmount, deleteRead, outDir, newOnly, deleteRead, secure, ssl);
    }
    else{
        cout << "Nejsou k dispozici žádné zprávy ke stažení." << endl;
        endCommunication(popSocket, secure, ssl);
        return 0;
    }

    /*
    *   UPDATE state
    */

    endCommunication(popSocket, secure, ssl);
    response = getResponse(popSocket, secure, ssl);


    if (secure){
        SSL_CTX_free(ctx);        /* release context */
        SSL_free(ssl);            /* free SSL */
    }

    close(popSocket);

	return 0;
}