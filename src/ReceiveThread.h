#pragma once

#include "ofMain.h"

#include <Poco/Net/DatagramSocket.h>

class ReceiveThread : protected ofThread
{
public:
	
	typedef ofPtr<ReceiveThread> Ptr;
	
	~ReceiveThread()
	{
		close();
	}
	
	void setup(int port)
	{
		close();
		sock.bind(Poco::Net::SocketAddress("0.0.0.0", port), true);
		startThread();
	}
	
	void close()
	{
		sock.close();
		waitForThread(true);
	}
	
	string getLatestMessage()
	{
		lock();
		string ret = latest_message;
		unlock();
		return ret;
	}
	
protected:
	
	Poco::Net::DatagramSocket sock;
	
	string latest_message;
	
	void threadedFunction()
	{
		while (isThreadRunning())
		{
			const size_t length = 512;
			char buf[length];
			string data;
			
			while (isThreadRunning())
			{
				int n = sock.receiveBytes(buf, length);
				data.append(buf, n);
				if (n != length) break;
			}
			
			if (data.size())
			{
				onDataReceived(data);
				
				lock();
				latest_message = data;
				unlock();
			}
		}
	}
	
	virtual void onDataReceived(const string& data) {}
};
