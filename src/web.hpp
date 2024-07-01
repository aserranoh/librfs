
#pragma once

extern "C"
{
    #include "mongoose.h"
}

#include <expected>
#include <format>
#include <iostream>
#include <list>
#include <memory>
#include <nlohmann/json.hpp>

#include "error.hpp"

using namespace std;

namespace rfs {

constexpr int HTTP_200_OK = 200;
constexpr int HTTP_ERROR_404_NOT_FOUND = 404;
constexpr int HTTP_ERROR_405_METHOD_NOT_ALLOWED = 405;
constexpr int HTTP_ERROR_500_SERVER_ERROR = 500;

class HTTPRequest {

public:

    mg_connection *connection;
    mg_http_message *http_message;

    HTTPRequest(mg_connection *connection, mg_http_message *http_message):
        connection(connection), http_message(http_message)
    {}


    string body() const
    {
        return {http_message->body.ptr, http_message->body.len};
    }

    bool is_get() const
    {
        return mg_vcmp(&http_message->method, "GET") == 0;
    }

    bool is_post() const
    {
        return mg_vcmp(&http_message->method, "POST") == 0;
    }

    expected<nlohmann::json, nlohmann::json::parse_error> json() const
    {
        const string string_body = body();
        try {
            return nlohmann::json::parse(string_body);
        } catch (const nlohmann::json::parse_error &error) {
            return unexpected(error);
        }
    }

    string method() const
    {
        return {http_message->method.ptr, http_message->method.len};
    }

    string uri() const
    {
        return {http_message->uri.ptr, http_message->uri.len};
    }

};

class HTTPEventHandler {

public:

    HTTPEventHandler()
    {}

    virtual ~HTTPEventHandler()
    {}

    virtual void get(HTTPRequest &request) const
    {
        mg_http_reply(request.connection, HTTP_ERROR_405_METHOD_NOT_ALLOWED, "", "405: Method Not Allowed");
    }

    static void ok(HTTPRequest &request)
    {
        reply(request, HTTP_200_OK, "");
    }

    virtual void post(HTTPRequest &request) const
    {
        mg_http_reply(request.connection, HTTP_ERROR_405_METHOD_NOT_ALLOWED, "", "405: Method Not Allowed");
    }

    static void reply(HTTPRequest &request, int status_code, const string &contents)
    {
        mg_http_reply(request.connection, status_code, "", contents.c_str());
    }

};

class HTTPFileHandler: public HTTPEventHandler {

public:

    HTTPFileHandler(const string &path):
        path(path), serve_file_opts{.mime_types = "html=text/html"}
    {}

    virtual ~HTTPFileHandler()
    {}

    inline virtual void get(HTTPRequest &request) const override
    {
        mg_http_serve_file(request.connection, request.http_message, path.c_str(), &serve_file_opts);
    }

private:

    string path;
    mg_http_serve_opts serve_file_opts;

};

class HTTPHandlerEndpoint {

public:

    string uri;
    std::unique_ptr<HTTPEventHandler> handler;

    HTTPHandlerEndpoint(const string &uri, std::unique_ptr<HTTPEventHandler> &handler):
        uri(uri), handler(std::move(handler))
    {}

};

class WebApplication {

public:

    WebApplication(bool debug = false):
        connection(nullptr), debug_flag(debug)
    {
        mg_mgr_init(&mgr);
    }

    ~WebApplication()
    {
        mg_mgr_free(&mgr);
    }

    void add_handler(const string &prefix, std::unique_ptr<HTTPEventHandler> &handler)
    {
        handlers.push_back(HTTPHandlerEndpoint{prefix, handler});
    }

    expected<void, Error> listen(const string &url)
    {
        connection = mg_http_listen(&mgr, url.c_str(), on_event, this);
        if (connection == nullptr) {
            return unexpected(Error(errno));
        }
        debug("Listening on " + url + "...");
        return {};
    }

    inline void poll(int timeout_ms)
    {
        mg_mgr_poll(&mgr, timeout_ms);
    }

private:

    void debug(const string &message)
    {
        if (debug_flag)
            cerr << message << endl;
    }

    static void execute_method(HTTPHandlerEndpoint &endpoint, HTTPRequest &request)
    {
        if (request.is_get()) {
            endpoint.handler->get(request);
        } else if (request.is_post()) {
            endpoint.handler->post(request);
        }
    }

    static void on_event(mg_connection *connection, int event, void *event_data)
    {
        if (event != MG_EV_HTTP_MSG)
            return;
        
        WebApplication *app = (WebApplication *)connection->fn_data;
        mg_http_message *http_message = (mg_http_message *)event_data;
        HTTPRequest request(connection, http_message);

        bool handler_found = false;
        for (HTTPHandlerEndpoint &handler: app->handlers) {
            if (mg_http_match_uri(http_message, handler.uri.c_str())) {
                handler_found = true;
                app->debug(request.method() + " " + request.uri());
                execute_method(handler, request);
            }
        }

        if (!handler_found) {
            mg_http_reply(connection, HTTP_ERROR_404_NOT_FOUND, "", "404: Not Found");
            app->debug(string(http_message->uri.ptr, http_message->uri.len) + ": Not Found");
        }
    }

    mg_mgr mgr;
    mg_connection *connection;
    std::list<HTTPHandlerEndpoint> handlers;
    bool debug_flag;

};

}