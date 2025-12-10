#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
using namespace std;
using namespace eosio;

class structs {
public:
    struct trx_tb {
        name from;
        name to;
        asset quantity;
        string memo;
    };
};
