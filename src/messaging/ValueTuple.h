/*
 * ValueTuple.h
 *
 *  Created on: May 20, 2016
 *      Author: edward
 */

#pragma once

namespace pddm {

namespace messaging {

struct ValueTuple : public MessageBody {
        int query_num;
        std::vector<FixedPoint_t> value;
        std::vector<int> proxies;
};

}  // namespace messaging

}


