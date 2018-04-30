#ifndef HEMIRT_QUERY_HPP
#define HEMIRT_QUERY_HPP

#include <ostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <byte>

namespace hemirt {
namespace DB {

enum class QueryType
{
    UNKNOWN,
    RAWSQL,
    ESCAPE,
    PARAMETER,

    NUM_TYPES,
};

inline std::ostream&
operator<<(std::ostream& os, QueryType qt)
{
    switch (qt) {
        case QueryType::UNKNOWN:
            os << "QueryType::UNKNOWN";
            break;
        case QueryType::RAWSQL:
            os << "QueryType::RAWSQL";
            break;
        case QueryType::ESCAPE:
            os << "QueryType::ESCAPE";
            break;
        case QueryType::NUM_TYPES:
            os << "QueryType::NUM_TYPES";
            break;
        case QueryType::PARAMETER:
            os << "QueryType::PARAMETER";
            break;
        default:
            os << "Unhandled QueryType";
            break;
    }
    return os;
}

enum class MysqlType {
    /*
    std::int8_t, -> MYSQL_TYPE_TINY (signed char)
    std::int16_t, -> MYSQL_TYPE_SHORT
    std::int32_t, -> MYSQL_TYPE_LONG
    std::int64_t, -> MYSQL_TYPE_LONGLONG
    std::uint8_t, -> unsigned ^
    std::uint16_t, -> unsigned ^
    std::uint32_t, -> unsigned ^
    std::uint64_t, -> unsigned ^
    std::string -> MYSQL_TYPE_STRING (char[])
    */
    mytiny,
    myshort,
    mylong,
    mylonglong,
    myutiny,
    myushort,
    myulong,
    myulonglong,
    mystring,
    NUM_TYPES
};

template<typename cvrefT> 
MysqlType TypeToIndex()
{
    using T = typename std::remove_cv_t<typename std::remove_reference_t<cvrefT>>;
    if constexpr (std::is_same_v<T, std::int8_t>) return MysqlType::mytiny;
    if constexpr (std::is_same_v<T, std::int16_t>) return MysqlType::myshort;
    if constexpr (std::is_same_v<T, std::int32_t>) return MysqlType::mylong;
    if constexpr (std::is_same_v<T, std::int64_t>) return MysqlType::mylonglong;
    if constexpr (std::is_same_v<T, std::uint8_t>) return MysqlType::myutiny;
    if constexpr (std::is_same_v<T, std::uint16_t>) return MysqlType::myushort;
    if constexpr (std::is_same_v<T, std::uint32_t>) return MysqlType::myulong;
    if constexpr (std::is_same_v<T, std::uint64_t>) return MysqlType::myulonglong;
    if constexpr (std::is_same_v<T, std::string>) return MysqlType::mystring;
    if constexpr (std::is_same_v<T, char[]>) return MysqlType::mystring;
    if constexpr (std::is_same_v<T, char*>) return MysqlType::mystring;
    return NUM_TYPES;
}template<typename cvrefT> 
MysqlType TypeToIndex()
{
    using T = typename std::remove_cv_t<typename std::remove_reference_t<cvrefT>>;
    if constexpr (std::is_same_v<T, std::int8_t>) return MysqlType::mytiny;
    if constexpr (std::is_same_v<T, std::int16_t>) return MysqlType::myshort;
    if constexpr (std::is_same_v<T, std::int32_t>) return MysqlType::mylong;
    if constexpr (std::is_same_v<T, std::int64_t>) return MysqlType::mylonglong;
    if constexpr (std::is_same_v<T, std::uint8_t>) return MysqlType::myutiny;
    if constexpr (std::is_same_v<T, std::uint16_t>) return MysqlType::myushort;
    if constexpr (std::is_same_v<T, std::uint32_t>) return MysqlType::myulong;
    if constexpr (std::is_same_v<T, std::uint64_t>) return MysqlType::myulonglong;
    if constexpr (std::is_same_v<T, std::string>) return MysqlType::mystring;
    if constexpr (std::is_same_v<T, char[]>) return MysqlType::mystring;
    if constexpr (std::is_same_v<T, char*>) return MysqlType::mystring;
    return NUM_TYPES;
}

template <typename Values>
class Query
{
public:
    QueryType type = QueryType::UNKNOWN;

    Query(std::string sql, std::vector<Values> vals);
    Query(std::string sql);

    const std::string& getSql() const &;
    std::string getSql() &&;

