#pragma once
#include "../../../lib/auths.hpp"
#include "../../../lib/consts.hpp"
#include "../../../lib/structs.hpp"

using namespace std;
using namespace eosio;

#define CODE_10001 "config has initialized"
#define CODE_10002 "NFT id exists already"
#define CODE_10003 "config does not exists; initialization is required"
#define CODE_10004 "NFT does not exists"
#define CODE_10005 "NFT does not belongs to org_contract"
#define CODE_10006 "can sale 20 at a time"
#define CODE_10007 "token account does not exist"
#define CODE_10008 "parse memo error"
#define CODE_10009 "invalid token contract"
#define CODE_10010 "invalid transfer quantity"

CONTRACT nftmarket : public contract {
public:
    using contract::contract;

    nftmarket(eosio::name receiver, eosio::name code, datastream<const char*> ds)
        : contract(receiver, code, ds) { }

    // singleton
    TABLE dsconf {
        name nft_contract;
        name org_contract;
    };

    // scope is self
    TABLE sale_list {
        uint64_t nft_id;
        name owner; // owner is org_contract
        name token;
        asset quantity;

        uint64_t primary_key() const { return nft_id; }
    };

    // contract:nft
    // scope is nft
    struct items {
        uint64_t id;
        uint64_t serial_number;
        name owner;
        name token_name;
        string des_cid;

        uint64_t primary_key() const { return id; }
        uint64_t get_owner() const { return owner.value; }
    };

    using dsconf_idx = singleton<"dsconf"_n, dsconf>;
    using salelist_idx = multi_index<"salelist"_n, sale_list>;
    // nft contract,read only
    using items_idx = multi_index<"item"_n, items,
                                  indexed_by<"byowner"_n, const_mem_fun<items, uint64_t, &items::get_owner>>>;

    const name VA_SALE = name("sale");
    const name VA_CLOSE_SALE = name("closesale");

    [[eosio::action]] void init(const name& nft_contract, const name& org_contract);

    [[eosio::action]] void sale(const name& caller,
                                const name& token,
                                const vector<uint64_t>& nft_ids,
                                const asset quantity);

    [[eosio::action]] void closesale(const name& caller, const uint64_t& nft_id);

    void mtransfer(const name& contract, const structs::trx_tb& tx);

private:
    void _check(const name& act, const name& caller);
    void _require_config();
    dsconf _get_config();
    void split_memo(vector<string> & results, string memo, char separator);
};

extern "C" {
void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    if (action == name("transfer").value) {
        nftmarket thiscontract(name(receiver), name(code), datastream<const char*>("", 0));
        auto trx = unpack_action_data<structs::trx_tb>();
        thiscontract.mtransfer(name(code), trx);
        return;
    }

    if (code != receiver)
        return;

    switch (action) {
        EOSIO_DISPATCH_HELPER(nftmarket, (init)(sale)(closesale))
    }
    eosio_exit(0);
}
}
