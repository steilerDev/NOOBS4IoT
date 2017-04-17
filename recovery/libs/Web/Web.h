//
// Created by Frank Steiler on 4/17/17.
// Copyright (c) 2017 Hewlett-Packard. All rights reserved.
//
// Web.h: [...]
//

#ifndef WEB_WEB_H
#define WEB_WEB_H

#include <string>
#include <map>

#include <dirent.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <algorithm>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include <QString>

#define BUFFER_SIZE 4096
#define HEADER_BUFFER 4096

namespace Web {

    using namespace std;

    class Web {
    public:
        map<string, string> header;
        string body;

    protected:
        Web(int socket): _socket(socket) {};

        // The header line (e.g. GET URI HTTP/1.1 or HTTP/1.1 200 OK)
        string headerLine;

        /*
         * Fills header and body member attribute;
         */
        bool receive();

        /*
         * Writes the header line, header and body to the socket, make sure that header line + header does not exceed HEADER BUFFER
         */
        bool send();


    private:
        int _socket;

        // This function will read from the socket and store the result in receiveString
        bool readReceive(string *bufferString);
        // This function will parse the request in request string and populate header line, header and body
        bool parseReceive(const string &bufferString, const bool skipHeader);

        // This function will parse a single header line and add it to the header map
        bool parseHeaderLine(const string &headerLineString);
        // This function will parse a single body line and append it to the body string
        bool parseBodyLine(const string &bodyLineString);
    };

    // This helper function splits the given string by the sep char
    vector<string> split(const string &text, const char sep);
    QString getIP();
}


#endif //WEB_WEB_H
