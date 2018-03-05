#include "hemirt/mariadbimpl.hpp"

#include <iostream>
#include <stdexcept>
#include <string_view>
#include <algorithm>
#include <cstring>

namespace hemirt {
namespace DB {

MariaDBImpl::MariaDBImpl(const char* host, const char* user, const char* passwd, const char* db, unsigned int port,
                         const char* unixSock, unsigned long flags)
{
    if (mysql_library_init(0, nullptr, nullptr)) {
        throw std::runtime_error("MariaDBImpl mysql_library_init");
    }
    if (mysql_thread_init() != 0) {
        throw std::runtime_error("MariaDBImpl mysql_thread_init");
    }
    this->mysql = mysql_init(nullptr);

    if (this->mysql == nullptr) {
        std::string error = "MariaDBImpl couldnt init MYSQL: ";
        error += mysql_error(this->mysql);
        mysql_thread_end();
        mysql_library_end();
        throw std::runtime_error(error);
    }

    if (mysql_real_connect(this->mysql, host, user, passwd, db, port, unixSock, flags) == nullptr) {
        std::string error = "MariaDBImpl couldnt connect to mysql: ";
        error += mysql_error(this->mysql);
        mysql_close(this->mysql);
        mysql_thread_end();
        mysql_library_end();
        throw std::runtime_error(error);
    }

    std::cout << "MARIADBIMPL constr" << std::endl;
}

MariaDBImpl::~MariaDBImpl() noexcept
{
    if (this->mysql != nullptr) {
        std::cout << "closed: " << std::endl;
        mysql_close(this->mysql);
        this->mysql = nullptr;
    }
    mysql_thread_end();
    mysql_library_end();
    
    std::cout << "MARIADBIMPL destr" << std::endl;
}

std::string
MariaDBImpl::handleError()
{
    std::string ret(std::to_string(mysql_errno(this->mysql)));
    ret += " ";
    ret += mysql_sqlstate(this->mysql);
    ret += " ";
    ret += mysql_error(this->mysql);
    return ret;
}

Result
MariaDBImpl::query(const Query<MariaDB_detail::Values>& query)
{
    switch (query.type) {
        case QueryType::UNKNOWN: {
            std::cout << "MariaDBImpl::query -> QueryType::UNKNOWN" << std::endl;
        } break;
        case QueryType::RAWSQL: {
            const auto& rawsql = query.getSql();
            if (mysql_real_query(this->mysql, rawsql.c_str(), rawsql.size())) {
                return ErrorResult(this->handleError());
            } else {
                MYSQL_RES* result = mysql_store_result(this->mysql);
                if (result) {
                    unsigned int num_fields = mysql_num_fields(result);
                    MYSQL_ROW row;
                    std::vector<std::vector<std::pair<bool, std::string>>> retData;
                    while ((row = mysql_fetch_row(result))) {
                        unsigned long* lengths = mysql_fetch_lengths(result);
                        std::vector<std::pair<bool, std::string>> retRow;
                        for (int i = 0; i < num_fields; ++i) {
                            if (row[i] == nullptr) {
                                retRow.push_back({false, std::string()});
                            } else {
                                retRow.push_back({true, std::string(row[i], lengths[i])});
                            }
                        }
                        retData.push_back(std::move(retRow));
                    }
                    mysql_free_result(result);
                    return ReturnedRowsResult{retData};
                } else {
                    if (mysql_errno(this->mysql)) {
                        return ErrorResult(this->handleError());
                    } else if (mysql_field_count(this->mysql) == 0) {
                        auto affected_rows = mysql_affected_rows(this->mysql);
                        return AffectedRowsResult(affected_rows);
                    }
                }
            }
        } break;
        case QueryType::ESCAPE: {
            if (query.getVals().empty()) {
                return ErrorResult("No values to escape");
            }
            std::vector<std::pair<bool, std::string>> retRow;
            const auto& ref = query.getVals();
            for (decltype(ref.size()) i = 0; i < ref.size(); ++i) {
                if (auto strval = std::get_if<std::string>(&ref[i]); !strval) {
                    return ErrorResult("Value at " + std::to_string(i) + " is not a string, the type is " + std::to_string(ref[i].index()));
                } else {
                    std::vector<char> vec;
                    vec.reserve(strval->size() * 2 + 1);
                    mysql_real_escape_string(this->mysql, vec.data(), strval->c_str(), strval->size());
                    retRow.push_back({true, std::string(std::move(vec.data()))});
                }
            }
            return ReturnedRowsResult{std::vector<std::vector<std::pair<bool, std::string>>>{retRow}};
        } break;
        case QueryType::PARAMETER: {
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
            
            const auto& params = query.getVals();
            const auto numparams = params.size();
            if (numparams == 0) {
                return ErrorResult("No values to parametrize");
            }
            const auto& sql = query.getSql();            
            if (numparams != std::count(sql.begin(), sql.end(), '?')) {
                return ErrorResult("Number of \'?\' and values(params) dont match");
            }
            MYSQL_BIND bind[numparams];            
            MYSQL_STMT *stmt = mysql_stmt_init(this->mysql);
            if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size())) {
                const auto b = mysql_stmt_close(stmt);
                return ErrorResult(this->handleError() + " \'" + std::to_string(b) + "\' " + mysql_stmt_error(stmt));
            }
            std::memset(bind, 0, sizeof(MYSQL_BIND) * numparams);
            
            for (std::size_t i = 0; i < numparams; ++i) {
                auto vindex = params[i].index();
                switch (vindex) {
                    case 0: { // std::monostate
                        mysql_stmt_close(stmt);
                        throw std::runtime_error("MariaDBImpl::query Variant index \"vindex\" is zero, that is std::monospace");
                    } break;
/*                    case 1: { // hemirt::DB::DefaultVal, -> default
                        bind[i].buffer_type = std::get<0>(params[i]).type;
                        bind[i].u.indicator = STMT_INDICATOR_DEFAULT;
                        // signed?
                    } break;
                    case 2: {
                        bind[i].buffer_type = std::get<1>(params[i]).type;
                        bind[i].u.indicator = STMT_INDICATOR_NULL;
                        
                    } break;
*/                    case 3: {
                        
                    } break;
                    case 4: {
                        
                    } break;
                    case 5: {
                        
                    } break;
                    case 6: {
                        
                    } break;
                    case 7: {
                        
                    } break;
                    case 8: {
                        
                    } break;
                    case 9: {
                        
                    } break;
                    case 10: {
                        
                    } break;
                    case 11: {
                        
                    } break;                    
                    default: {
                        mysql_stmt_close(stmt);
                        throw std::runtime_error("MariaDBImpl::query Variant Index \"vindex\" not handled");
                    } break;
                }
            }
            

            
            
        } break;
        default: {
            std::cout << "MariaDBImpl::query -> unhandled QueryType: " << query.type << std::endl;
        } break;
    }
    return Result();
}

}  // namespace DB
}  // namespace hemirt