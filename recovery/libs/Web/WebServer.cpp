//
// Created by Frank Steiler on 4/17/17 as part of NOOBS4IoT (https://github.com/steilerDev/NOOBS4IoT)
//
// WebServer.cpp:
//      This file contains several classes and helper functions providing extended WebServer functionalities. The
//      WebServer class can be used to expose a simple, blocking REST-API to clients. It allows the selection of
//      callback functions based on dynamic routing rules. No external, non-standard library is required for this file.
//      For more information see https://github.com/steilerDev/NOOBS4IoT/wiki.
//
// This file is licensed under a GNU General Public License v3.0 (c) Frank Steiler.
// See https://raw.githubusercontent.com/steilerDev/NOOBS4IoT/master/LICENSE for more information.
//

#include "WebServer.h"
#include "../easylogging++.h"

using namespace std;

void Web::WebServer::start(uint16_t port) {
    _stopServer = false;
    int newSocket;

    LDEBUG << "Creating listening socket";
    int listeningSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (listeningSocket < 0) {
        LFATAL << "Unable to open listening socket!";
        return;
    }

    struct sockaddr_in servAddr, peerAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port = htons(port);

    if(::bind(listeningSocket, (struct sockaddr *) &servAddr, sizeof(servAddr)) != 0) {
        LFATAL << "Unable to bind listening socket: " ; //<< strerror(errno);
        return;
    }

    LDEBUG << "Starting to listen on socket";

    listen(listeningSocket, 1);

    socklen_t peerLen;
    peerLen = sizeof(peerAddr);

    while(!_stopServer) {
        newSocket = accept(listeningSocket, (struct sockaddr *) &peerAddr, &peerLen);

        if(newSocket < 0) {
            LERROR << "Unable to accept connection";
            close(newSocket);
            continue;
        }

        LINFO << "Connection accepted!";

        Server::Request request(newSocket);
        Server::Response response(newSocket);

        if(!request.receiveRequest()) {
            LERROR << "Unable to process request";
            response.code = 400;
            response.phrase = "Bad Request";
            response.type = "text/plain";
            response.body = "Bad Request";
        } else if(!matchRoute(&request, &response)) {
            LWARNING << "Unable to match route";
            response.code = 404;
            response.phrase = "Not Found";
            response.type = "text/plain";
            response.body = "Not found";
        }

        if(!response.sendResponse()) {
            LERROR << "Unable to send response";
            close(newSocket);
            continue;
        }

        LINFO << "Finished processing request!";
        close(newSocket);
    }
    close(listeningSocket);
}

bool Web::WebServer::matchRoute(Server::Request* req, Server::Response* res) {
    LDEBUG << "Matching routes for " << req->method << " at " << req->path;

    for (vector<Server::Route>::size_type i = 0; i < _routes.size(); i++) {
        if(_routes[i].path == req->path && (_routes[i].method == req->method || _routes[i].method == "ALL")) {
            LDEBUG << "Found matching route for " << req->method << " at " << req->path << ", starting callback";
            _routes[i].callback(req, res);
            return true;
        }
    }
    LINFO << "Unable to find route for " << req->method << " at " << req->path;
    return false;
}

void Web::WebServer::addRoute(string path, string method, void (*callback)(Server::Request *, Server::Response *)) {
    Server::Route r = {
            path,
            method,
            callback
    };
    _routes.push_back(r);
}

void Web::WebServer::get(string path, void (*callback)(Server::Request*, Server::Response*)) {
    addRoute(path, "GET", callback);
}

void Web::WebServer::post(string path, void (*callback)(Server::Request*, Server::Response*)) {
    addRoute(path, "POST", callback);
}

void Web::WebServer::all(string path, void (*callback)(Server::Request*, Server::Response*)) {
    addRoute(path, "ALL", callback);
}

bool Web::Server::Request::receiveRequest() {

    if(!receive()) {
        LFATAL << "Unable to receive request";
        return false;
    } else {
        LDEBUG << "Parsing method line from header line";
        vector<string> methodLineVector = split(headerLine, ' ');
        if(methodLineVector.size() != 3) {
            LERROR << "Request-Line does not conform specifications";
            return false;
        } else if (methodLineVector[2].compare("HTTP/1.1\r") != 0) {
            // Specification says CRLF and getline splits at \n, to \r remains at end of line.
            LERROR << "This server only supports HTTP/1.1, found " << methodLineVector[2];
            return false;
        }

        this->method = methodLineVector[0];
        this->path = methodLineVector[1];

        LDEBUG << "Found request method: " << this->method;
        LDEBUG << "Found request path: " << this->path;
        return true;
    }
}

Web::Server::Response::Response(int socket): Web(socket) {
    code = 200;
    phrase = "OK";
    type = "text/plain";

    // set current date and time for "Date: " header
    char buffer[100];
    time_t now = time(0);
    struct tm tstruct = *gmtime(&now);
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S %Z", &tstruct);
    date = string(buffer);
}

bool Web::Server::Response::sendResponse() {
    LINFO << "Sending response";

    headerLine = "HTTP/1.0 ";
    headerLine.append(to_string(code));
    headerLine.append(" ");
    headerLine.append(phrase);

    header["Server"] = SERVER_NAME " " SERVER_VERSION;
    header["Date"] = date;
    header["Content-Type"] = type;
    header["Content-Length"] = to_string(strlen(body.c_str()));

    return send();
}
