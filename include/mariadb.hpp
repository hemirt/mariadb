#ifndef HEMIRT_MARIADB_HPP
#define HEMIRT_MARIADB_HPP

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <variant>

#include "query.hpp"
#include "result.hpp"

namespace hemirt {

namespace MariaDB_detail {

using Values = std::variant<std::nullptr_t, std::int8_t, std::int16_t, std::int32_t, std::int64_t, std::uint8_t,
                            std::uint16_t, std::uint32_t, std::uint64_t, std::string>;

class QueryHandle
{
public:
    QueryHandle(DB::Query<Values>&& q)
        : query(std::move(q))
    {
    }

    DB::Query<Values> query;
    DB::Result result;

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

class MariaDB
{
public:
    using Values = MariaDB_detail::Values;

    MariaDB(const std::string& dbName_);
    MariaDB(std::string&& dbName_);
    MariaDB(MariaDB&&) = delete;
    MariaDB(const MariaDB&) = delete;
    MariaDB& operator=(const MariaDB&) = delete;
    MariaDB& operator=(MariaDB&&) = delete;

    ~MariaDB() noexcept;

    DB::Result executeQuery(DB::Query<Values>&& query);
    DB::Result executeQuery(const DB::Query<Values>& query);

    const std::string& getDBName() const;

private:
    std::string dbName;

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

    DB::Result processQuery(DB::Query<Values>& query);
};

}  // namespace hemirt

#endif  // HEMIRT_MARIADB_HPP