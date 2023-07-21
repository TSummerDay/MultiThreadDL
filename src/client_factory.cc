#include "client_factory.h"
#include "client.h"
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace mltdl {
class ClientFactory {
public:
  using ClientCreator = std::function<std::shared_ptr<Client>()>;

  static ClientFactory *getInstance() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (ins_ == nullptr) {
      ins_ = new ClientFactory();
    }
    return ins_;
  }
  std::shared_ptr<Client> getClient(const std::string &protocol) const {
    auto it = clients_.find(protocol);
    if (it != clients_.end()) {
      return it->second();
    } else {
      std::cerr << "Unsupported protocol : " << protocol << std::endl;
      return nullptr;
      // throw std::runtime_error("Unsupported protocol");
    }
  }

private:
  ClientFactory() {
    registerProtocol("http", [] { return std::make_shared<HttpClient>(); });
    registerProtocol("https", [] { return std::make_shared<HttpClient>(); });
  }
  ~ClientFactory() = default;

  void registerProtocol(const std::string &protocol, ClientCreator createFunc) {
    clients_[protocol] = std::move(createFunc);
  }
  //   std::shared_ptr<Client> createHttpClient() {
  //     return std::make_shared<HttpClient>();
  //   }
  std::unordered_map<std::string, ClientCreator> clients_; /*= {
      {"http", [] { return std::make_shared<HttpClient>(); }},
      {"https", [] { return std::make_shared<HttpClient>(); }}};*/
  static ClientFactory *ins_;
  static std::mutex mutex_;
};

ClientFactory *ClientFactory::ins_ = nullptr;
std::mutex ClientFactory::mutex_;

std::shared_ptr<Client> get_clients(const std::string &protocol) {
  return ClientFactory::getInstance()->getClient(protocol);
}
} // namespace mltdl