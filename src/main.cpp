#include <iostream>
#include <memory>
#include <thread>
#include "mariadb.hpp"

int
main()
{
    {
        hemirt::MariaDB dab("Keepo");
        std::cout << dab.getDBName() << std::endl;
    }
    hemirt::MariaDB db("OMEGAZULUL");
    std::cout << db.getDBName() << std::endl;
    auto r = db.executeQuery({"Some Query"});
    std::this_thread::sleep_for(std::chrono::seconds(5));
    auto r2 = db.executeQuery({"Another Query"});
    hemirt::DB::Query<hemirt::MariaDB::Values> q("A third Query");
    auto r3 = db.executeQuery(q);
    hemirt::DB::Query<hemirt::MariaDB::Values> q2("A fourth Query");
    auto r4 = db.executeQuery(std::move(q2));

    return 0;
}