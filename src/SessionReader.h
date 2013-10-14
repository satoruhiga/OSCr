#pragma once

#include "ofMain.h"
#include "ofxMSATimer.h"
#include "sqlite3.h"

class SessionReader
{
public:
	
	typedef ofPtr<SessionReader> Ptr;
	typedef float TimeStamp;
	
	SessionReader() : db(NULL), last_time_stamp(0) {}
	~SessionReader() { close(); }
	
	static int onSelectLastTimeStamp(void *arg, int argc, char **argv, char **column)
	{
		if (argc == 0) return SQLITE_ERROR;
		if (string(column[0]) != "time") return SQLITE_ERROR;
		
		SessionReader *self = (SessionReader*)arg;
		self->last_time_stamp = ofToFloat(argv[0]);
		
		return SQLITE_OK;
	}
	
	bool open(const string& path)
	{
		close();
		
		if (!ofFile::doesFileExist(path))
		{
			ofLogError("SessionReader") << "no such file: " << path;
			return false;
		}
		
		sqlite3_open(ofToDataPath(path).c_str(), &db);
		
		if (SQLITE_OK != sqlite3_exec(db, "select time from Message order by time desc limit 1", onSelectLastTimeStamp, this, NULL))
		{
			ofLogError("SessionReader") << "invalid file: " << path;
			return false;
		}
		
		const char *sql = "select data from Message where time BETWEEN ? and ?";
		if (SQLITE_OK != sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL))
		{
			ofLogError("SessionReader") << sqlite3_errmsg(db);
			return false;
		}

		return true;
	}
	
	void close()
	{
		if (db)
		{
			sqlite3_close(db);
			db = NULL;
		}
	}
	
	void getRecords(TimeStamp from, TimeStamp to, vector<string> &records)
	{
		records.clear();
		
		sqlite3_bind_double(stmt, 1, from);
		sqlite3_bind_double(stmt, 2, to);
		
		while (SQLITE_ROW == sqlite3_step(stmt))
		{
			char *ptr = (char*)sqlite3_column_blob(stmt, 0);
			size_t len = sqlite3_column_bytes(stmt, 0);
			records.push_back(string(ptr, len));
		}
		
		sqlite3_clear_bindings(stmt);
		sqlite3_reset(stmt);
	}
	
	inline TimeStamp getLastTimeStamp() const { return last_time_stamp; }
	
protected:
	
	sqlite3 *db;
	sqlite3_stmt *stmt;
	
	bool is_opend;
	TimeStamp last_time_stamp;
	
	ofxMSATimer timer;
};
