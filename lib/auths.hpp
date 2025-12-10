#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/symbol.hpp>
#include <eosio/time.hpp>
#include <string>
#include <vector>

using namespace std;
using namespace eosio;

class auths {
public:
    const name AUTH_TOKEN_HOLDER = name("token.holder");

    void check_perm(const name& org_contract, const name& pcontract, const name& act, const name& caller) {
        check(has_permisson(org_contract, pcontract, act, caller), "Missing required authority");
    }

    bool has_permisson(const name& org_contract, const name& pcontract, const name& act, const name& caller) {
        permission_idx pt(org_contract, pcontract.value);
        auto pitr = pt.find(act.value);

        bool pass = false;
        if (pitr == pt.end()) {
            // default auth is organization founder
            pass = _is_org_founder(org_contract, caller);
        } else {
            if (pitr->access.size() > 0) {
                pass = std::find(pitr->access.begin(), pitr->access.end(), caller) != pitr->access.end();
            } else {
                // auth is organization founder when access is empty
                pass = _is_org_founder(org_contract, caller);
            }
        }
        return pass;
    }

    // contract:organization
    // scope is plugin.pcontract
    struct permission {
        name act;
        name pcode;
        vector<name> access;

        uint64_t primary_key() const { return act.value; }
    };

    struct org {
        string org_name;
        name founder;
        string des_cid;
    };

    using permission_idx = multi_index<"permission"_n, permission>;
    using org_idx = singleton<"org"_n, org>;

private:
    bool _is_org_founder(const name& org_contract, const name& caller) {
        org_idx ot(org_contract, org_contract.value);
        check(ot.exists(), "org contract info is empty");
        auto org = ot.get();
        return org.founder == caller;
    }
};
