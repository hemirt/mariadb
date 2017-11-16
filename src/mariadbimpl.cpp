#include "mariadbimpl.hpp"

#include <iostream>
#include <stdexcept>
#include <string_view>

namespace hemirt {
namespace DB {

MariaDBImpl::MariaDBImpl(const char* host, const char* user, const char* passwd, const char* db, unsigned int port,
                         const char* unixSock, unsigned long flags)
{
    if (mysql_thread_init() != 0) {
        throw std::runtime_error("MariaDBImpl mysql_thread_init");
    }
    this->mysql = mysql_init(nullptr);

    if (this->mysql == nullptr) {
        std::string error = "MariaDBImpl couldnt init MYSQL: ";
        error += mysql_error(this->mysql);
        mysql_thread_end();
        throw std::runtime_error(error);
    }

    if (mysql_real_connect(this->mysql, host, user, passwd, db, port, unixSock, flags) == nullptr) {
        std::string error = "MariaDBImpl couldnt connect to mysql: ";
        error += mysql_error(this->mysql);
        mysql_close(this->mysql);
        mysql_thread_end();
        throw std::runtime_error(error);
    }

    std::cout << "MARIADBIMPL constr" << std::endl;
}

MariaDBImpl::~MariaDBImpl() noexcept
{
    if (this->mysql != nullptr) {
        mysql_close(this->mysql);
    }
    mysql_thread_end();
    std::cout << "MARIADBIMPL destr" << std::endl;
}

std::string
MariaDBImpl::handleError()
{
    std::string ret(std::to_string(mysql_errno(this->mysql)));
    ret += " ";
    ret += mysql_sqlstate(this->mysql);
    ret += " ";
    ret += mysql_error(this->mysql);
    return ret;
}

Result
MariaDBImpl::query(const Query<MariaDB_detail::Values>& query)
{
    switch (query.type) {
        case QueryType::UNKNOWN: {
            std::cout << "MariaDBImpl::query -> QueryType::UNKNOWN" << std::endl;
        } break;
        case QueryType::RAWSQL: {
            const auto& rawsql = query.getSql();
            if (mysql_real_query(this->mysql, rawsql.c_str(), rawsql.size())) {
                std::cout << "MariaDBImpl::query RAWSQL: \"" << rawsql << "\" error: " << this->handleError()
                          << std::endl;
                this->handleError();
            }
            MYSQL_RES* result;
            int err;
            do {
                result = mysql_store_result(this->mysql);
                std::cout << "rrr:: " << (result == nullptr) << std::endl;
                if (result) {
                    unsigned int num_fields = mysql_num_fields(result);
                    MYSQL_ROW row;
                    while ((row = mysql_fetch_row(result))) {
                        unsigned long* lengths = mysql_fetch_lengths(result);
                        for (int i = 0; i < num_fields; ++i) {
                            if (row[i] == nullptr) {
                                std::cout << " NULL ";
                            } else {
                                std::string_view sv(row[i], lengths[i]);
                                std::cout << " " << sv << " ";
                            }
                        }
                        std::cout << std::endl;
                    }
                    std::cout << std::endl;
                    mysql_free_result(result);
                }
            } while ((err = mysql_next_result(this->mysql)) == 0);
            if (err > 0) {
                std::cout << "i error: " << this->handleError() << std::endl;
            }
        } break;
        default: {
            std::cout << "MariaDBImpl::query -> unhandled QueryType: " << query.type << std::endl;
        } break;
    }
    return Result();
}

}  // namespace DB
}  // namespace hemirt