//
// Created by Frank Steiler on 4/17/17 as part of NOOBS4IoT (https://github.com/steilerDev/NOOBS4IoT)
//
// WebClient.cpp:
//      This file contains several classes and helper functions providing extended WebClient functionalities. The
//      WebClient class can be used to retrieve string based content from the web in a blocking fashion. No external,
//      non-standard library is required for this file.
//      For more information see https://github.com/steilerDev/NOOBS4IoT/wiki.
//
// This file is licensed under a GNU General Public License v3.0 (c) Frank Steiler.
// See https://raw.githubusercontent.com/steilerDev/NOOBS4IoT/master/LICENSE for more information.
//

#include "WebClient.h"
#include "../easylogging++.h"

#include <netinet/tcp.h>
#include <netdb.h>

using namespace std;

string Web::WebClient::get(string &url) {

    if(strncmp(url.c_str(), "http://", strlen("http://")) == 0) {
        url = url.substr(strlen("http://"), url.size());
        LDEBUG << "Removed http:// from host: " << url;
    } else if(strncmp(url.c_str(), "https://", strlen("https://")) == 0) {
        LFATAL << "HTTPS is not supported!";
        return string();
    }

    vector<string> urlVector = split(url, '/');
    if(urlVector.size() < 1) {
        LFATAL << "Unable to split URL: " << url;
        return string();
    }

    string host = urlVector[0];
    string path = "/";
    path.append(urlVector[1]);
    if(urlVector.size() > 2) {
        LDEBUG << "Additional URL splits occured, re-merging";
        for (uint i = 2; i < urlVector.size(); i++) {
            path.append("/" + urlVector[i]);
        }
    }
    return get(host, path, 80);
}

string Web::WebClient::get(string &host, string &path, uint16_t port) {
    LINFO << "GETting resource " << path << " from " << host << ":" << port;


    int socket = openSocket(host, port);
    if(socket < 0) {
        LFATAL << "Unable to open socket";
        return string();
    }

    Client::Request request(socket);
    request.host = host;
    request.path = path;
    request.method = "GET";
    if(!request.sendRequest()) {
        LFATAL << "Unable to send request";
        close(socket);
        return string();
    }

    Client::Response response(socket);
    if(!response.receiveResponse()) {
        LFATAL << "Unable to receive response!";
        close(socket);
        return string();
    } else {
        LDEBUG << "Successfully received response";
        close(socket);
        return response.body;
    }
}

int Web::WebClient::openSocket(string &hostString, uint16_t port) {
    struct hostent *host;
    struct sockaddr_in addr;
    int on = 1, sock;

    if((host = gethostbyname(hostString.c_str())) == NULL){
        LFATAL << "Unable to get host by name!";
        return -1;
    }
    bcopy(host->h_addr, &addr.sin_addr, (size_t)((unsigned) host->h_length));
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));

    if(sock == -1){
        LFATAL << "Unable to set sock opts";
        return -1;
    }

    if(connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1){
        LFATAL << "Unable to connect to socket";
        return -1;
    }
    return sock;
}

bool Web::Client::Response::receiveResponse() {

    if(!receive()) {
        LFATAL << "Unable to receive response";
        return false;
    } else {
        LDEBUG << "Parsing method line from header line";
        vector<string> methodLineVector = split(headerLine, ' ');
        if(methodLineVector.size() != 3) {
            LERROR << "Request-Line does not conform specifications";
            return false;
        } else if (methodLineVector[0].compare("HTTP/1.1") != 0) {
            LERROR << "This server only supports HTTP/1.1, found " << methodLineVector[2];
            return false;
        }

        // Removing trailing \r
        this->phrase = methodLineVector[2].substr(0, methodLineVector[2].size()-1);
        this->code = stoi(methodLineVector[1]);

        LDEBUG << "Found response phrase: " << this->phrase;
        LDEBUG << "Found response code: " << this->code;
        return true;
    }
}

bool Web::Client::Request::sendRequest() {
    LINFO << "Sending request";

    headerLine = this->method;
    headerLine.append(" ");
    headerLine.append(this->path);
    headerLine.append(" HTTP/1.1");

    header["Host"] = this->host;
    header["User-Agent"] = USER_AGENT " " USER_AGENT_VERSION;
    header["Accept"] = this->accept;

    return send();
}
