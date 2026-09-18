#include "DbManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <array>
#include <cstring>

// --- Portable Self-Contained SHA-256 Implementation ---
namespace {
    inline uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
    inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
    inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
    inline uint32_t sig0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
    inline uint32_t sig1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
    inline uint32_t theta0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
    inline uint32_t theta1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

    const uint32_t K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    std::string computeSHA256(const std::string& input) {
        std::vector<uint8_t> msg(input.begin(), input.end());
        uint64_t bitLen = msg.size() * 8;

        msg.push_back(0x80);
        while ((msg.size() % 64) != 56) {
            msg.push_back(0x00);
        }

        for (int i = 7; i >= 0; --i) {
            msg.push_back(static_cast<uint8_t>((bitLen >> (i * 8)) & 0xFF));
        }

        uint32_t H[8] = {
            0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
        };

        for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
            uint32_t W[64];
            for (int t = 0; t < 16; ++t) {
                W[t] = (msg[chunk + t * 4] << 24) |
                       (msg[chunk + t * 4 + 1] << 16) |
                       (msg[chunk + t * 4 + 2] << 8) |
                       (msg[chunk + t * 4 + 3]);
            }
            for (int t = 16; t < 64; ++t) {
                W[t] = theta1(W[t - 2]) + W[t - 7] + theta0(W[t - 15]) + W[t - 16];
            }

            uint32_t a = H[0], b = H[1], c = H[2], d = H[3], e = H[4], f = H[5], g = H[6], h = H[7];

            for (int t = 0; t < 64; ++t) {
                uint32_t T1 = h + sig1(e) + ch(e, f, g) + K[t] + W[t];
                uint32_t T2 = sig0(a) + maj(a, b, c);
                h = g;
                g = f;
                f = e;
                e = d + T1;
                d = c;
                c = b;
                b = a;
                a = T1 + T2;
            }

            H[0] += a; H[1] += b; H[2] += c; H[3] += d;
            H[4] += e; H[5] += f; H[6] += g; H[7] += h;
        }

        std::ostringstream ss;
        for (int i = 0; i < 8; ++i) {
            ss << std::hex << std::setw(8) << std::setfill('0') << H[i];
        }
        return ss.str();
    }
}

DbManager::DbManager() : db_(nullptr), initialized_(false) {}

DbManager::~DbManager() {
    close();
}

DbManager& DbManager::instance() {
    static DbManager inst;
    return inst;
}

void DbManager::close() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
    initialized_ = false;
}

std::string DbManager::hashPassword(const std::string& plainText) {
    return computeSHA256(plainText);
}

bool DbManager::init(const std::string& dbPath, const std::string& schemaSqlPath) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (initialized_ && db_) return true;

    int rc = sqlite3_open(dbPath.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "[DbManager] Failed to open database: " << sqlite3_errmsg(db_) << std::endl;
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
        return false;
    }

    // Enable foreign keys and WAL mode for better concurrency
    char* err = nullptr;
    sqlite3_exec(db_, "PRAGMA foreign_keys = ON; PRAGMA journal_mode = WAL;", nullptr, nullptr, &err);
    if (err) sqlite3_free(err);

    // Check if users table exists
    sqlite3_stmt* checkStmt = nullptr;
    rc = sqlite3_prepare_v2(db_, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='users';", -1, &checkStmt, nullptr);
    bool needSchema = true;
    if (rc == SQLITE_OK && sqlite3_step(checkStmt) == SQLITE_ROW) {
        if (sqlite3_column_int(checkStmt, 0) > 0) {
            needSchema = false;
        }
    }
    if (checkStmt) sqlite3_finalize(checkStmt);

    if (needSchema) {
        std::cout << "[DbManager] Initializing database schema from " << schemaSqlPath << "..." << std::endl;
        std::ifstream sqlFile(schemaSqlPath);
        if (sqlFile.is_open()) {
            std::stringstream buffer;
            buffer << sqlFile.rdbuf();
            std::string sql = buffer.str();
            rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err);
            if (rc != SQLITE_OK) {
                std::cerr << "[DbManager] Error initializing schema: " << (err ? err : "unknown") << std::endl;
                if (err) sqlite3_free(err);
            } else {
                std::cout << "[DbManager] Database schema and initial seeds successfully loaded!" << std::endl;
            }
        } else {
            std::cerr << "[DbManager] Warning: Could not open " << schemaSqlPath << " for schema setup." << std::endl;
        }
    }

    initialized_ = true;
    return true;
}

Json::Value DbManager::query(const std::string& sql, const std::vector<std::string>& params) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    Json::Value rows(Json::arrayValue);

    if (!db_) {
        std::cerr << "[DbManager] Database not open" << std::endl;
        return rows;
    }

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[DbManager] Query prepare error: " << sqlite3_errmsg(db_) << " SQL: " << sql << std::endl;
        return rows;
    }

    for (size_t i = 0; i < params.size(); ++i) {
        sqlite3_bind_text(stmt, static_cast<int>(i + 1), params[i].c_str(), -1, SQLITE_TRANSIENT);
    }

    int colCount = sqlite3_column_count(stmt);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Json::Value row;
        for (int i = 0; i < colCount; ++i) {
            const char* colName = sqlite3_column_name(stmt, i);
            int colType = sqlite3_column_type(stmt, i);

            if (colType == SQLITE_INTEGER) {
                row[colName] = static_cast<Json::Int64>(sqlite3_column_int64(stmt, i));
            } else if (colType == SQLITE_FLOAT) {
                row[colName] = sqlite3_column_double(stmt, i);
            } else if (colType == SQLITE_TEXT) {
                const char* txt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                row[colName] = txt ? txt : "";
            } else if (colType == SQLITE_NULL) {
                row[colName] = Json::Value::null;
            } else {
                row[colName] = "";
            }
        }
        rows.append(row);
    }

    sqlite3_finalize(stmt);
    return rows;
}

int64_t DbManager::execute(const std::string& sql, const std::vector<std::string>& params) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!db_) return -1;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[DbManager] Execute prepare error: " << sqlite3_errmsg(db_) << " SQL: " << sql << std::endl;
        return -1;
    }

    for (size_t i = 0; i < params.size(); ++i) {
        sqlite3_bind_text(stmt, static_cast<int>(i + 1), params[i].c_str(), -1, SQLITE_TRANSIENT);
    }

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE || rc == SQLITE_ROW) {
        return sqlite3_last_insert_rowid(db_);
    }

    std::cerr << "[DbManager] Execute error: " << sqlite3_errmsg(db_) << std::endl;
    return -1;
}

bool DbManager::beginTransaction() {
    char* err = nullptr;
    int rc = sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, &err);
    if (err) sqlite3_free(err);
    return rc == SQLITE_OK;
}

bool DbManager::commit() {
    char* err = nullptr;
    int rc = sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &err);
    if (err) sqlite3_free(err);
    return rc == SQLITE_OK;
}

bool DbManager::rollback() {
    char* err = nullptr;
    int rc = sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, &err);
    if (err) sqlite3_free(err);
    return rc == SQLITE_OK;
}
