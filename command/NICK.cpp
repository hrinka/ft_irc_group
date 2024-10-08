#include "../Server.hpp"

void Server::handleNICK(std::string param, ClientData& client) {
  if (isValidNickname(param, client) == true) client.setNickname(param);
}

/*
nicknameは最大20字まで。第一引数以外は無視
Command: NICK
Parameters: <nickname> [ <hopcount> ]
sendCmdResponceの戻り値を0(false)にしている。
*/
bool Server::isValidNickname(std::string& param, const ClientData& client) {
  // paramを第一引数のみにする。
  std::string::size_type pos = 0;
  pos = param.find(" ");
  if (pos != std::string::npos) {
    param = param.substr(0, pos);
    param[pos] = '\0';
  }
  if (client.getNickname().empty() && param.size() == 0)
    return Server::sendCmdResponce(ERR_NONICKNAMEGIVEN, client);
  if (param.size() == 0) {
    std::stringstream ss;
    ss << "Your nickname is " << client.getNickname();
    Server::ft_send(ss.str(),client);
    return false;
  }
  if (param.size() > 20) return Server::sendCmdResponce(ERR_ERRONEUSNICKNAME, param, client);
  for (std::list<ClientData>::iterator it = Server::clients_.begin();it != Server::clients_.end();it++) {
    if (it->getNickname() == param)
      return Server::sendCmdResponce(ERR_NICKNAMEINUSE, param, client);
  }
  std::string special = "[]\\`_^{}|";
  if (!std::isalpha(param[0]) && special.find(param[0]) == std::string::npos)
    return Server::sendCmdResponce(ERR_ERRONEUSNICKNAME, param, client);
  for (size_t i = 1; param[i] != '\0'; i++) {
    if (!std::isalnum(param[i]) && special.find(param[i]) == std::string::npos && param[i] != '-')
      return Server::sendCmdResponce(ERR_ERRONEUSNICKNAME, param, client);
  }
  return true;
}
