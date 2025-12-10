#include <voting.hpp>

ACTION voting::init(const name& token,
                    const uint64_t& support_required,
                    const uint64_t& min_accept_quorum,
                    const uint64_t& vote_time_sec,
                    const name& org_contract) {
    require_auth(get_self());

    dsconf_idx ct(get_self(), get_self().value);
    check(!ct.exists(), CODE_10000);

    check(min_accept_quorum <= support_required, CODE_10001);
    check(support_required <= BASE_SCALE, CODE_10002);
    check(is_account(org_contract), CODE_10003);

    voting::dsconf conf_slt;
    conf_slt.org_contract = org_contract;
    conf_slt.token = token;
    conf_slt.support_required = support_required;
    conf_slt.min_accept_quorum = min_accept_quorum;
    conf_slt.vote_time_sec = vote_time_sec;
    ct.set(conf_slt, get_self());

    require_recipient(LOG_REC);
}

ACTION voting::changesr(const name& caller, const uint64_t& support_required) {
    require_auth(caller);
    _check(VA_CHANGE_SR, caller);
    _require_config();

    dsconf_idx ct(get_self(), get_self().value);
    auto config = ct.get();
    check(config.min_accept_quorum <= support_required, CODE_10004);

    config.support_required = support_required;
    ct.set(config, get_self());

    require_recipient(LOG_REC);
}

ACTION voting::changemaq(const name& caller, const uint64_t& min_accept_quorum) {
    require_auth(caller);
    _check(VA_CHANGE_MAQ, caller);
    _require_config();

    dsconf_idx ct(get_self(), get_self().value);
    auto config = ct.get();
    check(min_accept_quorum <= config.support_required, CODE_10005);

    config.min_accept_quorum = min_accept_quorum;
    ct.set(config, get_self());

    require_recipient(LOG_REC);
}

ACTION voting::changevts(const name& caller, const uint64_t& vote_time_sec) {
    require_auth(caller);
    _check(VA_CHANGE_VTS, caller);
    _require_config();

    dsconf_idx ct(get_self(), get_self().value);
    auto config = ct.get();
    config.vote_time_sec = vote_time_sec;
    ct.set(config, get_self());

    require_recipient(LOG_REC);
}

ACTION voting::newvote(const name& proposer,
                       const string& description,
                       const string& meta_cid,
                       const name& exec_contract,
                       const name& exec_act,
                       const vector<char>& exec_params) {
    require_auth(proposer);
    _require_config();
    check(description.size() <= 300, CODE_10006);

    auto org_contract = _get_config().org_contract;
    auto auth = auths();
    if (!auth.has_permisson(org_contract, get_self(), VA_NEWVOTE, proposer)) {
        if (auth.has_permisson(org_contract, get_self(), VA_NEWVOTE, auth.AUTH_TOKEN_HOLDER)) {
            check(_get_balance(proposer).amount > 0, CODE_10007);
        } else {
            check(false, CODE_10018);
        }
    }
    bool on_chain_exec = name { "" } != exec_contract && name { "" } != exec_act;

    if (on_chain_exec)
        check(is_account(exec_contract), CODE_10008);

    dsconf c = _get_config();
    vote_idx vt(get_self(), get_self().value);
    auto vote_id = vt.available_primary_key();
    const uint64_t expire_time = current_time_point().sec_since_epoch() + c.vote_time_sec;
    const votes vote = {
        .id = vote_id,
        .support_required = c.support_required,
        .min_accept_quorum = c.min_accept_quorum,
        .yea = 0,
        .nay = 0,
        .meta_cid = meta_cid,
        .proposer = proposer,
        .start_time = current_time_point().sec_since_epoch(),
        .expire_time = expire_time,
        .total_weight = _get_token_supply(),
        .chain_exec = on_chain_exec,
        .state = PENDING
    };
    vt.emplace(proposer, [&](auto& m) {
        m = vote;
    });

    exec_actions ea;
    if (on_chain_exec) {
        ea = {
            .vote_id = vote_id,
            .contract = exec_contract,
            .act = exec_act,
            .params = exec_params
        };
        execaction_idx exat(get_self(), get_self().value);
        exat.emplace(proposer, [&](auto& m) {
            m = ea;
        });
    }

    // save the token snapshot
    auto a = assets();
    assets::holder_idx ht(c.token, a.get_coin(c.token).symbol.code().raw());
    auto hitr = ht.begin();

    tksnapshot_idx nst(get_self(), vote_id);
    while (hitr != ht.end()) {
        nst.emplace(proposer, [&](auto& m) {
            m.account = hitr->account;
            m.quantity = hitr->quantity;
        });
        hitr++;
    }

    transaction trx;
    trx.actions.emplace_back(permission_level { get_self(), name("active") },
                             get_self(),
                             name("timeup"),
                             std::make_tuple(vote_id));
    trx.delay_sec = c.vote_time_sec + 1;
    trx.send(vote_id, get_self(), false);

    SEND_INLINE_ACTION(*this, lognewvote, { get_self(), name("active") }, { vote_id, description });
}