    const std::vector<Values>& getVals() const &;
    std::vector<Values>& getVals() &;
    std::vector<Values> getVals() &&;

    void setSqlVals(std::string sql_, std::vector<Values> vals_);
    void setSql(std::string sql_);
    void setVals(std::vector<Values> vals_);

    void clearSql();
    void clearVals();
    void clear();

    bool isValid() const;
    
    template<typename...Types>
    void setBuffer(std::vector<Types>&&...vecs);
    
private:
    bool valid = true;
    std::string sql;
    std::vector<Values> vals;
    // will store std::vector<type>...
    std::vector<std::byte> buf;
    std::size_t rowSize{0};
    std::vector<MysqlType> types;
};

template<typename Values>
template<typename...Types>
setBuffer(std::vector<Types>&&...vecs)
{
    auto sizeCalc = [this] (const auto& vec) mutable -> std::size_t {
        using itemType = typename std::remove_cv_t<typename std::remove_reference_t<decltype(vec)>>::value_type;
        std::size_t retsize{0};
        if constexpr (std::is_same_v<itemType, std::string>) {
            std::size_t maxItemSize{0};
            for (const auto& item : vec) {
                maxItemSize = std::max(item.size(), maxItemSize);
            }
            // add a byte for null terminator of the largest item
            maxItemSize += 1;
            this->rowSize += maxItemSize;
            retsize = maxItemSize * vec.size();
        } else {
            this->rowSize += sizeof(itemType);
            retsize = sizeof(itemType) * vec.size();
        }
        this->types.push_back(TypeToIndex<itemType>());
        return retsize;
    };
    auto size = (sizeCalc(vecs) + ...);
    this->buf.reserve(size);
    std::cout << "reserve: " << size << std::endl;
    
    auto copy = [this, curpos = this->buf.data()](const auto& vec) mutable {
        using itemType = typename std::remove_cv_t<typename std::remove_reference_t<decltype(vec)>>::value_type;
        if constexpr (std::is_same_v<itemType, std::string>) {
            std::size_t maxItemSize{0};
            for (const auto& item : vec) {
                maxItemSize = std::max(item.size(), maxItemSize);
            }
            // add a byte for null terminator of the largest item
            maxItemSize += 1;
            
            for (const auto& item : vec) {
                // copy including the null terminator
                std::memcpy(curpos, item.c_str(), item.size() + 1);
                curpos += maxItemSize;
            }
        } else {
            auto copysize = sizeof(itemType) * vec.size();
            std::memcpy(curpos, vec.data(), copysize);
            curpos += copysize;
        }
    };
    (copy(vecs), ...);
}

template <typename Values>
Query<Values>::Query(std::string sql_, std::vector<Values> vals_)
    : sql(std::move(sql_))
    , vals(std::move(vals_))
{
}

template <typename Values>
Query<Values>::Query(std::string sql_)
    : sql(std::move(sql_))
{
}

template <typename Values>
const std::string&
Query<Values>::getSql() const &
{
    return this->sql;
}

template <typename Values>
std::string
Query<Values>::getSql() &&
{
    return std::move(this->sql);
}

template <typename Values>
const std::vector<Values>&
Query<Values>::getVals() const &
{
    return this->vals;
}

template <typename Values>
std::vector<Values>&
Query<Values>::getVals() &
{
    return this->vals;
}


template <typename Values>
std::vector<Values>
Query<Values>::getVals() &&
{
    return std::move(this->vals);
}

template <typename Values>
void
Query<Values>::setSqlVals(std::string sql_, std::vector<Values> vals_)
{
    this->sql = std::move(sql_);
    this->vals = std::move(vals_);
}

template <typename Values>
void
Query<Values>::setSql(std::string sql_)
{
    this->sql = std::move(sql_);
}

template <typename Values>
void
Query<Values>::setVals(std::vector<Values> vals_)
{
    this->vals = std::move(vals_);
}

template <typename Values>
void
Query<Values>::clearSql()
{
    this->sql.clear();
}

template <typename Values>
void
Query<Values>::clearVals()
{
    this->vals.clear();
}

template <typename Values>
void
Query<Values>::clear()
{
    this->clearSql();
    this->clearVals();
}

template <typename Values>
bool
Query<Values>::isValid() const
{
    return this->valid;
}

}  // namespace DB
}  // namespace hemirt

#endif  // HEMIRT_QUERY_HPP