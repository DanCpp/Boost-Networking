#include <iostream>
#ifdef WIN32
#define _WIN32_WINNT 0x0501
#include <stdio.h>
#endif


#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
using namespace boost::asio;
io_service service;

ip::tcp::endpoint ep(ip::make_address("127.0.0.1"), 8000);
enum Packet {
    msg = 0,
    file,
    login,
    client,
    ping,
};

struct talker
{
    talker(const std::string& username)
        : sock_(service), started_(true), username_(username) {}
    void connect(ip::tcp::endpoint ep) {
        sock_.connect(ep);
    }
    void loop() {
        // read answer to our login
        write("login " + username_ + "\n");
        read_answer();
        while (started_) {
            std::string msg;
            std::getline(std::cin, msg);
            msg = username_ + ": " + msg;
            write_request(msg);
            read_answer();
            int millis = rand() % 7000;
            boost::this_thread::sleep(boost::posix_time::millisec(millis));
        }
    }
    std::string username() const { return username_; }
private:
    void write_request(std::string msg) {
        write(msg);
    }
    void read_answer() {
        read_ = 0;
        read(sock_, buffer(buff),
            boost::bind(&talker::read_complete, this, _1, _2));
        process_msg();
    }
    void process_packet(Packet packettype, const std::string& msg) {
        switch (packettype)
        {
        case Packet::file:
        {
            break;
        }
        case Packet::msg:
        {
            for (int i = 0; read_; --read_, ++i)
            {
                std::cout << msg[i];
            }
            std::cout << "\n";
            break;
        }
        case Packet::client:
        {
            on_clients(msg);
            break;
        }
        case Packet::ping:
        {
            on_ping(msg);
            break;
        }
        case Packet::login:
        {
            on_login();
            break;
        }
        default:
            throw new std::exception("Unrecognized packet");
        }
    }
    Packet get_typeofpacket(const std::string& msg)
    {
        if (!msg.rfind("\\") || !msg.rfind("/")) return Packet::file;
        if (!msg.find("login ")) return Packet::login;
        if (!msg.find("ping")) return Packet::ping;
        if (!msg.find("ask_clients")) return Packet::client;
        return Packet::msg;
    }
    void process_msg() {
        std::string msg(buff);
        Packet packettype = get_typeofpacket(msg);
        process_packet(packettype, msg);
    }

    void on_login() {
        std::cout << username_ << " logged in" << std::endl;
        do_ask_clients();
    }
    void on_ping(const std::string& msg) {
        std::istringstream in(msg);
        std::string answer;
        in >> answer >> answer;
        if (answer == "client_list_changed")
            do_ask_clients();
    }
    void on_clients(const std::string& msg) {
        std::string clients = msg.substr(8);
        std::cout << username_ << ", new client list:" << clients;
    }
    void do_ask_clients() {
        write("ask_clients\n");
        read_answer();
    }

    void write(const std::string& msg) {
        sock_.write_some(buffer(msg));
    }
    size_t read_complete(const boost::system::error_code& err, size_t bytes) {
        if (err) return 0;
        read_ = bytes;
        bool found = std::find(buff_, buff_ + bytes, '\n') < buff_ + bytes;
        // we read one-by-one until we get to enter, no buffering
        return found ? 0 : 1;
    }

private:
    ip::tcp::socket sock_;
    enum { max_msg = 1024 };
    char buff_[max_msg];
    std::string buff;
    std::string username_;
    int read_ = 0;
    bool started_;
};


void run_client(const std::string& client_name) {
    talker client(client_name);
    try {
        client.connect(ep);
        client.loop();
    }
    catch (boost::system::system_error& err) {
        std::cout << "client terminated " << client.username()
            << ": " << err.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "rus");
    run_client("Danil");
}