#include <organization.hpp>

ACTION organization::initorg(const string& org_name, const name& founder, const string& des_cid) {
    require_auth(get_self());
    org_idx ot(get_self(), get_self().value);
    check(!ot.exists(), CODE_10000);
    check(org_name.size() <= 40, CODE_10001);

    auto ot_slt = ot.get_or_create(get_self(), org { org_name, founder, des_cid });
    ot.set(ot_slt, get_self());

    // load the org basic plugin from plugincenter
    pgcenter_idx pgmt(GUIDE_CONTRACT, GUIDE_CONTRACT.value);
    auto pmitr = pgmt.find(name("organization").value);
    check(pmitr != pgmt.end(), CODE_10002);

    plugin_idx pgt(get_self(), get_self().value);
    auto pitr = pgt.find(get_self().value);
    if (pitr == pgt.end()) {
        pgt.emplace(get_self(), [&](auto& m) {
            m.pcontract = get_self();
            m.pcode = pmitr->pcode;
            m.pname = pmitr->pname;
            m.version = pmitr->version;
            m.des_cid = pmitr->des_cid;
            m.autonomous_acts = pmitr->autonomous_acts;
            m.is_basic = pmitr->is_basic;
        });
    }
    require_recipient(LOG_REC);
}

ACTION organization::changename(const name& caller, const string& org_name) {
    require_auth(caller);

    org_idx ot(get_self(), get_self().value);
    check(ot.exists(), CODE_10003);
    check(org_name.size() <= 40, CODE_10001);

    _check(VA_CHANGE_NAME, caller);

    auto org_slt = ot.get();
    org_slt.org_name = org_name;
    ot.set(org_slt, get_self());

    require_recipient(LOG_REC);
}

ACTION organization::changedescid(const name& caller, const string& des_cid) {
    require_auth(caller);

    org_idx ot(get_self(), get_self().value);
    check(ot.exists(), CODE_10003);

    _check(VA_CHANGE_DESCID, caller);

    auto org_slt = ot.get();
    org_slt.des_cid = des_cid;
    ot.set(org_slt, get_self());

    require_recipient(LOG_REC);
}

ACTION organization::changeperm(const name& caller, const name& pcontract, const name& pcode, const name& act, vector<name>& access) {
    require_auth(caller);

    _check(VA_CHANGE_PERM, caller);

    permission_idx pt(get_self(), pcontract.value);
    auto aitr = pt.find(act.value);
    if (aitr == pt.end()) {
        pt.emplace(get_self(), [&](auto& m) {
            m.act = act;
            m.pcode = pcode;
            m.access = access;
        });
    } else {
        pt.modify(aitr, same_payer, [&](auto& m) {
            m.access = access;
        });
    }

    require_recipient(LOG_REC);
}

ACTION organization::reg(const name& account, const string& email) {
    require_auth(account);

    regaccount_idx rt(get_self(), get_self().value);
    auto ritr = rt.find(account.value);
    if (ritr == rt.end()) {
        rt.emplace(account, [&](auto& m) {
            m.account = account;
            m.email = email;
        });
    } else {
        rt.modify(ritr, same_payer, [&](auto& m) {
            m.email = email;
        });
    }

    require_recipient(LOG_REC);
}

ACTION organization::addplugin(const name& caller,
                               const name& pcontract,
                               const name& pcode,
                               const string& pname,
                               const string& version,
                               const string& des_cid,
                               const vector<name>& autonomous_acts) {
    require_auth(caller);

    _check(VA_APPPLUGIN, caller);

    check(is_account(pcontract), CODE_10004);

    plugin_idx pgt(get_self(), get_self().value);
    auto pitr = pgt.find(pcontract.value);
    check(pitr == pgt.end(), CODE_10005);

    check(pname.size() <= 30, CODE_10006);

    pgt.emplace(get_self(), [&](auto& m) {
        m.pcontract = pcontract;
        m.pcode = pcode;
        m.pname = pname;
        m.version = version;
        m.des_cid = des_cid;
        m.autonomous_acts = autonomous_acts;
        m.is_basic = false;
    });

    require_recipient(LOG_REC);
}

ACTION organization::approvetrx(const name& caller,
                                const name& contract,
                                const name& to,
                                const asset& quantity,
                                const string& memo) {
    require_auth(caller);
    _check(VA_APPROVETRX, caller);

    _add_allow_trx(contract, to, quantity);
    action(permission_level { get_self(), name("active") },
           contract, name("transfer"),
           std::make_tuple(get_self(), to, quantity, memo))
        .send();
}

ACTION organization::approvenft(const name& caller,
                                const name& contract,
                                const name& to,
                                const vector<uint64_t>& ids,
                                const string& memo) {
    require_auth(caller);
    _check(VA_APPROVE_TRX_NFT, caller);

    action(permission_level { get_self(), name("active") },
           contract, name("transfernft"),
           std::make_tuple(get_self(), to, ids, memo))
        .send();
}

ACTION organization::mtransfer(const name& contract, const structs::trx_tb& tx) {
    if (tx.from == get_self()) {
        _remove_allow_trx(contract, tx.to, tx.quantity);
        require_recipient(LOG_REC);
        return;
    }

    if (tx.to != get_self())
        return;

    require_recipient(LOG_REC);
    if ("donate" == tx.memo) {
        donatepool_idx dpt(get_self(), contract.value);
        //A contract may have more than one token symbol
        auto idx = dpt.get_index<name("byaccount")>();
        auto itr = idx.lower_bound(tx.from.value);
        bool exist = false;
        while (itr != idx.end() && itr->account == tx.from) {
            if (itr->quantity.symbol == tx.quantity.symbol) {
                idx.modify(itr, same_payer, [&](auto& m) {
                    m.quantity += tx.quantity;
                });
                exist = true;
                break;
            }
            itr++;
        }
        if (!exist) {
            dpt.emplace(get_self(), [&](auto& m) {
                m.id = dpt.available_primary_key();
                m.account = tx.from;
                m.quantity = tx.quantity;
            });
        }
    }
}

void organization::_check(const name& act, const name& caller) {
    auto auth = auths();
    auth.check_perm(get_self(), get_self(), act, caller);
}

void organization::_add_allow_trx(const name& contract, const name& to, const asset& quantity) {
    allowtrx_idx allow_table(get_self(), contract.value);
    allow_table.emplace(get_self(), [&](auto& m) {
        m.id = allow_table.available_primary_key();
        m.to = to;
        m.quantity = quantity;
    });
}

void organization::_remove_allow_trx(const name& contract, const name& to, const asset& quantity) {
    allowtrx_idx allow_table(get_self(), contract.value);
    auto idx = allow_table.get_index<name("byto")>();
    auto itr = idx.lower_bound(to.value);
    bool exist = false;
    while (itr != idx.end() && itr->to == to) {
        if (itr->quantity == quantity) {
            idx.erase(itr);
            exist = true;
            break;
        }
        itr++;
    }
    check(exist, CODE_10007);
}