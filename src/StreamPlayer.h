#pragma once

#include <Poco/Net/DatagramSocket.h>
#include "ofxMSATimer.h"
#include "SessionReader.h"

class StreamPlayer : protected ofThread
{
public:
	
	typedef float TimeStamp;
	
	StreamPlayer() : fps(120), is_playing(false), loop(false)
	{
		setupSocket();
		setPosition(0);
	}
	
	~StreamPlayer() { close(); }
	
	void addHost(string ip, int port)
	{
		addresses.push_back(Poco::Net::SocketAddress(ip, port));
	}
	
	void clearHost()
	{
		addresses.clear();
	}
	
	vector<Poco::Net::SocketAddress> getAddresses() const
	{
		return addresses;
	}
	
	bool open(const string& path)
	{
		close();
		
		lock();
		
		session = SessionReader::Ptr(new SessionReader);
		if (!session->open(path))
		{
			session.reset();
			unlock();
			return false;
		}
		
		setPosition(0);
		
		unlock();
		
		return true;
	}
	
	void close()
	{
		stop();
	}
	
	void start()
	{
		is_playing = true;
		startThread();
	}
	
	void stop()
	{
		stopThread();
		is_playing = false;
		
		data = "";
	}
	
	void setPosition(int sec)
	{
		current_tick = sec;
		last_tick = sec;
	}
	
	TimeStamp getPosition() const { return current_tick; }
	TimeStamp getDuration() const
	{
		if (session)
			return session->getLastTimeStamp();
		else
			return 0;
	}
	
	//
	
	float getPlaybackFps() const { return playback_fps; }
	int getMessagePerFrame() const { return message_per_frame; }
	
	bool isOpend() const { return session; }
	bool isPlaying() const { return is_playing; }
	
	void setLoopState(bool yn) { loop = yn; }
	bool getLoopState() const { return loop; }
	
	string getLatestMessage()
	{
		lock();
		string ret = data;
		unlock();
		return ret;
	}
	
protected:
	
	bool loop;
	bool is_playing;
	SessionReader::Ptr session;
	
	ofxMSATimer timer;
	TimeStamp current_tick;
	TimeStamp last_tick;
	
	float fps;
	float playback_fps;
	int message_per_frame;
	
	string data;
	
	Poco::Net::DatagramSocket sock;
	vector<Poco::Net::SocketAddress> addresses;
	
	void threadedFunction()
	{
		while (isThreadRunning())
		{
			lock();
			
			TimeStamp inv_fps = 1. / fps;
			
			if (!session)
			{
				sleep(inv_fps * 1000);
				unlock();
				continue;
			}
			
			TimeStamp t = timer.getAppTimeSeconds();
			
			// TODO: buffer reocrds
			vector<string> records;
			session->getRecords(last_tick, current_tick, records);
			
			for (int i = 0; i < records.size(); i++)
			{
				const string &data = records[i];
				for (int n = 0; n < addresses.size(); n++)
				{
					sock.sendTo(data.data(), data.size(), addresses[n]);
				}
				
				this->data = data;
			}
			
			message_per_frame = records.size();
			
			TimeStamp tt = timer.getAppTimeSeconds();
			TimeStamp delta = tt - t;
			
			TimeStamp sleep_time = (inv_fps - delta);
			if (sleep_time > 0) sleep(sleep_time * 1000);
			
			TimeStamp spend = timer.getSecondsSinceLastCall();
			
			last_tick = current_tick;
			current_tick += spend;
			
			playback_fps = 1. / spend;
			
			// brake if finished
			if (current_tick > session->getLastTimeStamp())
			{
				if (loop)
				{
					setPosition(0);
				}
				else
				{
					break;
				}
			}
			
			unlock();
		}
		
		is_playing = false;
	}
	
	void setupSocket()
	{
		sock.setBlocking(false);
		sock.setReceiveTimeout(0);
	}
};
