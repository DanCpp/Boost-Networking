#include "handlers.hpp"
#include "libs.hpp"
#include "talker.hpp"

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
    ip::tcp::acceptor acceptor(service, ip::tcp::endpoint(ip::tcp::v4(), 8001));
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