ACTION voting::vote(const name& voter, const uint64_t& vote_id, const bool& support) {
    require_auth(voter);

    vote_idx vt(get_self(), get_self().value);
    auto vote = vt.find(vote_id);
    check(vote != vt.end(), CODE_10009);

    const uint64_t voter_weight = _get_balance_at_snapshot(voter, vote_id).amount;

    check(voter_weight > 0, CODE_10010);

    check(_is_vote_open(vote->state, vote->expire_time), CODE_10011);

    vote_record_idx vrt(get_self(), vote->id);
    auto vr = vrt.find(voter.value);
    if (vr == vrt.end()) {
        vrt.emplace(voter, [&](auto& m) {
            m.voter = voter;
            m.vote_type = support ? YEA : NAY;
            m.weight = voter_weight;
        });
    } else {
        const uint64_t old_votes = vr->weight;
        const uint8_t old_vt = vr->vote_type;
        //update the voter last voting
        vt.modify(vote, same_payer, [&](auto& m) {
            if (YEA == old_vt) {
                m.yea = safemath::sub(m.yea, old_votes);
            } else if (NAY == old_vt) {
                m.nay = safemath::sub(m.nay, old_votes);
            }
        });

        vrt.modify(vr, same_payer, [&](auto& m) {
            m.vote_type = support ? YEA : NAY;
            m.weight = voter_weight;
        });
    }

    vt.modify(vote, same_payer, [&](auto& m) {
        if (support) {
            m.yea = safemath::add(m.yea, voter_weight);
        } else {
            m.nay = safemath::add(m.nay, voter_weight);
        }
    });

    _maybe_pass(vote_id);
    require_recipient(LOG_REC);
}

ACTION voting::executevote(const uint64_t& vote_id) {
    vote_idx vt(get_self(), get_self().value);
    auto vote = vt.find(vote_id);
    check(vote != vt.end(), CODE_10009);

    check(_is_passed(*vote), CODE_10012);
    vt.modify(vote, same_payer, [&](auto& m) {
        m.state = EXECUTED;
    });

    if (vote->chain_exec) {
        // execute transaction
        execaction_idx exat(get_self(), get_self().value);
        auto eitr = exat.find(vote_id);
        check(eitr != exat.end(), CODE_10013);

        auto trx = trxs();
        trx.execute(eitr->contract, eitr->act, permission_level { get_self(), "active"_n }, eitr->params);

    } else {
        require_auth(vote->proposer);
    }

    //delete tokens snapshot
    tksnapshot_idx nst(get_self(), vote_id);
    auto ssitr = nst.begin();
    while (ssitr != nst.end()) {
        ssitr = nst.erase(ssitr);
    }

    require_recipient(vote->proposer);
    require_recipient(LOG_REC);
}

ACTION voting::timeup(const uint64_t& vote_id) {
    vote_idx vt(get_self(), get_self().value);
    auto vote = vt.find(vote_id);
    check(vote != vt.end(), CODE_10009);
    check(_is_expire(vote->expire_time), CODE_10014);
    check(vote->state == PENDING, CODE_10015);

    const bool passed = _is_passed(*vote);
    vt.modify(vote, same_payer, [&](auto& m) {
        m.state = passed ? PASSED : REJECTED;
    });

    require_recipient(LOG_REC);
}

ACTION voting::lognewvote(const uint64_t& vote_id, const string& description) {
    require_auth(get_self());
    require_recipient(LOG_REC);
}

void voting::_check(const name& act, const name& caller) {
    auto auth = auths();
    auth.check_perm(_get_config().org_contract, get_self(), act, caller);
}

void voting::_require_config() {
    dsconf_idx ct(get_self(), get_self().value);
    check(ct.exists(), CODE_10016);
}

bool voting::_is_vote_open(const uint64_t& state, const uint64_t& expire_time) {
    return !_is_expire(expire_time) && state == PENDING;
}

bool voting::_is_expire(const uint64_t& expire_time) {
    return current_time_point().sec_since_epoch() >= expire_time;
}

bool voting::_is_passed(const votes& v) {
    if (v.state == EXECUTED || v.state == REJECTED)
        return false;

    if (_is_reach(v.yea, v.total_weight, v.support_required)) {
        return true;
    } else {
        if (_is_expire(v.expire_time)) {
            const uint64_t totol_votes = safemath::add(v.yea, v.nay);
            return _is_reach(v.yea, totol_votes, v.support_required) && _is_reach(v.yea, v.total_weight, v.min_accept_quorum);
        } else {
            return false;
        }
    }
}

bool voting::_is_reach(const uint64_t& value, const uint64_t& total, const uint64_t& target) {
    if (total == 0)
        return false;
    check(value <= total, CODE_10017);
    double r = (double)value / (double)total;
    uint64_t p = r * BASE_SCALE;
    return p > target;
}

voting::dsconf voting::_get_config() {
    dsconf_idx ct(get_self(), get_self().value);
    return ct.get();
}

uint64_t voting::_get_token_supply() {
    auto a = assets();
    assets::currency_stats stats = a.get_stats(_get_config().token);
    return stats.supply.amount;
}

asset voting::_get_balance(const name& account) {
    auto a = assets();
    return a.get_balance(_get_config().token, account);
}

asset voting::_get_balance_at_snapshot(const name& account, const uint64_t& vote_id) {
    tksnapshot_idx nst(get_self(), vote_id);
    auto itr = nst.find(account.value);
    if (itr != nst.end())
        return itr->quantity;

    auto a = assets();
    return asset(0, a.get_coin(_get_config().token).symbol);
}

void voting::_maybe_pass(const uint64_t& vote_id) {
    vote_idx vt(get_self(), get_self().value);
    auto itr = vt.find(vote_id);
    if (_is_passed(*itr)) {
        vt.modify(itr, same_payer, [&](auto& m) {
            m.state = PASSED;
        });
        cancel_deferred(vote_id); //cancel timeup action
    }
}