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
    
    /*
    hemirt::DB::Query q("SHOW TABLES");
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
            
    hemirt::DB::Query q2(
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
    
    hemirt::DB::Query q4("INSERT INTO `Keepo` (`name`) VALUES (\'nuuls\'), (\'fourtf\'), (\'hemirt\'), (\'pajlada\')");
    q4.type = hemirt::DB::QueryType::RAWSQL;
    {
        auto res = db.executeQuery(q4);
        showRes(res);
    }
    
    hemirt::DB::Query q5("SELECT * FROM `Keepo`");
    q5.type = hemirt::DB::QueryType::RAWSQL;
    {
        auto res = db.executeQuery(q5);
        showRes(res);
    }
    
    hemirt::DB::Query q3("DROP TABLE IF EXISTS `Keepo`");
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
    hemirt::DB::Query q4("Keepo");
    q4.type = hemirt::DB::QueryType::RAWSQL;

    auto result = db.executeQuery(q4);
    if (auto pval = result.error(); !pval) {
        std::cout << "Successful Keepo" << std::endl;
    } else {
        std::cout << "Erroneous OMEGALUL" << " " << pval->error() << std::endl;
    }
    */   
    /*
    
    {
        hemirt::DB::Query q(
            "CREATE TABLE IF NOT EXISTS `Keepo` (`index` INT AUTO_INCREMENT, `name` CHAR(10), PRIMARY KEY(`index`))");
        q.type = hemirt::DB::QueryType::RAWSQL;
        auto res = db.executeQuery(q);
        showRes(res);
    }
    
    {
        hemirt::DB::Query q("INSERT INTO `Keepo` (`name`) VALUES (?), (?), (?), (?)", {"hemirt", "pajlada", "forsen", std::uint32_t{5}});
        q.type = hemirt::DB::QueryType::PARAMETER;
        auto res = db.executeQuery(q);
        showRes(res);
    }
    {
        hemirt::DB::Query q("INSERT INTO `Keepo` (`name`) VALUES (?), (?), (?), (?)", {"hemirt", "pajlada", "forsen", std::uint8_t{5}});
        q.type = hemirt::DB::QueryType::PARAMETER;
        auto res = db.executeQuery(q);
        showRes(res);
    }
    {
        hemirt::DB::Query q("INSERT INTO `Keepo` (`name`) VALUES (?), (?), (?), (?)", {"hemirt", "pajlada", hemirt::DB::DefaultVal{std::uint32_t{}}, hemirt::DB::NullVal{std::int64_t{}}});
        q.type = hemirt::DB::QueryType::PARAMETER;
        auto res = db.executeQuery(q);
        showRes(res);
    }
    
    {
        hemirt::DB::Query q("DROP TABLE IF EXISTS `Keepo`");
        q.type = hemirt::DB::QueryType::RAWSQL;
        auto res = db.executeQuery(q);
        showRes(res);
    }
    */
    
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
                                std::cout << column.second << "\n";
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
            
    
    std::vector<int> ids{1};      
    
    hemirt::DB::Query q("SELECT userid, username FROM `users` WHERE id = ?");
    q.setBuffer(ids);
    q.type = hemirt::DB::QueryType::PARAMETER;
    q.setRetTypes({hemirt::DB::MysqlType::mystring, hemirt::DB::MysqlType::mystring});
    auto res = db.executeQuery(q);
    showRes(res);
    
    
    std::vector<std::string> v{"\'SELECT `* FROM Users W`HERE\' Na\"me =\"\' + \nuName + \'\" AND Pass =\"\' + uPass + \'\"\'", "Keepo `, ` \' \" K\"", "OMEGALUL` OMEGALUL2\" OMEGALUL3 \'OMG"};
    {
        hemirt::DB::Query q("");
        q.type = hemirt::DB::QueryType::ESCAPE;
        q.setBuffer(v);
        auto res = db.executeQuery(q);
        showRes(res);
    }
    
    std::cout << std::endl;
    res = db.escapeString(v);
    showRes(res);

    return 0;
}