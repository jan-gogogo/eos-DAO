#include <swap.hpp>

ACTION dsswap::vote(const name& token_contract, const name& voter) {
    require_auth(voter);

    propose_idx pt(get_self(), get_self().value);
    auto pitr = pt.find(token_contract.value);
    check(pitr != pt.end(), "token_contract does not exists");

    vote_idx vt(get_self(), token_contract.value);
    auto vitr = vt.find(voter.value);
    check(vitr == vt.end(), "vote exists already");

    const uint64_t votes = safemath::add(pitr->total_votes, 1);
    if (votes >= 100) {
        token_idx tt(get_self(), get_self().value);
        tt.emplace(get_self(), [&](auto& m) {
            m.contract = pitr->contract;
            m.total_supply = pitr->total_supply;
            m.token_balance = pitr->token_balance;
            m.eos_balance = pitr->eos_balance;
        });

        pt.erase(pitr);

        //delete vote records
        auto ditr = vt.begin();
        while (ditr != vt.end()) {
            ditr = vt.erase(ditr);
        }
    } else {
        pt.modify(pitr, same_payer, [&](auto& m) {
            m.total_votes = votes;
        });

        vt.emplace(voter, [&](auto& m) {
            m.voter = voter;
        });
    }
}

ACTION dsswap::mtransfer(const name& contract, const structs::trx_tb& tx) {
    if (tx.from == get_self() || tx.to != get_self())
        return;
    if (tx.memo.empty())
        return;

    vector<string> vec;
    _split_memo(vec, tx.memo, ',');
    const string act = _split_val(vec, "action");

    if (act == "create_pair") {
        _check_eos(contract, tx.quantity);
        _create_pair(tx, vec);
    } else if (act == "add_liquidity") {
        _check_eos(contract, tx.quantity);
        _add_liquidity(tx, vec);
    } else if (act == "remove_liquidity") {
        _remove_liquidity(contract, tx, vec);
    } else if (act == "eos_to_token") {
        _check_eos(contract, tx.quantity);
        _eos_to_token(tx, vec);
    } else if (act == "token_to_eos") {
        _token_to_eos(contract, tx, vec);
    } else if (act == "token_to_token") {
        _token_to_token(contract, tx, vec);
    } else if (act == "approve") {
        _approve(contract, tx);
    } else if (act == "cancel_approve") {
        _cancel_approve(contract, tx, vec);
    } else if (act == "apply") {
        _check_eos(contract, tx.quantity);
        _apply_add_token(tx, vec);
    }
}

void dsswap::_create_pair(const structs::trx_tb& tx, vector<string> vec) {
    const name token_contract = name(_split_val(vec, "token_contract"));
    const uint64_t deadline = stoll(_split_val(vec, "deadline"));
    const uint64_t max_tokens = stoll(_split_val(vec, "max_tokens"));

    check(max_tokens > 0, "invalid max_tokens");
    check(deadline > current_time_point().sec_since_epoch(), "deadline is up");

    tokens tk = _get_token(token_contract);
    const uint64_t total_liquidity = tk.total_supply;
    check(total_liquidity == 0, "can't create pair");

    const asset token_amount = asset(max_tokens, tk.token_balance.symbol);
    const uint64_t initial_liquidity = safemath::add(tk.eos_balance.amount, tx.quantity.amount);

    mint_idx mt(get_self(), token_contract.value);
    auto mitr = mt.find(tx.from.value);
    check(mitr == mt.end(), "mint table exists already");
    mt.emplace(get_self(), [&](auto& m) {
        m.account = tx.from;
        m.amount = initial_liquidity;
    });

    _transfer_from(tx.from, token_contract, token_amount);

    // update token data
    token_idx tt(get_self(), get_self().value);
    auto itr = tt.find(token_contract.value);
    tt.modify(itr, same_payer, [&](auto& m) {
        m.eos_balance += tx.quantity;
        m.total_supply = initial_liquidity;
    });
}

