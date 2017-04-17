//
// Created by Frank Steiler on 4/17/17.
// Copyright (c) 2017 Hewlett-Packard. All rights reserved.
//
// WebClient.h: [...]
//

#ifndef WEB_WEBCLIENT_H
#define WEB_WEBCLIENT_H


#include "Web.h"

#include <string>

#define USER_AGENT "NOOBS4IoT"
#define USER_AGENT_VERSION "0.1a"

namespace Web {

    using namespace std;

    namespace Client {
        class Request : public Web {
        public:
            Request(int socket) : Web(socket), accept("*/*") {}

            bool sendRequest();

            // Make sure method is uppercase
            string method;
            string path;
            string host;
            string accept;
        };

        class Response : public Web {
        public:
            Response(int socket): Web(socket) {};

            bool receiveResponse();

            int code;
            string phrase;
            string type;
        };
    }

    class WebClient {
    public:
        string get(string &url);
        // Host cannot be prefixed with http
        string get(string &host, string &path, int port = 80);

    private:
        // Host cannot be prefixed with http
        int openSocket(string &host, uint16_t port);
    };
}


#endif //WEB_WEBCLIENT_H
