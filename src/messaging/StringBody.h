/**
 * @file StringBody.h
 * Contains a trivial class definition that happens to be used in more than one
 * place (so we need to use the import mechanism to make sure it's only defined
 * once).
 * @date Aug 10, 2016
 * @author edward
 */

#pragma once

#include <string>
#include <utility>
#include <istream>
#include <ostream>
#include <memory>
#include <mutils-serialization/SerializationSupport.hpp>

#include "MessageBody.h"
#include "MessageBodyType.h"

namespace pddm {

namespace messaging {

/** Decorates std::string with the MessageBody type so it can be the payload of a Message. */
class StringBody : public MessageBody {
    private:
        std::string data;
    public:
        static const constexpr MessageBodyType type = MessageBodyType::STRING;
        template<typename... A>
        StringBody(A&&... args) : data(std::forward<A>(args)...) {}
        virtual ~StringBody() = default;
        //Boilerplate copy-pasting of std::string interface follows
        bool empty() const noexcept { return data.empty(); }
        std::string::size_type length() const noexcept { return data.length(); }
        std::string::size_type size() const noexcept { return data.size(); }
        const std::string::value_type* c_str() const { return data.c_str(); }
        template<typename A>
        StringBody& operator=(A&& arg) {
            data.operator=(std::forward<A>(arg));
            return *this;
        }
        StringBody& operator=(const StringBody& other) {
            data = other.data;
            return *this;
        }
        friend std::ostream& operator<<(std::ostream& stream, const StringBody& item);
        friend std::istream& operator>>(std::istream& stream, StringBody& item);
        template<typename A>
        std::string& operator+=(A&& arg) { return data.operator+=(std::forward<A>(arg)); }
        //This is the only method that differs from std::string
        inline bool operator==(const MessageBody& _rhs) const {
            if (auto* rhs = dynamic_cast<const StringBody*>(&_rhs))
                return this->data == rhs->data;
            else return false;
        }
        //Forward the serialization methods to the already-implemented ones for std::string
        std::size_t bytes_size() const {
            return mutils::bytes_size(type) + mutils::bytes_size(data);
        }
        std::size_t to_bytes(char* buffer) const {
            std::size_t bytes_written = mutils::to_bytes(type, buffer);
            return bytes_written + mutils::to_bytes(data, buffer + bytes_written);
        }
        void post_object(const std::function<void (char const * const,std::size_t)>& f) const {
            mutils::post_object(f, type);
            mutils::post_object(f, data);
        }
        void ensure_registered(mutils::DeserializationManager&){}
        static std::unique_ptr<StringBody> from_bytes(mutils::DeserializationManager* m, char const* buffer) {
            /*"Skip past the MessageBodyType, then take the deserialized string
             * and wrap it in a new StringBody"*/
            return std::make_unique<StringBody>(
                    *mutils::from_bytes<std::string>(m, buffer + sizeof(type)));
        }
        //These conversions are necessary to be able to treat a StringBody like a std::string
        operator std::string() const { return data; }
        friend std::shared_ptr<std::string> as_string_pointer(const std::shared_ptr<StringBody>& string_body_ptr);
};

inline std::ostream& operator<<(std::ostream& stream, const StringBody& item) {  return stream << item.data; }
inline std::istream& operator>>(std::istream& stream, StringBody& item) {  return stream >> item.data; }

//Some utility methods for converting to and from shared_ptr<string>
inline std::shared_ptr<std::string> as_string_pointer(const std::shared_ptr<StringBody>& string_body_ptr) {
    return std::shared_ptr<std::string>(string_body_ptr, &(string_body_ptr->data));
}

inline std::shared_ptr<StringBody> wrap_string_pointer(const std::shared_ptr<std::string>& string_ptr) {
    return std::make_shared<StringBody>(*string_ptr);
}

}
}
