#include "talker.hpp"

void talker::on_clients()
{
    std::string msg;
    { boost::recursive_mutex::scoped_lock lk(cs);
    for (array::const_iterator b = clients.begin(), e = clients.end(); b != e; ++b)
        msg += (*b)->username() + " ";
    }
    write("clients " + msg + "\n");
}

void talker::on_ping()
{
    write(clients_changed_ ? "ping client_list_changed\n" : "ping ok\n");
    clients_changed_ = false;
}

void talker::on_login(const std::string& msg)
{
    std::istringstream in(msg);
    in >> username_ >> username_;
    std::cout << username_ << " logged in" << std::endl;
    write("login ok\n");
    update_clients();
}

size_t talker::on_read_message()
{
    boost::system::error_code error;
    size_t len = sock_.read_some(boost::asio::buffer(buff_), error);
    if (error) throw new boost::system::system_error(error);
    return len;
}

Packet talker::get_packet(const std::string& msg)
{
    if (msg.rfind("\\") || msg.rfind("/")) return Packet::file;
    if (msg.find("login ")) return Packet::login;
    if (msg.find("ping")) return Packet::ping;
    if (msg.find("ask_clients")) return Packet::clients;
    return Packet::msg;
}

void talker::proccess_packet(Packet packettype, std::string msg)
{
    switch (packettype)
    {
        case Packet::file:
        {
            break;
        }
        case Packet::msg:
        {
            sock_.write_some(buffer(buff_));
            break;
        }
        case Packet::clients:
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
        }
    }
}

void talker::send_answer()
{
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
}