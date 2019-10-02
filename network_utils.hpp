/*****************************************************************************/
/*                             Exercise: Minion                              */
/*                             Date: 28/03/19                                */
/*							   Version: 1.04								 */
/*                             Coder: RD-54569                               */
/*****************************************************************************/

#ifndef ILRD_NETWORK_UTILS_HPP
#define ILRD_NETWORK_UTILS_HPP

#include <iosfwd>                 // size_t
#include <netinet/in.h>           // sockaddr_in
#include <queue>                  // queue, vector
#include <exception>			  // exceptions 
#include <boost/noncopyable.hpp>  // noncopyable
#include <boost/shared_ptr.hpp>   // shared_ptr
#include <boost/bind.hpp>

#include "cpp98_utils.hpp"
#include "reactor.hpp"
#include "filelog_utils.hpp"

namespace ilrd
{

// - UDPSock opens a UDP socket on the given port for communication when 
//   created and closes it when destroyed
class UDPSock: private boost::noncopyable
{
public:
	explicit UDPSock(unsigned short port_);
	~UDPSock() noexcept;

	int GetFD()const;

	struct SocketCreationFailed : public std::exception
    {
        const char * what () const throw ()
        {
            return "Socket Creation Failed";
        }
    };

	struct SocketBindingFailed : public std::exception
    {
        const char * what () const throw ()
        {
            return "Socket Binding Failed";
        }
    };
	
private:
	int m_fd;
	struct sockaddr m_addr;
	// socklen_t m_addrLen;

};// UDPSock

// UDPSender handles the sending of messages through a UDP socket
class UDPSender
{
public:
	// messages are sent using reactor_ write set, fd_ is the udp socket
	explicit UDPSender(Reactor& reactor_, int fd_);
	~UDPSender() noexcept; 

	void SendReply(struct sockaddr_in addr_,
					boost::shared_ptr< std::vector<unsigned char> > msg_);

	struct PipeCreationFailed : public std::exception
	{
		const char *what() const noexcept
		{
			return "Pipe Creation Failed";
		}
	};

	struct ReadFromPipeFailed : public std::exception
    {
        const char *what() const noexcept
        {
            return "Read From Pipe Failed";
        }
    };

	struct WriteToPipeFailed : public std::exception
    {
        const char *what() const noexcept
        {
            return "Write To Pipe Failed";
        }
    };
	
	struct SendReplyFailed : public std::exception
    {
        const char *what() const noexcept
        {
            return "Send Reply Failed";
        }
    };

private:
	typedef unsigned char byte;
	typedef boost::shared_ptr< std::vector<byte> > MsgPtr;
	typedef std::pair<MsgPtr, struct sockaddr_in> ElType;

	void SendIMP(int fd_);
	void CallbackAdderIMP(int fd_);
	void WakeReactorIMP();

	Reactor& m_reactor;
	int m_pipeFD[2];
	int m_writeFD;
	std::queue<ElType> m_replyQueue;
	ElType m_currMsg;
};// UDPSender

}// ilrd

#endif //ILRD_NETWORK_UTILS_HPP 