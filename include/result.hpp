#ifndef HEMIRT_RESULT_HPP
#define HEMIRT_RESULT_HPP

#include <cstdint>
#include <string>
#include <variant>

namespace hemirt {
namespace DB {

class ErrorResult
{
public:
    ErrorResult() = default;
    ErrorResult(std::string&&);
    ErrorResult(const std::string&);

    ErrorResult(ErrorResult&&) = default;
    ErrorResult(const ErrorResult&) = default;
    ErrorResult& operator=(ErrorResult&&) = default;
    ErrorResult& operator=(const ErrorResult&) = default;
    ~ErrorResult() = default;
    // errors
    const std::string&
    error() const
    {
        return this->err;
    }

private:
    std::string err;
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
    AffectedRowsResult(std::uint64_t);

    std::uint64_t
    affected() const
    {
        return this->numAffected;
    }

private:
    std::uint64_t numAffected;
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

    ErrorResult* error();
    ReturnedRowsResult* returned();
    AffectedRowsResult* affected();

private:
    std::variant<ErrorResult, ReturnedRowsResult, AffectedRowsResult> result;
};

}  // namespace DB
}  // namespace hemirt

#endif  // HEMIRT_RESULT_HPP