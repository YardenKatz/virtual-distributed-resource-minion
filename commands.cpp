/*****************************************************************************/
/*                             Exercise: Minion                              */
/*                             Date: 27/03/19                                */
/*                             Author: Yarden Katz                           */
/*****************************************************************************/

#include "commands.hpp"

namespace ilrd
{

/****************************    Read   *****************************/

MasterProxy::Read::Read()
 :m_data(boost::make_shared<BufferType>()){}


boost::shared_ptr<MasterProxy::Command> MasterProxy::Read::Creator(const char* packet_)
{
	// create new read object
	boost::shared_ptr<Read> ret = boost::make_shared<Read>();
	
	// get members from packet
	memcpy(&(ret->m_header), packet_, sizeof(PacketHeader));

	//TODO: check endianess
/* 
	// convert byte order of all fields except of data block 
	ret->m_header.m_type = __builtin_bswap64(ret->m_header.m_type);
	ret->m_header.m_uid = __builtin_bswap64(ret->m_header.m_uid);
	ret->m_header.m_block = __builtin_bswap64(ret->m_header.m_block);
 */
	return ret;
}

void MasterProxy::Read::Do(System& sys_)
{
	// call sys_->drive->Read and save it into m_data
	Drive::Status status = sys_.m_drive.Read(m_header.m_block, m_data);

	// call masterproxy->reply with m_data
	struct ReplyHeader reply_header = {MasterProxy::READ, m_header.m_uid, 
																		status};
	boost::shared_ptr<BufferType> reply = boost::make_shared<BufferType>();
	reply->assign(reinterpret_cast<char *>(&reply_header), 
					reinterpret_cast<char *>(&reply_header + 1));
	reply->insert(reply->end(), m_data->begin(), m_data->end());
	
	sys_.m_master.Reply(reply);
}

/***************************   Write  ******************************/


MasterProxy::Write::Write()
 :m_data(boost::make_shared<BufferType>()){}

boost::shared_ptr<MasterProxy::Command> MasterProxy::Write::Creator(const char* packet_)
{
	// create new write object
	boost::shared_ptr<Write> ret = boost::make_shared<Write>();
	
	// get members from packet
 	memcpy(&(ret->m_header), packet_, sizeof(PacketHeader));

	 //TODO: check endianess
	/* 
	// convert byte order of all fields except of data block 
	ret->m_header.m_type = __builtin_bswap64(ret->m_header.m_type);
	ret->m_header.m_uid = __builtin_bswap64(ret->m_header.m_uid);
	ret->m_header.m_block = __builtin_bswap64(ret->m_header.m_block);
 */
	// get data for write
	ret->m_data->assign(packet_ + sizeof(PacketHeader), packet_ + sizeof(PacketHeader) + BLOCK_SIZE);
	
	return ret;
}

void MasterProxy::Write::Do(System& sys_)
{
	// call sys_->drive->write with m_data
	Drive::Status status = sys_.m_drive.Write(m_header.m_block, m_data);
	
	// call masterproxy->reply with status
	ReplyHeader reply = {m_header.m_type, m_header.m_uid, status};
	boost::shared_ptr<std::vector<byte> > replyMsg = 
									boost::make_shared<std::vector<byte> >();
	replyMsg->assign(reinterpret_cast<char *>(&reply), 
					reinterpret_cast<char *>(&reply + 1));

	sys_.m_master.Reply(replyMsg);
}

} // ilrd
