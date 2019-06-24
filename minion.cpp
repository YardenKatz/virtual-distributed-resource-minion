#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include "commands.hpp"
#include "filelog_utils.hpp"

namespace ilrd
{

//////////////////////////  master proxy //////////////////

MasterProxy::MasterProxy(System& sys_, unsigned short port_)
: m_factory(), m_sock(port_), m_sys(sys_),
									m_sender(m_sys.m_reactor, m_sock.GetFD())
{
	// add commands to factory
	m_factory.Add(READ, Read::Creator);
	m_factory.Add(WRITE, Write::Creator);

    // add callbak to reactor 
	sys_.m_reactor.Add(m_sock.GetFD(), Reactor::READ, 
						boost::bind(&MasterProxy::OnPacket, this, _1));

	LOGI("end of master proxy CTOR");
}

void MasterProxy::OnPacket(int fd_)
{
	const size_t buffer_size = sizeof(PacketHeader) + BLOCK_SIZE;
	char buffer[buffer_size];
	uint len = sizeof(sockaddr_in);

	// read message from fd to get the type (recivefrom)
	int mess_len = recvfrom(fd_, buffer, buffer_size, MSG_WAITALL, 
						reinterpret_cast<struct sockaddr*>(&m_address), &len);
	if(-1 == mess_len)
	{
		LOGE("OnPacket: receive from failed");
		return;
	}
	LOGI("OnPacket: got message");

	// create from factory create with key = type - read or write or whatever
	boost::shared_ptr<Command> command = m_factory.Create(buffer[0],buffer);

	// run do of specific command
	command->Do(m_sys);
}

void MasterProxy::Reply(boost::shared_ptr<BufferType> msg_)
{
	// send back reply through udpsender
	m_sender.SendReply(m_address, msg_);

	LOGI("Reply message sent");
}

//////////////////// drive /////////////////////////////

Drive::Drive(std::string storagePath_)
: m_path(storagePath_)
{
	boost::filesystem::path dir(m_path);

    if(!(boost::filesystem::exists(dir)))
    {
        LOGI("path Doesn't Exists");

        if (boost::filesystem::create_directory(dir))
        {
            LOGI ("....Successfully Created !");
        }    
    }	
}

Drive::Status Drive::Read(int64_t blockNum_, 
							boost::shared_ptr<MasterProxy::BufferType> buffer_)
{
	// create file path
	std::string filePath(m_path + "/" + 
						boost::lexical_cast<std::string>(blockNum_) /* + ".txt" */);
	
	// check that file exists
	if (!boost::filesystem::exists(filePath))
	{
		LOGE("File Does Not Exist!");
		return NOTEXIST;
	}

	// open file
	std::ifstream srcFile(filePath.c_str());

	if (!srcFile.is_open())
	{
		LOGE("failed to open file for reading");
		return UNKNOWN;
	}
	LOGI("opened file for reading");

	//read block
	char temp[4096];
	srcFile.read(temp, MasterProxy::BLOCK_SIZE);
	if (srcFile.fail())
	{
		LOGW("failed reading from file");
		return UNKNOWN;
	}

	// save data in temporary buffer into buffer_ vector
	buffer_->insert(buffer_->begin(), temp, temp + MasterProxy::BLOCK_SIZE);
	LOGI("read from file");

	srcFile.close();
	LOGI("closed file");

	return SUCCESS;												
}

Drive::Status Drive::Write(int64_t blockNum_, 
				const boost::shared_ptr<std::vector<MasterProxy::byte> > data_)
{
	// open file with overwrite setting
	std::string filePath(m_path + "/" + 
						boost::lexical_cast<std::string>(blockNum_) /* +  ".txt" */);
	std::ofstream destFile((filePath).c_str());

	if (!(destFile.is_open() | std::ofstream::goodbit))
	{
		LOGE("failed to open file for writing");
		return UNKNOWN;
	}
	LOGI("opened file for writing");

	destFile.write(reinterpret_cast<char *>(&(*data_)[0]), 
								MasterProxy::BLOCK_SIZE); 
	LOGI("write to file");
	destFile.close();
	LOGI("file successfully closed");

	return SUCCESS;
}

/////////////// system /////////////////////////////////

System::System(unsigned short port_, std::string storagePath_ , unsigned int threadNum_)
: m_drive(storagePath_), m_reactor(), m_tp(threadNum_), m_master(*this, port_)
{
	m_reactor.Run();
}

} // ilrd