void dsswap::_add_liquidity(const structs::trx_tb& tx, vector<string> vec) {
    const name token_contract = name(_split_val(vec, "token_contract"));
    const uint64_t deadline = stoll(_split_val(vec, "deadline"));
    const uint64_t min_liquidity = stoll(_split_val(vec, "min_liquidity"));
    const uint64_t max_tokens = stoll(_split_val(vec, "max_tokens"));

    check(min_liquidity > 0, "invalid min_liquidity");
    check(max_tokens > 0, "invalid max_tokens");
    check(deadline > current_time_point().sec_since_epoch(), "deadline is up");

    tokens tk = _get_token(token_contract);
    const uint64_t total_liquidity = tk.total_supply;
    check(total_liquidity > 0, "total_supply must be positive");

    const asset eos_reserve = tk.eos_balance;
    const asset token_reserve = tk.token_balance;

    const asset token_amount = token_reserve * tx.quantity.amount / eos_reserve.amount + asset(1, token_reserve.symbol);
    const uint64_t liquidity_minted = safemath::div(safemath::mul(tx.quantity.amount, total_liquidity), eos_reserve.amount);
    check(max_tokens >= token_amount.amount && liquidity_minted >= min_liquidity, "did not meet expectations");

    // increase provider balance
    mint_idx mt(get_self(), token_contract.value);
    auto mitr = mt.find(tx.from.value);
    if (mitr == mt.end()) {
        mt.emplace(get_self(), [&](auto& m) {
            m.account = tx.from;
            m.amount = liquidity_minted;
        });
    } else {
        mt.modify(mitr, same_payer, [&](auto& m) {
            m.amount = safemath::add(m.amount, liquidity_minted);
        });
    }

    _transfer_from(tx.from, token_contract, token_amount);

    // update token data
    token_idx tt(get_self(), get_self().value);
    auto itr = tt.find(token_contract.value);
    tt.modify(itr, same_payer, [&](auto& m) {
        m.eos_balance += tx.quantity;
        m.total_supply = safemath::add(total_liquidity, liquidity_minted);
    });
}

void dsswap::_remove_liquidity(const name& contract, const structs::trx_tb& tx, vector<string> vec) {
    const name token_contract = name(_split_val(vec, "token_contract"));
    const uint64_t amount = stoll(_split_val(vec, "amount"));
    const uint64_t min_eos = stoll(_split_val(vec, "min_eos"));
    const uint64_t min_tokens = stoll(_split_val(vec, "min_tokens"));
    const uint64_t deadline = stoll(_split_val(vec, "deadline"));

    check(amount > 0, "invalid amount");
    check(min_eos > 0, "invalid min_eos");
    check(min_tokens > 0, "invalid min_tokens");
    check(deadline > current_time_point().sec_since_epoch(), "deadline is up");

    tokens tk = _get_token(token_contract);
    check(tk.total_supply > 0, "invalid total_supply value");

    const uint64_t total_liquidity = tk.total_supply;
    const asset token_reserve = tk.token_balance;
    const asset eos_amount = tk.eos_balance * amount / total_liquidity;
    const asset token_amount = token_reserve * amount / total_liquidity;
    check(eos_amount.amount >= min_eos and token_amount.amount >= min_tokens, "tokens did not meet expectations");

    //update liquidity provider balance
    mint_idx mt(get_self(), token_contract.value);
    auto mitr = mt.find(tx.from.value);
    check(mitr != mt.end() && mitr->amount >= amount, "amount too high");
    mt.modify(mitr, same_payer, [&](auto& m) {
        m.amount -= amount;
    });

    //update token balance
    token_idx tt(get_self(), get_self().value);
    auto itr = tt.find(token_contract.value);
    tt.modify(itr, same_payer, [&](auto& m) {
        m.token_balance -= token_amount;
        m.eos_balance -= eos_amount;
        m.total_supply = safemath::sub(total_liquidity, amount);
    });

    _transfer_action(EOS_CONTRACT, tx.from, eos_amount, "remove liquidity");
    _transfer_action(token_contract, tx.from, token_amount, "remove liquidity");

    //returns
    _transfer_action(contract, tx.from, tx.quantity, "returns");
}

void dsswap::_eos_to_token(const structs::trx_tb& tx, vector<string> vec) {
    const name token_contract = name(_split_val(vec, "token_contract"));
    const uint64_t deadline = stoll(_split_val(vec, "deadline"));
    const uint64_t min_tokens = stoll(_split_val(vec, "min_tokens"));

    check(min_tokens > 0, "invalid min_tokens");
    check(deadline > current_time_point().sec_since_epoch(), "deadline is up");

    tokens tk = _get_token(token_contract);
    const asset token_reserve = tk.token_balance;
    const uint64_t tokens_bought = _get_input_price(tx.quantity.amount, tk.eos_balance.amount, token_reserve.amount);
    check(tokens_bought >= min_tokens, "tokens did not meet expectations");

    const asset tokens_bought_asset = asset(tokens_bought, tk.token_balance.symbol);

    token_idx tt(get_self(), get_self().value);
    auto itr = tt.find(token_contract.value);
    check(itr->token_balance >= tokens_bought_asset, "eos to token overdrawn balance");

    tt.modify(itr, same_payer, [&](auto& m) {
        m.token_balance -= tokens_bought_asset;
        m.eos_balance += tx.quantity;
    });

    _transfer_action(token_contract, tx.from, tokens_bought_asset, "eos to token");
}

