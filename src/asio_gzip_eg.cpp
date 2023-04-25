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
    TcpConnection(ip::tcp::socket socket)
            : mSocket(std::move(socket)) {
    }

    void start() {
      asyncRead();
    }

private:
    void asyncRead() {
      async_read_until(mSocket, mBuffer, '\n',
                       [&, self = shared_from_this()](const boost::system::error_code& error, size_t bytesTransferred) {
          if (!error) {
            std::string message = std::string(boost::asio::buffers_begin(self->mBuffer.data()), boost::asio::buffers_begin(self->mBuffer.data()) + bytesTransferred);
            std::cout << "Received: " << message;

            async_write(self->mSocket, buffer("You said: " + message), [this, self](const boost::system::error_code& error, size_t bytesTransferred) {
                if (!error) {
                  asyncRead();
                } else {
                  std::cerr << "Error writing to socket: " << error.message() << std::endl;
                }

            });
          } else {
            std::cerr << "Error reading from socket: " << error.message() << std::endl;
            // Close the connection if necessary
          }
          mBuffer.consume(bytesTransferred);
      });
    }

    ip::tcp::socket mSocket;
    boost::asio::streambuf mBuffer;
};

class TcpServer {
public:
    TcpServer(io_service& ioService, short port)
            : mAcceptor(ioService, ip::tcp::endpoint(ip::tcp::v4(), port)), mSocket(ioService) {
      startAccept();
    }

private:
    void startAccept() {
      mAcceptor.async_accept(mSocket, [this](const boost::system::error_code& error) {
          if (!error) {
            std::cout << "New connection accepted" << std::endl;
            auto connection = std::make_shared<TcpConnection>(std::move(mSocket));
            connection->start();
          }
          startAccept();
      });
    }

    ip::tcp::acceptor mAcceptor;
    ip::tcp::socket mSocket;
};

int main() {
  io_service ioService;

  TcpServer server(ioService, 5555);

  ioService.run(); // blocks here until all handlers have sorted themselves

  return 0;
}
