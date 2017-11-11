#ifndef HEMIRT_QUERY_HPP
#define HEMIRT_QUERY_HPP

#include <string>
#include <utility>
#include <vector>

namespace hemirt {
namespace DB {

template <typename Values>
class Query
{
public:
    Query(std::string sql, std::vector<Values> vals);
    Query(std::string sql);

    const std::string& getSql() const &;
    std::string getSql() &&;

    const std::vector<Values>& getVals() const &;
    std::vector<Values> getVals() &&;

    void setSql(std::string sql_, std::vector<Values> vals_);
    void setSql(std::string sql_);

    bool isValid() const;

protected:
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
Query<Values>::setSql(std::string sql_, std::vector<Values> vals_)
{
    this->sql = std::move(sql_);
    this->vals = std::move(vals_);
}

template <typename Values>
void
Query<Values>::setSql(std::string sql_)
{
    this->sql = std::move(sql_);
    this->vals.clear();
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