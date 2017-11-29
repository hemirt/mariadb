#include "result.hpp"

namespace hemirt {
namespace DB {

ErrorResult::ErrorResult(const std::string& error)
    : err(error)
{
}

ErrorResult::ErrorResult(std::string&& error)
    : err(error)
{
}

AffectedRowsResult::AffectedRowsResult(std::uint64_t num)
    : numAffected(num)
{
}

bool
Result::success() const
{
    return !std::holds_alternative<ErrorResult>(this->result);
}

Result::Result(ErrorResult&& err)
    : result(err)
{
}
Result::Result(ReturnedRowsResult&& rrr)
    : result(rrr)
{
}
Result::Result(AffectedRowsResult&& arr)
    : result(arr)
{
}

ErrorResult&
Result::errorResult()
{
    return std::get<ErrorResult>(this->result);
}

ReturnedRowsResult&
Result::returnedRows()
{
    return std::get<ReturnedRowsResult>(this->result);
}

AffectedRowsResult&
Result::affectedRows()
{
    return std::get<AffectedRowsResult>(this->result);
}

}  // namespace DB
}  // namespace hemirt