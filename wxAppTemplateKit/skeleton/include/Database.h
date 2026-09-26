// Copyright (C) 2026 Fation Coga
// SPDX-License-Identifier: LGPL-3.0-or-later
// This file is part of Template App - see COPYING and COPYING.LESSER.

#pragma once

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>

struct sqlite3;
struct sqlite3_stmt;

/*!
 * \file Database.h
 * \brief Minimal RAII wrapper around one SQLite database file.
 *
 *     Database db;
 *     db.open(DataDir::file("history.db"));
 *     db.exec("CREATE TABLE IF NOT EXISTS events(time_ms INTEGER, text TEXT)");
 *     Database::Statement ins(db, "INSERT INTO events VALUES(?, ?)");
 *     ins.bind(1, nowMs).bind(2, "started");
 *     ins.run();
 *
 * Opened in WAL mode with a busy timeout, so a reader (e.g. an export) does not block the writer.
 * One connection may be used from several threads (SQLite's default "serialized" mode), but a
 * Statement belongs to one thread at a time. Text is UTF-8: pass wxString::utf8_string(), read back
 * with wxString::FromUTF8().
 */
class Database {
public:
	Database() = default;
	~Database();
	Database(const Database&) = delete;
	Database& operator=(const Database&) = delete;

	bool open(const std::filesystem::path& file);
	void close();
	bool isOpen() const { return db_ != nullptr; }
	sqlite3* handle() const { return db_; }
	const std::filesystem::path& file() const { return file_; }

	//! Runs one or more statements without results. Logs and returns false on error.
	bool exec(const std::string& sql);
	int64_t lastInsertId() const;
	std::string lastError() const;

	//! A prepared statement: bind (1-based), then run() once or step() through the rows.
	class Statement {
	public:
		Statement(Database& db, const std::string& sql);
		~Statement();
		Statement(const Statement&) = delete;
		Statement& operator=(const Statement&) = delete;

		bool ok() const { return stmt_ != nullptr; }
		Statement& bind(int index, int64_t value);
		Statement& bind(int index, int value) { return bind(index, static_cast<int64_t>(value)); }
		Statement& bind(int index, double value);
		Statement& bind(int index, const std::string& utf8);
		Statement& bind(int index, const char* utf8) { return bind(index, std::string(utf8 ? utf8 : "")); }
		Statement& bindNull(int index);

		//! Next row: true while a row is available, false when done (or on error, logged).
		bool step();
		//! Executes a statement that returns no rows; resets it so it can be bound and run again.
		bool run();
		void reset();

		int64_t columnInt(int column) const;
		double columnDouble(int column) const;
		std::string columnText(int column) const;
		bool columnIsNull(int column) const;

	private:
		Database& db_;
		sqlite3_stmt* stmt_ = nullptr;
	};

	//! BEGIN on construction; ROLLBACK on destruction unless commit() was called. Much faster for
	//! many inserts, and all-or-nothing.
	class Transaction {
	public:
		explicit Transaction(Database& db);
		~Transaction();
		bool commit();
	private:
		Database& db_;
		bool done_ = false;
	};

private:
	sqlite3* db_ = nullptr;
	std::filesystem::path file_;
};
