#ifndef DBMANGER_H
#define DBMANGER_H

#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <sqlite3.h>
#include <json/json.h>

/**
 * @brief Thread-safe SQLite Database Manager for MONICA MART
 * Handles connection, schema initialization, parameterized queries,
 * transactions, and SHA-256 password hashing.
 * 
 * Author: Monica Sornam (Capstone Project)
 */
class DbManager {
public:
    static DbManager& instance();

    // Prevent copying
    DbManager(const DbManager&) = delete;
    DbManager& operator=(const DbManager&) = delete;

    // Initialize database connection and run schema/seeds if empty
    bool init(const std::string& dbPath = "monica_mart.db", const std::string& schemaSqlPath = "database/database.sql");
    void close();

    // Execute parameterized SELECT query returning Json::Value array
    Json::Value query(const std::string& sql, const std::vector<std::string>& params = {});

    // Execute parameterized INSERT/UPDATE/DELETE query returning affected rows or last insert ID
    int64_t execute(const std::string& sql, const std::vector<std::string>& params = {});

    // Transaction helpers
    bool beginTransaction();
    bool commit();
    bool rollback();

    // Utility: SHA-256 password hashing
    static std::string hashPassword(const std::string& plainText);

    // Escape or sanitize helper
    sqlite3* getRawDb() { return db_; }

private:
    DbManager();
    ~DbManager();

    sqlite3* db_;
    std::mutex dbMutex_;
    bool initialized_;
};

#endif // DBMANGER_H
