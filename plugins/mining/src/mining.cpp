#include <mining.hpp>

ACTION mining::init(const name& token,
                       const bool& limit,
                       const asset& max_quantity,
                       const vector<exchange_in>& ins,
                       const name& org_contract) {
    require_auth(get_self());

    _save_conf(token, limit, max_quantity, org_contract, true);

    exin_idx et(get_self(), get_self().value);
    check(et.begin() == et.end(), CODE_10006);
    check(ins.size() > 0, CODE_10001);

    const uint64_t precision_scale = pow(10, safemath::sub(12, max_quantity.symbol.precision()));
    for (auto const& r : ins) {
        check(r.rate >= precision_scale, CODE_10002);
        et.emplace(get_self(), [&](auto& m) {
            m = r;
        });
    }
    require_recipient(LOG_REC);
}

ACTION mining::changeconf(const name& caller,
                             const name& token,
                             const bool& limit,
                             const asset& max_quantity) {
    require_auth(caller);
    _check(VA_CHANGE_CONF, caller);

    _save_conf(token, limit, max_quantity, name { "" }, false);
}

ACTION mining::addexin(const name& caller,
                          const name& token,
                          const symbol_code& symbol_code,
                          const uint64_t rate) {
    require_auth(caller);
    _check(VA_ADD_EXIN, caller);

    exin_idx et(get_self(), get_self().value);
    check(et.find(token.value) == et.end(), CODE_10003);

    dsconf_idx dt(get_self(), get_self().value);

    const uint64_t precision_scale = pow(10, safemath::sub(12, dt.get().max_quantity.symbol.precision()));
    check(rate >= precision_scale, CODE_10002);

    et.emplace(get_self(), [&](auto& m) {
        m.token = token;
        m.symbol_code = symbol_code;
        m.rate = rate;
    });
}

ACTION mining::rmexin(const name& caller, const name& token) {
    require_auth(caller);
    _check(VA_ADD_EXIN, caller);

    exin_idx et(get_self(), get_self().value);
    auto itr = et.find(token.value);
    check(itr != et.end(), CODE_10004);
    et.erase(itr);
}

ACTION mining::mtransfer(const name& contract, const structs::trx_tb& tx) {
    if (tx.from == get_self() || tx.to != get_self())
        return;

    if ("exchange" != tx.memo)
        return;

    exin_idx rt(get_self(), get_self().value);
    auto itr = rt.find(contract.value);
    if (itr == rt.end())
        return;

    if (tx.quantity.symbol.code() != itr->symbol_code)
        return;

    dsconf_idx dt(get_self(), get_self().value);
    auto conf = dt.get();

    double r = (double)itr->rate / (double)BASE_SCALE;
    asset out = asset((double)tx.quantity.amount * r, conf.max_quantity.symbol);

    if (conf.limit) {
        check(conf.remaining_quantity.amount >= out.amount, CODE_10007);
        conf.remaining_quantity -= out;
        dt.set(conf, get_self());
    }

    // IN
    action(permission_level { get_self(), name("active") },
           contract, name("transfer"),
           std::make_tuple(get_self(), conf.org_contract, tx.quantity, tx.memo))
        .send();

    // OUT
    action(permission_level { get_self(), name("active") },
           conf.token, name("mint"),
           std::make_tuple(get_self(), tx.from, out, tx.memo))
        .send();
}

void mining::_check(const name& act, const name& caller) {
    auto auth = auths();
    auth.check_perm(_get_config().org_contract, get_self(), act, caller);
}

mining::dsconf mining::_get_config() {
    dsconf_idx ct(get_self(), get_self().value);
    return ct.get();
}

void mining::_save_conf(const name& token,
                           const bool& limit,
                           const asset& max_quantity,
                           const name& org_contract,
                           const bool& is_init) {
    dsconf_idx dt(get_self(), get_self().value);

    if (is_init)
        check(!dt.exists(), CODE_10006);
    else
        check(dt.exists(), CODE_10005);

    auto a = assets();
    auto stat = a.get_stats(token, max_quantity.symbol);
    check(stat.supply.symbol == max_quantity.symbol, CODE_10000);

    if (limit) {
        check(max_quantity.amount > 0, CODE_10000);
    }

    dsconf conf_slt;
    if (!is_init) {
        conf_slt = dt.get();
        conf_slt.token = token;
        conf_slt.limit = limit;
        conf_slt.max_quantity = max_quantity;
    } else {
        conf_slt.token = token;
        conf_slt.limit = limit;
        conf_slt.max_quantity = max_quantity;
        conf_slt.remaining_quantity = limit ? max_quantity : asset(0, max_quantity.symbol);
        conf_slt.org_contract = org_contract;
    }

    dt.set(conf_slt, get_self());
}