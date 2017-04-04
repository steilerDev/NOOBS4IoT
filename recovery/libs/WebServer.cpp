//
// Created by Frank Steiler on 4/2/17 as part of NOOBS4IoT (https://github.com/steilerDev/NOOBS4IoT)
//
// WebServer.cpp:
//      A simple HTTP/1.1 Web Server implementation, providing callback functions on multiple routes.
//      The server is only accepting a single request at a time and blocks until the callback returns.
//      For more information see https://github.com/steilerDev/NOOBS4IoT/wiki.
//
// This file is licensed under a GNU General Public License v3.0 (c) Frank Steiler.
// See https://raw.githubusercontent.com/steilerDev/NOOBS4IoT/master/LICENSE for more information.
//

#include "WebServer.h"
#include "easylogging++.h"
#include <QtNetwork/QNetworkInterface>

//
// Response functions
//

Response::Response(int clientSocket): clientSocket(clientSocket) {
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

bool Response::send() {
    LINFO << "Sending response";
    char header_buffer[HEADER_BUFFER];
    size_t body_len = strlen(body.c_str());

    // build http response
    sprintf(header_buffer, "HTTP/1.0 %d %s\r\n", code, phrase.c_str());

    // append headers
    sprintf(&header_buffer[strlen(header_buffer)], "Server: %s %s\r\n", SERVER_NAME, SERVER_VERSION);
    sprintf(&header_buffer[strlen(header_buffer)], "Date: %s\r\n", date.c_str());
    sprintf(&header_buffer[strlen(header_buffer)], "Content-Type: %s\r\n", type.c_str());
    sprintf(&header_buffer[strlen(header_buffer)], "Content-Length: %zd\r\n", body_len);

    // append extra crlf to indicate start of body
    strcat(header_buffer, "\r\n");

    LDEBUG << "Response:\n" << header_buffer << body.c_str();

    LINFO << "Sending response";
    ssize_t t = 0;
    t += write(clientSocket, header_buffer, strlen(header_buffer));
    t += write(clientSocket, body.c_str(), body_len);
    if(t <= 0) {
        LERROR << "Unable to send response!";
        return false;
    } else {
        LDEBUG << "Wrote response of size " << t;
        return true;
    }
}

//
// Request functions
//

bool Request::processRequest(int clientSocket) {
    LINFO << "Parsing request";
    string requestString;
    if(!readRequest(clientSocket, &requestString)) {
        LERROR << "Unable to read request from socket";
        return false;
    }

    if(!parseRequest(requestString, false)) {
        LERROR << "Unable to parse request";
        return false;
    }

    if(headers.find("Expect") != headers.end() && headers["Expect"].compare("100-continue\r") == 0) {
        LDEBUG << "Found expect header, building continue response";
        Response expectResponse(clientSocket);
        expectResponse.code = 100;
        expectResponse.phrase = "Continue";
        LDEBUG << "Sending continue response";
        expectResponse.send();
        LDEBUG << "Continuing reading";
        requestString.clear();
        if(!readRequest(clientSocket, &requestString)) {
            LERROR << "Unable to continue to read from socket";
            return false;
        }
        if(!parseRequest(requestString, true)) {
            LERROR << "Unable to parse continue request";
            return false;
        }
    }
    // Handle 100 Continue header

    return true;
}

bool Request::readRequest(int clientSocket, string *requestString) {
    LDEBUG << "Reading request from socket";
    static char buffer[BUFFER_SIZE];
    ssize_t readCount;


    fd_set set;
    int rv;
    // Clear the set
    FD_ZERO(&set);
    // Add our socket to the set
    FD_SET(clientSocket, &set);

    do {
        // Redefine timeout struct, since it is consumed by select
        struct timeval timeout;
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;

        // Clear the buffer every time
        memset(buffer, 0, BUFFER_SIZE);

        // Wait for incoming IO or timeout (If content == buffer size we would re-run the loop and read would wait for
        // additional information, but none would be available, blocking the execution forever.
        rv = select(clientSocket + 1, &set, NULL, NULL, &timeout);
        if(rv == -1) {
            LERROR << "An error occured within select";
            readCount = -1;
        } else if (rv == 0) {
            LWARNING << "Read timeout occured, this should be okay and due to the fact, that the buffer size == request size";
            readCount = 0;
        } else {
            readCount = read(clientSocket, buffer, BUFFER_SIZE);
            LDEBUG << "Read " << readCount << " bytes";
            requestString->append(buffer);
        }
    } while (readCount == BUFFER_SIZE);

    LDEBUG << "Finished read";
    if(readCount < 0)  {
        LERROR << "Error during read!";
        return false;
    }
    LDEBUG << "Read: \n" << *requestString;
    return true;
}

bool Request::parseRequest(string &requestString, bool skipHeader) {
    LDEBUG << "Parsing request line by line";
    // Indicates if we are still in the header
    bool header = !skipHeader;
    // Indicates if we are at the request line
    bool methodLine = true;

    istringstream requestStream(requestString);
    for(string line; getline(requestStream, line); ) {
        if(header) {
            if(methodLine) { //Processing method line
                if(!parseMethodLine(line)) {
                    LERROR << "Unable to parse method line: " << line;
                    return false;
                }
                methodLine = false;
            } else { // Processing headers
                if (!(line.size() == 1 && line[0] == '\r')) { // Processing a header field
                    if(!parseHeaderLine(line)) {
                        LERROR << "Unable to parse header line: " << line;
                        return false;
                    }
                } else { // Found start of body
                    LDEBUG << "Found start of body";
                    header = false;
                }
            }
        } else { // Processing body line
            if(!parseBodyLine(line)) {
                LERROR << "Unable to parse body line: " << line;
                return false;
            }
        }
    }
    LDEBUG << "Finished parsing request";
    return true;
}

bool Request::parseMethodLine(string &methodLineString) {
    vector<string> methodLineVector = Server::split(methodLineString, ' ');
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

bool Request::parseHeaderLine(string &headerLineString) {
    vector<string> headerLineVector = Server::split(headerLineString, ':');
    if(headerLineVector.size() >= 2) {
        string key = headerLineVector[0];
        string value = headerLineVector[1];
        if (headerLineVector.size() > 2) {
            LDEBUG << "Additional splits occurred, re-merging";
            for (uint i = 2; i < headerLineVector.size(); i++) {
                value.append(":" + headerLineVector[i]);
            }
        }
        if (value[0] == ' ') {
            value = value.substr(1, value.length());
        }
        headers[key] = value;
        LDEBUG << "Found header field: Key = " << key << ", value = " << value;
    } else {
        LWARNING << "Split resulted in single result, unable to process: " << headerLineString;
        // Not returning false, only ignoring this specific line
    }
    return true;
}

bool Request::parseBodyLine(string &bodyLineString) {
    LDEBUG << "Found body line: " << bodyLineString;
    this->body.append(bodyLineString);
    return true;
}

//
// Server functions
//

void Server::start(uint16_t port) {
    stopServer = false;
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

    while(!stopServer) {
        newSocket = accept(listeningSocket, (struct sockaddr *) &peerAddr, &peerLen);

        if(newSocket < 0) {
            LERROR << "Unable to accept connection";
            close(newSocket);
            continue;
        }

        LINFO << "Connection accepted!";

        Request request;
        Response response(newSocket);

        if(!request.processRequest(newSocket)) {
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

        if(!response.send()) {
            LERROR << "Unable to send response";
            close(newSocket);
            continue;
        }

        LINFO << "Finished processing request!";
        close(newSocket);
    }
    close(listeningSocket);
}

bool Server::matchRoute(Request* req, Response* res) {
    LDEBUG << "Matching routes for " << req->method << " at " << req->path;

    for (vector<Route>::size_type i = 0; i < ROUTES.size(); i++) {
        if(ROUTES[i].path == req->path && (ROUTES[i].method == req->method || ROUTES[i].method == "ALL")) {
            LDEBUG << "Found matching route for " << req->method << " at " << req->path << ", starting callback";
            ROUTES[i].callback(req, res);
            return true;
        }
    }
    LINFO << "Unable to find route for " << req->method << " at " << req->path;
    return false;
}

void Server::addRoute(string path, string method, void (*callback)(Request *, Response *)) {
    Route r = {
            path,
            method,
            callback
    };
    ROUTES.push_back(r);
}

void Server::get(string path, void (*callback)(Request*, Response*)) {
    addRoute(path, "GET", callback);
}

void Server::post(string path, void (*callback)(Request*, Response*)) {
    addRoute(path, "POST", callback);
}

void Server::all(string path, void (*callback)(Request*, Response*)) {
    addRoute(path, "ALL", callback);
}

QString Server::getIP() {
    QString ip;
    QList<QHostAddress> ipAddresses = QNetworkInterface::allAddresses();
            foreach(QHostAddress address, ipAddresses) {
            if(ip.isEmpty()) {
                ip.append(address.toString());
            } else {
                if(!ip.startsWith("[")) {
                    ip.prepend("[");
                }
                ip.append("|");
                ip.append(address.toString());
            }
        }
    if(ip.startsWith("[")) {
        ip.append("]");
    }
    return ip;
}

vector<string> Server::split(const string &text, char sep) {
    vector<string> tokens;
    size_t start = 0, end = 0;
    while ((end = text.find(sep, start)) != string::npos) {
        tokens.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    tokens.push_back(text.substr(start));
    return tokens;
}