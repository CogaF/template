// Copyright (C) 2026 Fation Coga
// SPDX-License-Identifier: LGPL-3.0-or-later
// This file is part of Template App - see COPYING and COPYING.LESSER.

#include "Database.h"
#include "Log.h"

#include "sqlite3.h"

Database::~Database() { close(); }

bool Database::open(const std::filesystem::path& file) {
	close();
	const std::string utf8 = std::string(reinterpret_cast<const char*>(file.u8string().c_str()));
	if (sqlite3_open_v2(utf8.c_str(), &db_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
		Log::error("Database: cannot open " + utf8 + ": " + (db_ ? sqlite3_errmsg(db_) : "out of memory"));
		close();
		return false;
	}
	file_ = file;
	sqlite3_busy_timeout(db_, 3000);
	exec("PRAGMA journal_mode=WAL; PRAGMA synchronous=NORMAL; PRAGMA foreign_keys=ON;");
	Log::info("Database: opened " + utf8);
	return true;
}

void Database::close() {
	if (db_) sqlite3_close_v2(db_);
	db_ = nullptr;
}

bool Database::exec(const std::string& sql) {
	if (!db_) return false;
	char* err = nullptr;
	if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
		Log::error("Database: " + std::string(err ? err : "error") + " in: " + sql);
		sqlite3_free(err);
		return false;
	}
	return true;
}

int64_t Database::lastInsertId() const { return db_ ? sqlite3_last_insert_rowid(db_) : 0; }
std::string Database::lastError() const { return db_ ? sqlite3_errmsg(db_) : "not open"; }

// --- Statement ---

Database::Statement::Statement(Database& db, const std::string& sql) : db_(db) {
	if (!db.db_) return;
	if (sqlite3_prepare_v2(db.db_, sql.c_str(), -1, &stmt_, nullptr) != SQLITE_OK) {
		Log::error("Database: cannot prepare \"" + sql + "\": " + db.lastError());
		stmt_ = nullptr;
	}
}

Database::Statement::~Statement() { if (stmt_) sqlite3_finalize(stmt_); }

Database::Statement& Database::Statement::bind(int index, int64_t value) { if (stmt_) sqlite3_bind_int64(stmt_, index, value); return *this; }
Database::Statement& Database::Statement::bind(int index, double value) { if (stmt_) sqlite3_bind_double(stmt_, index, value); return *this; }
Database::Statement& Database::Statement::bind(int index, const std::string& utf8) {
	if (stmt_) sqlite3_bind_text(stmt_, index, utf8.c_str(), static_cast<int>(utf8.size()), SQLITE_TRANSIENT);
	return *this;
}
Database::Statement& Database::Statement::bindNull(int index) { if (stmt_) sqlite3_bind_null(stmt_, index); return *this; }

bool Database::Statement::step() {
	if (!stmt_) return false;
	const int rc = sqlite3_step(stmt_);
	if (rc == SQLITE_ROW) return true;
	if (rc != SQLITE_DONE) Log::error("Database: step failed: " + db_.lastError());
	return false;
}

bool Database::Statement::run() {
	if (!stmt_) return false;
	const int rc = sqlite3_step(stmt_);
	reset();
	if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
		Log::error("Database: statement failed: " + db_.lastError());
		return false;
	}
	return true;
}

void Database::Statement::reset() {
	if (!stmt_) return;
	sqlite3_reset(stmt_);
	sqlite3_clear_bindings(stmt_);
}

int64_t Database::Statement::columnInt(int column) const { return stmt_ ? sqlite3_column_int64(stmt_, column) : 0; }
double Database::Statement::columnDouble(int column) const { return stmt_ ? sqlite3_column_double(stmt_, column) : 0.0; }
std::string Database::Statement::columnText(int column) const {
	if (!stmt_) return {};
	const unsigned char* t = sqlite3_column_text(stmt_, column);
	return t ? std::string(reinterpret_cast<const char*>(t)) : std::string();
}
bool Database::Statement::columnIsNull(int column) const { return !stmt_ || sqlite3_column_type(stmt_, column) == SQLITE_NULL; }

// --- Transaction ---

Database::Transaction::Transaction(Database& db) : db_(db) { db_.exec("BEGIN"); }
Database::Transaction::~Transaction() { if (!done_) db_.exec("ROLLBACK"); }
bool Database::Transaction::commit() {
	done_ = true;
	return db_.exec("COMMIT");
}
