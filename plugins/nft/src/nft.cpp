#include <nft.hpp>

ACTION nft::init(const name& org_contract) {
    require_auth(get_self());
    dsconf_idx ct(get_self(), get_self().value);
    check(!ct.exists(), CODE_10002);
    check(is_account(org_contract), CODE_10003);

    dsconf conf;
    conf.org_contract = org_contract;
    ct.set(conf, get_self());
}

ACTION nft::create(const name& caller,
                   const name& issuer,
                   const name& token_name,
                   const bool& transferable,
                   const uint64_t& max_supply,
                   const string& des_cid) {
    require_auth(caller);
    _check(VA_CREATE, caller);
    _require_config();

    stats_idx st(get_self(), get_self().value);
    check(st.find(token_name.value) == st.end(), CODE_10001);

    st.emplace(get_self(), [&](auto& m) {
        m.token_name = token_name;
        m.transferable = transferable;
        m.issuer = issuer;
        m.max_supply = max_supply;
        m.issued_supply = 0;
        m.des_cid = des_cid;
    });
    require_recipient(LOG_REC);
}

ACTION nft::issue(const name& caller,
                  const name& token_name,
                  const name& to,
                  const uint64_t& amount,
                  const string& des_cid) {
    require_auth(caller);
    _require_config();
    check(amount <= 20, CODE_10007);
    check(is_account(to), CODE_10006);

    stats_idx st(get_self(), get_self().value);
    auto itr = st.find(token_name.value);
    check(itr != st.end(), CODE_10004);

    if (itr->issuer == _get_config().org_contract) {
        _check(VA_ISSUE, caller);
    } else {
        require_auth(itr->issuer);
    }

    check(itr->max_supply >= safemath::add(itr->issued_supply, amount), CODE_10005);

    items_idx it(get_self(), get_self().value);

    uint64_t sn = itr->issued_supply + 1;
    for (int i = 0; i < amount; i++) {
        it.emplace(get_self(), [&](auto& m) {
            m.id = it.available_primary_key();
            m.serial_number = sn;
            m.owner = to;
            m.token_name = token_name;
            m.des_cid = des_cid;
        });
        sn++;
    }

    st.modify(itr, same_payer, [&](auto& m) {
        m.issued_supply += amount; //safe math already
    });

    _add_balance(to, token_name, amount, has_auth(itr->issuer) ? itr->issuer : caller);

    require_recipient(LOG_REC);
}

ACTION nft::transfernft(const name& from,
                        const name& to,
                        const vector<uint64_t>& ids,
                        const string& memo) {
    check(from != to, CODE_10008);
    check(ids.size() <= 10, CODE_10012);
    require_auth(from);
    check(is_account(to), CODE_10009);

    stats_idx st(get_self(), get_self().value);
    items_idx it(get_self(), get_self().value);
    for (auto const& id : ids) {
        auto itr = it.find(id);
        check(itr != it.end(), CODE_10014);

        auto sitr = st.find(itr->token_name.value);
        check(itr != it.end(), CODE_10004);
        check(sitr->transferable, CODE_10013);

        it.modify(itr, same_payer, [&](auto& t) {
            t.owner = to;
        });
        _sub_balance(from, itr->token_name, 1);
        _add_balance(to, itr->token_name, 1, has_auth(to) ? to : from);
    }

    require_recipient(from);
    require_recipient(to);
    require_recipient(LOG_REC);
}

void nft::_check(const name& act, const name& caller) {
    auto auth = auths();
    auth.check_perm(_get_config().org_contract, get_self(), act, caller);
}

void nft::_require_config() {
    dsconf_idx ct(get_self(), get_self().value);
    check(ct.exists(), CODE_10000);
}

nft::dsconf nft::_get_config() {
    dsconf_idx ct(get_self(), get_self().value);
    return ct.get();
}

void nft::_add_balance(const name& owner, const name& token_name, const uint64_t& quantity, const name& ram_payer) {
    acct_idx to_acnts(get_self(), owner.value);
    auto itr = to_acnts.find(token_name.value);
    if (itr == to_acnts.end()) {
        to_acnts.emplace(ram_payer, [&](auto& a) {
            a.token_name = token_name;
            a.amount = quantity;
        });
    } else {
        to_acnts.modify(itr, same_payer, [&](auto& a) {
            a.amount = safemath::add(a.amount, quantity);
        });
    }
}

void nft::_sub_balance(const name& owner, const name& token_name, const uint64_t& quantity) {
    acct_idx from_acnts(get_self(), owner.value);
    auto itr = from_acnts.find(token_name.value);
    check(itr != from_acnts.end(), CODE_10010);
    check(itr->amount >= quantity, CODE_10011);

    if (itr->amount == quantity) {
        from_acnts.erase(itr);
    } else {
        from_acnts.modify(itr, same_payer, [&](auto& a) {
            a.amount -= quantity;
        });
    }
}