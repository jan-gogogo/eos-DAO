#include <exchange.hpp>

ACTION tkexchange::init(const name& org_contract) {
    require_auth(get_self());

    dsconf_idx dt(get_self(), get_self().value);
    check(!dt.exists(), "config has initialized");

    dsconf conf_slt;
    conf_slt.org_contract = org_contract;
    dt.set(conf_slt, get_self());
}

ACTION tkexchange::createpair(const name& caller,
                            const name& token_contract,
                            const uint64_t& deadline,
                            const uint64_t& max_tokens,
                            const asset& quantity) {
    require_auth(caller);
    _check(VA_CREATE_PAIR, caller);

    string memo;
    _splicing_action(&memo, "create_pair");
    _splicing_name(&memo, "token_contract", token_contract);
    _splicing_uint64(&memo, "deadline", deadline);
    _splicing_uint64_last(&memo, "max_tokens", max_tokens);

    _send_transaction(EOS_CONTRACT, quantity, memo);
}

ACTION tkexchange::addliquidity(const name& caller,
                              const name& token_contract,
                              const uint64_t& deadline,
                              const uint64_t& min_liquidity,
                              const uint64_t& max_tokens,
                              const asset& quantity) {
    require_auth(caller);
    _check(VA_ADD_LIQUIDITY, caller);

    string memo;
    _splicing_action(&memo, "add_liquidity");
    _splicing_name(&memo, "token_contract", token_contract);
    _splicing_uint64(&memo, "deadline", deadline);
    _splicing_uint64(&memo, "min_liquidity", min_liquidity);
    _splicing_uint64_last(&memo, "max_tokens", max_tokens);

    _send_transaction(EOS_CONTRACT, quantity, memo);
}

ACTION tkexchange::rmliquidity(const name& caller,
                             const name& token_contract,
                             const uint64_t& deadline,
                             const uint64_t& amount,
                             const uint64_t& min_eos,
                             const uint64_t& min_tokens) {
    require_auth(caller);
    _check(VA_REMOVE_LIQUIDITY, caller);

    string memo;
    _splicing_action(&memo, "remove_liquidity");
    _splicing_name(&memo, "token_contract", token_contract);
    _splicing_uint64(&memo, "deadline", deadline);
    _splicing_uint64(&memo, "amount", amount);
    _splicing_uint64(&memo, "min_eos", min_eos);
    _splicing_uint64_last(&memo, "min_tokens", min_tokens);

    _send_transaction(EOS_CONTRACT, asset(1, EOS_SYMBOL), memo);
}

ACTION tkexchange::eostotoken(const name& caller,
                            const name& token_contract,
                            const uint64_t& deadline,
                            const uint64_t& min_tokens,
                            const asset& quantity) {
    require_auth(caller);
    _check(VA_EOS_TO_TOKEN, caller);

    string memo;
    _splicing_action(&memo, "eos_to_token");
    _splicing_name(&memo, "token_contract", token_contract);
    _splicing_uint64(&memo, "deadline", deadline);
    _splicing_uint64_last(&memo, "min_tokens", min_tokens);

    _send_transaction(EOS_CONTRACT, quantity, memo);
}

ACTION tkexchange::tokentoeos(const name& caller,
                            const name& token_contract,
                            const asset& quantity,
                            const uint64_t& deadline,
                            const uint64_t& min_eos) {
    require_auth(caller);
    _check(VA_TOKEN_TO_EOS, caller);

    string memo;
    _splicing_action(&memo, "token_to_eos");
    _splicing_uint64(&memo, "deadline", deadline);
    _splicing_uint64_last(&memo, "min_eos", min_eos);

    _send_transaction(token_contract, quantity, memo);
}

ACTION tkexchange::tokentotoken(const name& caller,
                              const name& sold_token_contract,
                              const asset& tokens_sold,
                              const name& bought_token_contract,
                              const uint64_t& deadline,
                              const uint64_t& min_tokens_bought,
                              const uint64_t& min_eos_bought) {
    require_auth(caller);
    _check(VA_TOKEN_TO_TOKEN, caller);

    string memo;
    _splicing_action(&memo, "token_to_token");
    _splicing_name(&memo, "token_contract", bought_token_contract);
    _splicing_uint64(&memo, "deadline", deadline);
    _splicing_uint64(&memo, "min_tokens_bought", min_tokens_bought);
    _splicing_uint64_last(&memo, "min_eos_bought", min_eos_bought);

    _send_transaction(sold_token_contract, tokens_sold, memo);
}

ACTION tkexchange::approve(const name& caller, const name& token_contract, const asset& quantity) {
    require_auth(caller);
    _check(VA_APPROVE, caller);

    string memo;
    _splicing_action(&memo, "approve");

    _send_transaction(token_contract, quantity, memo);
}

ACTION tkexchange::withdraw(const name& caller, const name& token_contract) {
    require_auth(caller);
    _check(VA_WITHDRAW, caller);

    string memo;
    _splicing_action(&memo, "cancel_approve");
    _splicing_name_last(&memo, "token_contract", token_contract);

    _send_transaction(EOS_CONTRACT, asset(1, EOS_SYMBOL), memo);
}

ACTION tkexchange::apply(const name& caller, const name& token_contract, const symbol_code& symbol_code) {
    require_auth(caller);
    _check(VA_APPLY, caller);

    string memo;
    _splicing_action(&memo, "apply");
    _splicing_name(&memo, "token_contract", token_contract);
    memo += "symbol_code:";
    memo += symbol_code.to_string();

    _send_transaction(EOS_CONTRACT, asset(100 * 10000, EOS_SYMBOL), memo);
}

void tkexchange::_send_transaction(const name& token_contract, const asset& quantity, const string& memo) {
    action(permission_level { get_self(), name("active") },
           _get_config().org_contract, name("approvetrx"),
           std::make_tuple(get_self(), token_contract, SWAP_CONTRACT, quantity, memo))
        .send();
}

void tkexchange::_check(const name& act, const name& caller) {
    auto auth = auths();
    auth.check_perm(_get_config().org_contract, get_self(), act, caller);
}

tkexchange::dsconf tkexchange::_get_config() {
    dsconf_idx ct(get_self(), get_self().value);
    return ct.get();
}

void tkexchange::_splicing_action(string* memo, const string& val) {
    *memo += "action:";
    *memo += val;
    *memo += ",";
}

void tkexchange::_splicing_uint64(string* memo, const string& key, const uint64_t& val) {
    _splicing_uint64(memo, key, val, false);
}

void tkexchange::_splicing_uint64_last(string* memo, const string& key, const uint64_t& val) {
    _splicing_uint64(memo, key, val, true);
}

void tkexchange::_splicing_uint64(string* memo, const string& key, const uint64_t& val, const bool& is_last) {
    *memo += key;
    *memo += ":";
    *memo += _uint64_string(val);
    if (!is_last)
        *memo += ",";
}

void tkexchange::_splicing_name(string* memo, const string& key, const name& val) {
    _splicing_name(memo, key, val, false);
}

void tkexchange::_splicing_name_last(string* memo, const string& key, const name& val) {
    _splicing_name(memo, key, val, true);
}

void tkexchange::_splicing_name(string* memo, const string& key, const name& val, const bool& is_last) {
    *memo += key;
    *memo += ":";
    *memo += val.to_string();
    if (!is_last)
        *memo += ",";
}

string tkexchange::_uint64_string(uint64_t input) {
    string result;
    uint8_t base = 10;
    do {
        char c = input % base;
        input /= base;
        if (c < 10)
            c += '0';
        else
            c += 'A' - 10;
        result = c + result;
    } while (input);
    return result;
}