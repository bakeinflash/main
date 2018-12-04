// BAKEINFLASH_TCP.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// HTTP implementation of tu_file

#ifndef BAKEINFLASH_TCP_H
#define BAKEINFLASH_TCP_H

#define HTTP_SERVER_PORT 80
#define HTTP_TIMEOUT 60	// sec

#ifdef _WIN32

	#include <winsock2.h>
	#include <ws2tcpip.h>

	#define socklen_t int

#else
	#include <sys/socket.h>
	#include <fcntl.h>
	#include <unistd.h>
	#include <errno.h>
	#include <arpa/inet.h>
	#include <netdb.h>

	typedef int SOCKET;
	#define closesocket ::close
	#define SOCKET_ERROR -1
	#define WSAGetLastError() errno
	#define WSAEWOULDBLOCK EAGAIN
	#define WSAENOTCONN ENOTCONN
	#define INVALID_SOCKET ENOTSOCK
	#define WSAENOBUFS EAGAIN
	#define WSAEISCONN EISCONN
//	#define WSAEINVAL EINVAL //49 
	#define SOCKADDR_IN sockaddr_in
	#define LPSOCKADDR sockaddr*
#endif

#include "bakeinflash/bakeinflash.h"
#include "base/membuf.h"

#if TU_CONFIG_LINK_TO_SSL == 1
	#include <openssl/ssl.h>
#endif

namespace bakeinflash
{

	enum net_status
	{
		UNDEFINED,
		CONNECTING,
		CONNECTED,
		READING,
		READING_MEMBUF,
		HANDLE_EVENTS,
		LOAD_INIT,
		DOWNLOADING
	};

	// HTTP client interface.
	struct tcp : public ref_counted
	{
		tcp();
		tcp(SOCKET client);
		~tcp();
		bool connect();
		bool connect(const tu_string& url);
		bool connect(const tu_string& host, int port);
		int read(char* data, int bytes);
		int read_data();
		bool read_http(const void** data, int* size, string_hash<as_value>* headers);
		bool read_http_packet(const void** http_data, int* http_size, string_hash<as_value>* headers);
		void read_line(const char* p, const char* end, tu_string* str);
		int write(const char* data, int bytes);
		bool write_http(const tu_string& data);
		bool write_http(const membuf& data);
		void close();
		bool is_connected();
		bool is_readable() const;
		bool is_writeable() const;
		bool get_string(tu_string* str);
		int get_status() const;
		void set_status(const tu_string& val) { m_status = val; }
		void set_method(const tu_string& method) { m_method = method; }
		int size() const { return m_membuf == NULL ? 0 : m_membuf->size(); }
		void set_nonblock();
		bool is_alive() const;
		int get_socket_state() const;
		void connect_curent();

		// server
		int start_server(int port);
		SOCKET accept(tu_string* ip);
		int get_accept_time() const { return m_accept_time; }
		void set_accept_time(Uint32 val) { m_accept_time = val; }
		const tu_string& get_method() const { return m_method; }
		const tu_string& get_resource() const { return m_resource; }

		tu_string m_status;

		// maybe multiple Set-Cookie header
		string_hash<as_value> m_headers;

		tu_string m_host;
		int m_port;

	private:
		SOCKET m_socket;
		addrinfo* m_addrinfo_list;
		addrinfo* m_addrinfo_current;
		tu_string m_uri;
		tu_string m_method;
		tu_string m_resource;		// for server
		membuf* m_membuf;
		char* m_readbuf;
		int m_readbuf_size;

#if TU_CONFIG_LINK_TO_SSL == 1
		SSL* m_ssl_connection;
#else
		void* m_ssl_connection;
#endif

		// for client timeout
		int m_accept_time;
	};

}
#endif

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
