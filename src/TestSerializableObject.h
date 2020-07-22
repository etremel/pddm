/**
 * @file TestSerializableObject.h
 *
 * @date Nov 18, 2016
 * @author edward
 */

#pragma once

#include <mutils-serialization/SerializationSupport.hpp>
#include <cstddef>
#include <cstring>
#include <functional>
#include <memory>
#include <ostream>
#include <string>

namespace messaging {

class TestSerializableObject : public mutils::ByteRepresentable {
    public:
        int sender_id;
        int message_num;
        std::string message;

        TestSerializableObject(const int sender_id, const int message_num, const std::string& message) :
            sender_id(sender_id), message_num(message_num), message(message) {}

        std::size_t bytes_size() const {
            return mutils::bytes_size(sender_id) +
                    mutils::bytes_size(message_num) +
                    mutils::bytes_size(message);
        }

        std::size_t to_bytes(char* buffer) const {
            std::size_t s = mutils::to_bytes(sender_id, buffer);
            s += mutils::to_bytes(message_num, buffer + s);
            s += mutils::to_bytes(message, buffer + s);
            return s;
        }

        void post_object(const std::function<void(const char* const, std::size_t)>& f) const {
            mutils::post_object(f, sender_id);
            mutils::post_object(f, message_num);
            mutils::post_object(f, message);
        }

        static std::unique_ptr<TestSerializableObject> from_bytes(mutils::DeserializationManager* m, const char * buffer) {
            std::size_t read = 0;
            int sender_id;
            std::memcpy(&sender_id, buffer + read, sizeof(sender_id));
            read += sizeof(sender_id);
            int message_num;
            std::memcpy(&message_num, buffer + read, sizeof(message_num));
            read += sizeof(message_num);
            auto message = mutils::from_bytes<std::string>(m, buffer + read);
            return std::make_unique<TestSerializableObject>(sender_id, message_num, *message);
        }
        void ensure_registered(mutils::DeserializationManager&) {}

};


inline std::ostream& operator<< (std::ostream& out, const TestSerializableObject& obj) {
    out << "TestObject: {" << obj.sender_id << ", " << obj.message_num << ", " << obj.message << "}";
    return out;
}

}

