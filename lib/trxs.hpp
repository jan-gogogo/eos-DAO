#include <eosio/eosio.hpp>

using namespace std;
using namespace eosio;

class trxs {
public:
    /**
     * Execute transaction
     *
     * @param contract - The transaction action account.
     * @param act - The transaction action name.
     * @param perm - The transaction permission.
     * @param params - The transaction action params.pack action data(Hex)
     */
    void execute(const name& contract, const name& act, const permission_level& perm, const vector<char> params) {
        vector<permission_level> perms;
        perms.emplace_back(perm);

        action eosact;
        eosact.account = contract;
        eosact.name = act;
        eosact.authorization = perms;
        eosact.data = params;
        eosact.send();
    }
};
