/*!
 * Modified example to send and receive data using a single socket for asynchronous operations (read & accept)
 * but synchronous writing (user implemented)
 *
 *
 *
 * */

#include <iostream>
#include <string>
#include <memory>
#include <queue>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <fstream>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/lzma.hpp>


using namespace boost::asio;
using namespace boost::system;
using namespace std;

std::string make_daytime_string()
{
  using namespace std; // For time_t, time and ctime;
  time_t now = time(0);
  return ctime(&now);
}

#define MAX_LEN 750 // emulate low bandwidth transmission medium


class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    explicit TcpConnection(ip::tcp::socket socket)
            : socket_(std::move(socket)), is_open_(true) {
    }

    void start() {
      asyncRead();
    }

    std::string getData() {
      mtx_.lock();
      std::string re_data(data_);
      data_.clear();
      mtx_.unlock();
      return re_data;
    }

    //synchronous data write
    size_t Send(const std::string& message) {
      size_t written = 0;
      if(is_open_) {
        boost::system::error_code ec;
        written = boost::asio::write(socket_, boost::asio::buffer(message), ec);
        if(ec)
          std::cout << "Error writing to socket: " << ec.message() << std::endl;
      }
      return written;
    }

    bool IsOpen() {
      return is_open_;
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
            mtx_.lock();
            data_ = message;
            mtx_.unlock();
          } else {
            std::cerr << "Error reading from socket: " << error.message() << std::endl;
            // Close the connection if necessary
          }
          mBuffer.consume(bytesTransferred); // consume the bytes from the buffer

          if(!error)
            this->asyncRead(); // restart the read operation with an emptied buffer
          else{
            socket_.cancel();
            socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
            if(socket_.is_open())
              socket_.close();
            is_open_ = false;
          }
      });
    }

    boost::mutex mtx_;
    std::string data_;
    ip::tcp::socket socket_;
    boost::asio::streambuf mBuffer;
    std::atomic<bool> is_open_;
    std::function<void(const std::string&)> callback_;
};

class TcpServer {
public:
    TcpServer(io_service& ioService, short port)
            : connections_(),
              acceptor_(ioService, ip::tcp::endpoint(ip::tcp::v4(), port)),
              socket_(ioService) {
      startAccept();
    }

    std::vector<std::pair<std::string, std::shared_ptr<TcpConnection>>> getCurrentData() {
      std::vector<std::pair<std::string, std::shared_ptr<TcpConnection>>> ret_data;
      ret_data.reserve(connections_.size());
      for(const auto &c: connections_){
        ret_data.emplace_back(c->getData(), c);
      }
      return ret_data;
    }

    bool Send(const std::string &msg) {
      bool ret_val = false;
      for (const auto &p: connections_) {
          if(p->Send(msg) != 0)
            ret_val = true; // at least one connection sent data
      }
      return ret_val;
    }

    void removeClosedConnections () {
      auto itr = connections_.begin();
      while (itr != connections_.end()) {
        if(!itr->get()->IsOpen()){
          itr = connections_.erase(itr);
          std::cout << "Connection closed, removing" << std::endl;
        } else
            ++itr;
      }
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

  std::vector<std::shared_ptr<TcpConnection>> connections;
  // Load large amount of data here
  std::ifstream input_file("nav_data.csv", std::ios::binary);
  boost::iostreams::filtering_ostream gzip_stream;
  std::stringstream compressed_data;
  gzip_stream.push(boost::iostreams::gzip_compressor ());
  gzip_stream.push(compressed_data);

  gzip_stream << input_file.rdbuf();

//  const std::string to_send = "Start time of Server: " + make_daytime_string() + ". Let's goooooo";

  const std::string to_send = compressed_data.str();

  std::cout << "Data size: " << to_send.size() << " bytes\n";

  std::queue<std::string> q;
  size_t total_bytes_sent = 0;
  bool large_data_transfer = false;

  while (1) {
    // Remove any connections that have been closed
    server.removeClosedConnections();

    auto data = server.getCurrentData();
    for(const auto & d: data)
      if(!d.first.empty()) {
        cout << "last available data: " << d.first << std::endl;
        if(d.first == "start\n") { // begin the data transmission of compressed data till all is sent properly
          // lets grab the connection that asked for the data transmission
          connections.push_back(d.second);
          size_t offset = 0;
          // split up the data into packets and place into a container to clear
          while (offset < to_send.size()) {
            size_t packet_size = std::min(static_cast<size_t>(MAX_LEN), to_send.size() - offset);
            q.push(to_send.substr(offset, packet_size));
            offset += packet_size;
          }
          large_data_transfer = true;
        }
      }

    if(!q.empty()) {
      auto msg = q.front();
      cout << "Sending: " << msg << endl;
      for(const auto & c: connections) {
        total_bytes_sent+=c->Send(msg);
      }
      q.pop();
    } else if(q.empty() && large_data_transfer) {
      // we've finished, so we can clear it up and confirm the amount of data we wanted to send was sent
      cout << "Data transferred: " << total_bytes_sent << ", expected size: " << to_send.size() << std::endl;
      total_bytes_sent = 0; // reset
      large_data_transfer = false;
      connections.clear();
    }

//    if(!server.Send(make_daytime_string()))
//      cout << "No requests for data received" << std::endl;

    usleep(100000);
  }

  worker.join();
  return 0;
}
