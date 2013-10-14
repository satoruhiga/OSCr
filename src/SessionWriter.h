#pragma once

#include "ofMain.h"
#include "ofxMSATimer.h"
#include "sqlite3.h"

class SessionWriter
{
public:
	
	typedef ofPtr<SessionWriter> Ptr;
	typedef float TimeStamp;
	
	SessionWriter() : db(NULL), is_recording(false), start_time(-1) {}
	~SessionWriter() { close(); }
	
	bool open(const string& path, bool use_journal = false)
	{
		close();
		
		if (ofFile::doesFileExist(path))
		{
			ofLogError("SessionWriter") << "file already exists: " << path;
			return false;
		}
		
		sqlite3_open(ofToDataPath(path).c_str(), &db);
		migrateDB(use_journal);
		
		const char *sql = "insert into Message(time, data) VALUES(?, ?)";
		sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
		
		return true;
	}
	
	void close()
	{
		if (is_recording)
		{
			is_recording = false;
			start_time = -1;
		}
		
		if (db)
		{
			exec("pragma synchronous = ON");
			exec("create index time_index on Message(time)");
			
			sqlite3_finalize(stmt);
			stmt = NULL;
			
			sqlite3_close(db);
			db = NULL;
		}
	}
	
	void start()
	{
		if (!db)
		{
			ofLogError("SessionWriter") << "open session first";
			throw;
		}
		
		timer.setStartTime();
		is_recording = true;
	}
	
	void stop()
	{
		close();
	}
	
	bool isRecording() const { return is_recording; }
	
	void addMessage(const string &data)
	{
		TimeStamp t = timer.getElapsedSeconds();

		sqlite3_bind_double(stmt, 1, t);
		sqlite3_bind_blob(stmt, 2, data.data(), data.size(), SQLITE_TRANSIENT);
		
		if (SQLITE_DONE != sqlite3_step(stmt))
		{
			ofLogError("SessionWriter") << sqlite3_errmsg(db);
		}
		
		sqlite3_clear_bindings(stmt);
		sqlite3_reset(stmt);
	}
	
	void clear()
	{
		try
		{
			exec("drop table if exists Message");
		}
		catch (exception &e)
		{
			ofLogError("SessionWriter") << e.what();
			throw;
		}
	}
	
protected:
	
	sqlite3* db;
	sqlite3_stmt *stmt;
	
	bool is_recording;
	TimeStamp start_time;
	
	ofxMSATimer timer;
	
	void migrateDB(bool use_journal)
	{
		clear();
		
		try
		{
			if (!use_journal)
			{
				exec("pragma journal_mode = MEMORY");
				exec("pragma synchronous = OFF");
			}
			
			exec("create table if not exists Message(time double, data blob)");
		}
		catch (exception &e)
		{
			ofLogError("SessionWriter") << e.what();
			throw;
		}
	}
	
	void exec(const char* sql)
	{
		char *err = NULL;
		sqlite3_exec(db, sql, 0, 0, &err);
		
		if (err != NULL)
		{
			ofLogError("SessionWriter") << err;
			sqlite3_free(err);
		}
	}
};

