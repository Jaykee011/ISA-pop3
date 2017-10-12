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
        case SOCKERR:
            cerr << "Error: SOCKET_ERROR";
            exit(SOCKERR);
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

    return s;
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

    if (argc - optind != 1) errorHandle(ARGERR);
    else host = argv[optind];

    /*
    *   TODO: Illegal option combinations
    */
    //if (stls && pop3s) errorHandle(ARGERR)

    sockaddr_in sa;
    sockaddr_in6 sa6;
    int IPv;


    /*
    *   Resolving Hostname or IP address
    */
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
    
    // char str[INET_ADDRSTRLEN];
    // int dom = AF_INET;  

    // inet_ntop(dom,&(sa.sin_addr), str, INET_ADDRSTRLEN);
    // cout << str << endl;
    
    // cout << "port:" << port << endl << "pop3s:" << pop3s << endl << "stls:" << stls << endl <<  "certfile:" << certFile << endl << "certdir:" << certDir << endl << "deleteRead:" << deleteRead << endl << "newonly" << newOnly << endl << "authFile:" << authFile << endl << "outDir:" << outDir << endl << "host: " << host << endl;

    popSocket = socketInit(IPv);



	return 0;
}