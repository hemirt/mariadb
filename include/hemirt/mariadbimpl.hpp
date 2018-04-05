#ifndef HEMIRT_MARIADB_IMPL_HPP
#define HEMIRT_MARIADB_IMPL_HPP

#include <mariadb/mysql.h>

#include "mariadbdetail.hpp"
#include "query.hpp"
#include "result.hpp"

namespace hemirt {
namespace DB {

class MariaDBImpl
{
public:
    MariaDBImpl(const char* host, const char* user, const char* passwd, const char* db, unsigned int port,
                const char* unixSock, unsigned long flags);
    MariaDBImpl(MariaDBImpl&&) = delete;
    MariaDBImpl(const MariaDBImpl&) = delete;
    MariaDBImpl& operator=(const MariaDBImpl&) = delete;
    MariaDBImpl& operator=(MariaDBImpl&&) = delete;

    ~MariaDBImpl() noexcept;

    std::string handleError();

    Result query(Query<MariaDB_detail::Values>& query);

private:
    MYSQL* mysql;
};

}  // namespace DB
}  // namespace hemirt

#endif  // HEMIRT_MARIADB_IMPL_HPP