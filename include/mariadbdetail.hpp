#ifndef HEMIRT_MARIADBDETAIL_HPP
#define HEMIRT_MARIADBDETAIL_HPP

#include <mutex>
#include <condition_variable>

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

}  // namespace DB
}  // namespace hemirt

#endif // HEMIRT_MARIADBDETAIL_HPP