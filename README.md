# Popcl

This is a project for Network Applications class at Brno University of Technology, Faculty of Information Technology.

This is a simple pop3(s) client. SSL/TLS is implemented using the openssl library without the usage of BIO structures as per assignment.

## Description 
  *   Popcl is a simple pop3 client written in c++ using BSD sockets. It reads mail from server using pop3 or pop3 over SSL.
  * STLS is supported.
  * Popcl allows deleting the mail on server and downloading new mail only, through the usage of local message-id database.

## Usage
  * popcl \<host> [-p \<port>] [-T|-S [-c \<certFile>] [-C \<certDir>]] [-d] [-n] -a \<auth_file> -o \<out_dir>
  * host - IPv4/IPv6/hostname of mail server
  * port - Server port number to which the program will try to connect. Implicit 110 for pop3, 995 for pop3s
  * T - Using SSL (cannot be used with -S)
  * S - Using STLS to initiate SSL (cannot be used with -T)   
  * certFile - File with trusted certificates in .pem format
  * certDir - Directory with trusted certificates
  * d - Delete downloaded messages
  * n - Download only new messages
  * a - Plaintext file with username and password (see AuthFile section for format)
  * o - Directory to be used to save messages. Is created if it does not exists (does not create whole path tree)

## AuthFile
  * Authorization file should have this format:

    username = user

    password = pass
