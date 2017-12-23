#include <iostream>
#include <memory>
#include <thread>
#include "mariadb.hpp"

int
main(int argc, char** argv)
{
    if (argc < 2)
        return 0;
    hemirt::DB::MariaDB db({"localhost", "hemirt", argv[1], "wnmabot", 0, "", 0});

    hemirt::DB::Query<hemirt::DB::MariaDB::Values> q("SHOW TABLES");
    q.type = hemirt::DB::QueryType::RAWSQL;

    db.executeQuery(q);

    hemirt::DB::Query<hemirt::DB::MariaDB::Values> q2(
        "CREATE TABLE IF NOT EXISTS `Keepo` (`index` INT, `name` CHAR(10), PRIMARY KEY(`index`))");
    q2.type = hemirt::DB::QueryType::RAWSQL;

    db.executeQuery(q2);
    auto res = db.executeQuery(q);
    if (auto pval = res.returned(); pval) {
        std::cout << "table returned" << std::endl;
    }
    
    hemirt::DB::Query<hemirt::DB::MariaDB::Values> q3("DROP TABLE IF EXISTS `Keepo`");
    q3.type = hemirt::DB::QueryType::RAWSQL;

    db.executeQuery(q3);
    db.executeQuery(q);

    hemirt::DB::Query<hemirt::DB::MariaDB::Values> q4("Keepo");
    q4.type = hemirt::DB::QueryType::RAWSQL;

    auto result = db.executeQuery(q4);
    if (auto pval = result.error(); !pval) {
        std::cout << "Successful Keepo" << std::endl;
    } else {
        std::cout << "Erroneous OMEGALUL" << " " << pval->error() << std::endl;
    }

    return 0;
}