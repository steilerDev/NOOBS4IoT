//
// Created by Frank Steiler on 4/17/17 as part of NOOBS4IoT (https://github.com/steilerDev/NOOBS4IoT)
//
// WebClient.h:
//      This file contains several classes and helper functions providing extended WebClient functionalities. The
//      WebClient class can be used to retrieve string based content from the web in a blocking fashion. No external,
//      non-standard library is required for this file.
//      For more information see https://github.com/steilerDev/NOOBS4IoT/wiki.
//
// This file is licensed under a GNU General Public License v3.0 (c) Frank Steiler.
// See https://raw.githubusercontent.com/steilerDev/NOOBS4IoT/master/LICENSE for more information.
//

#ifndef WEB_WEBCLIENT_H
#define WEB_WEBCLIENT_H

#include "Web.h"
#include <string>

#define USER_AGENT "NOOBS4IoT"
#define USER_AGENT_VERSION "0.1a"

using namespace std;

namespace Web {

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
        string get(string &host, string &path, uint16_t port = 80);

    private:
        // Host cannot be prefixed with http
        int openSocket(string &host, uint16_t port);
    };
}

#endif //WEB_WEBCLIENT_H
