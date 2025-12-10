#include <guide.hpp>

ACTION guide::saveplugin(const name& pcode,
                         const string& pname,
                         const string& version,
                         const string& des_cid,
                         vector<name> autonomous_acts,
                         const bool& is_basic) {
    require_auth(get_self());

    pgcenter_idx pgmt(get_self(), get_self().value);
    auto pmitr = pgmt.find(pcode.value);
    if (pmitr == pgmt.end()) {
        pgmt.emplace(get_self(), [&](auto& m) {
            m.pcode = pcode;
            m.pname = pname;
            m.version = version;
            m.des_cid = des_cid;
            m.autonomous_acts = autonomous_acts;
            m.is_basic = is_basic;
        });
    } else {
        pgmt.modify(pmitr, same_payer, [&](auto& m) {
            m.pname = pname;
            m.version = version;
            m.des_cid = des_cid;
            m.autonomous_acts = autonomous_acts;
            m.is_basic = is_basic;
        });
    }
}

ACTION guide::removeplugin(const name& pcode) {
    require_auth(get_self());
    pgcenter_idx pgmt(get_self(), get_self().value);
    auto pmitr = pgmt.find(pcode.value);
    if (pmitr != pgmt.end()) {
        pgmt.erase(pmitr);
    }
}