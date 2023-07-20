#pragma once

#include <memory>

namespace mltdl {
class Client;
std::shared_ptr<Client> get_clients(const std::string &protocol);
} // namespace mltdl