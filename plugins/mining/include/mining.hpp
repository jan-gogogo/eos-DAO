#pragma once
#include "../../../lib/assets.hpp"
#include "../../../lib/auths.hpp"
#include "../../../lib/consts.hpp"
#include "../../../lib/safemath.hpp"
#include "../../../lib/structs.hpp"
#include <math.h>

using namespace std;
using namespace eosio;

#define CODE_10000 "invalid max_quantity value"
#define CODE_10001 "ins can not be empty"
#define CODE_10002 "invalid rate value"
#define CODE_10003 "token exists already"
#define CODE_10004 "exchange in data does not exist"
#define CODE_10005 "config does not exists; initialization is required"
#define CODE_10006 "config has initialized"
#define CODE_10007 "overdrawn balance"

CONTRACT mining : public contract {
public:
    using contract::contract;

    mining(eosio::name receiver, eosio::name code, datastream<const char*> ds)
        : contract(receiver, code, ds) { }

    // singleton
    TABLE dsconf {
        name token;
        bool limit;
        asset max_quantity;
        asset remaining_quantity;
        name org_contract;
    };

    // scope is self
    TABLE exchange_in {
        name token;
        symbol_code symbol_code;
        uint64_t rate; //BASE_SCALE

        uint64_t primary_key() const { return token.value; }
    };

    using dsconf_idx = singleton<"dsconf"_n, dsconf>;
    using exin_idx = multi_index<"exchangein"_n, exchange_in>;

    //0.0 = 0
    //0.01 = 10^10
    //0.10 = 10^11
    //1.00 = 10^12
    //10.0 = 10^13
    //...
    const uint64_t BASE_SCALE = 1000000000000; //10^12

    const name VA_CHANGE_CONF = name("changeconf");
    const name VA_ADD_EXIN = name("addexin");
    const name VA_REMOVE_EXIN = name("rmexin");

    /**
     * Initial contract to save the config.
     * 
     * @param token - Asset contract transferred from the institution.
     * @param limit - Whether to limit the amount of asset transferred out.
     * @param max_quantity - Limit the output quantity.
     * @param ins - Acceptable asset datas.
     * @param org_contract - The organization to which the token contract belongs.
     */
    [[eosio::action]] void init(const name& token,
                                const bool& limit,
                                const asset& max_quantity,
                                const vector<exchange_in>& ins,
                                const name& org_contract);

    /**
     * Change the config.
     *
     * @param caller - Account who send the transaction.Used as permission check.
     * @param token - Asset contract transferred from the institution.
     * @param limit - Whether to limit the amount of asset transferred out.
     * @param max_quantity - Limit the output quantity.
     */
    [[eosio::action]] void changeconf(const name& caller,
                                      const name& token,
                                      const bool& limit,
                                      const asset& max_quantity);

    /**
     * Add Acceptable asset data.
     *
     * @param caller - Account who send the transaction.Used as permission check.
     * @param token -  Acceptable asset contract.
     * @param symbol_code - Acceptable asset symbol.
     * @param rate - Asset exchange ratio. 1 = 10^12
     */
    [[eosio::action]] void addexin(const name& caller,
                                   const name& token,
                                   const symbol_code& symbol_code,
                                   const uint64_t rate);

    /**
     * Remove Acceptable asset data.
     *
     * @param caller - Account who send the transaction.Used as permission check.
     * @param token -  Acceptable asset contract.
     */
    [[eosio::action]] void rmexin(const name& caller, const name& token);

    void mtransfer(const name& contract, const structs::trx_tb& tx);

private:
    void _check(const name& act, const name& caller);

    dsconf _get_config();

    void _save_conf(const name& token,
                    const bool& limit,
                    const asset& max_quantity,
                    const name& org_contract,
                    const bool& is_init);
};

extern "C" {
void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    if (action == name("transfer").value) {
        mining thiscontract(name(receiver), name(code), datastream<const char*>("", 0));
        auto trx = unpack_action_data<structs::trx_tb>();
        thiscontract.mtransfer(name(code), trx);
        return;
    }

    if (code != receiver)
        return;

    switch (action) {
        EOSIO_DISPATCH_HELPER(mining, (init)(changeconf)(addexin)(rmexin))
    }
    eosio_exit(0);
}
}