void dsswap::_token_to_eos(const name& contract, const structs::trx_tb& tx, vector<string> vec) {
    const name token_contract = contract;
    const uint64_t deadline = stoll(_split_val(vec, "deadline"));
    const uint64_t min_eos = stoll(_split_val(vec, "min_eos"));
    const asset tokens_sold = tx.quantity;

    check(min_eos > 0, "invalid min_eos");
    check(deadline > current_time_point().sec_since_epoch(), "deadline is up");

    tokens tk = _get_token(token_contract);
    _check_asset(tokens_sold, tk.token_balance.symbol);

    const uint64_t token_reserve = tk.token_balance.amount;
    const uint64_t eos_bought = _get_input_price(tokens_sold.amount, token_reserve, tk.eos_balance.amount);
    check(eos_bought >= min_eos, "eos did not meet expectations");

    token_idx tt(get_self(), get_self().value);
    auto itr = tt.find(token_contract.value);
    check(itr->eos_balance.amount >= eos_bought, "token to eos overdrawn balance");

    const asset eos_bought_asset = asset(eos_bought, EOS_SYMBOL);
    tt.modify(itr, same_payer, [&](auto& m) {
        m.token_balance += tokens_sold;
        m.eos_balance -= eos_bought_asset;
    });

    _transfer_action(EOS_CONTRACT, tx.from, eos_bought_asset, "token to eos");
}

void dsswap::_token_to_token(const name& contract, const structs::trx_tb& tx, vector<string> vec) {
    const name sold_contract = contract;
    const asset tokens_sold = tx.quantity;
    const name bought_contract = name(_split_val(vec, "token_contract")); // The contract of the token being purchased.
    const uint64_t deadline = stoll(_split_val(vec, "deadline"));
    const string min_tokens_bought = _split_val(vec, "min_tokens_bought");
    const uint64_t min_eos_bought = stoll(_split_val(vec, "min_eos_bought"));

    check(min_eos_bought > 0, "invalid min_eos_bought");
    check(deadline > current_time_point().sec_since_epoch(), "deadline is up");
    check(sold_contract != bought_contract, "must be different than sold token contract");

    tokens sold_tk = _get_token(sold_contract);
    _check_asset(tokens_sold, sold_tk.token_balance.symbol);

    const uint64_t token_reserve = sold_tk.token_balance.amount;
    const uint64_t eos_bought = _get_input_price(tokens_sold.amount, token_reserve, sold_tk.eos_balance.amount);
    check(eos_bought >= min_eos_bought, "eos did not meet expectations");

    const asset eos_bought_asset = asset(eos_bought, EOS_SYMBOL);
    token_idx tt(get_self(), get_self().value);
    auto itr = tt.find(sold_contract.value);
    tt.modify(itr, same_payer, [&](auto& m) {
        m.token_balance += tokens_sold;
        m.eos_balance -= eos_bought_asset;
    });

    structs::trx_tb req_tx = { .from = tx.from,
                               .to = bought_contract,
                               .quantity = eos_bought_asset,
                               string("") };
    vector<string> req_vec;
    req_vec.emplace_back(_split_val(vec, "token_contract", true));
    req_vec.emplace_back(_split_val(vec, "deadline", true));
    req_vec.emplace_back(string("min_tokens:") + min_tokens_bought);

    _eos_to_token(req_tx, req_vec);
}

void dsswap::_approve(const name& contract, const structs::trx_tb& tx) {
    token_idx tt(get_self(), get_self().value);
    auto titr = tt.find(contract.value);
    check(titr != tt.end(), "token contract does not register");
    _check_asset(tx.quantity, titr->token_balance.symbol);

    allow_idx atb(get_self(), contract.value);
    auto itr = atb.find(tx.from.value);
    if (itr == atb.end()) {
        atb.emplace(get_self(), [&](auto& m) {
            m.account = tx.from;
            m.balance = tx.quantity;
        });
    } else {
        atb.modify(itr, same_payer, [&](auto& m) {
            m.balance += tx.quantity;
        });
    }
}

void dsswap::_cancel_approve(const name& contract, const structs::trx_tb& tx, vector<string> vec) {
    const name token_contract = name(_split_val(vec, "token_contract"));

    allow_idx atb(get_self(), token_contract.value);
    auto itr = atb.find(tx.from.value);
    check(itr != atb.end(), "allows data does not exist");

    const asset return_amount = itr->balance;
    atb.erase(itr);

    _transfer_action(token_contract, tx.from, return_amount, "cancel approve");

    // returns
    _transfer_action(contract, tx.from, tx.quantity, "returns");
}

