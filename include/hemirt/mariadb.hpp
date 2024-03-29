#ifndef HEMIRT_MARIADB_HPP
#define HEMIRT_MARIADB_HPP

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "mariadbdetail.hpp"
#include "mariadbimpl.hpp"
#include "query.hpp"
#include "result.hpp"

namespace hemirt {
namespace DB {

class Credentials
{
public:
    Credentials(std::string host_, std::string user_, std::string pass_, std::string db_, unsigned int port_,
                std::string unixsock_, unsigned long flags_)
        : host(std::move(host_))
        , user(std::move(user_))
        , pass(std::move(pass_))
        , db(std::move(db_))
        , unixsock(std::move(unixsock_))
        , port(port_)
        , flags(flags_)
    {
    }

    std::string host;
    std::string user;
    std::string pass;
    std::string db;
    std::string unixsock;
    unsigned int port;
    unsigned long flags;
};

class MariaDB
{
public:
    MariaDB(const Credentials& creds);
    MariaDB(Credentials&& creds);
    MariaDB(MariaDB&&) = delete;
    MariaDB(const MariaDB&) = delete;
    MariaDB& operator=(const MariaDB&) = delete;
    MariaDB& operator=(MariaDB&&) = delete;

    ~MariaDB() noexcept;

    Result executeQuery(Query&& query);
    Result executeQuery(const Query& query);

    const std::string& getDB() const;
    
    Result escapeString(const std::vector<std::string>& str);

private:
    void runWorker();
    void exitWorker();

    void establishWorker();
    std::thread worker;

    std::condition_variable_any workerCV;
    std::mutex workerM;
    bool workerReady = false;
    std::atomic<bool> workerExit = false;

    void wakeWorker();

    std::mutex queueM;

    std::queue<std::shared_ptr<MariaDB_detail::QueryHandle>> queue;

private:  // MYSQL related stuff
    std::unique_ptr<MariaDBImpl> impl;
    Credentials creds;
};

}  // namespace DB
}  // namespace hemirt

#endif  // HEMIRT_MARIADB_HPP