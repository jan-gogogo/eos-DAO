#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/symbol.hpp>

using namespace std;
using namespace eosio;

class assets {
public:
    struct account {
        asset balance;

        uint64_t primary_key() const { return balance.symbol.code().raw(); }
    };

    struct token_holder {
        name account;
        asset quantity;

        uint64_t primary_key() const { return account.value; }
    };

    struct currency_stats {
        asset supply;
        asset max_supply;
        name issuer;

        uint64_t primary_key() const { return supply.symbol.code().raw(); }
    };

    struct ds_conf {
        name org_contract;
        asset coin;
    };

    using acct_idx = multi_index<"accounts"_n, account>;
    using stats_idx = multi_index<"stat"_n, currency_stats>;
    using dfconfs = singleton<"dfconf"_n, ds_conf>;
    using holder_idx = multi_index<"holders"_n, token_holder>;

    asset get_balance(const name& token_contract, const name& account) {
        acct_idx att(token_contract, account.value);
        asset coin = get_coin(token_contract);
        auto itr = att.find(coin.symbol.code().raw());
        if (itr != att.end()) {
            return itr->balance;
        } else {
            return asset(0, coin.symbol);
        }
    }

    asset get_coin(const name& token_contract) {
        dfconfs dc(token_contract, token_contract.value);
        return dc.get().coin;
    }

    currency_stats get_stats(const name& token_contract) {
        asset coin = get_coin(token_contract);
        return get_stats(token_contract, coin.symbol);
    }

    currency_stats get_stats(const name& contract, const symbol& symbol) {
        stats_idx st(contract, symbol.code().raw());
        auto itr = st.find(symbol.code().raw());
        check(itr != st.end(), "asset symbol does not exist");
        return *itr;
    }
};
