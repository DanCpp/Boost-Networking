#include <iostream>
#include <memory>
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
using namespace boost::posix_time;
io_service service;
ip::tcp::endpoint ep(ip::make_address("127.0.0.1"), 8000);

struct talker;
typedef boost::shared_ptr<talker> client_ptr;
typedef std::vector<client_ptr> array;
array clients;
// thread-safe access to clients array
boost::recursive_mutex cs;



void client_handler();

void accept_thread();

void update_clients();

enum Packet {
    msg = 0,
    file,
    login,
    client,
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
    Packet get_packet(const std::string& msg)
    {
        if (!msg.rfind("\\") || !msg.rfind("/")) return Packet::file;
        if (!msg.find("login ")) return Packet::login;
        if (!msg.find("ping")) return Packet::ping;
        if (!msg.find("ask_clients")) return Packet::client;
        return Packet::msg;
    }
    void proccess_packet(Packet packettype, std::string msg)
    {
        switch (packettype)
        {
        case Packet::file:
        {
            break;
        }
        case Packet::msg:
        {
            write(msg);
            break;
        }
        case Packet::client:
        {
            on_clients();
            break;
        }
        case Packet::ping:
        {
            on_ping();
            break;
        }
        case Packet::login:
        {
            on_login(msg);
            break;
        }
        default:
            throw new std::exception("Unrecognized packet");
        }
    }
    size_t on_read_message()
    {
        boost::system::error_code error;
        size_t len = sock_.read_some(boost::asio::buffer(buff_), error);
        if (error) throw new boost::system::system_error(error);
        return len;
    }
    void on_login(const std::string& msg)
    {
        std::istringstream in(msg);
        in >> username_ >> username_;
        std::cout << username_ << " logged in" << std::endl;
        write("login ok\n");
        update_clients();
    }
    void on_ping()
    {
        write(clients_changed_ ? "ping client_list_changed\n" : "ping ok\n");
        clients_changed_ = false;
    }
    void on_clients()
    {
        std::string msg;
        { boost::recursive_mutex::scoped_lock lk(cs);
        for (array::const_iterator b = clients.begin(), e = clients.end(); b != e; ++b)
            msg += (*b)->username() + " ";
        }
        write("clients " + msg + "\n");
    }
    void write(const std::string& msg) {
        sock_.write_some(buffer(msg));
    }
    //public methods
public:
    std::string username() const { return username_; }
    void set_clients_changed() { clients_changed_ = true; }
    ip::tcp::socket& sock() { return sock_; }
    void send_answer() {
        try
        {
            size_t len = on_read_message();
            Packet packettype = get_packet(std::string(buff_));
            proccess_packet(packettype, std::string(buff_));
        }
        catch (boost::system::system_error&)
        {
            stop();
        }
        catch (std::exception&)
        {
            stop();
        }
    }
};

void client_handler()
{
    while (true) {
        boost::this_thread::sleep(millisec(1));
        boost::recursive_mutex::scoped_lock lk(cs);
        for (array::iterator b = clients.begin(), e = clients.end(); b != e; ++b)
            (*b)->send_answer();
    }
}

void accept_thread()
{
    ip::tcp::acceptor acceptor(service, ip::tcp::endpoint(ip::make_address("127.0.0.1"), 8000));
    while (true) {
        client_ptr new_(new talker);
        acceptor.accept(new_->sock());

        boost::recursive_mutex::scoped_lock lk(cs);
        clients.push_back(new_);
    }
}

void update_clients()
{
    boost::recursive_mutex::scoped_lock lk(cs);
    for (array::iterator b = clients.begin(), e = clients.end(); b != e; ++b)
        (*b)->set_clients_changed();
}

int main(int argc, char* argv[]) {
    boost::thread_group threads;
    threads.create_thread(accept_thread);
    threads.create_thread(client_handler);
    threads.join_all();
}