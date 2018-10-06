#ifndef HEMIRT_QUERY_HPP
#define HEMIRT_QUERY_HPP

#include <ostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <algorithm>

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
    return MysqlType::NUM_TYPES;
}

// type, is_unsigned
inline std::pair<decltype(MYSQL_TYPE_TINY), bool> ConvertToMysqlBufferType(const MysqlType& type)
{
    std::pair<decltype(MYSQL_TYPE_TINY), bool> p;
    switch (type) {
        case MysqlType::mytiny: {
            p.first = MYSQL_TYPE_TINY;
            p.second = false;
        } break;
        case MysqlType::myshort: {
            p.first = MYSQL_TYPE_SHORT;
            p.second = false;
        } break;
        case MysqlType::mylong: {
            p.first = MYSQL_TYPE_LONG;
            p.second = false;
        } break;
        case MysqlType::mylonglong: {
            p.first = MYSQL_TYPE_LONGLONG;
            p.second = false;
        } break;
        case MysqlType::myutiny: {
            p.first = MYSQL_TYPE_TINY;
            p.second = true;
        } break;
        case MysqlType::myushort: {
            p.first = MYSQL_TYPE_SHORT;
            p.second = true;
        } break;
        case MysqlType::myulong: {
            p.first = MYSQL_TYPE_LONG;
            p.second = true;
        } break;
        case MysqlType::myulonglong: {
            p.first = MYSQL_TYPE_LONGLONG;
            p.second = true;
        } break;
        case MysqlType::mystring: {
            p.first = MYSQL_TYPE_STRING;
            p.second = false;
        } break;
        case MysqlType::NUM_TYPES: {
            throw std::runtime_error("Query print NUM_TYPES");
        } break;
        default: {
            throw std::runtime_error("Query print default");
        } break;
    }
    return p;
}

struct Info
{
    std::size_t pos{0};
    std::byte* begin = nullptr;
    // unsigned long is the type mysql expects for sizes
    std::vector<unsigned long> sizes;
    MysqlType type;
};

class Query
{
public:
    QueryType type = QueryType::UNKNOWN;

    Query(std::string sql);

    const std::string& getSql() const &;
    std::string getSql() &&;

    void setSql(std::string sql_);

    void clearSql();
    void clearBuffer();
    void clear();
    
    template<typename...Types>
    void setBuffer(const std::vector<Types>&...vecs);
    
    const auto& getBuf() const {
        return this->buf;
    }
    const auto& getInfos() const {
        return this->infos;
    }
    const auto& getBufSize() const {
        return this->bufsize;
    }
    const auto& getRowCount() const {
        return this->rowcount;
    }
    const auto& getColumnCount() const {
        return this->columncount;
    }

    
private:
    std::string sql;
    std::vector<std::byte> buf;
    std::vector<char> strings;
    std::vector<Info> infos;
    std::size_t bufsize{0};
    std::size_t rowcount{0};
    std::size_t columncount{0};
    std::size_t stringsizes{0};
};

template<typename...Types>
void
Query::setBuffer(const std::vector<Types>&...vecs)
{
    std::vector<std::size_t> vecsizes{(vecs.size(), ...)};
    if (!vecsizes.empty() && (std::adjacent_find(vecsizes.begin(), vecsizes.end(), std::not_equal_to<int>()) == vecsizes.end())) {
        this->rowcount = vecsizes[0];
    } else {
        throw std::runtime_error("columns do not have the same amount of items");
    }
    auto sizeCalc = [this] (const auto& vec) mutable -> std::size_t {
        using itemType = typename std::remove_cv_t<typename std::remove_reference_t<decltype(vec)>>::value_type;
        std::size_t retsize{0};
        Info info;
        info.pos = this->columncount;
        info.type = TypeToIndex<itemType>();
        if constexpr (std::is_same_v<itemType, std::string>) {
            std::size_t sumsizes{0};
            std::vector<unsigned long> sizes;
            for (const auto& item : vec) {
                sumsizes += sizeof(char *);
                sizes.push_back(item.size());
                this->stringsizes += item.size();
            }
            info.sizes = std::move(sizes);
            retsize = sumsizes;
        } else {
            retsize = sizeof(itemType) * vec.size();
        }
        ++this->columncount;
        this->infos.push_back(std::move(info));
        return retsize;
    };
    this->bufsize = (sizeCalc(vecs) + ...);
    this->buf.reserve(this->bufsize);
    this->strings.reserve(this->stringsizes);
    //std::cout << "bufsize: " << this->bufsize << "\nrowcount: " << this->rowcount << "\ncolumncount: " << this->columncount << std::endl;
    
    auto copy = [this, curpos = this->buf.data(), stringpos = this->strings.data(), pos = std::size_t{0}](const auto& vec) mutable {
        using itemType = typename std::remove_cv_t<typename std::remove_reference_t<decltype(vec)>>::value_type;
        this->infos[pos].begin = curpos;
        if constexpr (std::is_same_v<itemType, std::string>) {
            for (const auto& item : vec) {
                std::memcpy(stringpos, item.c_str(), item.size());
                std::memcpy(curpos, &stringpos, sizeof(char*));
                stringpos += item.size();
                curpos += sizeof(char*);
            }
        } else {
            auto copysize = sizeof(itemType) * vec.size();
            std::memcpy(curpos, vec.data(), copysize);
            curpos += copysize;
        }
        ++pos;
    };
    (copy(vecs), ...);
}

