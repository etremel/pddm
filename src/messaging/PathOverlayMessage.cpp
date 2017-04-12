/**
 * @file PathOverlayMessage.cpp
 *
 * @date Oct 14, 2016
 * @author edward
 */

#include "PathOverlayMessage.h"

#include <cstring>
#include <ostream>
#include <sstream>
#include <string>
#include <mutils-serialization/SerializationSupport.hpp>

#include "../util/OStreams.h"

namespace pddm {
namespace messaging {

const constexpr MessageBodyType PathOverlayMessage::type;

std::ostream& operator<< (std::ostream& out, const PathOverlayMessage& message) {
    std::stringstream super_streamout;
    super_streamout << static_cast<OverlayMessage>(message);
    std::string output_string = super_streamout.str();
    std::stringstream path_string_builder;
    path_string_builder << "|RemainingPath=" << message.remaining_path;
    output_string.insert(output_string.find("|Body="), path_string_builder.str());
    return out << output_string;
}


std::size_t PathOverlayMessage::to_bytes(char* buffer) const {
    std::size_t bytes_written = 0;
    bytes_written += mutils::to_bytes(type, buffer);
    bytes_written += mutils::to_bytes(remaining_path, buffer + bytes_written);
    bytes_written += to_bytes_common(buffer + bytes_written);
    return bytes_written;
}

//Is it OK to implement post_object this way?? it seems much easier than recursive post_object calls
void PathOverlayMessage::post_object(const std::function<void(const char* const, std::size_t)>& function) const {
    char buffer[bytes_size()];
    to_bytes(buffer);
    function(buffer, bytes_size());
}

std::size_t PathOverlayMessage::bytes_size() const {
    //The superclass bytes_size already includes the size of a MessageBodyType
    return OverlayMessage::bytes_size() + mutils::bytes_size(remaining_path);
}

std::unique_ptr<PathOverlayMessage> PathOverlayMessage::from_bytes(mutils::DeserializationManager* m, char const * buffer) {
    std::size_t bytes_read = 0;
    MessageBodyType type;
    std::memcpy(&type, buffer + bytes_read, sizeof(type));
    bytes_read += sizeof(type);
    assert(type == MessageBodyType::PATH_OVERLAY);

    auto constructed_message = std::unique_ptr<PathOverlayMessage>(new PathOverlayMessage());
    auto deserialized_list = mutils::from_bytes<std::list<int>>(nullptr, buffer + bytes_read);
    constructed_message->remaining_path = *deserialized_list;
    bytes_read += mutils::bytes_size(constructed_message->remaining_path);
    bytes_read += OverlayMessage::from_bytes_common(*constructed_message, buffer + bytes_read);
    return constructed_message;
}

}
}
