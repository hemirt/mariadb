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
    db.executeQuery(q);

    hemirt::DB::Query<hemirt::DB::MariaDB::Values> q3("DROP TABLE IF EXISTS `Keepo`");
    q3.type = hemirt::DB::QueryType::RAWSQL;

    db.executeQuery(q3);
    db.executeQuery(q);

    return 0;
}