#include <regex>
#include <fstream>
#include <csignal>
#include <unordered_set>
#include "TCPServer.h"
#include "debug.h"


namespace utility {
    void to_lower_string(std::string &data) {
        std::transform(data.begin(), data.end(), data.begin(),
                       [](char c) { return std::tolower(c); });
    }

    ssize_t parse_int(const std::string &num) {
        ssize_t val = 0;

        for (char c : num) {
            if (c < '0' || c > '9') {
                return -1;
            }
            val *= 10;
            val += c - '0';
        }
        return val;
    }
}

[[noreturn]] void TCPServer::run() {

    while (true) {
        if (communicationManager.is_connected() && should_disconnect) {
            communicationManager.disconnect();
        }

        if (!communicationManager.is_connected()) {
            communicationManager.connect();
        }

        try {
            get_request();
            log("GOT REQUEST:");
            log(message.method + " " + message.resource + message.content_length + message.connection);


        } catch (...) {
            should_disconnect = true;
            if (errno != SIGPIPE)
                send_error_and_disconnect(http_codes::SERVER_ERROR_CODE, http_codes::SERVER_ERROR_REASON);
        }
    }

}

void TCPServer::get_request() {
    clear_message();
    should_disconnect = false;

    bool startline_matched = get_startline(message.method, message.resource);
    log("Checked startline.");
    if (!startline_matched) {
        should_disconnect = true;
        send_error_and_disconnect(http_codes::BAD_REQUEST_CODE, http_codes::BAD_REQUEST_REASON);
        debug(message.resource);
        return;
    }


    std::unordered_set <std::string> header_set;

    std::string name, value;
    while (true) {
        communicationManager.getline(buffer);
        if (buffer == "\r\n") {
            log("End of headers reached.");
            break;
        }

        bool matched = get_header(name, value);
        if (!matched) {
            should_disconnect = true;
            send_error_and_disconnect(http_codes::BAD_REQUEST_CODE, http_codes::BAD_REQUEST_REASON);
            return;
        }
        log("Checked header.");

        utility::to_lower_string(name);
        utility::to_lower_string(value);

        if (is_ignorable_name(name)) {
            log("Ignoring a header.");
            continue;
        }

        if (header_set.count(name) > 0) {
            log("Headers repeated.");
            should_disconnect = true;
            send_error_and_disconnect(http_codes::BAD_REQUEST_CODE, http_codes::BAD_REQUEST_REASON);
            return;
        } else {
            header_set.insert(name);
        }

        if (name == "connection") {
            message.connection = value;
            if (value == "close") {
                should_disconnect = true;
            }
        } else if (name == "content-length") {
            message.content_length = value;
        }
    }

    log("Starting processing request.");
    process_request();
}

bool TCPServer::get_startline(std::string &method, std::string &target) {
    communicationManager.getline(buffer);
    static const std::regex startline_regex(R"(^([a-zA-Z]+) (\/+[^ ]*) HTTP\/1\.1\r\n$)");
    std::smatch match;
    bool matched = std::regex_match(buffer, match, startline_regex);

    if (matched) {
        method = match.str(1);
        target = match.str(2);
    }

    buffer.clear();

    return matched;
}

bool TCPServer::get_header(std::string &name, std::string &value) {
    static const std::regex startline_regex(R"(^(\S+): *(.*) *\r\n$)");
    std::smatch match;
    bool matched = std::regex_match(buffer, match, startline_regex);

    if (matched) {
        name = match.str(1);
        value = match.str(2);
    }

    buffer.clear();

    return matched;
}

bool TCPServer::is_ignorable_name(const std::string &name) const noexcept {
    return name != "connection" && name != "content-length";
}

void TCPServer::clear_message() noexcept {
    message.resource.clear();
    message.connection.clear();
    message.resource.clear();
    message.content_length.clear();
}

void TCPServer::send_error_and_disconnect(const std::string &code, const std::string &reason) {
    send_status_line(code, reason);

    if (should_disconnect) {
        send_disconnect();
    }
    send_end_of_headers();

    log("Sent error code:" + code + " and disconnected.");
}

void TCPServer::process_request() {
    if (is_ignorable_method(message.method)) {
        log("Not implemented method.");
        should_disconnect = true;
        send_error_and_disconnect(http_codes::NOT_IMPLEMENTED_CODE, http_codes::NOT_IMPLEMENTED_REASON);
        return;
    }

    ssize_t content_length = utility::parse_int(message.content_length);
    if (content_length == -1) {
        log("Bad content-length.");
        should_disconnect = true;
        send_error_and_disconnect(http_codes::BAD_REQUEST_CODE, http_codes::BAD_REQUEST_REASON);
        return;
    }

    if (!resourceManager.in_directory(message.resource)) {
        send_error_and_disconnect(http_codes::NOT_FOUND_CODE, http_codes::NOT_FOUND_REASON);
        return;
    }

    std::ifstream resource;
    bool is_local;
    try {
        is_local = resourceManager.try_get_resource(root + message.resource, resource);
        if (!is_local) {
            log("Couldn't find resource in root.");
        }
    } catch (...) {
        debug("Something caught while getting the resource.");
        is_local = false;
    }

    if (is_local) {
        send_found_local(resource);
        return;
    }

    std::string address;
    if (resourceManager.search_correlated(address, message.resource)) {
        send_found_redirect(address);
        return;
    }
    log("Couldn't find resource on correlated servers.");

    send_error_and_disconnect(http_codes::NOT_FOUND_CODE, http_codes::NOT_FOUND_REASON);
}

bool TCPServer::is_ignorable_method(const std::string &method) const noexcept {
    return method != "GET" && method != "HEAD";
}

TCPServer::TCPServer(const std::string &server_dir, const std::string &redirects, size_t port) :
        communicationManager(port), resourceManager(server_dir, redirects), root(server_dir), should_disconnect(false) {
    buffer.reserve(1 << 16);
}

void TCPServer::send_found_local(std::ifstream &stream) {
    send_status_line(http_codes::GOOD_REQUEST_CODE, http_codes::GOOD_REQUEST_REASON);
    if (message.connection == "close") {
        send_disconnect();
    }
    buffer.clear();

    send_content_info(resourceManager.get_size());
    send_end_of_headers();

    // I know that it is not the best solution, but other solutions didn't work as desired.
    if (message.method == "GET") {
        while (true) {
            int c = stream.get();
            if (c == EOF)
                break;
            communicationManager.putchar(c);
        }
    }

    stream.close();
    log("Sent file found locally.");
}

void TCPServer::send_status_line(const std::string &code, const std::string &reason) {
    std::string status_line = "HTTP/1.1 " + code + " " + reason + "\r\n";
    communicationManager.write_data(status_line);
}

void TCPServer::send_content_info(size_t length) {
    std::string content_info = "Content-Length: " + std::to_string(length) + "\r\n" +
                               "Content-Type: application/octet-stream\r\n";
    communicationManager.write_data(content_info);
}

void TCPServer::send_disconnect() {
    communicationManager.write_data("Connection: close\r\n");
}

void TCPServer::send_end_of_headers() {
    communicationManager.write_data("\r\n");
}

void TCPServer::send_location(const std::string &location) {

    communicationManager.write_data("Location: " + location + "\r\n");
}

void TCPServer::send_found_redirect(const std::string &redirect) {
    send_status_line(http_codes::CORRELATED_CODE, http_codes::CORRELATED_REASON);
    if (should_disconnect) {
        send_disconnect();
    }
    send_location(redirect);
    send_end_of_headers();
}


