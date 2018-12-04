// http_client.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// HTTP implementation of tu_file

#include "base/utility.h"
#include "base/tu_file.h"
#include "net/http_client.h"
#include "net/net_interface_tcp.h"

//#define TRACE_HTTP

#ifdef _WIN32
	#define snprintf _snprintf
	#define stricmp _stricmp
#else
	#define stricmp strcasecmp
#endif        

http_connection::http_connection() :
	m_iface(NULL),
	m_ns(NULL),
	m_status(0)
{
}

http_connection::~http_connection()
{
	close();
}

void http_connection::close()
{
	delete m_ns;
	m_ns = NULL;
	delete m_iface;
	m_iface = NULL;
}

int http_connection::read(char* data, int bytes)
{
	return m_ns->read(data, bytes, 0);
}

int http_connection::read_line(tu_string* data, int maxbytes)
{
	return m_ns->read_line(data, maxbytes, HTTP_TIMEOUT);
}

int http_connection::write(const char* data, int bytes)
{
#ifdef TRACE_HTTP
	printf("HTTP write:\n%s\n", data);
#endif
	return m_ns->write(data, bytes, HTTP_TIMEOUT);
}

int http_connection::write_string(const char* str)
{
	return m_ns->write_string(str, HTTP_TIMEOUT);
}

bool http_connection::is_open()
{
	return m_ns != NULL && m_iface != NULL;
}

bool http_connection::request(const tu_string& url, const tu_string& method, const tu_string& data, string_hash<tu_string>* headers)
{
	// skip 'http://'
	if (strncasecmp(url, "http://", 7) != 0)
	{
		return false;
	}

	// check proxy addr
	// http://IP-address-of-proxy:port/http://IP-address-of-site :port/folder-chain/page.htm
	//todo

	int proxy_port = get_proxy_port();
	tu_string proxy = get_proxy_addr();

	// get host name from url, find the first '/' or ':'
	int i = 7;
	int n = url.size();
	const char* p1 = url.c_str() + 7;
	const char* p2 = p1;
	for (; url[i] != '/' && url[i] != ':' && i < n; i++) p2++;
	if (i == n)
	{
		// '/' is not found
		printf("invalid url '%s'\n", url.c_str());
		return NULL;
	}
	tu_string host = tu_string(p1, (int) (p2 - p1));

	// get port if any
	int port = 80;	// default
	if (url[i] == ':')
	{
		p1 = url.c_str() + i + 1;
		p2 = p1;
		for (; url[i] != '/' && i < n; i++) p2++;
		tu_string s(p1, (int) (p2 - p1 - 1));
		port = atoi(s.c_str());
	}

	tu_string uri = url.c_str() + i;

	if (net_init() == false)
	{
		return false;
	}

	m_iface = new net_interface_tcp();

	// Open a socket to receive connections on.
	m_ns = m_iface->connect(proxy.size() > 0 ? proxy.c_str() : host.c_str(), proxy.size() > 0 ? proxy_port : port);
	if (m_ns == NULL)
	{
		printf("Couldn't open net interface\n");
		delete m_iface;
		m_iface = NULL;
		return false;
	}

	// request
	tu_string r;

	// connect to HTTP server
	m_status = 0;
	if (proxy.size() > 0)
	{
		// FIXME 
		assert(0);	
/*
		char buf[512];
		snprintf(buf, 512, "CONNECT %s:%d HTTP/1.0\r\n", host.c_str(), port);
		r += buf;
		r += "Connection:Keep-Alive\r\n";
		r += "\r\n";
		write(r.c_str(), r.size());
		read_response();
		if (m_status != 200)
		{
			printf("Couldn't connect to '%s' through proxy '%s'\n", host.c_str(), proxy.c_str());
			delete m_ns;
			m_ns = NULL;
			return false;
		}
		r.clear();
		*/
	}

	// We use HTTP/1.0 because we do not wont get chunked encoding data
	r += method + tu_string(" ") + uri + tu_string(" HTTP/1.0\r\n");
	r += tu_string("Host:") + host + tu_string("\r\n");
	r += tu_string("User-Agent:bakeinflash\r\n");
	//	m_ns->write_string("Accept:*\r\n", HTTP_TIMEOUT);
	//	m_ns->write_string("Accept-Language: en\r\n", HTTP_TIMEOUT);
	//	m_ns->write_string("Accept-Encoding: gzip, deflate, chunked\r\n", HTTP_TIMEOUT);
	//	m_ns->write_string("Accept-Encoding: *\r\n", HTTP_TIMEOUT);
	//	m_ns->write_string("Proxy-Authenticate:prg\r\n", HTTP_TIMEOUT);
	//	m_ns->write_string("Proxy-Authorization:123\r\n", HTTP_TIMEOUT);
	if (headers)
	{
		// Takes all members of the object and sets it
		for (string_hash<tu_string>::const_iterator it = headers->begin(); it != headers->end(); ++it)
		{
			r += it->first;
			r += ':';
			r += it->second;
			r += "\r\n";
		}
	}

//	r += method == "POST" ? "Connection:Keep-Alive\r\n" : "Connection:Close\r\n";		// needs to test ??
	r += "Connection:Close\r\n";
	if (method == "POST" && data.size() > 0)
	{
		r += "Content-Type: text/xml;charset=UTF-8\r\n";
		char buf[256];
		snprintf(buf, 256, "Content-Length: %d\r\n", data.size());
		r += buf;
	}
	r += "\r\n";

	if (method == "POST" && data.size() > 0)
	{
		r += data;
	}
	write(r.c_str(), r.size());
	return true;
}

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
