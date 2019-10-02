/************************************************************
 * 															*
 * 					Network Utils							*
 * 					Author: Yarden Katz						*
 * 					Date: 1.4.19							*
 * 															*
 ***********************************************************/
#include "network_utils.hpp"

// #include <iostream>

namespace ilrd
{

/************************   UDPSock ****************************/

UDPSock::UDPSock(unsigned short port_)
{
	//create socket
	if (-1 == (m_fd = socket(AF_INET, SOCK_DGRAM, 0)))
	{
		LOGE("socket creation failed");
		throw SocketCreationFailed();
	}
	
	// make socket reusable 
	int enable = 1;
	if (0 > setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)))
	{
		LOGE("setsockopt(SO_REUSEADDR) failed");
	}

	struct sockaddr_in servAddr;
    memset(&m_addr, 0, sizeof(m_addr)); 
    memset(&servAddr, 0, sizeof(servAddr));   

    // Filling server information 
    servAddr.sin_family = AF_INET; // IPv4 
    servAddr.sin_addr.s_addr = INADDR_ANY; 
    servAddr.sin_port = htons(port_); 
      
	//bind address to socket
	if (-1 == bind(m_fd, (sockaddr *)&servAddr, sizeof(servAddr)))
	{
		LOGE("socket binding failed");
		throw SocketBindingFailed();
	}
	LOGI("socket created");
}

UDPSock::~UDPSock() noexcept
{
	close(m_fd);
	LOGI("socket closed");
}

int UDPSock::GetFD() const
{
	return m_fd;
}


/**************************  UDPSender  *********************/

UDPSender::UDPSender(Reactor& reactor_, int fd_)
 :m_reactor(reactor_), m_writeFD(fd_), m_replyQueue()
{
	// create communication pipe with reactor
	if (-1 == pipe(m_pipeFD))
	{
		LOGE("Pipe creation failed");
		throw PipeCreationFailed();
	}

	LOGI("pipe created");

	// add pipe/callbackAdderIMP pair to reactor read set
	m_reactor.Add(m_pipeFD[0], Reactor::READ, 
						boost::bind(&UDPSender::CallbackAdderIMP, this, _1));
	LOGI("pipe/CallBackAdder added to reactor");																
	LOGI("UDPSender created");
}

UDPSender::~UDPSender() noexcept
{
	m_reactor.Remove(m_pipeFD[0], Reactor::READ);
	LOGI("pipe removed from reactor");

	close (m_pipeFD[0]);
	close (m_pipeFD[1]);
	LOGI("pipe closed");
}

void UDPSender::CallbackAdderIMP(int fd_)
{
	(void) fd_;
	// read from pipe to empty it
	char buffer;
	if (1 != read(m_pipeFD[0], &buffer, sizeof(char)))
	{
		LOGW("failed reading from pipe");
		throw ReadFromPipeFailed();
	}
	else
	{
		LOGI("read from pipe");
	}
	
	// if queue is empty add m_writeFD/SendIMP pair to reactor write set
	if (1 == m_replyQueue.size())
	{
		m_reactor.Add(m_writeFD, Reactor::WRITE, 
									boost::bind(&UDPSender::SendIMP, this, _1));
		LOGI("SendIMP added to reactor");
	}
}

void UDPSender::SendReply(struct sockaddr_in addr_,
					boost::shared_ptr< std::vector<unsigned char> > msg_)
{
	// add msg_ to queue
	m_replyQueue.push(std::make_pair(msg_, addr_));
	LOGI("pushed message to queue");

	// call WakeReactorIMP
	WakeReactorIMP();
}	


void UDPSender::SendIMP(int fd_)
{
	(void)fd_;

	
	// send top message from queue to m_writeFD
	m_currMsg = m_replyQueue.front();
	ssize_t sendStatus = sendto(m_writeFD,&(*m_currMsg.first)[0], 
						m_currMsg.first->size(), 0, 
						reinterpret_cast<struct sockaddr *>(&m_currMsg.second),	
						sizeof(m_currMsg.second));	
						
	if ((-1 == sendStatus) ||(m_currMsg.first->size() != 
								static_cast<size_t>(sendStatus)))
	{
		LOGW("failed sending complete reply");
		throw SendReplyFailed();
	}
	else
	{
		LOGI("reply sent");
	}

	// deque 
	m_replyQueue.pop();
	LOGI("popped message from queue");

	// if queue is empty remove m_writeFD from reactor
	if (m_replyQueue.empty())
	{
		m_reactor.Remove(m_writeFD, Reactor::WRITE);
		LOGI("removed SendIMP from reactor");
	}
	
}

void UDPSender::WakeReactorIMP()
{
	// write char on pipe
	char wakeMsg = 1;
	if (-1 == write(m_pipeFD[1], &wakeMsg, sizeof(char)))
	{
		LOGW("failed writing to pipe");
		throw WriteToPipeFailed();
	}
	LOGI("write to pipe");
}


} // ilrd


