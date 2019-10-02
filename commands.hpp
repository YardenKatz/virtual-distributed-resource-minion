/*****************************************************************************/
/*                             Exercise: Minion                              */
/*                             Date: 24/03/19                                */
/*                             Version: 1.03                                 */
/*****************************************************************************/

#ifndef ILRD_COMMANDS_HPP
#define ILRD_COMMANDS_HPP

#include "minion.hpp"
	
namespace ilrd
{

// PacketHeader stores the data structure of Read, Write and similarly 
// formatted commands
#pragma pack(1)
struct PacketHeader
{
	unsigned char m_type;
	int64_t m_uid;
	int64_t m_block;
}; // PacketHeader
#pragma pack()

#pragma pack(1)
struct ReplyHeader
{
	unsigned char m_type;
	int64_t m_uid;
	unsigned char m_status;
}; // PacketHeader
#pragma pack()

// Read is a command for reading a block from minion's storage
class MasterProxy::Read: public MasterProxy::Command
{
public:
	static boost::shared_ptr<MasterProxy::Command> Creator(const char* packet_);

	explicit Read();
	~Read () noexcept{}

private:
	virtual void Do(System& sys_);

	PacketHeader m_header;
	boost::shared_ptr<MasterProxy::BufferType> m_data;

}; // Read

// Write is a command for writing a block into minion's storage
class MasterProxy::Write: public MasterProxy::Command
{
public:
	static boost::shared_ptr<MasterProxy::Command> Creator(const char* packet_);

	explicit Write();
	~Write () noexcept{}

private:
	virtual void Do(System& sys_);

	PacketHeader m_header;
	boost::shared_ptr<MasterProxy::BufferType> m_data;
}; // Write

}// ilrd

#endif //ILRD_COMMANDS_HPP