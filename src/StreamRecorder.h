#pragma once

#include "ReceiveThread.h"
#include "SessionWriter.h"

class StreamRecorder : protected ReceiveThread
{
public:
	
	~StreamRecorder()
	{
		stop();
	}
	
	void open(int port)
	{
		ReceiveThread::setup(port);
		sock.setBlocking(false);
	}
	
	void close()
	{
		ReceiveThread::close();
	}
	
	void start(const string& session_filename = "", bool overwrite = false)
	{
		this->session_filename = session_filename;
		
		if (overwrite)
		{
			ofFile file(session_filename);
			if (file.exists())
			{
				file.remove();
			}
		}
		
		setupSession(session_filename);
		session->start();
	}
	
	void stop()
	{
		closeSession();
	}
	
	inline bool isRecording() const { return session != NULL; }
	
	string getFilename() const
	{
		return session_filename;
	}
	
	string getLatestMessage()
	{
		return ReceiveThread::getLatestMessage();
	}
	
	void setThroughAddresses(vector<Poco::Net::SocketAddress> addresses)
	{
		this->addresses = addresses;
	}
	
	bool getEnableThroughOut() const { return enable_through_out; }
	void setEnableThroughOut(bool yn) { enable_through_out = yn; }
	
protected:
	
	SessionWriter::Ptr session;
	ofMutex mutex;

	string session_filename;
	
	vector<Poco::Net::SocketAddress> addresses;
	Poco::Net::DatagramSocket sock;
	
	bool enable_through_out;
	
	void onDataReceived(const string& data)
	{
		mutex.lock();
		if (session) session->addMessage(data);
		mutex.unlock();
		
		if (enable_through_out)
		{
			for (int i = 0; i < addresses.size(); i++)
			{
				sock.sendTo(data.data(), data.size(), addresses[i]);
			}
		}
	}
	
	void setupSession(const string& session_filename)
	{
		mutex.lock();
		session = SessionWriter::Ptr(new SessionWriter);
		assert(session);
		assert(session->open(session_filename, true));
		mutex.unlock();
	}
	
	void closeSession()
	{
		mutex.lock();
		session_filename = "";
		session.reset();
		mutex.unlock();
	}
};
