#include "mariadb.hpp"
#include <chrono>
#include <iostream>

namespace hemirt {

using MariaDB_detail::QueryHandle;

MariaDB::MariaDB(std::string &&dbName_) : dbName(std::move(dbName_)) {
  this->establishWorker();
}

MariaDB::MariaDB(const std::string &dbName_) : dbName(dbName_) {
  this->establishWorker();
}

MariaDB::~MariaDB() noexcept {
  this->exitWorker();
  this->worker.join();
  std::cout << "~MariaDB: " << this->getDBName() << std::endl;
}

const std::string &MariaDB::getDBName() const { return this->dbName; }

void MariaDB::establishWorker() {
  this->worker = std::thread(&MariaDB::runWorker, this);
}

void MariaDB::runWorker() {
  std::unique_lock lk(this->workerM);
  while (!this->workerExit) {
    this->workerCV.wait(lk, [this] { return this->workerReady; });
    // process queue
    std::lock_guard qlk(this->queueM);
    while (!this->queue.empty()) {
      std::shared_ptr<QueryHandle> qH = std::move(this->queue.front());
      this->queue.pop();
      // process query
      qH->result = processQuery(qH->query);

      qH->wake();
    }
    this->workerReady = false;
  }
}

DB::Result MariaDB::processQuery(DB::Query<MariaDB::Values> &query) {
  std::cout << "Processing Query: " << query.getSql() << std::endl;
  switch (query.type) {
  case DB::QueryType::UNKNOWN: {
    std::cout << "query type is QueryType::UNKNOWN" << std::endl;
    return DB::Result();
  } break;
  default: {
    std::cout << "QueryType not handled: " << query.type << std::endl;
  } break;
  }
  return DB::Result();
}

void MariaDB::wakeWorker() {
  {
    std::lock_guard<std::mutex> lk(this->workerM);
    this->workerReady = true;
  }
  this->workerCV.notify_all();
}

void MariaDB::exitWorker() {
  {
    std::lock_guard<std::mutex> lock(this->workerM);
    this->workerReady = true;
    this->workerExit = true;
  }
  this->workerCV.notify_all();
}

DB::Result MariaDB::executeQuery(const DB::Query<Values> &query) {
  std::cout << "const" << std::endl;
  return this->executeQuery(DB::Query<Values>(query));
}

DB::Result MariaDB::executeQuery(DB::Query<Values> &&query) {
  std::cout << "move" << std::endl;
  std::shared_ptr<QueryHandle> qH =
      std::make_shared<QueryHandle>(std::move(query));
  {
    std::lock_guard<std::mutex> lk(this->queueM);
    this->queue.push(qH);
  }

  auto lk = qH->lock();
  this->wakeWorker();
  qH->wait(lk);

  return std::move(qH->result);
}

} // namespace hemirt