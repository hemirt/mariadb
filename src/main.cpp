#include <iostream>
#include <memory>
#include <thread>
#include "mariadb.hpp"

int
main(int argc, char** argv)
{
    if (argc < 2)
        return 0;
    hemirt::DB::MariaDB db({"localhost", "hemirt", argv[1], "", 0, "", 0});
    std::cout << db.getDB() << std::endl;
    auto r = db.executeQuery({"Some Query"});
    std::this_thread::sleep_for(std::chrono::seconds(5));
    auto r2 = db.executeQuery({"Another Query"});
    hemirt::DB::Query<hemirt::DB::MariaDB::Values> q("A third Query");
    q.type = hemirt::DB::QueryType::RAWSQL;
    auto r3 = db.executeQuery(q);
    hemirt::DB::Query<hemirt::DB::MariaDB::Values> q2("A fourth Query");
    q2.type = hemirt::DB::QueryType::NUM_TYPES;
    auto r4 = db.executeQuery(std::move(q2));

    return 0;
}