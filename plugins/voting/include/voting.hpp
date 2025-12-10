#pragma once

#include "../../../lib/assets.hpp"
#include "../../../lib/auths.hpp"
#include "../../../lib/consts.hpp"
#include "../../../lib/safemath.hpp"
#include "../../../lib/trxs.hpp"
#include <eosio/transaction.hpp>

using namespace std;
using namespace eosio;
using namespace safemath;

#define CODE_10000 "voting config has initialized"
#define CODE_10001 "invalid support_required value"
#define CODE_10002 "invalid min_accept_quorum value"
#define CODE_10003 "invalid org_contract account"
#define CODE_10004 "invalid support_required value"
#define CODE_10005 "invalid min_accept_quorum value"
#define CODE_10006 "description exceeds 300 bytes limit"
#define CODE_10007 "proposal must be created by token holders"
#define CODE_10008 "contract does not exist"
#define CODE_10009 "vote does not exist"
#define CODE_10010 "must be token holder"
#define CODE_10011 "vote is not open or has ended"
#define CODE_10012 "vote cannot be executed"
#define CODE_10013 "execute action data for voting does not exist"
#define CODE_10014 "voting time is not over"
#define CODE_10015 "invalid voting status"
#define CODE_10016 "config does not exists; initialization is required"
#define CODE_10017 "invalid divisor value"
#define CODE_10018 "authorization for execution is missing"

CONTRACT voting : public contract {
public:
    using contract::contract;

    /**
     * Initial contract to save the vote config.
     *
     * @param token - Token contract for voting.
     * @param support_required - Percentage of yeas in casted votes for a vote to succeed.
     * @param min_accept_quorum - Percentage of yeas in total possible votes for a vote to succeed.
     * @param vote_time_sec - Seconds that a vote will be open for token holders to vote.
     * @param org_contract - The organization contract which this contract belongs.
     */
    [[eosio::action]] void init(const name& token,
                                const uint64_t& support_required,
                                const uint64_t& min_accept_quorum,
                                const uint64_t& vote_time_sec,
                                const name& org_contract);

    /**
     * Change the vote config:support_required.
     *
     * @param caller - Account who send the transaction.Used as permission check.
     * @param support_required - Percentage of yeas in casted votes for a vote to succeed.
     */
    [[eosio::action]] void changesr(const name& caller, const uint64_t& support_required);

    /**
     * Change the vote config:min_accept_quorum.
     *
     * @param caller - Account who send the transaction.Used as permission check.
     * @param min_accept_quorum - Percentage of yeas in total possible votes for a vote to succeed.
     */
    [[eosio::action]] void changemaq(const name& caller, const uint64_t& min_accept_quorum);

    /**
     * Change the vote config:vote_time_sec.
     *
     * @param caller - Account who send the transaction.Used as permission check.
     * @param vote_time_sec - Seconds that a vote will be open for token holders to vote.
     */
    [[eosio::action]] void changevts(const name& caller, const uint64_t& vote_time_sec);

    /**
     * Create a new vote.
     *
     * @param proposer - The vote proposer.
     * @param description - The vote description(<= 300 bytes).
     * @param meta_cid - The vote meta data IPFS CID.
     * @param exec_contract - Create a vote to call a EOS contract.
     * @param exec_act - A action of this contract that is called.
     * @param exec_params - Params of this contract that is called. pack action data(Hex)
     */
    [[eosio::action]] void newvote(const name& proposer,
                                   const string& description,
                                   const string& meta_cid,
                                   const name& exec_contract,
                                   const name& exec_act,
                                   const vector<char>& exec_params);

    /**
     * Vote
     *
     * @param voter - Voter EOS account.
     * @param vote_id - Id for vote.
     * @param support - Whether voter supports the vote.
     */
    [[eosio::action]] void vote(const name& voter, const uint64_t& vote_id, const bool& support);

    /**
     * Execute the vote
     * (The vote must be passed)
     *
     * @param vote_id - Id for vote.
     */
    [[eosio::action]] void executevote(const uint64_t& vote_id);

    /**
     * Send a defer transaction to update vote state when call `newvote`.
     *
     * @param vote_id - Id for vote.
     */
    [[eosio::action]] void timeup(const uint64_t& vote_id);

    /**
     * Send a log data when create vote.
     *
     * @param vote_id - Id for vote.
     * @param description - The vote description(<= 300 bytes).
     */
    [[eosio::action]] void lognewvote(const uint64_t& vote_id, const string& description);

    enum vote_type {
        NAY = 0,
        YEA = 1
    };

    enum vote_state {
        PENDING = 1,
        PASSED = 2,
        EXECUTED = 3,
        REJECTED = 4
    };

    // singleton
    TABLE dsconf {
        name token;
        uint64_t support_required;
        uint64_t min_accept_quorum;
        uint64_t vote_time_sec;
        name org_contract;
    };

    // scope is self
    TABLE votes {
        uint64_t id;
        uint64_t support_required;
        uint64_t min_accept_quorum;
        uint64_t yea;
        uint64_t nay;
        string meta_cid;
        name proposer;
        uint64_t start_time;
        uint64_t expire_time;
        uint64_t total_weight;
        bool chain_exec;
        uint64_t state;

        uint64_t primary_key() const { return id; }
    };

    //scope is vote.id
    //ram payer:voter
    TABLE vote_records {
        name voter;
        uint8_t vote_type;
        uint64_t weight;

        uint64_t primary_key() const { return voter.value; }
    };

    // scope is self
    TABLE exec_actions {
        uint64_t vote_id;
        name contract;
        name act;
        vector<char> params; // pack_action_data

        uint64_t primary_key() const { return vote_id; }
    };

    //scope is vote id
    TABLE token_snapshot {
        name account;
        asset quantity;

        uint64_t primary_key() const { return account.value; }
    };

    using dsconf_idx = singleton<"dsconf"_n, dsconf>;
    using vote_idx = multi_index<"vote"_n, votes>;
    using execaction_idx = multi_index<"execaction"_n, exec_actions>;
    using vote_record_idx = multi_index<"voterecord"_n, vote_records>;
    using tksnapshot_idx = multi_index<"tksnapshot"_n, token_snapshot>;

    //0% = 0
    //1% = 10^10
    //100% = 10^12
    const uint64_t BASE_SCALE = 1000000000000; //10^12

    // autonomous actions
    const name VA_CHANGE_SR = name("changesr");
    const name VA_CHANGE_MAQ = name("changemaq");
    const name VA_CHANGE_VTS = name("changevts");
    const name VA_NEWVOTE = name("newvote");

private:
    void _check(const name& act, const name& caller);

    void _require_config();

    bool _is_vote_open(const uint64_t& state, const uint64_t& expire_time);

    bool _is_expire(const uint64_t& expire_time);

    bool _is_passed(const votes& v);

    bool _is_reach(const uint64_t& value, const uint64_t& total, const uint64_t& target);

    dsconf _get_config();

    uint64_t _get_token_supply();

    asset _get_balance(const name& account);

    asset _get_balance_at_snapshot(const name& account, const uint64_t& vote_id);

    void _maybe_pass(const uint64_t& vote_id);
};
