#ifndef PIERWSZE_COMMUNICATIONMANAGER_H
#define PIERWSZE_COMMUNICATIONMANAGER_H


#include <cstdio>
#include <string>

class CommunicationManager {
public:
    class InternalError : public std::exception {};
    class CommunicationError : public std::exception {};
    constexpr static size_t QUEUE_SIZE = 5;

    explicit CommunicationManager(size_t _port);
    ~CommunicationManager() noexcept;

    void connect();
    void disconnect();
    [[nodiscard]] bool is_connected() const;

    void getline(std::string &buffer);
    void write_data(const std::string &data);
    void putchar(int c);

private:
    bool connected;
    size_t port{};
    FILE *in_socket_file{};
    FILE *out_socket_file{};
    int socket_fd;
};


#endif //PIERWSZE_COMMUNICATIONMANAGER_H
