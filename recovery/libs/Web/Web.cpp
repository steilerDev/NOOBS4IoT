//
// Created by Frank Steiler on 4/17/17.
// Copyright (c) 2017 Hewlett-Packard. All rights reserved.
//
// Web.cpp: [...]
//

#include "Web.h"
#include "../easylogging++.h"

#include <QtNetwork/QNetworkInterface>

using namespace std;

bool Web::Web::send() {
    LINFO << "Sending http message";

    char header_buffer[HEADER_BUFFER];

    // build http message
    sprintf(header_buffer, "%s\r\n", headerLine.c_str());

    // append headers
    for(auto const& x: header) {
        sprintf(&header_buffer[strlen(header_buffer)], "%s: %s\r\n", x.first.c_str(), x.second.c_str());
    }

    // append extra crlf to indicate start of body
    strcat(header_buffer, "\r\n");

    LDEBUG << "HTTP message:\n" << header_buffer << body.c_str();

    LINFO << "Sending HTTP message";
    ssize_t t = 0;
    t += write(_socket, header_buffer, strlen(header_buffer));
    t += write(_socket, body.c_str(), strlen(body.c_str()));
    if(t <= 0) {
        LERROR << "Unable to send HTTP message!";
        return false;
    } else {
        LDEBUG << "Send HTTP message of size " << t;
        return true;
    }
}

 bool Web::Web::receive() {
    LINFO << "Parsing received HTTP message";
    string bufferString;
    if(!readReceive(&bufferString)) {
        LERROR << "Unable to read receive from socket";
        return false;
    }

    if(!parseReceive(bufferString, false)) {
        LERROR << "Unable to parse receive";
        return false;
    }

    if(header.find("Expect") != header.end() && header["Expect"].compare("100-continue\r") == 0) {
        LDEBUG << "Found expect header, building continue response";
        Web expectResponse(_socket);

        expectResponse.headerLine = "HTTP/1.0 100 Continue";
        LDEBUG << "Sending continue response";
        expectResponse.send();
        LDEBUG << "Continuing reading";
        bufferString.clear();
        if(!readReceive(&bufferString)) {
            LERROR << "Unable to continue to read from socket";
            return false;
        }
        if(!parseReceive(bufferString, true)) {
            LERROR << "Unable to parse continue request";
            return false;
        }
    }

    return true;
}

bool Web::Web::readReceive(string *bufferString) {
    LDEBUG << "Reading HTTP message from socket";
    static char buffer[BUFFER_SIZE];
    ssize_t readCount;

    fd_set set;
    int rv;
    // Clear the set
    FD_ZERO(&set);
    // Add our socket to the set
    FD_SET(_socket, &set);

    do {
        // Redefine timeout struct, since it is consumed by select
        struct timeval timeout;
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;

        // Clear the buffer every time
        memset(buffer, 0, BUFFER_SIZE);

        // Wait for incoming IO or timeout (If content == buffer size we would re-run the loop and read would wait for
        // additional information, but none would be available, blocking the execution forever.
        rv = select(_socket + 1, &set, NULL, NULL, &timeout);
        if(rv == -1) {
            LERROR << "An error occurred within select";
            readCount = -1;
        } else if (rv == 0) {
            LWARNING << "Read timeout occurred, this should be okay and due to the fact, that the buffer size == request size";
            readCount = 0;
        } else {
            readCount = read(_socket, buffer, BUFFER_SIZE);
            LDEBUG << "Read " << readCount << " bytes";
            bufferString->append(buffer);
        }
    } while (readCount == BUFFER_SIZE);

    LDEBUG << "Finished read";
    if(readCount < 0)  {
        LERROR << "Error during read!";
        return false;
    }
    LDEBUG << "Read: \n" << *bufferString;
    return true;
}

bool Web::Web::parseReceive(const string &bufferString, const bool skipHeader) {
    LDEBUG << "Parsing received HTTP message line by line";
    // Indicates if we are still in the header
    bool header = !skipHeader;
    // Indicates if we are at the request line
    bool methodLine = true;

    istringstream bufferStream(bufferString);
    for(string line; getline(bufferStream, line); ) {
        if(header) {
            if(methodLine) { //Processing method line
                headerLine = line;
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

bool Web::Web::parseHeaderLine(const string &headerLineString) {
    vector<string> headerLineVector = split(headerLineString, ':');
    if(headerLineVector.size() >= 2) {
        string key = headerLineVector[0];
        string value = headerLineVector[1];
        if (headerLineVector.size() > 2) {
            LDEBUG << "Additional splits occurred, re-merging";
            for (uint i = 2; i < headerLineVector.size(); i++) {
                value.append(":" + headerLineVector[i]);
            }
        }
        // Removing unnecessary space at the start
        if (value[0] == ' ') {
            value = value.substr(1, value.length());
        }
        header[key] = value;
        LDEBUG << "Found header field: Key = " << key << ", value = " << value;
    } else {
        LWARNING << "Split resulted in single result, unable to process: " << headerLineString;
        // Not returning false, only ignoring this specific line
    }
    return true;
}

bool Web::Web::parseBodyLine(const string &bodyLineString) {
    LDEBUG << "Found body line: " << bodyLineString;
    this->body.append(bodyLineString);
    this->body.append("\n");
    return true;
}

vector<string> Web::split(const string &text, const char sep) {
    vector<string> tokens;
    size_t start = 0, end = 0;
    while ((end = text.find(sep, start)) != string::npos) {
        tokens.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    tokens.push_back(text.substr(start));
    return tokens;
}

QString Web::getIP() {
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

