//
// Created by Frank Steiler on 4/2/17 as part of NOOBS4IoT (https://github.com/steilerDev/NOOBS4IoT)
//
// WebServer.h:
//      A simple HTTP/1.1 Web Server implementation, providing callback functions on multiple routes.
//      The server is only accepting a single request at a time and blocks until the callback returns.
//
// This file is licensed under a GNU General Public License v3.0 (c) Frank Steiler.
// See https://raw.githubusercontent.com/steilerDev/NOOBS4IoT/master/LICENSE for more information.
//

#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <dirent.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <algorithm>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include <QString>

#define BUFFER_SIZE 175
#define HEADER_BUFFER 4096
#define SERVER_NAME "NOOBS4IoT"
#define SERVER_VERSION "0.1a"

using namespace std;

class Request {
public:
    Request() {};
    bool processRequest(int clientSocket);
    string method;
    string path;
    string body;
    map<string, string> headers;
private:
    // This function will read from the socket and store the result in requestString
    bool readRequest(int clientSocket, string* requestString);
    // This function will parse the request in request string and populate all class members
    bool parseRequest(string &requestString);
    // This function will parse the method/request line and populate the members 'method' and 'path'
    bool parseMethodLine(string &methodLineString);

    bool parseHeaderLine(string &headerLineString);
    bool parseBodyLine(string &bodyLineString);

private:

};

class Response {
public:
    Response(int clientSocket);
    bool send();

    int code;
    int clientSocket;
    string phrase;
    string type;
    string date;
    string body;

private:
};

struct Route {
    string path;
    string method;
    void (*callback)(Request*, Response*);
};

class Server {
public:
    void get(string path, void (*callback)(Request*, Response*));
    void post(string path, void (*callback)(Request*, Response*));
    void all(string path, void (*callback)(Request*, Response*));

    void start(uint16_t port);

    static string getIP();
    static vector<string> split(const string &text, char sep);

private:
    std::vector<Route> ROUTES;
    bool stopServer = false;

    void addRoute(string path, string method, void (*callback)(Request*, Response*));
    bool matchRoute(Request* request, Response* response);
};


#endif //WEBSERVER_WEBSERVER_H
