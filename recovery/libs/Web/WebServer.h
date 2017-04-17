//
// Created by Frank Steiler on 4/17/17.
// Copyright (c) 2017 Hewlett-Packard. All rights reserved.
//
// Server.h: [...]
//

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

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


#endif //WEB_SERVER_H
