#pragma once
#include "../../../lib/auths.hpp"
#include "../../../lib/consts.hpp"
#include <math.h>

using namespace std;
using namespace eosio;

CONTRACT tkexchange : public contract {
public:
    using contract::contract;

    [[eosio::action]] void init(const name& org_contract);

    /**
    * Create pair
    * @param caller - Account who send the transaction.Used as permission check.
    * @param token_contract Transfer token contract.
    * @param deadline Time after which this transaction can no longer be executed.
    * @param max_tokens Maximum number of tokens deposited.
    * @param quantity Transfer quantity.
    */
    [[eosio::action]] void createpair(const name& caller,
                                      const name& token_contract,
                                      const uint64_t& deadline,
                                      const uint64_t& max_tokens,
                                      const asset& quantity);

    /**
    * Add liquidity
    * @param caller - Account who send the transaction.Used as permission check.
    * @param token_contract Transfer token contract.
    * @param deadline Time after which this transaction can no longer be executed.
    * @param min_liquidity Minimum number of Token sender will mint.
    * @param max_tokens Maximum number of tokens deposited.
    * @param quantity Transfer quantity.
    */
    [[eosio::action]] void addliquidity(const name& caller,
                                        const name& token_contract,
                                        const uint64_t& deadline,
                                        const uint64_t& min_liquidity,
                                        const uint64_t& max_tokens,
                                        const asset& quantity);

    /**
    * Burn tokens to withdraw EOS and Tokens at current ratio.
    * @param caller - Account who send the transaction.Used as permission check.
    * @param token_contract Transfer token contract.
    * @param deadline Time after which this transaction can no longer be executed.
    * @param amount Amount of UNI burned.
    * @param min_eos Minimum EOS withdrawn.
    * @param min_tokens Minimum Tokens withdrawn.
    */
    [[eosio::action]] void rmliquidity(const name& caller,
                                       const name& token_contract,
                                       const uint64_t& deadline,
                                       const uint64_t& amount,
                                       const uint64_t& min_eos,
                                       const uint64_t& min_tokens);

    /**
    * Convert EOS to Tokens.
    * User specifies exact input (param.quantity) and minimum output.
    * @param caller - Account who send the transaction.Used as permission check.
    * @param token_contract Token contract you want to buy.
    * @param deadline Time after which this transaction can no longer be executed.
    * @param min_tokens Minimum Tokens bought.
    * @param quantity EOS sold.
    */
    [[eosio::action]] void eostotoken(const name& caller,
                                      const name& token_contract,
                                      const uint64_t& deadline,
                                      const uint64_t& min_tokens,
                                      const asset& quantity);
    /**
    * Convert Tokens to EOS.
    * User specifies exact input and minimum output.
    * @param caller - Account who send the transaction.Used as permission check.
    * @param token_contract Token contract you want to sold.
    * @param quantity Amount of Tokens sold.
    * @param deadline Time after which this transaction can no longer be executed.
    * @param min_eos Minimum EOS purchased.
    */
    [[eosio::action]] void tokentoeos(const name& caller,
                                      const name& token_contract,
                                      const asset& quantity,
                                      const uint64_t& deadline,
                                      const uint64_t& min_eos);
    /**
    * Convert Tokens (param.tokens_sold) to Tokens (bought_token_contract).
    * User specifies exact input and minimum output.
    * @param caller - Account who send the transaction.Used as permission check.
    * @param sold_token_contract Token contract you want to sold.
    * @param tokens_sold Amount of Tokens sold.
    * @param bought_token_contract Token contract you want to bought.
    * @param deadline Time after which this transaction can no longer be executed.
    * @param min_tokens_bought Minimum Tokens (bought_token_contract) purchased.
    * @param min_eos_bought Minimum EOS purchased as intermediary.
    */
    [[eosio::action]] void tokentotoken(const name& caller,
                                        const name& sold_token_contract,
                                        const asset& tokens_sold,
                                        const name& bought_token_contract,
                                        const uint64_t& deadline,
                                        const uint64_t& min_tokens_bought,
                                        const uint64_t& min_eos_bought);
    
    [[eosio::action]] void approve(const name& caller, const name& token_contract, const asset& quantity);

    [[eosio::action]] void withdraw(const name& caller, const name& token_contract);

    [[eosio::action]] void apply(const name& caller, const name& token_contract, const symbol_code& symbol_code);

    // singleton
    TABLE dsconf {
        name org_contract;
    };

    using dsconf_idx = singleton<"dsconf"_n, dsconf>;

    const name VA_CREATE_PAIR = name("createpair");
    const name VA_ADD_LIQUIDITY = name("addliquidity");
    const name VA_REMOVE_LIQUIDITY = name("rmliquidity");
    const name VA_EOS_TO_TOKEN = name("eostotoken");
    const name VA_TOKEN_TO_EOS = name("tokentoeos");
    const name VA_TOKEN_TO_TOKEN = name("tokentotoken");
    const name VA_APPROVE = name("approve");
    const name VA_WITHDRAW = name("withdraw");
    const name VA_APPLY = name("apply");

    const name EOS_CONTRACT = name("eosio.token");
    const symbol EOS_SYMBOL = symbol(symbol_code("EOS"), 4);

private:
    void _send_transaction(const name& token_contract, const asset& quantity, const string& memo);

    void _check(const name& act, const name& caller);
    dsconf _get_config();

    void _splicing_action(string * memo, const string& val);
    void _splicing_uint64(string * memo, const string& key, const uint64_t& val);
    void _splicing_uint64_last(string * memo, const string& key, const uint64_t& val);
    void _splicing_uint64(string * memo, const string& key, const uint64_t& val, const bool& is_last);

    void _splicing_name(string * memo, const string& key, const name& val);
    void _splicing_name_last(string * memo, const string& key, const name& val);
    void _splicing_name(string * memo, const string& key, const name& val, const bool& is_last);

    string _uint64_string(uint64_t input);
};
