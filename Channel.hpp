#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>
#include <set>
#include "ClientData.hpp" // ClientDataクラスを使うためにインクルード


class ClientData;
class Channel {
public:
    Channel(const std::string& name);
    ~Channel();

    void addClient(ClientData* client);
    void removeClient(ClientData* client);
    bool isOperator(ClientData* client) const;
    void broadcastMessage(const std::string& message, ClientData* sender = nullptr);

    void kickClient(ClientData* client, ClientData* target, const std::string& reason);
    void inviteClient(ClientData* client, ClientData* target);
    void setTopic(ClientData* client, const std::string& topic);
    const std::string& getTopic() const;
    const std::vector<ClientData*>& getClients() const;
    void setMode(ClientData* client, char mode, bool enable);

private:
    std::string name_;
    std::string topic_;
    std::vector<ClientData*> clients_;  // ClientData型を使う
    std::vector<ClientData*> operators_;  // ClientData型を使う
};

#endif // CHANNEL_HPP
