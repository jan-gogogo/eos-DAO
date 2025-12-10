#pragma once
#include "../../lib/safemath.hpp"
#include "../../lib/structs.hpp"
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/symbol.hpp>
#include <eosio/time.hpp>
#include <string>
#include <vector>

using namespace std;
using namespace eosio;
using namespace safemath;

CONTRACT dsswap : public contract {
public:
    using contract::contract;

    dsswap(eosio::name receiver, eosio::name code, datastream<const char*> ds)
        : contract(receiver, code, ds) { }

    [[eosio::action]] void vote(const name& token_contract, const name& voter);

    /**
     * Receive all transfers of this contract
     */
    void mtransfer(const name& contract, const structs::trx_tb& tx);

    // scope is self
    TABLE tokens {
        name contract;
        uint64_t total_supply;
        asset token_balance;
        asset eos_balance;

        uint64_t primary_key() const { return contract.value; }
    };

    // scope is self
    TABLE propose_tokens {
        name contract;
        uint64_t total_supply;
        asset token_balance;
        asset eos_balance;
        uint64_t total_votes;

        uint64_t primary_key() const { return contract.value; }
    };

    // scope is token contract
    TABLE mints {
        name account;
        uint64_t amount;

        uint64_t primary_key() const { return account.value; }
    };

    // scope is token contract
    TABLE allows {
        name account;
        asset balance;

        uint64_t primary_key() const { return account.value; }
    };

    // scope is token contract
    TABLE votes {
        name voter;

        uint64_t primary_key() const { return voter.value; }
    };

    struct currency_stats {
        asset supply;
        asset max_supply;
        name issuer;

        uint64_t primary_key() const { return supply.symbol.code().raw(); }
    };

    using token_idx = multi_index<"token"_n, tokens>;
    using propose_idx = multi_index<"propose"_n, propose_tokens>;
    using mint_idx = multi_index<"mint"_n, mints>;
    using allow_idx = multi_index<"allow"_n, allows>;
    using vote_idx = multi_index<"vote"_n, votes>;
    using stats_idx = multi_index<"stat"_n, currency_stats>;

private:
    void _create_pair(const structs::trx_tb& tx, vector<string> vec);
    void _add_liquidity(const structs::trx_tb& tx, vector<string> vec);
    void _remove_liquidity(const name& contract, const structs::trx_tb& tx, vector<string> vec);

    void _eos_to_token(const structs::trx_tb& tx, vector<string> vec);
    void _token_to_eos(const name& contract, const structs::trx_tb& tx, vector<string> vec);
    void _token_to_token(const name& contract, const structs::trx_tb& tx, vector<string> vec);

    void _approve(const name& contract, const structs::trx_tb& tx);
    void _cancel_approve(const name& contract, const structs::trx_tb& tx, vector<string> vec);

    void _apply_add_token(const structs::trx_tb& tx, vector<string> vec);

    void _check_eos(const name& contract, const asset& quantity);
    void _check_asset_eos(const asset& quantity);
    void _check_asset(const asset& quantity, const symbol& symbol);

    void _split_memo(vector<string> & results, string memo, char separator);
    string _split_val(const vector<string>& kv_vec, string k);
    string _split_val(const vector<string>& kv_vec, string k, bool kv);

    dsswap::tokens _get_token(const name& contract);
    uint64_t _get_input_price(const uint64_t& input_amount, const uint64_t& input_reserve, const uint64_t& output_reserve);

    void _transfer_action(const name& contract, const name& to, const asset& quantity, const string& memo);
    void _transfer_from(const name& from, const name& contract, const asset& quantity);

    const name EOS_CONTRACT = name("eosio.token");
    const symbol EOS_SYMBOL = symbol(symbol_code("EOS"), 4);
};

extern "C" {
void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    if (action == name("transfer").value) {
        dsswap thiscontract(name(receiver), name(code), datastream<const char*>("", 0));
        auto trx = unpack_action_data<structs::trx_tb>();
        thiscontract.mtransfer(name(code), trx);
        return;
    }

    if (code != receiver)
        return;

    switch (action) {
        EOSIO_DISPATCH_HELPER(dsswap, (vote))
    }
    eosio_exit(0);
}
}
