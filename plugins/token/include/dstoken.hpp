#pragma once

#include "../../../lib/auths.hpp"

namespace eosiosystem {
class system_contract;
}

namespace eosio {

using std::string;

class [[eosio::contract("dstoken")]] token : public contract {
public:
    using contract::contract;

    /**
     * Allows `issuer` account to create a token in supply of `maximum_supply`. If validation is successful a new entry in statstable for token symbol scope gets created.
     *
     * @param issuer - the account that creates the token.
     * @param maximum_supply - the maximum supply set for the token created.
     * @param org_contract - the organization to which the token contract belongs.
     *
     * @pre Token symbol has to be valid,
     * @pre Token symbol must not be already created,
     * @pre maximum_supply has to be smaller than the maximum supply allowed by the system: 1^62 - 1.
     * @pre Maximum supply must be positive;
     */
    [[eosio::action]] void init(const name& issuer, const asset& maximum_supply, const name& org_contract);

    /**
     * The opposite for create action, if all validations succeed,
     * it debits the statstable.supply amount.
     * @param caller - retire sponsor
     * @param quantity - the quantity of tokens to retire,
     * @param memo - the memo string to accompany the transaction.
     */
    [[eosio::action]] void retire(const name& caller, const asset& quantity, const string& memo);

    /**
     * Allows `from` account to transfer to `to` account the `quantity` tokens.
     * One account is debited and the other is credited with quantity tokens.
     *
     * @param from - the account to transfer from,
     * @param to - the account to be transferred to,
     * @param quantity - the quantity of tokens to be transferred,
     * @param memo - the memo string to accompany the transaction.
     */
    [[eosio::action]] void transfer(const name& from, const name& to, const asset& quantity, const string& memo);

    /**
     *  mint: Allow caller to issue tokens to `to` account the `quantity` tokens
     **/
    [[eosio::action]] void mint(const name& caller, const name& to, const asset& quantity, const string& memo);

    static asset get_supply(const name& token_contract_account, const symbol_code& sym_code) {
        stats statstable(token_contract_account, sym_code.raw());
        const auto& st = statstable.get(sym_code.raw());
        return st.supply;
    }

    static asset get_balance(const name& token_contract_account, const name& owner, const symbol_code& sym_code) {
        accounts accountstable(token_contract_account, owner.value);
        const auto& ac = accountstable.get(sym_code.raw());
        return ac.balance;
    }

    using init_action = eosio::action_wrapper<"init"_n, &token::init>;
    using retire_action = eosio::action_wrapper<"retire"_n, &token::retire>;
    using transfer_action = eosio::action_wrapper<"transfer"_n, &token::transfer>;
    using mint_action = eosio::action_wrapper<"mint"_n, &token::mint>;

private:
    struct [[eosio::table]] account {
        asset balance;

        uint64_t primary_key() const { return balance.symbol.code().raw(); }
    };

    struct [[eosio::table]] currency_stats {
        asset supply;
        asset max_supply;
        name issuer;

        uint64_t primary_key() const { return supply.symbol.code().raw(); }
    };

    struct [[eosio::table]] token_holder {
        name account;
        asset quantity;

        uint64_t primary_key() const { return account.value; }
    };

    struct [[eosio::table]] dsconf {
        name org_contract;
        asset coin;
    };

    typedef eosio::multi_index<"accounts"_n, account> accounts;
    typedef eosio::multi_index<"stat"_n, currency_stats> stats;
    typedef eosio::multi_index<"holders"_n, token_holder> holders;
    typedef eosio::singleton<"dsconf"_n, dsconf> dsconfs;

    const name VA_MINT = name("mint");
    const name VA_RETIRE = name("retire");

    void _create(const name& issuer, const asset& maximum_supply);
    void _sub_balance(const name& owner, const asset& value);
    void _add_balance(const name& owner, const asset& value, const name& ram_payer);
    void _reset_holder(const name& owner, const symbol_code& sym_code, const name& ram_payer);
    void _issue(const name& to, const asset& quantity, const string& memo);
    void _check(const name& act, const name& caller);
    dsconf _get_config();
};

}