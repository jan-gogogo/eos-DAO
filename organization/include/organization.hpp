#pragma once

#include "../../lib/auths.hpp"
#include "../../lib/consts.hpp"
#include "../../lib/structs.hpp"

using namespace std;
using namespace eosio;

#define CODE_10000 "org info has been initialized"
#define CODE_10001 "org_name exceeds 40 bytes limit"
#define CODE_10002 "the organization plugin exists already"
#define CODE_10003 "org catalog does not exist;initialization is required"
#define CODE_10004 "pcontract account does not exist"
#define CODE_10005 "the plugin contract has installed"
#define CODE_10006 "pname exceeds 40 bytes limit"
#define CODE_10007 "unauthorized transaction"

CONTRACT organization : public contract {
public:
    using contract::contract;

    organization(eosio::name receiver, eosio::name code, datastream<const char*> ds)
        : contract(receiver, code, ds) { }

    /**
     * Initial contract to save the organization information
     *
     * @param org_name - The organization name.
     * @param founder - The founder of the organization.
     * @param des_cid - The organization description IPFS CID. Start with Qm.
     */
    [[eosio::action]] void initorg(const string& org_name, const name& founder, const string& des_cid);

    /**
     * Change the organization name.We can vote whether we have the permission to call this action.
     *
     * @param caller - Account who send the transaction.Used as permission check.
     * @param org_name - The organization name.
     */
    [[eosio::action]] void changename(const name& caller, const string& org_name);

    /**
     * Change the organization description IPFS CID.
     *
     * @param caller - Account who send the transaction.Used as permission check.
     * @param des_cid - The organization description IPFS CID. Start with Qm.
     */
    [[eosio::action]] void changedescid(const name& caller, const string& des_cid);

    /**
     * Change permission of plugin contract actions(Include organization contract).
     *
     * @param caller - Account who send the transaction.Used as permission check.
     * @param pcontract - Plugin contract account we want to change
     * @param pcode - Plugin code. e.g. voting.
     * @param act - Plugin contract action.
     * @param access - The new permission.
     */
    [[eosio::action]] void changeperm(const name& caller, const name& pcontract, const name& pcode, const name& act, vector<name>& access);

    /**
     * Register in this organization.
     *
     * @param account - Register account.
     * @param email - Register email.
     */
    [[eosio::action]] void reg(const name& account, const string& email);

    /**
     * Install a new plugin.
     *
     * @param caller - Account who send the transaction.Used as permission check.
     * @param pcontract - Plugin contract account.must be deploy before call this action.
     * @param pcode - Plugin code. e.g. voting.
     * @param pname - Plugin name.
     * @param version - Plugin version.
     * @param des_cid - Plugin description IPFS CID.
     * @param autonomous_acts - Plugin autonomous actions.
     */
    [[eosio::action]] void addplugin(const name& caller,
                                     const name& pcontract,
                                     const name& pcode,
                                     const string& pname,
                                     const string& version,
                                     const string& des_cid,
                                     const vector<name>& autonomous_acts);

    /**
     *  Approve Transfer.
     *  `from` is self
     *
     * @param caller - Account who send the transaction.Used as permission check.
     * @param contract - Token contract.
     * @param to - The account to be transferred to.
     * @param quantity - The quantity of tokens to be transferred.
     * @param memo - The memo string to accompany the transaction.
     */
    [[eosio::action]] void approvetrx(const name& caller,
                                      const name& contract,
                                      const name& to,
                                      const asset& quantity,
                                      const string& memo);

    /**
     *  Approve NFT(non-fungible Token) Transfer.
     *  `from` is self
     *
     * @param caller - Account who send the transaction.Used as permission check.
     * @param contract - NFT contract.
     * @param to - The account to be transferred to.
     * @param ids - The NFT ids.
     * @param memo - The memo string to accompany the transaction.
     */
    [[eosio::action]] void approvenft(const name& caller,
                                      const name& contract,
                                      const name& to,
                                      const vector<uint64_t>& ids,
                                      const string& memo);

    /**
     * Receive all transfers of this contract
     */
    void mtransfer(const name& contract, const structs::trx_tb& tx);

    // singleton
    TABLE org {
        string org_name;
        name founder;
        string des_cid;
    };

    // scope is self
    TABLE plugin {
        name pcontract;
        name pcode;
        string pname;
        string version;
        string des_cid;
        vector<name> autonomous_acts;
        bool is_basic;

        uint64_t primary_key() const { return pcontract.value; }
    };

    // scope is plugin.pcontract
    TABLE permission {
        name act;
        name pcode;
        vector<name> access;

        uint64_t primary_key() const { return act.value; }
    };

    // scope is self
    // ram payer:caller
    TABLE reg_account {
        name account;
        string email;

        uint64_t primary_key() const { return account.value; }
    };

    //scope is token contract
    TABLE donate_pool {
        uint64_t id;
        name account;
        asset quantity;

        uint64_t primary_key() const { return id; }
        uint64_t by_account() const { return account.value; }
    };

    //scope is token contract
    TABLE allow_trx {
        uint64_t id;
        name to;
        asset quantity;

        uint64_t primary_key() const { return id; }
        uint64_t by_to() const { return to.value; }
    };

    struct plugin_center {
        name pcode;
        string pname;
        string version;
        string des_cid;
        vector<name> autonomous_acts;
        bool is_basic;

        uint64_t primary_key() const { return pcode.value; }
    };

    using org_idx = singleton<"org"_n, org>;
    using plugin_idx = multi_index<"plugin"_n, plugin>;
    using permission_idx = multi_index<"permission"_n, permission>;
    using regaccount_idx = multi_index<"regaccount"_n, reg_account>;
    using donatepool_idx = multi_index<"donatepool"_n, donate_pool,
                                       indexed_by<"byaccount"_n, const_mem_fun<donate_pool, uint64_t, &donate_pool::by_account>>>;
    using allowtrx_idx = multi_index<"allowtrx"_n, allow_trx,
                                     indexed_by<"byto"_n, const_mem_fun<allow_trx, uint64_t, &allow_trx::by_to>>>;

    using pgcenter_idx = multi_index<"plugincenter"_n, plugin_center>;

    // autonomous actions
    const name VA_CHANGE_NAME = name("changename");
    const name VA_CHANGE_DESCID = name("changedescid");
    const name VA_CHANGE_PERM = name("changeperm");
    const name VA_APPPLUGIN = name("addplugin");
    const name VA_APPROVETRX = name("approvetrx");
    const name VA_APPROVE_TRX_NFT = name("approvenft");

private:
    void _check(const name& act, const name& caller);
    void _add_allow_trx(const name& contract, const name& to, const asset& quantity);
    void _remove_allow_trx(const name& contract, const name& to, const asset& quantity);
};

extern "C" {
void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    if (action == name("transfer").value) {
        organization thiscontract(name(receiver), name(code), datastream<const char*>("", 0));
        auto trx = unpack_action_data<structs::trx_tb>();
        thiscontract.mtransfer(name(code), trx);
        return;
    }

    if (code != receiver)
        return;

    switch (action) {
        EOSIO_DISPATCH_HELPER(organization, (initorg)(changename)(changedescid)(changeperm)(reg)(addplugin)(approvetrx)(approvenft))
    }
    eosio_exit(0);
}
}
