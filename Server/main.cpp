#include "libs.hpp"
#include "handlers.hpp"
#include "talker.hpp"


int main(int argc, char* argv[]) {
    boost::thread_group threads;
    threads.create_thread(accept_thread);
    threads.create_thread(client_handler);
    threads.join_all();
}