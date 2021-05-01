#include <sys/socket.h>
#include <netinet/in.h>
#include <csignal>
#include "unistd.h"
#include "CommunicationManager.h"
#include "debug.h"


CommunicationManager::CommunicationManager(size_t _port) : connected(false), port(_port) {
    sockaddr_in server_address{};

    signal(SIGPIPE, SIG_IGN);

    socket_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        debug("Creating socket via socket() failed.");
        throw InternalError();
    }
    log("Created socket.");

    server_address.sin_family = AF_INET; // IPv4
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // Listening on all interfaces.
    server_address.sin_port = htons(port); // Listening on port [port].

    if (bind(socket_fd, (sockaddr *) &server_address, sizeof server_address) < 0) {
        debug("Binding socket via bind() failed.");
        throw InternalError();
    }
    log("bind() was successful.");

    if (listen(socket_fd, QUEUE_SIZE) < 0) {
        debug("Listening via listen() failed.");
        throw InternalError();
    }
    std::string message = "Started listening on port: " + std::to_string(port);
    log(message);
}

CommunicationManager::~CommunicationManager() noexcept {
    if (close(socket_fd) < 0) {
        debug("Closing socket via close() failed.");
    }
    log("Closed socket.");
}

void CommunicationManager::connect() {
    if (connected) {
        debug("Client is already connected.");
        throw InternalError();
    }
    sockaddr_in client_address{};
    socklen_t client_address_len = sizeof client_address;

    int client_fd = accept(socket_fd, (sockaddr *) &client_address, &client_address_len);
    if (client_fd < 0) {
        debug("accept() on client failed.");
        throw InternalError();
    }

    in_socket_file = fdopen(client_fd, "r");
    out_socket_file = fdopen(client_fd, "w");
    if (in_socket_file == nullptr || out_socket_file == nullptr) {
        debug("InternalError in invoking fdopen() on socket.");
        throw InternalError();
    }
    connected = true;
    log("Connection successful.");
}

void CommunicationManager::disconnect() {
    log("Trying to disconnect.");
    if (!connected) {
        debug("No client is connected.");
        throw InternalError();
    }
    if (fclose(in_socket_file) < 0) {
        debug("InternalError in disconnecting via fclose().");
        debug(std::to_string(errno));
        throw InternalError();
    }
    in_socket_file = nullptr;
    out_socket_file = nullptr;
    connected = false;

    log("Disconnection successful.");
}

void CommunicationManager::getline(std::string &buffer) {
    int state = 0;
    buffer.clear();
    while (true) {
        int c = fgetc(in_socket_file);
        if (c == EOF) {
            debug("InternalError in reading via fgetc().");
            throw CommunicationError();
        }
        buffer += static_cast<char>(c);

        if (c == '\r') {
            state = 1;
        } else if (c == '\n' && state == 1) {
            break;
        } else {
            state = 0;
        }
    }
    log("Got line: " + buffer);
    log("Read data successfully.");
}

void CommunicationManager::write_data(const std::string &data) {
    size_t written = 0;
    do {
        size_t ret = fwrite(data.c_str() + written, sizeof(char), data.size() - written, out_socket_file);
        if (ret == 0) {
            debug("InternalError in writing to socket.");
            throw CommunicationError();
        }
        written += ret;
    } while (written < data.size());
    fflush(out_socket_file);
}

bool CommunicationManager::is_connected() const {
    return connected;
}

void CommunicationManager::putchar(int c) {
    int ret = fputc(c, out_socket_file);
    if (ret == EOF) {
        throw CommunicationError();
    }
    fflush(out_socket_file);
}

