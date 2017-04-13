/**
 * @file LibraryCollisionTest.cpp
 *
 * @date Nov 18, 2016
 * @author edward
 */

#ifdef LIBRARY_TEST

#include "TestSerializableObject.h"
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

int main(int argc, char** argv) {
    auto global_logger = spdlog::rotating_logger_mt("global_logger", "simulation-log", 1024 * 1024 * 500, 3);
    global_logger->set_pattern("[%H:%M:%S.%e] [%l] %v");
    global_logger->set_level(spdlog::level::trace);

    std::shared_ptr<spdlog::logger> logger(spdlog::get("global_logger"));

    auto message = std::make_shared<messaging::TestSerializableObject>(1, 2, "A message!");

    logger->trace("Received a message: {}", *message);
}

#endif
