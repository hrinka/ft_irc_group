#include "Server.hpp"

Server::Server() {}
Server::Server(short port) : port_(port), servername_("servername"), hostname_("hostname") {
  std::memset(msg_, 0, sizeof(msg_));
}
Server::~Server() {}
Server::Server(const Server &other) { *this = other; }
Server &Server::operator=(const Server &other) {
  if (this != &other) {
    port_ = other.port_;
    socket_ = other.socket_;
    servername_ = other.servername_;
    hostname_ = other.hostname_;
    size_t i = 0;
    for (i = 0; other.msg_[i] != '\0'; i++) this->msg_[i] = other.msg_[i];
    this->msg_[i] = '\0';
    this->clients_ = other.clients_;
  }
  return (*this);
}

void Server::startServer() {
  struct sockaddr_in sockaddr;
  fd_set read_fds;
  int max_sock = 0;
  struct timeval timeout;
  int sel_ret = 0;
  int client_sock = 0;

  // listenできるところまでsocketを設定
  initServerSocket(sockaddr);
  initSelectArgs(read_fds, max_sock, timeout);
  while (1) {
    setReadfds(read_fds);
    max_sock = getMaxSocket();
    sel_ret = select(max_sock + 1, &read_fds, NULL, NULL, &timeout);
    if (sel_ret < 0) putFunctionError("select failed");
    if (sel_ret == 0) {
      std::cout << "Time out" << std::endl;
      break;
    }
    for (int i = 0; i < sel_ret; i++) {
      if (FD_ISSET(socket_, &read_fds)) {
        // FD_CLR(socket_, &read_fds);
        client_sock = acceptNewClient();
        authenticatedNewClient(client_sock);
        for (size_t i = 0;i < clients_.size();i++)
        {
          if (clients_[i].getSocket() == client_sock)
            break;
        }
        if (i == clients_.size())
         FD_CLR(socket_, &read_fds);
      }
      else if (i < clients_.size() && FD_ISSET(clients_[i].getSocket(), &read_fds))
      {
        // FD_CLR(clients_[i].getSocket(), &read_fds);
        ft_recv(clients_[i].getSocket());
      }
    }
  }
  closeAllSocket();
}

void Server::initServerSocket(struct sockaddr_in &sockaddr) {
  int on = 1;

  if ((socket_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) putFunctionError("socket failed");
  if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    putFunctionError("setsockopt failed");
  std::memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  sockaddr.sin_port = htons(port_);
  if (bind(socket_, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0)
    putFunctionError("bind failed");
  if (listen(socket_, SOMAXCONN) < 0) putFunctionError("listen failed");
  if (fcntl(socket_, F_SETFL, O_NONBLOCK) < 0) putFunctionError("fcntl failed");
}

void Server::initSelectArgs(fd_set &read_fds, int &socket_max, timeval &timeout) {
  FD_ZERO(&read_fds);
  socket_max = getMaxSocket();
  timeout.tv_sec = 5000;
  timeout.tv_usec = 0;
}

void Server::setReadfds(fd_set &read_fds) {
  FD_SET(socket_, &read_fds);
  for (size_t i = 0; i < clients_.size(); i++) FD_SET(clients_[i].getSocket(), &read_fds);
}

int Server::getMaxSocket() {
  int max = 0;
  if (clients_.size() == 0) return socket_;
  for (size_t i = 0; i < clients_.size(); i++) {
    if (max < clients_[i].getSocket()) max = clients_[i].getSocket();
  }
  return max;
}

int Server::acceptNewClient() {
  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);
  int new_client_sock = 0;

  if ((new_client_sock = accept(socket_, (struct sockaddr *)&client_addr, &addr_len)) < 0)
    putFunctionError("accept failed");
  if (fcntl(new_client_sock, F_SETFL, O_NONBLOCK) < 0) putFunctionError("fcntl failed");
  std::cout << "connected sockfd: " << new_client_sock << std::endl;
  return (new_client_sock);
}

size_t Server::ft_recv(int socket) {
  int recv_size = 0;

  while (1) {
    recv_size = recv(socket, msg_, MAX_BUFSIZE, 0);
    if (recv_size < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
      continue;
    else if (recv_size == 0) {
      std::cout << "recv..." << std::endl;
      disconnectClient(socket);
      return 0;
    } else if (recv_size < 0)
      putFunctionError("recv failed");
    else
      break;
  }
  msg_[recv_size - 1] = '\0';
  std::cout << "recieved: " << msg_ << std::endl;
  return recv_size - 1;  // 改行文字分減らす
}

// eraseしたイテレーターを参照しないか確認する！！！
void Server::disconnectClient(ClientData client) {
  int socket = client.getSocket();
  if (clients_.size() == 0) {
    if (close(socket) < 0) putFunctionError("close failed");
  } else {
    std::vector<ClientData>::iterator it = clients_.begin();
    std::vector<ClientData>::iterator erase_it = it;
    while (it != clients_.end()) {
      if (it->getSocket() == socket) break;
      it++;
    }
    if (close(socket) < 0) putFunctionError("close failed");
    clients_.erase(it);
  }
  std::cout << "disconnected sockfd : " << socket << std::endl;
}

std::string::size_type Server::splitCommand(std::string casted_msg, std::string &command) {
  std::string::size_type pos = casted_msg.find(" ");
  if (pos != std::string::npos) {
    command = casted_msg.substr(0, pos);
    command[pos] = '\0';
  } else
    command = casted_msg;
  return pos;
}

void Server::splitParam(std::string casted_msg, std::string &param, std::string::size_type pos) {
  size_t i = 0;
  if (pos == std::string::npos) {
    param.clear();
    return;
  }
  param = casted_msg.substr(pos + 1);
}

void Server::ft_send(ClientData client, size_t send_size) {
  int send_ret = 0;

  while (1) {
    send_ret = send(client.getSocket(), msg_, send_size, 0);
    if (send_ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
      continue;
    else if (send_ret == 0) {
      msg_[0] = '\0';
      std::cout << "send..." << std::endl;
      disconnectClient(client);
      return;
    } else if (send_ret < 0)
      putFunctionError("send failed");
    else
      break;
  }
}

size_t Server::createSendMsg(const std::string &casted_msg) {
  std::memset(msg_, 0, sizeof(msg_));
  size_t i = 0;
  while (casted_msg[i] != '\0' && i < MAX_BUFSIZE) {
    msg_[i] = casted_msg[i];
    i++;
  }
  msg_[i] = '\r';
  msg_[i + 1] = '\n';
  return i + 2;
}

int Server::sendCmdResponce(int code, const std::string &str, const ClientData &client) {
  std::string resp_msg;
  size_t send_size = 0;

  resp_msg = createCmdRespMsg(servername_, code, str);
  send_size = createSendMsg(resp_msg);
  ft_send(client, send_size);
  return 0;
}

int Server::sendCmdResponce(int code, const ClientData &client) {
  std::string resp_msg;
  size_t send_size = 0;

  resp_msg = createCmdRespMsg(servername_, code);
  send_size = createSendMsg(resp_msg);
  ft_send(client, send_size);
  return 0;
}

void Server::putFunctionError(const char *errmsg) {
  perror(errmsg);
  closeAllSocket();
  throw std::exception();
}

void Server::closeAllSocket() {
  for (size_t i = 0; i < clients_.size(); i++) {
    if (close(clients_[i].getSocket()) < 0) perror("close failed");
  }
  if (close(socket_) < 0) perror("close failed");
}

const std::string &Server::getServername() const { return servername_; }

const std::string &Server::getHostname() const { return hostname_; }
