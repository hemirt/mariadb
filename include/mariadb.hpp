#ifndef HEMIRT_MARIADB_HPP
#define HEMIRT_MARIADB_HPP

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <variant>

#include "mariadbimpl.hpp"
#include "query.hpp"
#include "result.hpp"

namespace hemirt {
namespace DB {

namespace MariaDB_detail {

using Values = std::variant<std::nullptr_t, std::int8_t, std::int16_t, std::int32_t, std::int64_t, std::uint8_t,
                            std::uint16_t, std::uint32_t, std::uint64_t, std::string>;

class QueryHandle
{
public:
    QueryHandle(Query<Values>&& q)
        : query(std::move(q))
    {
    }

    Query<Values> query;
    Result result;

    std::unique_lock<std::mutex>
    lock()
    {
        return std::unique_lock<std::mutex>(this->m);
    }

    void
    wait(std::unique_lock<std::mutex>& lk)
    {
        this->cv.wait(lk, [this]() { return this->ready; });
    }

    void
    wake()
    {
        {
            std::unique_lock<std::mutex> lk(this->m);
            this->ready = true;
        }
        this->cv.notify_all();
    }

private:
    std::condition_variable cv;
    std::mutex m;
    bool ready = false;
};

}  // namespace MariaDB_detail

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
    unsigned int port = 0;
    unsigned long flags = 0;
};

class MariaDB
{
public:
    using Values = MariaDB_detail::Values;

    MariaDB(const Credentials& creds);
    MariaDB(Credentials&& creds);
    MariaDB(MariaDB&&) = delete;
    MariaDB(const MariaDB&) = delete;
    MariaDB& operator=(const MariaDB&) = delete;
    MariaDB& operator=(MariaDB&&) = delete;

    ~MariaDB() noexcept;

    Result executeQuery(Query<Values>&& query);
    Result executeQuery(const Query<Values>& query);

    const std::string& getDB() const;

private:
    void runWorker();
    void exitWorker();

    void establishWorker();
    std::thread worker;

    std::condition_variable workerCV;
    std::mutex workerM;
    bool workerReady = false;
    std::atomic<bool> workerExit = false;

    void wakeWorker();

    std::mutex queueM;

    std::queue<std::shared_ptr<MariaDB_detail::QueryHandle>> queue;

    Result processQuery(Query<Values>& query);

private:  // MYSQL related stuff
    std::unique_ptr<MariaDBImpl> impl;
    Credentials creds;
};

}  // namespace DB
}  // namespace hemirt

#endif  // HEMIRT_MARIADB_HPP