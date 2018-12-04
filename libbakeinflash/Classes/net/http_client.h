// http_client.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// HTTP implementation of tu_file

#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#define HTTP_SERVER_PORT 80
#define HTTP_TIMEOUT 60	// sec

#include "base/container.h"
#include "net/net_interface_tcp.h"

// HTTP client interface.
struct http_connection : public net_interface_tcp
{
	http_connection();
	~http_connection();
	bool is_open();
	bool request(const tu_string& url, const tu_string& method, const tu_string& data, string_hash<tu_string>* headers);
	int read(char* data, int bytes);
	int read_line(tu_string* data, int maxbytes);
	int write(const char* data, int bytes);
	int write_string(const char* str);
	void close();

	net_interface* m_iface;
	net_socket* m_ns;
	tu_string m_status_msg;
	int m_status;

};

#endif

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
