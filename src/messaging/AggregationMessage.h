/*
 * AggregationMessage.h
 *
 *  Created on: May 18, 2016
 *      Author: edward
 */

#pragma once

#include <memory>
#include <vector>
#include <utility>

#include "../FixedPoint_t.h"
#include "Message.h"
#include "../util/Hash.h"

namespace pddm {
namespace messaging {

/** Decorates std::vector<FixedPoint_t> with the MessageBody type so it can be the payload of a Message. */
class AggregationMessageValue : public MessageBody {
    private:
        std::vector<FixedPoint_t> data;
    public:
        template<typename... A>
        AggregationMessageValue(A&&... args) : data(std::forward<A>(args)...) {}
        AggregationMessageValue(const AggregationMessageValue&) = default;
        AggregationMessageValue(AggregationMessageValue&&) = default;
        operator std::vector<FixedPoint_t>() const { return data; }
        //Boilerplate copy-and-pasting of the entire interface of std::vector follows
        decltype(data)::size_type size() const noexcept { return data.size(); }
        template<typename... A>
        void resize(A&&... args) { return data.resize(std::forward<A>(args)...); }
        template<typename... A>
        void emplace_back(A&&... args) { return data.emplace_back(std::forward<A>(args)...); }
        decltype(data)::iterator begin() { return data.begin(); }
        decltype(data)::const_iterator begin() const { return data.begin(); }
        decltype(data)::iterator end() { return data.end(); }
        decltype(data)::const_iterator end() const { return data.end(); }
        decltype(data)::const_iterator cbegin() const { return data.cbegin(); }
        decltype(data)::const_iterator cend() const { return data.cend(); }
        bool empty() const noexcept { return data.empty(); }
        void clear() noexcept { data.clear(); }
        decltype(data)::reference at(decltype(data)::size_type i) { return data.at(i); }
        decltype(data)::const_reference at(decltype(data)::size_type i) const { return data.at(i); }
        decltype(data)::reference operator[](decltype(data)::size_type i) { return data[i]; }
        decltype(data)::const_reference operator[](decltype(data)::size_type i) const { return data[i]; }
        template<typename A>
        AggregationMessageValue& operator=(A&& other) {
            data.operator=(std::forward<A>(other));
            return *this;
        }
        //An argument of type AggregationMessageValue won't forward to std::vector::operator=
        AggregationMessageValue& operator=(const AggregationMessageValue& other) {
            data = other.data;
            return *this;
        }
        //This is the only method that differs from std::vector<FixedPoint_t>
        inline bool operator==(const MessageBody& _rhs) const {
            if (auto* rhs = dynamic_cast<const AggregationMessageValue*>(&_rhs))
                return this->data == rhs->data;
            else return false;
        }
};

class AggregationMessage: public Message {
    private:
        int num_contributors;
    public:
        int query_num;
        AggregationMessage() : Message(0, nullptr), num_contributors(0), query_num(0) {}
        AggregationMessage(const int sender_id, const int query_num, std::shared_ptr<AggregationMessageValue> value) :
            Message(sender_id, value), num_contributors(1), query_num(query_num) {}
        std::shared_ptr<AggregationMessageValue> get_body() { return std::static_pointer_cast<AggregationMessageValue>(body); };
        const std::shared_ptr<AggregationMessageValue> get_body() const { return std::static_pointer_cast<AggregationMessageValue>(body); };
        void add_value(const FixedPoint_t& value, int num_contributors);
        void add_values(const std::vector<FixedPoint_t>& values, const int num_contributors);
        int get_num_contributors() const { return num_contributors; }

        friend bool operator==(const AggregationMessage& lhs, const AggregationMessage& rhs);
        friend struct std::hash<AggregationMessage>;
};

inline bool operator==(const AggregationMessage& lhs, const AggregationMessage& rhs) {
    return lhs.num_contributors == rhs.num_contributors && lhs.query_num == rhs.query_num && (*lhs.body) == (*rhs.body);
}

inline bool operator!=(const AggregationMessage& lhs, const AggregationMessage& rhs) {
    return !(lhs == rhs);
}

} /* namespace messaging */
} /* namespace pddm */

namespace std {

template<>
struct hash<pddm::messaging::AggregationMessageValue> {
    size_t operator()(const pddm::messaging::AggregationMessageValue& input) const {
        std::size_t seed = input.size();
        //Convert the FixedPoints to their base integral type, since we just want the bits
        for (pddm::FixedPoint_t::base_type i : input) {
            seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

template<>
struct hash<pddm::messaging::AggregationMessage> {
        size_t operator()(const pddm::messaging::AggregationMessage& input) const {
            using pddm::util::hash_combine;

            size_t result = 1;
            hash_combine(result, input.num_contributors);
            hash_combine(result, input.query_num);
            hash_combine(result, *std::static_pointer_cast<pddm::messaging::AggregationMessageValue>(input.body));
            return result;
        }
};

} // namespace std

