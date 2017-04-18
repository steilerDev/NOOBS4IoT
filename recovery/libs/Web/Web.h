//
// Created by Frank Steiler on 4/17/17 as part of NOOBS4IoT (https://github.com/steilerDev/NOOBS4IoT)
//
// Web.h:
//      This file contains several classes and helper functions providing basic socket communication based on standard
//      C sockets. Besides the getIP() function (which uses QtNetwork) no external non-standard library is required.
//      For more information see https://github.com/steilerDev/NOOBS4IoT/wiki.
//
// This file is licensed under a GNU General Public License v3.0 (c) Frank Steiler.
// See https://raw.githubusercontent.com/steilerDev/NOOBS4IoT/master/LICENSE for more information.
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

using namespace std;

namespace Web {

    class Web {
    public:
        map<string, string> header;
        string body;

    protected:
        Web(int socket): _socket(socket) {};

        /*
         * Fills header and body member attribute;
         */
        bool receive();

        /*
         * Writes the header line, header and body to the socket, make sure that header line + header does not exceed HEADER BUFFER
         */
        bool send();

        // The header line (e.g. GET URI HTTP/1.1 or HTTP/1.1 200 OK)
        string headerLine;

    private:
        // This function will read from the socket and store the result in receiveString
        bool readReceive(string *bufferString);
        // This function will parse the request in request string and populate header line, header and body
        bool parseReceive(const string &bufferString, const bool skipHeader);

        // This function will parse a single header line and add it to the header map
        bool parseHeaderLine(const string &headerLineString);
        // This function will parse a single body line and append it to the body string
        bool parseBodyLine(const string &bodyLineString);

        int _socket;
    };

    // This helper function splits the given string by the sep char
    vector<string> split(const string &text, const char sep);
    // This helper returns the IP address of all available interfaces using the QtNetworking API
    QString getIP();
}

#endif //WEB_WEB_H
