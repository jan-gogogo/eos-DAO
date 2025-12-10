#include <nftmarket.hpp>

ACTION nftmarket::init(const name& nft_contract, const name& org_contract) {
    require_auth(get_self());

    dsconf_idx dt(get_self(), get_self().value);
    check(!dt.exists(), CODE_10001);

    dsconf conf_slt;
    conf_slt.nft_contract = nft_contract;
    conf_slt.org_contract = org_contract;
    dt.set(conf_slt, get_self());

    require_recipient(LOG_REC);
}

//JUST FOR ORG_CONTRACT
ACTION nftmarket::sale(const name& caller,
                     const name& token,
                     const vector<uint64_t>& nft_ids,
                     const asset quantity) {
    require_auth(caller);
    _check(VA_SALE, caller);
    _require_config();
    check(nft_ids.size() <= 20, CODE_10006);
    check(is_account(token), CODE_10007);

    salelist_idx st(get_self(), get_self().value);

    auto conf = _get_config();

    for (auto const& id : nft_ids) {
        check(st.find(id) == st.end(), CODE_10002);

        items_idx it(conf.nft_contract, conf.nft_contract.value);
        auto item_itr = it.find(id);
        check(item_itr != it.end(), CODE_10004);
        check(item_itr->owner == conf.org_contract, CODE_10005);

        st.emplace(get_self(), [&](auto& m) {
            m.nft_id = id;
            m.owner = item_itr->owner;
            m.token = token;
            m.quantity = quantity;
        });
    }
    require_recipient(LOG_REC);
}

//JUST FOR ORG_CONTRACT
ACTION nftmarket::closesale(const name& caller, const uint64_t& nft_id) {
    require_auth(caller);
    _check(VA_CLOSE_SALE, caller);
    _require_config();

    salelist_idx st(get_self(), get_self().value);
    auto itr = st.find(nft_id);
    check(itr != st.end(), CODE_10004);

    st.erase(itr);

    require_recipient(LOG_REC);
}

ACTION nftmarket::mtransfer(const name& contract, const structs::trx_tb& tx) {
    if (tx.from == get_self() || tx.to != get_self())
        return;

    //if start with `buy-`
    //format: buy-id1-id2-id3...
    bool buy_action = (int)tx.memo[0] == 98 && (int)tx.memo[1] == 117 && (int)tx.memo[2] == 121 && (int)tx.memo[3] == 45;
    if (!buy_action)
        return;

    vector<string> vec;
    split_memo(vec, tx.memo, '-');
    check(vec.size() > 1, CODE_10008);

    vector<uint64_t> ids;
    salelist_idx st(get_self(), get_self().value);

    //first one is `buy-`
    asset price = asset(0, tx.quantity.symbol);
    for (int i = 1; i < vec.size(); i++) {
        auto id = stoll(vec[i]);
        auto itr = st.find(id);
        check(itr != st.end(), CODE_10004);
        check(itr->token == contract, CODE_10009);
        check(itr->quantity.symbol == tx.quantity.symbol, CODE_10010);

        price += itr->quantity;
        st.erase(itr);
        ids.emplace_back(id);
    }
    check(price == tx.quantity, CODE_10010);

    auto conf = _get_config();

    // IN
    action(permission_level { get_self(), name("active") },
           contract, name("transfer"),
           std::make_tuple(get_self(), conf.org_contract, tx.quantity, tx.memo))
        .send();

    // OUT
    action(permission_level { get_self(), name("active") },
           conf.org_contract, name("approvenft"),
           std::make_tuple(get_self(), conf.nft_contract, tx.from, ids, tx.memo))
        .send();

    require_recipient(LOG_REC);
}

void nftmarket::_check(const name& act, const name& caller) {
    auto auth = auths();
    auth.check_perm(_get_config().org_contract, get_self(), act, caller);
}

void nftmarket::_require_config() {
    dsconf_idx ct(get_self(), get_self().value);
    check(ct.exists(), CODE_10003);
}

nftmarket::dsconf nftmarket::_get_config() {
    dsconf_idx ct(get_self(), get_self().value);
    return ct.get();
}

void nftmarket::split_memo(vector<string>& results, string memo, char separator) {
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
        results.emplace_back(string(""));
}