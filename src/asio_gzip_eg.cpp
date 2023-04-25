/*!
 * Modified example to send and receive data using a single socket for asynchronous operations (read, write & accept)
 *
 *
 *
 *
 * */





#include <iostream>
#include <string>
#include <memory>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

using namespace boost::asio;
using namespace boost::system;
using namespace std;

std::string make_daytime_string()
{
  using namespace std; // For time_t, time and ctime;
  time_t now = time(0);
  return ctime(&now);
}

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    explicit TcpConnection(ip::tcp::socket socket)
            : socket_(std::move(socket)) {
    }

    void start() {
      asyncRead();
    }

private:
    void asyncRead() {
      auto self = shared_from_this();
      async_read_until(socket_, mBuffer, '\n',
                       [&, self](const boost::system::error_code& error, size_t bytesTransferred) {
          if (!error) {
            std::string message = std::string(boost::asio::buffers_begin(self->mBuffer.data()),
                                              boost::asio::buffers_begin(self->mBuffer.data())
                                              + bytesTransferred);
            std::cout << "Received: " << message;

            // todo: add mtx control for this
            data_ = message;
//            async_write(self->socket_, buffer("You said: " + message), [this](const boost::system::error_code& error, size_t bytesTransferred) {
//                if (!error) {
//                  asyncRead();
//                } else {
//                  std::cerr << "Error writing to socket: " << error.message() << std::endl;
//                }
//
//            });
          } else {
            std::cerr << "Error reading from socket: " << error.message() << std::endl;
            // Close the connection if necessary
          }
          mBuffer.consume(bytesTransferred); // consume the bytes from the buffer

          if(!error)
            this->asyncRead(); // restart the read operation with an emptied buffer
      });
    }

    boost::mutex mtx_;
    std::string data_;
    ip::tcp::socket socket_;
    boost::asio::streambuf mBuffer;
};

class TcpServer {
public:
    TcpServer(io_service& ioService, short port)
            : connections_(),
              acceptor_(ioService, ip::tcp::endpoint(ip::tcp::v4(), port)),
              socket_(ioService) {
      startAccept();
    }

private:
    void startAccept() {
      acceptor_.async_accept(socket_, [this](const boost::system::error_code& error) {
          if (!error) {
            std::cout << "New connection accepted" << std::endl;
            auto connection = std::make_shared<TcpConnection>(std::move(socket_));
            connection->start();
            connections_.push_back(connection);
          }
          startAccept();
      });
    }

    std::vector<std::shared_ptr<TcpConnection>> connections_;
    ip::tcp::acceptor acceptor_;
    ip::tcp::socket socket_;
};

int main() {
  io_service ioService;

  TcpServer server(ioService, 5555);

  boost::asio::io_service::work w(ioService);
  boost::thread worker([&]()->void {
      ioService.run(); // blocks here until all handlers have sorted themselves
  });

  std::cout << "Entering main loop " << std::endl;
  while (1) {

    sleep(1);
  }

  worker.join();
  return 0;
}
