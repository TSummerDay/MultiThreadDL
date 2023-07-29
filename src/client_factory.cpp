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

  ClientFactory() {
    registerProtocol("http", [] { return std::make_shared<HttpClient>(); });
    registerProtocol("https", [] { return std::make_shared<HttpClient>(); });
  }
  ~ClientFactory() = default;

  std::shared_ptr<Client> getClient(const std::string &protocol) const {
    auto it = clients_.find(protocol);
    if (it != clients_.end()) {
      return it->second();
    } else {
      std::cerr << "Unsupported protocol : " << protocol << std::endl;
      return nullptr;
    }
  }

private:
  // Register the corresponding client to client factory
  void registerProtocol(const std::string &protocol, ClientCreator createFunc) {
    clients_[protocol] = std::move(createFunc);
  }
  std::unordered_map<std::string, ClientCreator> clients_;
};

// curl instance is thread-safe
// A curl can execute only one request task at a time
// Make sure that each thread has an instance of curl
std::shared_ptr<Client> get_clients(const std::string &protocol) {
  static ClientFactory cf;
  return cf.getClient(protocol);
}
} // namespace mltdl