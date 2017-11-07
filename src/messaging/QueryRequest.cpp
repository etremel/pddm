/**
 * @file QueryRequest.cpp
 *
 * @date Oct 14, 2016
 * @author edward
 */

#include <ostream>
#include <memory>
#include <mutils-serialization/SerializationSupport.hpp>

#include "QueryRequest.h"

namespace pddm {
namespace messaging {

//I can't believe there's no built-in function for printing out the value of an enum class. And they say Java is full of boilerplate?
std::ostream& operator<<(std::ostream& out, const QueryType& type) {
    switch(type) {
    case QueryType::CURR_USAGE_SUM:
        return out << "CURR_USAGE_SUM";
    case QueryType::CURR_USAGE_HISTOGRAM:
        return out << "CURR_USAGE_HISTOGRAM";
    case QueryType::AVAILABLE_OFFSET_BREAKDOWN:
        return out << "AVAILABLE_OFFSET_BREAKDOWN";
    case QueryType::CUMULATIVE_USAGE:
        return out << "CUMULATIVE_USAGE";
    case QueryType::PROJECTED_SUM:
        return out << "PROJECTED_SUM";
    case QueryType::PROJECTED_HISTOGRAM:
        return out << "PROJECTED_HISTOGRAM";
    default:
        return out;
    }
}

const constexpr MessageType QueryRequest::type;

std::size_t QueryRequest::bytes_size() const {
    return mutils::bytes_size(type) +
            mutils::bytes_size(sender_id) +
            mutils::bytes_size(request_type) +
            mutils::bytes_size(time_window) +
            mutils::bytes_size(query_number);// +
//            mutils::bytes_size(proposed_price_function);
}

std::size_t QueryRequest::to_bytes(char* buffer) const {
    std::size_t bytes_written = mutils::to_bytes(type, buffer);
    bytes_written += mutils::to_bytes(sender_id, buffer + bytes_written);
    bytes_written += mutils::to_bytes(request_type, buffer + bytes_written);
    bytes_written += mutils::to_bytes(time_window, buffer + bytes_written);
    bytes_written += mutils::to_bytes(query_number, buffer + bytes_written);
//    bytes_written += mutils::to_bytes(proposed_price_function, buffer + bytes_written);
    return bytes_written;

}

void QueryRequest::post_object(const std::function<void(const char* const, std::size_t)>& function) const {
    mutils::post_object(function, type);
    mutils::post_object(function, sender_id);
    mutils::post_object(function, request_type);
    mutils::post_object(function, time_window);
    mutils::post_object(function, query_number);
//    mutils::post_object(function, proposed_price_function);

}

std::unique_ptr<QueryRequest> QueryRequest::from_bytes(mutils::DeserializationManager<>* m, const char* buffer) {
    std::size_t bytes_read = 0;
    MessageType message_type;
    std::memcpy(&message_type, buffer + bytes_read, sizeof(MessageType));
    bytes_read += sizeof(MessageType);

    int sender_id;
    std::memcpy(&sender_id, buffer + bytes_read, sizeof(sender_id));
    bytes_read += sizeof(sender_id);

    QueryType req_type;
    std::memcpy(&req_type, buffer + bytes_read, sizeof(req_type));
    bytes_read += sizeof(req_type);

    int time_window;
    std::memcpy(&time_window, buffer + bytes_read, sizeof(time_window));
    bytes_read += sizeof(time_window);

    int query_number;
    std::memcpy(&query_number, buffer + bytes_read, sizeof(query_number));
    bytes_read += sizeof(query_number);

    //price function???
    return std::make_unique<QueryRequest>(req_type, time_window, query_number);

}

std::ostream& operator<<(std::ostream& out, const QueryRequest& qr) {
    return out << "{QueryRequest: Type=" << qr.request_type << " | query_number=" << qr.query_number << " | time_window=" << qr.time_window << " }";
}

} /* namespace messaging */
} /* namespace pddm */

