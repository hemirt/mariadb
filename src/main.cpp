#include <iostream>
#include <memory>
#include <thread>
#include <fstream>

#include "hemirt/mariadb.hpp"

int
main(int argc, char** argv)
{
    hemirt::DB::Credentials* cred = nullptr;
    if (argc < 2) {
        try {
        std::ifstream configcfg("config.cfg");
        if (!configcfg) { 
            std::cout << "Config file \"config.cfg\" is not valid" << std::endl;
            return 1;
        }
        std::string vars[7];
        int varnm = 0;
        while (std::getline(configcfg, vars[varnm++])) {
            if (varnm == 7) break;
        }
        
        cred = new hemirt::DB::Credentials(vars[0], vars[1], vars[2], vars[3], std::stoull(vars[4]), vars[5], std::stoull(vars[6]));
        } catch (std::exception &e) {
            std::cout << "Exception occured reading config file: " << e.what() << std::endl;
            return 1;
        }
    } else {
        cred = new hemirt::DB::Credentials("localhost", "hemirt", argv[1], "wnmabot", 0, "", 0);
    }
    if (cred == nullptr) {
        std::cout << "Credentials* cred is nullptr" << std::endl;
        return 1;
    }
    hemirt::DB::MariaDB db(std::move(*cred));
    delete cred;
    cred = nullptr;
    
    hemirt::DB::Query<hemirt::DB::MariaDB::Values> q("SHOW TABLES");
    q.type = hemirt::DB::QueryType::RAWSQL;

    db.executeQuery(q);

    auto showRes = [](auto& res) {
                std::cout << "res value type: " << res.getType() << std::endl;
                if (auto pval = res.returned(); pval) {
                    std::cout << "table returned" << std::endl;
                    if (pval->data.empty()) {
                        std::cout << std::endl;
                    }
                    for (const auto& row : pval->data) {
                        for (const auto& column : row) {
                            if (column.first) {
                                std::cout << column.second << " ";
                            } else {
                                std::cout << "NULL";
                            }
                        }
                        std::cout << std::endl;
                    }
                } else if (auto eval = res.error(); eval) { 
                    std::cout << eval->error() << std::endl;
                } else if (auto aval = res.affected(); aval) {
                    std::cout << aval->affected() << std::endl;
                }
            };
            
    hemirt::DB::Query<hemirt::DB::MariaDB::Values> q2(
        "CREATE TABLE IF NOT EXISTS `Keepo` (`index` INT AUTO_INCREMENT, `name` CHAR(10), PRIMARY KEY(`index`))");
    q2.type = hemirt::DB::QueryType::RAWSQL;
    {
        auto res = db.executeQuery(q2);
        showRes(res);
    }
    
    {
        auto res = db.executeQuery(q);
        showRes(res);
    }
    
    hemirt::DB::Query<hemirt::DB::MariaDB::Values> q4("INSERT INTO `Keepo` (`name`) VALUES (\'nuuls\'), (\'fourtf\'), (\'hemirt\'), (\'pajlada\')");
    q4.type = hemirt::DB::QueryType::RAWSQL;
    {
        auto res = db.executeQuery(q4);
        showRes(res);
    }
    
    hemirt::DB::Query<hemirt::DB::MariaDB::Values> q5("SELECT * FROM `Keepo`");
    q5.type = hemirt::DB::QueryType::RAWSQL;
    {
        auto res = db.executeQuery(q5);
        showRes(res);
    }
    
    hemirt::DB::Query<hemirt::DB::MariaDB::Values> q3("DROP TABLE IF EXISTS `Keepo`");
    q3.type = hemirt::DB::QueryType::RAWSQL;
    {
        auto res = db.executeQuery(q3);
        showRes(res);
    }
    
    {
        auto res = db.executeQuery(q);
        showRes(res);
    }

    /*
    hemirt::DB::Query<hemirt::DB::MariaDB::Values> q4("Keepo");
    q4.type = hemirt::DB::QueryType::RAWSQL;

    auto result = db.executeQuery(q4);
    if (auto pval = result.error(); !pval) {
        std::cout << "Successful Keepo" << std::endl;
    } else {
        std::cout << "Erroneous OMEGALUL" << " " << pval->error() << std::endl;
    }
    */

    return 0;
}