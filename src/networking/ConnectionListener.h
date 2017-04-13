/**
 * @file ConnectionListener.h
 *
 * @date Oct 12, 2016
 * @author edward
 */

#pragma once

#include <memory>
#include <functional>

namespace pddm {
namespace networking {

class Socket;

class ConnectionListener {
        std::unique_ptr<int, std::function<void(int*)>> fd;
    public:
        explicit ConnectionListener(int port);
        Socket accept();
};

} /* namespace networking */
} /* namespace pddm */