/*
void
Query::printBuf() const
{
    std::cout << "bufsize: " << this->bufsize  << "\nrowcount: " << this->rowcount << "\ncolumncount: " << this->columncount << std::endl;
    for (int col = 0; col < this->columncount; ++col) {
        auto* beg = this->infos[col].begin;
        std::size_t current_stringsizes_position = 0;
        for (int i = 0; i < this->rowcount; ++i) {
            switch (this->infos[col].type) {
                case MysqlType::mytiny: {
                    std::cout << reinterpret_cast<std::int8_t*>(beg) << " ";
                    beg += sizeof(std::int8_t);
                } break;
                case MysqlType::myshort: {
                    std::cout << *reinterpret_cast<std::int16_t*>(beg) << " ";
                    beg += sizeof(std::int16_t);
                } break;
                case MysqlType::mylong: {
                    std::cout << *reinterpret_cast<std::int32_t*>(beg) << " ";
                    beg += sizeof(std::int32_t);
                } break;
                case MysqlType::mylonglong: {
                    std::cout << *reinterpret_cast<std::int64_t*>(beg) << " ";
                    beg += sizeof(std::int64_t);  
                } break;
                case MysqlType::myutiny: {
                    std::cout << *reinterpret_cast<std::uint8_t*>(beg) << " ";
                    beg += sizeof(std::uint8_t);
                } break;
                case MysqlType::myushort: {
                    std::cout << *reinterpret_cast<std::uint16_t*>(beg) << " ";
                    beg += sizeof(std::uint16_t);
                } break;
                case MysqlType::myulong: {
                    std::cout << *reinterpret_cast<std::uint32_t*>(beg) << " ";
                    beg += sizeof(std::uint32_t);
                } break;
                case MysqlType::myulonglong: {
                    std::cout << *reinterpret_cast<std::uint64_t*>(beg) << " ";
                    beg += sizeof(std::uint64_t);
                } break;
                case MysqlType::mystring: {
                    const char* s = reinterpret_cast<const char*>(beg);
                    std::cout << std::string(s, this->infos[col].sizes[i]) << " ";
                    // plus null byte
                    beg += sizeof(char) * this->infos[col].sizes[i] + 1;
                } break;
                case MysqlType::NUM_TYPES: {
                    throw std::runtime_error("Query print NUM_TYPES");
                } break;
                default: {
                    throw std::runtime_error("Query print default");
                } break;
            }
        }
    }
}
*/

inline Query::Query(std::string sql_)
    : sql(std::move(sql_))
{
}

inline
const std::string&
Query::getSql() const &
{
    return this->sql;
}

inline
std::string
Query::getSql() &&
{
    return std::move(this->sql);
}

inline
void
Query::setSql(std::string sql_)
{
    this->sql = std::move(sql_);
}

inline
void
Query::clearSql()
{
    this->sql.clear();
}

inline
void
Query::clearBuffer()
{
    this->buf.clear();
    this->infos.clear();
    this->strings.clear();
    this->bufsize = 0;
    this->rowcount = 0;
    this->columncount = 0;
    this->stringsizes = 0;
    
}

inline
void
Query::clear()
{
    this->clearSql();
    this->clearBuffer();
}


}  // namespace DB
}  // namespace hemirt

#endif  // HEMIRT_QUERY_HPP