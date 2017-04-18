//
// Created by Frank Steiler on 4/17/17 as part of NOOBS4IoT (https://github.com/steilerDev/NOOBS4IoT)
//
// WebServer.h:
//      This file contains several classes and helper functions providing extended WebServer functionalities. The
//      WebServer class can be used to expose a simple, blocking REST-API to clients. It allows the selection of
//      callback functions based on dynamic routing rules. No external, non-standard library is required for this file.
//      For more information see https://github.com/steilerDev/NOOBS4IoT/wiki.
//
// This file is licensed under a GNU General Public License v3.0 (c) Frank Steiler.
// See https://raw.githubusercontent.com/steilerDev/NOOBS4IoT/master/LICENSE for more information.
//

#ifndef WEB_WEBSERVER_H
#define WEB_WEBSERVER_H

#include "Web.h"

#define SERVER_NAME "NOOBS4IoT"
#define SERVER_VERSION "0.1a"

namespace Web {
    namespace Server {

        class Request: public Web {
        public:
            Request(int socket): Web(socket) {}

            bool receiveRequest();

            string method;
            string path;
        };

        class Response: public Web {
        public:
            Response(int socket);

            bool sendResponse();

            int code;
            string phrase;
            string type;
            string date;
        };

        struct Route {
            string path;
            string method;
            void (*callback)(Request*, Response*);
        };
    }

    class WebServer {
    public:
        void get(string path, void (*callback)(Server::Request*, Server::Response*));
        void post(string path, void (*callback)(Server::Request*, Server::Response*));
        void all(string path, void (*callback)(Server::Request*, Server::Response*));
        void start(uint16_t port);

    private:
        std::vector<Server::Route> _routes;
        bool _stopServer;

        void addRoute(string path, string method, void (*callback)(Server::Request*, Server::Response*));
        bool matchRoute(Server::Request* request, Server::Response* response);
    };

}

#endif //WEB_WEBSERVER_H
