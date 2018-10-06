#ifndef HEMIRT_MARIADBDETAIL_HPP
#define HEMIRT_MARIADBDETAIL_HPP

#include <condition_variable>
#include <mutex>
#include <type_traits>

#include <mariadb/mysql.h>

#include "query.hpp"
#include "result.hpp"

namespace hemirt {
namespace DB {

namespace MariaDB_detail {
    
template<auto T, auto... Ts>
struct are_same : std::conjunction<std::is_same<decltype(T), decltype(Ts)>...> {};
    
} // MariaDB_detail
    
class NullVal
{
public:
    NullVal(std::int8_t)
        : isSigned(true)
        , type(MYSQL_TYPE_TINY)
    {}
    
    NullVal(std::int16_t)
        : isSigned(true)
        , type(MYSQL_TYPE_SHORT)
    {}
    
    NullVal(std::int32_t)
        : isSigned(true)
        , type(MYSQL_TYPE_LONG)
    {}
    
    NullVal(std::int64_t)
        : isSigned(true)
        , type(MYSQL_TYPE_LONGLONG)
    {}
    
    NullVal(std::uint8_t)
        : isSigned(false)
        , type(MYSQL_TYPE_TINY)
    {}
    
    NullVal(std::uint16_t)
        : isSigned(false)
        , type(MYSQL_TYPE_SHORT)
    {}
    
    NullVal(std::uint32_t)
        : isSigned(false)
        , type(MYSQL_TYPE_LONG)
    {}
    
    NullVal(std::uint64_t)
        : isSigned(false)
        , type(MYSQL_TYPE_LONGLONG)
    {}
    
    NullVal(std::string)
        : isSigned(false)
        , type(MYSQL_TYPE_STRING)
    {}
    
        
    
    bool isSigned;
    std::enable_if<MariaDB_detail::are_same<MYSQL_TYPE_TINY, MYSQL_TYPE_LONG, MYSQL_TYPE_LONGLONG, MYSQL_TYPE_STRING>::value, 
                   decltype(MYSQL_TYPE_TINY)>::type type;
};

class DefaultVal
{
public:
    DefaultVal(std::int8_t)
        : isSigned(true)
        , type(MYSQL_TYPE_TINY)
    {}
    
    DefaultVal(std::int16_t)
        : isSigned(true)
        , type(MYSQL_TYPE_SHORT)
    {}
    
    DefaultVal(std::int32_t)
        : isSigned(true)
        , type(MYSQL_TYPE_LONG)
    {}
    
    DefaultVal(std::int64_t)
        : isSigned(true)
        , type(MYSQL_TYPE_LONGLONG)
    {}
    
    DefaultVal(std::uint8_t)
        : isSigned(false)
        , type(MYSQL_TYPE_TINY)
    {}
    
    DefaultVal(std::uint16_t)
        : isSigned(false)
        , type(MYSQL_TYPE_SHORT)
    {}
    
    DefaultVal(std::uint32_t)
        : isSigned(false)
        , type(MYSQL_TYPE_LONG)
    {}
    
    DefaultVal(std::uint64_t)
        : isSigned(false)
        , type(MYSQL_TYPE_LONGLONG)
    {}
    
    DefaultVal(const std::string&)
        : isSigned(false)
        , type(MYSQL_TYPE_STRING)
    {}
    
        
    
    bool isSigned;
    std::enable_if<MariaDB_detail::are_same<MYSQL_TYPE_TINY, MYSQL_TYPE_LONG, MYSQL_TYPE_LONGLONG, MYSQL_TYPE_STRING>::value, 
                   decltype(MYSQL_TYPE_TINY)>::type type;
};
/*
                            std::monostate, -> blank
                            hemirt::DB::DefaultVal, -> default
                            hemirt::DB::NullVal, -> null (auto generate)
                            std::int8_t, -> MYSQL_TYPE_TINY (signed char)
                            std::int16_t, -> MYSQL_TYPE_SHORT
                            std::int32_t, -> MYSQL_TYPE_LONG
                            std::int64_t, -> MYSQL_TYPE_LONGLONG
                            std::uint8_t, -> unsigned ^
                            std::uint16_t, -> unsigned ^
                            std::uint32_t, -> unsigned ^
                            std::uint64_t, -> unsigned ^
                            std::string -> MYSQL_TYPE_STRING (char[])
                            >;*/

namespace MariaDB_detail {

/*
using Values = std::variant<std::monostate, ::hemirt::DB::DefaultVal, ::hemirt::DB::NullVal, std::int8_t, std::int16_t, std::int32_t, std::int64_t, std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t, std::string>;
*/

class QueryHandle
{
public:
    QueryHandle(Query&& q)
        : query(std::move(q))
    {
    }

    Query query;
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

#endif  // HEMIRT_MARIADBDETAIL_HPP