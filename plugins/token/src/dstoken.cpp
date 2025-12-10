#include <dstoken.hpp>

namespace eosio {

void token::init(const name& issuer, const asset& maximum_supply, const name& org_contract) {
    require_auth(get_self());
    _create(issuer, maximum_supply);

    dsconfs sft(get_self(), get_self().value);
    if (!sft.exists()) {
        auto dfc_slt = sft.get_or_create(get_self(), dsconf { .org_contract = org_contract, .coin = asset(0, maximum_supply.symbol) });
        sft.set(dfc_slt, get_self());
    }
}

void token::retire(const name& caller, const asset& quantity, const string& memo) {
    _check(VA_RETIRE, caller);
    auto sym = quantity.symbol;
    check(sym.is_valid(), "invalid symbol name");
    check(memo.size() <= 256, "memo has more than 256 bytes");

    stats statstable(get_self(), sym.code().raw());
    auto existing = statstable.find(sym.code().raw());
    check(existing != statstable.end(), "token with symbol does not exist");
    const auto& st = *existing;

    check(quantity.is_valid(), "invalid quantity");
    check(quantity.amount > 0, "must retire positive quantity");

    check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");

    statstable.modify(st, same_payer, [&](auto& s) {
        s.supply -= quantity;
    });

    _sub_balance(st.issuer, quantity);
}

void token::transfer(const name& from, const name& to, const asset& quantity, const string& memo) {
    check(from != to, "cannot transfer to self");
    require_auth(from);
    check(is_account(to), "to account does not exist");
    auto sym = quantity.symbol.code();
    stats statstable(get_self(), sym.raw());
    const auto& st = statstable.get(sym.raw());

    require_recipient(from);
    require_recipient(to);

    check(quantity.is_valid(), "invalid quantity");
    check(quantity.amount > 0, "must transfer positive quantity");
    check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
    check(memo.size() <= 256, "memo has more than 256 bytes");

    auto payer = has_auth(to) ? to : from;

    _sub_balance(from, quantity);
    _add_balance(to, quantity, payer);
}

void token::mint(const name& caller, const name& to, const asset& quantity, const string& memo) {
    require_auth(caller);
    _check(VA_MINT, caller);

    _issue(to, quantity, memo);
}

void token::_create(const name& issuer, const asset& maximum_supply) {
    auto sym = maximum_supply.symbol;
    check(sym.is_valid(), "invalid symbol name");
    check(maximum_supply.is_valid(), "invalid supply");
    check(maximum_supply.amount > 0, "max-supply must be positive");

    stats statstable(get_self(), sym.code().raw());
    auto existing = statstable.find(sym.code().raw());
    check(existing == statstable.end(), "token with symbol already exists");

    statstable.emplace(get_self(), [&](auto& s) {
        s.supply.symbol = maximum_supply.symbol;
        s.max_supply = maximum_supply;
        s.issuer = issuer;
    });
}

void token::_sub_balance(const name& owner, const asset& value) {
    accounts from_acnts(get_self(), owner.value);

    const auto& from = from_acnts.get(value.symbol.code().raw(), "no balance object found");
    check(from.balance.amount >= value.amount, "overdrawn balance");

    from_acnts.modify(from, owner, [&](auto& a) {
        a.balance -= value;
    });

    _reset_holder(owner, value.symbol.code(), owner);
}

void token::_add_balance(const name& owner, const asset& value, const name& ram_payer) {
    accounts to_acnts(get_self(), owner.value);
    auto to = to_acnts.find(value.symbol.code().raw());
    if (to == to_acnts.end()) {
        to_acnts.emplace(ram_payer, [&](auto& a) {
            a.balance = value;
        });
    } else {
        to_acnts.modify(to, same_payer, [&](auto& a) {
            a.balance += value;
        });
    }

    _reset_holder(owner, value.symbol.code(), ram_payer);
}

void token::_reset_holder(const name& owner, const symbol_code& sym_code, const name& ram_payer) {
    accounts to_acnts(get_self(), owner.value);
    auto to = to_acnts.find(sym_code.raw());

    holders ht(get_self(), sym_code.raw());
    auto hitr = ht.find(owner.value);

    if (to == to_acnts.end()) {
        if (hitr != ht.end()) {
            ht.erase(hitr);
        }
        return;
    }

    if (hitr == ht.end()) {
        ht.emplace(ram_payer, [&](auto& m) {
            m.account = owner;
            m.quantity = to->balance;
        });
    } else {
        if (to->balance.amount == 0) {
            ht.erase(hitr);
        } else {
            ht.modify(hitr, same_payer, [&](auto& a) {
                a.quantity = to->balance;
            });
        }
    }
}

void token::_issue(const name& to, const asset& quantity, const string& memo) {
    auto sym = quantity.symbol;
    check(sym.is_valid(), "invalid symbol name");
    check(memo.size() <= 256, "memo has more than 256 bytes");

    stats statstable(get_self(), sym.code().raw());
    auto existing = statstable.find(sym.code().raw());
    check(existing != statstable.end(), "token with symbol does not exist, create token before issue");
    const auto& st = *existing;

    check(quantity.is_valid(), "invalid quantity");
    check(quantity.amount > 0, "must issue positive quantity");

    check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
    check(quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    statstable.modify(st, same_payer, [&](auto& s) {
        s.supply += quantity;
    });

    _add_balance(st.issuer, quantity, st.issuer);
    if (to != st.issuer) {
        SEND_INLINE_ACTION(*this, transfer, { st.issuer, name("active") }, { st.issuer, to, quantity, memo });
    }
}

void token::_check(const name& act, const name& caller) {
    auto auth = auths();
    auth.check_perm(_get_config().org_contract, get_self(), act, caller);
}

token::dsconf token::_get_config() {
    dsconfs ct(get_self(), get_self().value);
    return ct.get();
}
}