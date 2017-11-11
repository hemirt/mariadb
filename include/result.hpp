#ifndef HEMIRT_RESULT_HPP
#define HEMIRT_RESULT_HPP

#include <string>
#include <variant>

namespace hemirt {
namespace DB {

class ErrorResult
{
public:
    // errors
private:
};

class ReturnedRowsResult
{
public:
    // SELECT and SELECT like queries
    // return results
private:
};

class AffectedRowsResult
{
public:
    // UPDATE, CREATE, INSERT
    // returns affected rows
private:
};

class Result
{
public:
    Result() = default;
    Result(ErrorResult&&);
    Result(ReturnedRowsResult&&);
    Result(AffectedRowsResult&&);

    Result(Result&&) = default;
    Result(const Result&) = default;
    Result& operator=(Result&&) = default;
    Result& operator=(const Result&) = default;
    ~Result() = default;

    bool success();
    // operator bool
    ErrorResult& errorResult();
    ReturnedRowsResult& returnedRows();
    AffectedRowsResult& affectedRows();  // is returning non const reference a const method?

private:
    std::variant<std::monostate, ErrorResult, ReturnedRowsResult, AffectedRowsResult> result;
};

}  // namespace DB
}  // namespace hemirt

#endif  // HEMIRT_RESULT_HPP