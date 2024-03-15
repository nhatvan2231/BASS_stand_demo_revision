/*  TCP Client Code  */
#include "messenger.h"

// Default constructor
//Messenger::Messenger()
//{
//	sock = 0;
//
//	strcpy(server_address, DEFAULT_SERVER_ADDRESS);
//	server_port = DEFAULT_SERVER_PORT;
//}
//
//// Constructor with IP address and port number specified
//Messenger::Messenger(const char* address, int port)
//{
//	sock = 0;
//
//	strcpy(server_address, address);
//	server_port = port;
//}
//
//// Destructor
//Messenger::~Messenger()
//{
//	// close if already open
//	if (sock) {
//		shutdown(sock, 2);
//	}
//}
Messenger::Messenger()
{
	strcpy(fifo_file, DEFAULT_FIFO);
}

// Constructor with IP address and port number specified
Messenger::Messenger(const char* filename, int)
{
	strcpy(fifo_file, filename);
}

// Sends a timestamp message, returns timestamp
std::string Messenger::send_time()
{
	std::string timestamp = get_timestamp();

	//open_socket();

	// send to server
	char message[1024];
	sprintf(message, "T,%s", timestamp.c_str());
	//printf("send_detect() : message = '%s'\n", message);

	size_t chars_sent = send( sock, message, strlen(message), 0 );
	if (chars_sent != strlen(message)) {
		//printf("send_detect() : Tried to send %ld chars, actually sent %ld\n", strlen(message), chars_sent);
	} else {
		//printf("send_detect() : successfully sent %ld chars\n", chars_sent);
	}
    return timestamp;
}

// Sends a detect message, returns timestamp
std::string Messenger::send_detect()
{
	std::string timestamp = get_timestamp();

	//open_socket();

	// send to server
	//char message[1024];
	//sprintf(message, "%f%f", -42069, -42069);
	//sprintf(message, "%s", timestamp.c_str());
	//printf("send_detect() : message = '%s'\n", message);

	//size_t chars_sent = send( sock, message, strlen(message), 0 );
	//size_t chars_sent = ( sock, message, strlen(message), 0 );
	simplePipe<double, 2> tmp(fifo_file, O_WRONLY);
	double message[2] = {-42069, -42069};
	size_t chars_sent = tmp.pipeOut(message);
	tmp.pipeOut((double (&)[2])message);
//	if (chars_sent != strlen(message)) {
//		//printf("send_detect() : Tried to send %ld chars, actually sent %ld\n", strlen(message), chars_sent);
//	} else {
//		//printf("send_detect() : successfully sent %ld chars\n", chars_sent);
//	}
    return timestamp;
}

// Sends an angle message, takes azimuth, elevation, and detect timestamp as arguments
void Messenger::send_angle(double azimuth, double elevation, std::string detect_ts)
{
	std::string current_ts = get_timestamp();

	//open_socket();

	// send to server
	//char message[1024];
	//sprintf(message, "A,%s,%s,%.2f,%.2f", current_ts.c_str(), detect_ts.c_str(), azimuth, elevation);
	//sprintf(message, "%f%f", azimuth, elevation);
	//printf("send_angle() : message = '%s'\n", message);
	simplePipe<double, 2> tmp(fifo_file, O_WRONLY);
	double message[2] = {azimuth, elevation};
	size_t chars_sent = tmp.pipeOut(message);
	tmp.pipeOut((double (&)[2])message);

	//size_t chars_sent = send( sock, message, strlen(message), 0 );
//	if (chars_sent != strlen(message)) {
//		//printf("send_angle() : Tried to send %ld chars, actually sent %ld\n", strlen(message), chars_sent);
//	} else {
//		//printf("send_angle() : successfully sent %ld chars\n", chars_sent);
//	}
}

// private function, returns a timestamp
std::string Messenger::get_timestamp()
{
	char ts_char[84];

	timeval current_time;
	gettimeofday(&current_time, NULL);
	int millis = current_time.tv_usec / 1000;

	char tmp_ts[80];
	strftime(tmp_ts, 80, "%H%M%S", localtime(&current_time.tv_sec));
	sprintf(ts_char, "%s%03d", tmp_ts, millis);

	return std::string(ts_char);
}

// private function, opens a socket
//void Messenger::open_socket()
//{
//	// close if already open
//	if (sock) {
//		shutdown(sock, 2);
//	}
//
//	// create the socket
//	if ( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
//		printf("Socket creation error\n");
//		exit(1);
//	}
//
//	serv_addr.sin_family = AF_INET;
//	serv_addr.sin_port = htons(server_port);
//
//	// convert ip address from text to binary
//	if ( inet_pton(AF_INET, server_address, &serv_addr.sin_addr) <= 0 ) {
//		printf("invalid address\n");
//		exit(1);
//	}
//
//	// connect to the server
//	if ( connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0 ) {
//		printf("Connection failed\n");
//		exit(1);
//	}
//}
