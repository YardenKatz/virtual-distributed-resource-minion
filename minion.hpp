/*****************************************************************************/
/*                             Exercise: Minion                              */
/*                             Date: 24/03/19                                */
/*                             Version: 1.04                                 */
/*****************************************************************************/
#ifndef ILRD_MINION_HPP
#define ILRD_MINION_HPP

#include <iosfwd>                 // size_t
#include <netinet/in.h>           // sockaddr_in 
#include <boost/noncopyable.hpp>  // noncopyable
#include <boost/shared_ptr.hpp>   // shared_ptr

#include "cpp98_utils.hpp"
#include "reactor.hpp"
#include "factory.hpp"
#include "threadPool.hpp"
#include "timer.hpp"
#include "fileLogger.hpp"
#include "network_utils.hpp"
#include "filelog_utils.hpp"

namespace ilrd
{

struct System;

// MasterProxy manages communication with the master
class MasterProxy/*:  private boost::noncopyable */
{
public:
	typedef unsigned char byte;
	typedef std::vector<byte> BufferType;

	static const int BLOCK_SIZE = 4096;

	// - sys_ is the System MasterProxy is a part of, port_ is the port on
	//   which communication with the master takes place
	explicit MasterProxy(System& sys_, unsigned short port_);
	//~MasterProxy()noexcept; Generated is good enough
	
private:
	static const byte READ = 0;
	static const byte WRITE = 1;

	// - Command is the pure virtual base class all command types derive from
	// - the 'Do' method implements the execution of the command
	class Command: private boost::noncopyable
	{
	public:
		static const unsigned char SUCCESS = 0;
		static const unsigned char FAILURE = 1;

		//Command(); Generated is good enough
		virtual ~Command()noexcept{}

	private:
		friend class MasterProxy;

		virtual void Do(System& sys_) = 0;
		
	};// Command

	class Read; // defined in commands.hpp
	class Write; // defined in commands.hpp

	// - OnPacket handles the receiving of UDP packets from the master and
	//   runs the received commands' 'Do' method
	void OnPacket(int fd_);
	
	// - Reply sends the message contained in msg_ to the master. Any message
	//   formatting should be done by the caller
	void Reply(boost::shared_ptr<std::vector<byte> > msg_);

	Factory<MasterProxy::Command, char*, char> m_factory;
	UDPSock m_sock;
	System& m_sys;
	struct sockaddr_in m_address;
	UDPSender m_sender;

};// MasterProxy

// Drive handles Read/Write operations for the minion
class Drive: private boost::noncopyable
{
public:
	// Read/Write return statuses
	enum Status {SUCCESS = 0, INVALID, NOTEXIST, NOMEM, NOACCESS, UNKNOWN};

	explicit Drive(std::string storagePath_);
	//~Drive()noexcept; Generated is good enough

	// - Reads block from storage location blockNum_ into the buffer
	Status Read(int64_t blockNum_, 
					boost::shared_ptr<std::vector<MasterProxy::byte> > buffer_);
	
	// - Writes block from data_ to storage location blockNum_
	Status Write(int64_t blockNum_, 
				const boost::shared_ptr<std::vector<MasterProxy::byte> > data_);

private:
	std::string m_path;

};// Drive

// System holds and initializes the various components of the minion
struct System: private boost::noncopyable
{
	// - port_ is the UDP port passed on to  m_master
	// - storagePath_ is the path where m_drive will access data
	// - threadNum_ is the number of threads being run by m_tp
	explicit System(unsigned short port_ = 12345, 
					std::string storagePath_ = "./storage", 
					unsigned int threadNum_ = 0);
	//~System()noexcept; Generated is good enough

	Drive m_drive; // Keep m_drive at top of mill to make sure logger is 
	               // initialized before any objects in System
	Reactor m_reactor;
	ThreadPool m_tp;
	MasterProxy m_master;

};// System

}// ilrd

#endif // ILRD_MINION_HPP

