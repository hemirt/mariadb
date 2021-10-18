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
    
    my_bool mytrue = 1;
    mysql_options(this->mysql, MYSQL_OPT_RECONNECT, &mytrue);

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
MariaDBImpl::query(Query& query)
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
            const auto& infos = query.getInfos();
            if (infos.size() != 1) {
                return ErrorResult("Incorrect number of infos(vectors): " + std::to_string(infos.size()));
            }
            if (infos[0].type != MysqlType::mystring) {
                return ErrorResult("MysqlType is not mystring");
            }
            std::vector<std::pair<bool, std::string>> retRow;
            {
                std::vector<char> vec;
                for (std::size_t i = 0; i < infos[0].sizes.size(); ++i) {
                    vec.reserve(infos[0].sizes[i] * 2 + 1);
                    mysql_real_escape_string(this->mysql, vec.data(), *reinterpret_cast<char **>(infos[0].begin + i * sizeof(char *)), infos[0].sizes[i]);
                    retRow.push_back({true, std::string(std::move(vec.data()))});
                    vec.clear();
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

            unsigned int rowcount = query.getRowCount();
            if (rowcount == 0) {
                return ErrorResult("No values to parametrize");
            }
            const auto columncount = query.getColumnCount();
            const auto& sql = query.getSql();            
            if (columncount != std::count(sql.begin(), sql.end(), '?')) {
                return ErrorResult("Number of \'?\' and columns dont match");
            }
         
            MYSQL_STMT *stmt = mysql_stmt_init(this->mysql);
            if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size())) {
                auto e = ErrorResult("statement prepare error: " + this->handleError() + " " + mysql_stmt_error(stmt));
                mysql_stmt_close(stmt);
                return e;
            }
            MYSQL_BIND bind[columncount];   
            std::memset(bind, 0, sizeof(MYSQL_BIND) * columncount);
            
            /*
            struct {
                using Type = char;
                Type nts = STMT_INDICATOR_NTS; // string is null terminated
                Type none = STMT_INDICATOR_NONE; // no semantics
                Type null = STMT_INDICATOR_NULL; // NULL value
                Type def = STMT_INDICATOR_DEFAULT; // use columns default value
                Type ignore = STMT_INDICATOR_IGNORE; // skip update of column
                char is_null = true; // its a c library and uses my_bool = char
            } static ind;
            */
            const auto& infos = query.getInfos();
            for (std::size_t i = 0; i < columncount; ++i) {
                auto p = ConvertToMysqlBufferType(infos[i].type);
                bind[i].buffer_type = p.first;
                bind[i].is_unsigned = p.second;
                /*if (p.first == MYSQL_TYPE_STRING) {
                    bind[i].buffer = reinterpret_cast<char **>(const_cast<std::byte**>(&infos[i].begin));
                } else {*/
                    bind[i].buffer = reinterpret_cast<char *>(infos[i].begin);
                //}
                if (!infos[i].sizes.empty()) {
                    bind[i].length = const_cast<unsigned long *>(infos[i].sizes.data());
                }
            }
            
            /*
            if(mysql_stmt_attr_set(stmt, STMT_ATTR_ARRAY_SIZE, &rowcount)) {
                auto e = ErrorResult("attr array size error: " + this->handleError() + " " + mysql_stmt_error(stmt));
                mysql_stmt_close(stmt);
                return e;
            }
            */
            if (mysql_stmt_bind_param(stmt, bind)) {
                auto e = ErrorResult("statement error bind: " + this->handleError() + " " + mysql_stmt_error(stmt));
                mysql_stmt_close(stmt);
                return e;
            }
            
            int updatemaxlength = 1;
            if(mysql_stmt_attr_set(stmt, STMT_ATTR_UPDATE_MAX_LENGTH, &updatemaxlength)) {
                auto e = ErrorResult("attr array size error: " + this->handleError() + " " + mysql_stmt_error(stmt));
                mysql_stmt_close(stmt);
                return e;    
            }
            
            auto metadata = mysql_stmt_result_metadata(stmt);
            unsigned int ret_num_fields = 0;
            if (metadata) {
                ret_num_fields = mysql_num_fields(metadata);
            }
        
            if (mysql_stmt_execute(stmt)) {
                auto e = ErrorResult("statement error execute: " + this->handleError() + " " + mysql_stmt_error(stmt));
                mysql_free_result(metadata);
                mysql_stmt_close(stmt);
                return e;
            }
            
            if (ret_num_fields) {
                const auto& retTypes = query.getRetTypes();
                if (ret_num_fields != retTypes.size()) {
                    mysql_free_result(metadata);
                    mysql_stmt_close(stmt);
                    return ErrorResult("Number of return types and actual value do not match");
                }
                
                MYSQL_BIND ret_bind[ret_num_fields];
                std::memset(ret_bind, 0, sizeof(MYSQL_BIND) * ret_num_fields);
                unsigned long length[ret_num_fields];
                std::memset(length, 0, sizeof(unsigned long)*ret_num_fields);
                bool isNull[ret_num_fields];
                std::memset(isNull, 0, sizeof(bool)*ret_num_fields);
                
                if (mysql_stmt_store_result(stmt)) {
                    auto e = ErrorResult("stmt store error: " + this->handleError() + " " + mysql_stmt_error(stmt));
                    mysql_free_result(metadata);
                    mysql_stmt_close(stmt);
                    return e;
                }
                
                auto fields = mysql_fetch_fields(metadata);
                if (!fields) {
                    auto e = ErrorResult("statement error mysql_fetch_fields : " + this->handleError() + " " + mysql_stmt_error(stmt));
                    mysql_free_result(metadata);
                    mysql_stmt_close(stmt);
                    return e;
                }
                
                std::vector<std::string> retbufs{retTypes.size(), std::string{}};
                decltype(retTypes.size()) i = 0;
                for (const auto& type : retTypes) {
                    auto p = ConvertToMysqlBufferType(type);
                    ret_bind[i].buffer_type = p.first;
                    ret_bind[i].is_unsigned = p.second;
                    ret_bind[i].length = &length[i];
                    ret_bind[i].is_null = reinterpret_cast<char*>(&isNull[i]);
                    retbufs[i].resize(fields[i].max_length);
                    ret_bind[i].buffer = retbufs[i].data();
                    ret_bind[i].buffer_length = fields[i].max_length;
                    ++i;
                }
                
                if (mysql_stmt_bind_result(stmt, ret_bind)) {
                    auto e = ErrorResult("stmt bind result error: " + this->handleError() + " " + mysql_stmt_error(stmt));
                    mysql_free_result(metadata);
                    mysql_stmt_close(stmt);
                    return e;
                }
                
                
                std::vector<std::vector<std::pair<bool, std::string>>> retData;
                std::cout << "WHILE";
                auto r = mysql_stmt_fetch(stmt);
                while (!r) {
                    std::cout << "ABC" << std::endl;
                    std::vector<std::pair<bool, std::string>> retRow;
                    for (int i = 0; i < ret_num_fields; ++i) {
                        if (isNull[i]) {
                            retRow.push_back({false, std::string()});
                        } else {
                            retRow.push_back({true, std::string(retbufs[i].data(), length[i])});
                        }
                    }
                    std::cout << "RETDATA: " << retRow.size();
                    for (const auto& i : retRow) {
                        std::cout << i.second << " ";
                    }
                    retData.push_back(std::move(retRow));
                    r = mysql_stmt_fetch(stmt);
                    
                }
                std::cout << "ENDWHILE";
                mysql_free_result(metadata);
                mysql_stmt_close(stmt);
                return ReturnedRowsResult{retData};
            } else {
                auto affected_rows = mysql_stmt_affected_rows(stmt);
                mysql_free_result(metadata);
                if (mysql_stmt_close(stmt)) {
                    return ErrorResult("statement error close: " + this->handleError() + " ");
                }
                return AffectedRowsResult(affected_rows);
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