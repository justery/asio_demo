/*
 * Implementation of async tcp server working by single-thread.
 * */


#include <ctime>
#include <iostream>
#include <string>
#include <mutex>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <thread>
#include <boost/shared_array.hpp>
#include <list>

#include "util.hpp"


using namespace boost;
using namespace boost::asio;
typedef boost::shared_ptr<ip::tcp::socket> socket_ptr;
typedef boost::shared_ptr<io_service::work> work_ptr;


io_service service;
ip::tcp::endpoint ep(ip::tcp::v4(), 13); // 监听端口2001
ip::tcp::acceptor acc(service, ep);

struct Head{
    char hd[4];
};

struct Msg {
    boost::shared_array<char> shb;
    boost::shared_array<char> sbb;
};

std::recursive_mutex mtx_;
std::list<Msg> msg_que_;

char buff_write[1024];

void start_accept();
void write_handler(socket_ptr sock, const system::error_code& ec);
void on_write( const boost::system::error_code &err, std::size_t bytes);
void on_read(socket_ptr sock, boost::shared_array<char> shb, const boost::system::error_code &err, std::size_t bytes);


void on_read_body(socket_ptr sock, boost::shared_array<char> shb, boost::shared_array<char> sbb, const boost::system::error_code &err, std::size_t bytes) {
    if (err) {//error::eof   = 2
        std::cout << "on_read err.what = " << err << " bytes: " << bytes << std::endl;
        sock->close();
        return;
    }

    for (auto i = 0; i < sizeof(Head); ++i) {
        std::cout << shb[i] << " ";
    }
    for (auto i = 0; i < bytes; ++i) {
        std::cout << sbb[i] << " ";
    }
    std::cout << std::endl;

    {
        mtx_.lock();
        msg_que_.emplace_back(Msg{shb, sbb});
        mtx_.unlock();
    }

    boost::shared_array<char> shared_buff(new char[sizeof(Head)]);

    boost::asio::async_read(*sock.get(),
        buffer(shared_buff.get(), sizeof(Head)),
        boost::asio::transfer_exactly(sizeof(Head)),
        boost::bind(on_read, sock, shared_buff, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void on_read(socket_ptr sock, boost::shared_array<char> shb, const boost::system::error_code &err, std::size_t bytes) {
    if (err) {//error::eof   = 2
        std::cout << "on_read err.what = " << err << " bytes: " << bytes << std::endl;
        sock->close();
        return;
    }

    std::string str(shb.get(), bytes);
    std::cout << bytes<< "\n#svr got: " << str << std::endl;

    int body_len = 4;
    if (auto c = shb[bytes-1]; c >= '0' && c <= '9')
        body_len += 4 * (c - '0');

    boost::shared_array<char> shared_buff(new char[body_len]);

    boost::asio::async_read(*sock.get(),
        buffer(shared_buff.get(),body_len),
        boost::asio::transfer_exactly(body_len),
        boost::bind(on_read_body, sock, shb, shared_buff, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void create_session(socket_ptr sock, const boost::system::error_code &err) {
    if (err) {//error::eof   = 2
        std::cout << "create_session err.what = " << err << std::endl;
        sock->close();
        return;
    }

    boost::shared_array<char> shared_buff(new char[sizeof(Head)]);
    boost::asio::async_read(*sock.get(), buffer(shared_buff.get(),sizeof(Head)), transfer_exactly(sizeof(Head)), boost::bind(on_read, sock, shared_buff, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void handle_accept(socket_ptr sock, const boost::system::error_code &err) {
    if (err){
        std::cout << __FUNCTION__ << " err not nil." << std::endl;
        return;
    }

    create_session(sock, err);

    start_accept();
}

void start_accept() {
    socket_ptr sock(new ip::tcp::socket(service));
    acc.async_accept(*sock, boost::bind( handle_accept, sock, _1) );
}

int main() {
    start_accept();
    std::cout << "main pid = " << getpid() << std::endl;
    //work_ptr dummy_work(new io_service::work(service));

    thread t([](){
        for(;;) {
            std::cout << "main pid = "<< getpid() << " " << make_daytime_string() << std::endl;

            if (!msg_que_.empty()) {

                bool a = false;
                mtx_.lock();
                if (!msg_que_.empty()) {
                    auto msg = msg_que_.front();
                    msg_que_.pop_front();
                    a = true;
                }

                mtx_.unlock();

                if (a)
                    std::cout << "main  get msg. pid = " << getpid() << std::endl;
            }
            else {
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }

    });

    t.detach();

    service.run();
}


void on_write( const boost::system::error_code &err, std::size_t bytes) {
    std::cout << "on_write pid = " << getpid() << std::endl;
}

void write_handler(socket_ptr sock, const system::error_code& err){
    if (err) {//error::eof   = 2
        std::cout << "write_handler err.what = " << err << std::endl;
        sock->close();
        return;
    }

    std::cout << "write_handler pid = " << getpid() << std::endl;
    //sock->async_read_some(buffer(buff_read,4),bind(on_read, sock, boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred));
    //boost::asio::async_read(*sock.get(),buffer(buff_read), boost::asio::transfer_exactly(4), boost::bind(on_read, sock, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}