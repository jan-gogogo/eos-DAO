#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/symbol.hpp>
#include <eosio/time.hpp>
#include <string>
#include <vector>

using namespace std;
using namespace eosio;

CONTRACT guide : public contract {
public:
    using contract::contract;

    /**
     * Register a new plugin template. that will be display in plugin center
     * @param pcode - Plugin code
     * @param pname - plugin name
     * @param version - Plugin version
     * @param des_cid - Plugin description IPFS CID.
     * @param autonomous_acts - Plugin autonomous actions.
     * @param is_basic - Is basic plugin.
     */
    [[eosio::action]] void saveplugin(const name& pcode,
                                      const string& pname,
                                      const string& version,
                                      const string& des_cid,
                                      vector<name> autonomous_acts,
                                      const bool& is_basic);

    /**
     * Remove a plugin.
     * @param is_basic -  Plugin code.
     */
    [[eosio::action]] void removeplugin(const name& pcode);

    // scope is self
    TABLE plugin_center {
        name pcode;
        string pname;
        string version;
        string des_cid;
        vector<name> autonomous_acts;
        bool is_basic;

        uint64_t primary_key() const { return pcode.value; }
    };

    using pgcenter_idx = multi_index<"plugincenter"_n, plugin_center>;
};
