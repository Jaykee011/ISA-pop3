#include "popcl.h"

void errorHandle (int type){
    switch (type) {
        case ARGERR:
            cerr << "Error: PARAMETER_ERROR\n";
            cout << "\nUsage: popcl <server> [-p <port>] [-T|-S [-c <certfile>] [-C <certaddr>]] [-d] [-n] -a <auth_file> -o <out_dir>\n";
            exit(ARGERR);
            break;
        case HOSTERR:
            cerr << "Error: HOSTNAME_ERROR - INVALID ADDRESS OR HOSTNAME";
            exit(HOSTERR);
        case AUTHERR:
            cerr << "Error: AUTHORIZATION_ERROR - COULD NOT AUTHORIZE";
            exit(AUTHERR);
        case SOCKERR:
            cerr << "Error: SOCKET_ERROR";
            exit(SOCKERR);
        case CONNECTERR:
            cerr << "Error: CONNECT_ERROR - ERROR CONNECTING TO SERVER";
            exit(CONNECTERR);
        case COMERR:
            cerr << "Error: COMMUNICATION_ERROR - ERROR SENDING MESSAGE";
            exit(COMERR);
        case RESPERR:
            cerr << "Error: RESPONSE_ERROR - SERVER RESPONSE ERROR";
            exit(RESPERR);
        case RETRERR:
            cerr << "Error: RETRIEVE_ERROR - MAIL RETRIEVE ERROR";
            exit(RETRERR);
        default:
            exit(-1);
            break;
    }
}

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
        DWORD timeout = 100;
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    #else
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));
    #endif

    return s;
}

string getResponse(int socket){

    char buffer[BUFF_SIZE];
    memset(buffer, '\0', BUFF_SIZE);
    int response;

    if( response = recv(socket , buffer , BUFF_SIZE , 0) < 0){
        errorHandle(RESPERR);        
    }
        
    string s(buffer);

    return s;
}

string retrieveMessage(int socket){
    char buffer[BUFF_SIZE];
    bool found = false;
    int result;
    string s = "";
    smatch match;
    regex rgx("\r\n\\.\r\n");

    do{
        memset(buffer, '\0', BUFF_SIZE);
        result = recv(socket , buffer , BUFF_SIZE , 0);
        if(result < 0){
            if (errno = EAGAIN)
                break;
            errorHandle(RESPERR);
        }
        s.append(buffer);
        if (!found){
            if (regex_search(buffer, rgx))
                found = true;
        }
        //FIXME: SMAZAT COMMENT
       //cout << buffer << endl << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl << result << endl <<  "found:" << found << endl << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl << endl;
    // }while(!regex_search(s, rgx));
    }while(!found || result > 0);

    // not taking the finishing .\r\n as part of message
    s.pop_back();
    s.pop_back();
    s.pop_back();

    // regex response("\\+OK.*?\r\n([\\s\\S]*)");
    // if (regex_search(s, match, response))
    //     s = match[1];

    return s;
}

vector<string> getMessages(int socket, int num, bool del){
    vector<string>  messages;
    string message;
    int result = 0;

    for (int i=1; i <= num; i++){
        message = "RETR "+to_string(i)+"\r\n";

        if (send(socket, message.c_str(), message.length(), 0) < 0)
            errorHandle(COMERR);
        
        //FIXME: NAJDI LEPSI ReSENI!  
        message = retrieveMessage(socket);
        if (message[0] == '-'){
            i--;
            continue;
        }
        // erasing the +OK ... line
        message.erase(0, message.find("\r\n") + 2);
        
        messages.push_back(message);
    }

    return messages;
}

int countMessages(int socket){
    string buffer;
    int count;

    if (send(socket, "STAT\r\n", 7, 0) < 0)
        errorHandle(COMERR);

    buffer = getResponse(socket);

    smatch match;
    regex rgx("\\+OK ([0-9]*) .*");
    if (regex_search(buffer, match, rgx))
        count=stoi(match[1]);
    else 
        count=0; 

    return count;
}

int writeMessages(string out, vector<string> messages){
    int fileIndex = 0;
    ofstream output;
    
    for(auto const& msg: messages){
        fileIndex++;
        output.open(out+'/'+to_string(fileIndex));
        output << msg;
        output.close();
    }

    return 0;
}

int main(int argc, char* argv[])
{
    bool pop3s      = false;
    bool stls       = false;
    bool deleteRead = false;
    bool newOnly    = false;
    int port        = 0;
    int popSocket;
    string certFile;
    
    string certDir;
    string authFile;
    string outDir;
	string host;
	

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
                break;
            case 'S':
                stls = true;
                break;
            case 'c':
                certFile = optarg;
                break;
            case 'C':
                certDir = optarg;
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

    /*
    *   TODO: Illegal option combinations
    */
    //if (stls && pop3s) errorHandle(ARGERR)


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

    if(connect(popSocket, (struct sockaddr *)&sa, sizeof(sa)) < 0 )
        errorHandle(CONNECTERR);

    response = getResponse(popSocket);


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
    regex_search(username, match, rgx);
    username = match[1];
    regex_search(password, match, rgx);
    password = match[1];

    /*
    *   AUTHORIZATION state
    */
    string authString = "USER " + username + "\r\n";
    string passString = "PASS " + password + "\r\n";

    if (send(popSocket, authString.c_str(), authString.length(), 0) < 0)
        errorHandle(COMERR);
    
    response = getResponse(popSocket);
    
    if (send(popSocket, passString.c_str(), passString.length(), 0) < 0)
        errorHandle(COMERR);

    response = getResponse(popSocket);

    /*
    *   TRANSACTION state
    */

    vector<string> messages;
    int messageNum;
    messageNum = countMessages(popSocket);

    if (messageNum){
        messages = getMessages(popSocket, messageNum, deleteRead);
    }
    else{
        cout << "Nejsou k dispozici žádné zprávy ke stažení.";
        //TODO: JESTE -d posefit
        if (send(popSocket, "QUIT\r\n", 7, 0) < 0)
            errorHandle(COMERR);
        return 0;
    }


    /*
    *   UPDATE state
    */

    if (send(popSocket, "QUIT\r\n", 7, 0) < 0)
        errorHandle(COMERR);

    response = getResponse(popSocket);

    
    writeMessages(outDir, messages);

	return 0;
}