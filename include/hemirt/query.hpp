#ifndef HEMIRT_QUERY_HPP
#define HEMIRT_QUERY_HPP

#include <ostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace hemirt {
namespace DB {

enum class QueryType
{
    UNKNOWN,
    RAWSQL,
    ESCAPE,

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
        default:
            os << "Unhandled QueryType";
            break;
    }
    return os;
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
    std::vector<Values> getVals() &&;

    void setSqlVals(std::string sql_, std::vector<Values> vals_);
    void setSql(std::string sql_);
    void setVals(std::vector<Values> vals_);

    void clearSql();
    void clearVals();
    void clear();

    bool isValid() const;

private:
    bool valid = true;
    std::string sql;
    std::vector<Values> vals;
};

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