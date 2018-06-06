#include "hemirt/mariadb.hpp"
#include <chrono>
#include <iostream>

namespace hemirt {
namespace DB {

using MariaDB_detail::QueryHandle;

MariaDB::MariaDB(Credentials&& creds_)
    : creds(std::move(creds_))
{
    this->establishWorker();
}

MariaDB::MariaDB(const Credentials& creds_)
    : creds(creds_)
{
    this->establishWorker();
}

MariaDB::~MariaDB() noexcept
{
    this->exitWorker();
    this->worker.join();
    std::cout << "~MariaDB: " << this->getDB() << std::endl;
}

const std::string&
MariaDB::getDB() const
{
    return this->creds.db;
}

void
MariaDB::establishWorker()
{
    this->worker = std::thread(&MariaDB::runWorker, this);
}

void
MariaDB::runWorker()
{
    {
        std::unique_lock lk(this->workerM);
        const char* dbptr;
        if (creds.db.empty()) {
            dbptr = nullptr;
        } else {
            dbptr = creds.db.c_str();
        }

        const char* unixsockptr;
        if (creds.unixsock.empty()) {
            unixsockptr = nullptr;
        } else {
            unixsockptr = creds.unixsock.c_str();
        }

        this->impl = std::make_unique<MariaDBImpl>(creds.host.c_str(), creds.user.c_str(), creds.pass.c_str(), dbptr,
                                                   creds.port, unixsockptr, creds.flags);
    }

    while (!this->workerExit) {
        
        struct DualLock {
            DualLock(std::mutex& m1, std::mutex& m2) 
                : l1(m1, std::defer_lock)
                , l2(m2, std::defer_lock)
            {
                std::lock(this->l1, this->l2);
            }
            
            void lock() {
                std::lock(this->l1, this->l2);
            }
            
            void unlock() {
                this->l1.unlock();
                this->l2.unlock();
            }
            
            std::unique_lock<std::mutex> l1;
            std::unique_lock<std::mutex> l2;
        };
        
        std::vector<std::shared_ptr<QueryHandle>> toWake;
        DualLock lk(this->workerM, this->queueM);
        this->workerCV.wait(lk, [this] { return this->workerReady; });
        // process queue
        //std::lock_guard qlk(this->queueM);
        while (!this->queue.empty()) {
            std::shared_ptr<QueryHandle> qH = std::move(this->queue.front());
            this->queue.pop();
            // process query
            qH->result = this->impl->query(qH->query);

            toWake.push_back(std::move(qH));
            //qH->wake();
        }
        this->workerReady = false;
        lk.unlock();
        for (auto& qH : toWake)
        {
            qH->wake();
        }
    }
    
    this->impl.reset();
}

void
MariaDB::wakeWorker()
{
    {
        std::lock_guard<std::mutex> lk(this->workerM);
        this->workerReady = true;
    }
    this->workerCV.notify_all();
}

void
MariaDB::exitWorker()
{
    {
        std::lock_guard<std::mutex> lock(this->workerM);
        this->workerReady = true;
        this->workerExit = true;
    }
    this->workerCV.notify_all();
}

Result
MariaDB::executeQuery(const Query& query)
{
    return this->executeQuery(Query(query));
}

Result
MariaDB::executeQuery(Query&& query)
{
    std::shared_ptr<QueryHandle> qH = std::make_shared<QueryHandle>(std::move(query));
    {
        std::lock_guard<std::mutex> lk(this->queueM);
        this->queue.push(qH);
    }

    auto lk = qH->lock();
    this->wakeWorker();
    qH->wait(lk);

    return std::move(qH->result);
}

Result
MariaDB::escapeString(const std::vector<std::string>& toesc)
{
    hemirt::DB::Query q("");
    q.type = hemirt::DB::QueryType::ESCAPE;
    q.setBuffer(toesc);
    return this->executeQuery(q);
}

}  // namespace DB
}  // namespace hemirt