void dsswap::_apply_add_token(const structs::trx_tb& tx, vector<string> vec) {
    const name token_contract = name(_split_val(vec, "token_contract"));
    const symbol_code code = symbol_code(_split_val(vec, "symbol_code"));

    check(tx.quantity.amount == 100 * 10000, "100 EOS required");
    check(is_account(token_contract), "token_contract does not exist");
    check(token_contract != EOS_CONTRACT, "invalid token_contract");

    stats_idx st(token_contract, code.raw());
    auto sitr = st.find(code.raw());
    check(sitr != st.end(), "asset symbol does not exist");
    check(sitr->max_supply.symbol.precision() == 4, "asset precision 4");

    propose_idx pt(get_self(), get_self().value);
    auto itr = pt.find(token_contract.value);
    check(itr == pt.end(), "token_contract exists already");
    pt.emplace(get_self(), [&](auto& m) {
        m.contract = token_contract;
        m.total_supply = 0;
        m.token_balance = asset(0, sitr->max_supply.symbol);
        m.eos_balance = asset(0, EOS_SYMBOL);
        m.total_votes = 0;
    });
}

void dsswap::_check_eos(const name& contract, const asset& quantity) {
    check(contract == EOS_CONTRACT, "eosio.token only");
    _check_asset_eos(quantity);
}

void dsswap::_check_asset_eos(const asset& quantity) {
    _check_asset(quantity, EOS_SYMBOL);
}

void dsswap::_check_asset(const asset& quantity, const symbol& symbol) {
    check(quantity.is_valid() && quantity.amount > 0 && quantity.symbol == symbol, "invalid asset quantity");
}

void dsswap::_split_memo(vector<string>& results, string memo, char separator) {
    results.clear();
    auto start_inx = memo.cbegin();
    auto end_inx = memo.cend();
    bool is_empty_str_last = false;
    size_t len = memo.size();
    size_t index = 0;
    for (auto it = start_inx; it != end_inx; ++it) {
        if (*it == separator) {
            results.emplace_back(start_inx, it);
            start_inx = it + 1;
            is_empty_str_last = index == len - 1;
        }
        index++;
    }
    if (start_inx != end_inx)
        results.emplace_back(start_inx, end_inx);
    if (is_empty_str_last)
        results.emplace_back(std::string(""));
}

string dsswap::_split_val(const vector<string>& kv_vec, string k) {
    return _split_val(kv_vec, k, false);
}

string dsswap::_split_val(const vector<string>& kv_vec, string k, bool kv) {
    vector<string> vec;
    for (size_t i = 0; i < kv_vec.size(); i++) {
        _split_memo(vec, kv_vec[i], ':');
        if (vec[0] == k) {
            return kv ? kv_vec[i] : vec[1];
        }
    }
    check(false, "parse memo error");
    return "";
}

dsswap::tokens dsswap::_get_token(const name& contract) {
    token_idx tt(get_self(), get_self().value);
    auto itr = tt.find(contract.value);
    check(itr != tt.end(), "token contract does not register");
    return *itr;
}

uint64_t dsswap::_get_input_price(const uint64_t& input_amount, const uint64_t& input_reserve, const uint64_t& output_reserve) {
    check(input_reserve > 0 && output_reserve > 0, "invalid input amount or reserve");
    const uint64_t input_amount_with_fee = safemath::mul(input_amount, 997);
    const uint64_t numerator = safemath::mul(input_amount_with_fee, output_reserve);
    const uint64_t denominator = safemath::add(safemath::mul(input_reserve, 1000), input_amount_with_fee);
    return safemath::div(numerator, denominator);
}

void dsswap::_transfer_action(const name& contract, const name& to, const asset& quantity, const string& memo) {
    action(permission_level { get_self(), name("active") },
           contract, name("transfer"),
           std::make_tuple(get_self(), to, quantity, memo))
        .send();
}

void dsswap::_transfer_from(const name& from, const name& contract, const asset& quantity) {
    allow_idx atb(get_self(), contract.value);
    auto itr = atb.find(from.value);
    check(itr != atb.end() && itr->balance >= quantity, "transfer_from overdrawn balance");

    atb.modify(itr, same_payer, [&](auto& m) {
        m.balance -= quantity;
    });

    token_idx tt(get_self(), get_self().value);
    auto titr = tt.find(contract.value);
    tt.modify(titr, same_payer, [&](auto& m) {
        m.token_balance += quantity;
    });
}
