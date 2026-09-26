/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

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
/*!
 * \brief One SQLite database connection (see the file comment for an example).
 */
class Database {
public:
	Database() = default;
	/*! \brief Closes the database if open. */
	~Database();
	Database(const Database&) = delete;
	Database& operator=(const Database&) = delete;

	/*!
	 * \brief Opens (creating if needed) the database file, in WAL mode with a 3 s busy timeout.
	 * \param file path of the database file.
	 * \return false (and logs why) if it cannot be opened.
	 */
	bool open(const std::filesystem::path& file);
	/*! \brief Closes the database (does nothing if not open). */
	void close();
	/*! \brief true while a database is open. */
	bool isOpen() const { return db_ != nullptr; }
	/*! \brief The raw sqlite3 handle, for SQLite functions this class does not wrap. */
	sqlite3* handle() const { return db_; }
	/*! \brief The path given to open(). */
	const std::filesystem::path& file() const { return file_; }

	/*! \brief Runs one or more statements without results. Logs and returns false on error. */
	bool exec(const std::string& sql);
	/*! \brief Row id of the last INSERT on this connection. */
	int64_t lastInsertId() const;
	/*! \brief SQLite's message for the last error on this connection. */
	std::string lastError() const;

	/*! \brief A prepared statement: bind (1-based), then run() once or step() through the rows. */
	class Statement {
	public:
		/*!
		 * \brief Prepares sql on db. Check ok() - a syntax error is logged and leaves it unusable.
		 * \param db  an open database.
		 * \param sql one SQL statement, with ? placeholders for bind().
		 */
		Statement(Database& db, const std::string& sql);
		/*! \brief Finalizes the statement. */
		~Statement();
		Statement(const Statement&) = delete;
		Statement& operator=(const Statement&) = delete;

		/*! \brief true if the statement was prepared successfully. */
		bool ok() const { return stmt_ != nullptr; }
		/*! \brief Binds an integer to placeholder index (1-based); returns *this for chaining. */
		Statement& bind(int index, int64_t value);
		/*! \brief Binds an int to placeholder index (1-based). */
		Statement& bind(int index, int value) { return bind(index, static_cast<int64_t>(value)); }
		/*! \brief Binds a floating-point value to placeholder index (1-based). */
		Statement& bind(int index, double value);
		/*! \brief Binds UTF-8 text to placeholder index (1-based); the text is copied. */
		Statement& bind(int index, const std::string& utf8);
		/*! \brief Binds UTF-8 text to placeholder index (1-based); nullptr binds "". */
		Statement& bind(int index, const char* utf8) { return bind(index, std::string(utf8 ? utf8 : "")); }
		/*! \brief Binds SQL NULL to placeholder index (1-based). */
		Statement& bindNull(int index);

		/*! \brief Next row: true while a row is available, false when done (or on error, logged). */
		bool step();
		/*! \brief Executes a statement that returns no rows; resets it so it can be bound and run again. */
		bool run();
		/*! \brief Rewinds the statement and clears its bindings, ready to bind and run again. */
		void reset();

		/*! \brief Integer value of column (0-based) of the current row. */
		int64_t columnInt(int column) const;
		/*! \brief Floating-point value of column (0-based) of the current row. */
		double columnDouble(int column) const;
		/*! \brief UTF-8 text of column (0-based) of the current row ("" for NULL). */
		std::string columnText(int column) const;
		/*! \brief true if column (0-based) of the current row is NULL. */
		bool columnIsNull(int column) const;

	private:
		Database& db_;
		sqlite3_stmt* stmt_ = nullptr;
	};

	/*!
	 * \brief BEGIN on construction; ROLLBACK on destruction unless commit() was called. Much faster for
	 * many inserts, and all-or-nothing.
	 */
	class Transaction {
	public:
		/*! \brief Starts a transaction on db (BEGIN). */
		explicit Transaction(Database& db);
		/*! \brief Rolls back unless commit() was called. */
		~Transaction();
		/*! \brief Commits the transaction; returns false if COMMIT failed. */
		bool commit();
	private:
		Database& db_;
		bool done_ = false;
	};

private:
	sqlite3* db_ = nullptr;
	std::filesystem::path file_;
};
