#pragma once
#include "libs.hpp"
#include "handlers.hpp"
enum Packet {
    msg = 0,
    file,
    login,
    clients,
    ping,
};

struct talker : boost::enable_shared_from_this<talker>
{
//static methods
    talker()
        : sock_(service) {}
    void stop() {
        // close client connection
        boost::system::error_code err;
        sock_.close(err);
    }
//private fields
private:
    ip::tcp::socket sock_;
    enum { max_msg = 1024 };

    char buff_[max_msg];
    std::string username_;
    bool clients_changed_;
//private methods
private:
    Packet get_packet(const std::string& msg);
    void proccess_packet(Packet packettype, std::string msg);
    size_t on_read_message();
    void on_login(const std::string& msg);
    void on_ping();
    void on_clients();
    void write(const std::string& msg) {
        sock_.write_some(buffer(msg));
    }
//public methods
public:
    std::string username() const { return username_; }
    void set_clients_changed() { clients_changed_ = true; }
    ip::tcp::socket& sock() { return sock_; }
    void send_answer();
};

