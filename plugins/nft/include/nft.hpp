#pragma once
#include "../../../lib/auths.hpp"
#include "../../../lib/consts.hpp"
#include "../../../lib/safemath.hpp"

using namespace std;
using namespace eosio;

#define CODE_10000 "config does not exists; initialization is required"
#define CODE_10001 "the token_name nft exists already"
#define CODE_10002 "nft config has initialized"
#define CODE_10003 "org_contract account does not exist"
#define CODE_10004 "token_name does not exist"
#define CODE_10005 "can not issue more than max supply"
#define CODE_10006 "to account does not exist"
#define CODE_10007 "can issue up to 20 at a time"
#define CODE_10008 "cannot transfer to self"
#define CODE_10009 "to account does not exist"
#define CODE_10010 "no balance object found"
#define CODE_10011 "overdrawn balance"
#define CODE_10012 "can transfer 10 NFT at a time"
#define CODE_10013 "not transferable"
#define CODE_10014 "NFT does not exist"

CONTRACT nft : public contract {
public:
    using contract::contract;

    /**
     * Initial contract to save the config.
     *
     * @param org_contract - The organization to which the token contract belongs.
     */
    [[eosio::action]] void init(const name& org_contract);

    /**
     * Create NFT.
     * 
     * @param caller - Account who send the transaction.Used as permission check.
     * @param issuer - The account that creates the NFT.
     * @param token_name - The unique name of this NFT.
     * @param transferable - Whether to allow NFT to trade.
     * @param max_supply - The maximum supply set for the NFT created.
     * @param des_cid - The NFT description IPFS ID.
     */
    [[eosio::action]] void create(const name& caller,
                                  const name& issuer,
                                  const name& token_name,
                                  const bool& transferable,
                                  const uint64_t& max_supply,
                                  const string& des_cid);

    /**
     * Issue NFT.
     * 
     * @param caller - Account who send the transaction.Used as permission check.
     * @param token_name - NFT name.
     * @param to - The account to issue NFT to.
     * @param amount - The amount of NFT to be issued.
     * @param des_cid - The NFT description IPFS ID.
     */
    [[eosio::action]] void issue(const name& caller,
                                 const name& token_name,
                                 const name& to,
                                 const uint64_t& amount,
                                 const string& des_cid);

    /**
     * Transfer NFT.
     * 
     * @details Allows `from` account to transfer to `to` account the `ids` NFTs.
     * 
     * @param from - The account to transfer from.
     * @param to - The account to be transferred to.
     * @param ids - NFT multiple id of the transaction.
     * @param memo - The memo string to accompany the transaction.
     */
    [[eosio::action]] void transfernft(const name& from,
                                       const name& to,
                                       const vector<uint64_t>& ids,
                                       const string& memo);

    // singleton
    TABLE dsconf {
        name org_contract;
    };

    // scope is self
    TABLE stats {
        name token_name;
        bool transferable;
        name issuer;
        uint64_t max_supply;
        uint64_t issued_supply;
        string des_cid;

        uint64_t primary_key() const { return token_name.value; }
    };

    // scope is self
    TABLE items {
        uint64_t id;
        uint64_t serial_number;
        name owner;
        name token_name;
        string des_cid;

        uint64_t primary_key() const { return id; }
        uint64_t get_owner() const { return owner.value; }
    };

    // scope is owner
    TABLE accounts {
        name token_name;
        uint64_t amount;

        uint64_t primary_key() const { return token_name.value; }
    };

    using dsconf_idx = singleton<"dsconf"_n, dsconf>;
    using stats_idx = multi_index<"stat"_n, stats>;
    using items_idx = multi_index<"item"_n, items,
                                  indexed_by<"byowner"_n, const_mem_fun<items, uint64_t, &items::get_owner>>>;
    using acct_idx = multi_index<"accounts"_n, accounts>;

    const name VA_CREATE = name("create");
    const name VA_ISSUE = name("issue");
    const name VA_TRANSFER = name("transfernft");

private:
    void _check(const name& act, const name& caller);
    void _require_config();
    dsconf _get_config();
    void _add_balance(const name& owner, const name& token_name, const uint64_t& quantity, const name& ram_payer);
    void _sub_balance(const name& owner, const name& token_name, const uint64_t& quantity);
};
