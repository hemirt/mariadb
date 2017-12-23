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

std::size_t
Result::getType() const noexcept
{
    return this->result.index();
}

ErrorResult*
Result::error()
{
    return std::get_if<ErrorResult>(&this->result);
}

ReturnedRowsResult*
Result::returned()
{
    return std::get_if<ReturnedRowsResult>(&this->result);
}

AffectedRowsResult*
Result::affected()
{
    return std::get_if<AffectedRowsResult>(&this->result);
}

}  // namespace DB
}  // namespace hemirt