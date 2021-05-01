#ifndef PIERWSZE_TCPSERVER_H
#define PIERWSZE_TCPSERVER_H

#include "CommunicationManager.h"
#include "ResourceManager.h"


namespace http_codes {
    constexpr char GOOD_REQUEST_CODE[] = "200";
    constexpr char GOOD_REQUEST_REASON[] = "Request completed successfully. Target found on server.";

    constexpr char CORRELATED_CODE[] = "302";
    constexpr char CORRELATED_REASON[] = "Request completed successfully. Target found on correlated server.";

    constexpr char BAD_REQUEST_CODE[] = "400";
    constexpr char BAD_REQUEST_REASON[] = "Bad request.";

    constexpr char NOT_FOUND_CODE[] = "404";
    constexpr char NOT_FOUND_REASON[] = "Target not found.";

    constexpr char SERVER_ERROR_CODE[] = "500";
    constexpr char SERVER_ERROR_REASON[] = "Server error.";

    constexpr char NOT_IMPLEMENTED_CODE[] = "501";
    constexpr char NOT_IMPLEMENTED_REASON[] = "Request goes beyond server's capabilities.";
}

class TCPServer {
public:
    TCPServer(const std::string &server_dir, const std::string &redirects, size_t port);

    [[noreturn]] void run();
private:
    struct Message {
        std::string method;
        std::string resource;
        std::string content_length;
        std::string connection;
    };
    void get_request();
    bool get_startline(std::string &method, std::string &target);
    bool get_header(std::string &name, std::string &value);
    bool is_ignorable_name(const std::string &name) const noexcept;
    bool is_ignorable_method(const std::string &method) const noexcept;

    void process_request();

    void send_error_and_disconnect(const std::string &code, const std::string &reason);
    void send_found_local(std::ifstream &stream);
    void send_found_redirect(const std::string &redirect);

    void send_disconnect();
    void send_content_info(size_t length);
    void send_status_line(const std::string &code, const std::string &reason);
    void send_end_of_headers();
    void send_location(const std::string &location);
    void clear_message() noexcept;

    CommunicationManager communicationManager;
    ResourceManager resourceManager;
    std::string root;

    Message message;
    std::string buffer;
    bool should_disconnect;
};


#endif //PIERWSZE_TCPSERVER_H
