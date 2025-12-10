#pragma once

#include <algorithm>
#include <eosio/eosio.hpp>
using namespace std;
using namespace eosio;

namespace safemath {

uint64_t add(const uint64_t& a, const uint64_t& b) {
    uint64_t c = a + b;
    check(c >= a, "math add overflow");
    return c;
}

uint64_t sub(const uint64_t& a, const uint64_t& b) {
    check(b <= a, "math sub underflow");
    return a - b;
}

uint64_t mul(const uint64_t& a, const uint64_t& b) {
    if (a == 0)
        return 0;

    uint64_t c = a * b;
    check(c / a == b, "math mul overflow");
    return c;
}

uint64_t div(const uint64_t& a, const uint64_t& b) {
    check(b > 0, "math div zero");
    uint64_t c = a / b;
    return c;
}

}