#ifndef __MESSENGER_H__
#define __MESSENGER_H__

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <sys/time.h>
#include <ctime>

#include "simplePipe.h"
#define DEFAULT_SERVER_ADDRESS "127.0.0.1"
#define DEFAULT_SERVER_PORT 12000
#define DEFAULT_FIFO "/tmp/noah_angle"

class Messenger
{
public:
	// Constructor
	//Messenger();							// default
	//Messenger(const char*, int);		// specify address and port
	Messenger();							// default
	Messenger(const char*, int);		// specify fifo and size of message

	// Destructor
	~Messenger();

	// send detect timestamp, returns the timestamp
	std::string send_detect();

	// send timestamp, returns the timestamp
	std::string send_time();

	// send angle timestamp
	void send_angle(double, double, std::string);

private:
	char fifo_file[50];
	char server_address[50];
	int server_port;

	int sock;
	struct sockaddr_in serv_addr;

	// creates a timestamp
	std::string get_timestamp();

	// opens a socket
	void open_socket();

};


#endif /* __MESSENGER_H__ */
