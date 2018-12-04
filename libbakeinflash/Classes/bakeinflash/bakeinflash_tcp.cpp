// http_client.cpp	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// HTTP implementation of tu_file

#include "base/utility.h"
#include "base/tu_file.h"
#include "base/tu_timer.h"
#include "bakeinflash/bakeinflash_tcp.h"
#include "bakeinflash/bakeinflash_log.h"
#include "bakeinflash/bakeinflash_value.h"
#include "bakeinflash/bakeinflash_as_classes/as_array.h"

#ifdef _WIN32
	#define snprintf _snprintf
	#define stricmp _stricmp
#else
	#define stricmp strcasecmp
#endif        

#define SSL_PORT 443

namespace bakeinflash
{
	unsigned long	unzip(Uint8* buffer, int buffer_size, const Uint8* zimage, int zlen);

#if TU_CONFIG_LINK_TO_SSL == 1
	SSL_CTX* get_ssl_context();
#endif

	tcp::tcp() :
		m_status(0),
		m_socket(INVALID_SOCKET),
		m_addrinfo_list(NULL),
		m_addrinfo_current(NULL),
		m_ssl_connection(NULL),
		m_port(-1),
		m_accept_time(0)
	{
#ifdef iOS
    signal(SIGPIPE, SIG_IGN);
#endif
    
		m_readbuf_size = 1024 * 100;
		m_readbuf = (char*) malloc(m_readbuf_size + 1024);		// hack +1024
		m_membuf = new membuf();
	}

	// for server
	tcp::tcp(SOCKET client) :
		m_status(0),
		m_socket(client),
		m_addrinfo_list(NULL),
		m_addrinfo_current(NULL),
		m_ssl_connection(NULL),
		m_port(-1),
		m_accept_time(0)
	{
#ifdef iOS
    signal(SIGPIPE, SIG_IGN);
#endif
    
		m_readbuf_size = 1024 * 100;
		m_readbuf = (char*) malloc(m_readbuf_size + 1024);		// hack +1024
		m_membuf = new membuf();
	}

	tcp::~tcp()
	{
		close();

		free(m_readbuf);
		delete m_membuf;
	}

	void tcp::close()
	{
		if (m_ssl_connection)
		{
			// SSL_shutdown/SSL_free before close()
#if TU_CONFIG_LINK_TO_SSL == 1
			SSL_shutdown(m_ssl_connection);
			SSL_free(m_ssl_connection);
#endif
			m_ssl_connection = NULL;
		}

		if (m_addrinfo_list)
		{
			freeaddrinfo(m_addrinfo_list);
			m_addrinfo_list = NULL;
		}

		if (m_socket != INVALID_SOCKET)
		{
			m_status = 0;
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
		}

	}

	bool tcp::is_alive() const
	{
		return m_socket != INVALID_SOCKET;
	}

	bool tcp::is_readable() const
	// Return true if this socket has incoming data available.
	{
		if (m_socket == INVALID_SOCKET)
		{
			return false;
		}

		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(m_socket, &fds);
		struct timeval tv = { 0, 0 };
		
	#ifdef WIN32
		// the first arg to select in win32 is ignored.
		// It's included only for compatibility with Berkeley sockets.
		select(1, &fds, NULL, NULL, &tv);
	#else
		// It should be the value of the highest numbered FD within any of the passed fd_sets,
		// plus one... Because, the max FD value + 1 == the number of FDs
		// that select() must concern itself with, from within the passed fd_sets...)
		select(m_socket + 1, &fds, NULL, NULL, &tv);
	#endif
		
		return FD_ISSET(m_socket, &fds) != 0;
	}

	bool tcp::is_writeable() const
	{
		if (m_socket == INVALID_SOCKET)
		{
			return false;
		}

		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(m_socket, &fds);
		struct timeval tv = { 0, 0 };
		
	#ifdef WIN32
		// the first arg to select in win32 is ignored.
		// It's included only for compatibility with Berkeley sockets.
		select(1, NULL, &fds, NULL, &tv);
	#else
		// It should be the value of the highest numbered FD within any of the passed fd_sets,
		// plus one... Because, the max FD value + 1 == the number of FDs
		// that select() must concern itself with, from within the passed fd_sets...)
		select(m_socket + 1, NULL, &fds, NULL, &tv);
	#endif
		
		bool rc = FD_ISSET(m_socket, &fds) != 0;
   // printf("socket is_writeable %d\n", rc);
    return rc;
	}

	int tcp::read(char* data, int bytes)
	{
		if (m_socket == INVALID_SOCKET)
		{
			return -1;
		}

		if (is_readable() == false)
		{
			return 0;
		}
		
		int bytes_read = 0;
		if (m_port == SSL_PORT)
		{
#if TU_CONFIG_LINK_TO_SSL == 1
			bytes_read = SSL_read(m_ssl_connection, (char*) data, bytes);
			int rc = SSL_get_error(m_ssl_connection, bytes_read);
			if (rc == SSL_ERROR_ZERO_RETURN)
			{
			}
			else
			if (rc != SSL_ERROR_NONE)
			{
				int err = WSAGetLastError();
				if (err == WSAEWOULDBLOCK || err == WSAENOTCONN)
				{
					// No data ready.
					return 0;
				}
				myprintf("SSL_read failed %d\n", err);
			}
#else
			myprintf("HTTPS requires SSL library\n");
#endif
		}
		else
		{
			bytes_read = recv(m_socket, (char*) data, bytes, 0);
		}


		if (bytes_read > 0)
		{
			as_value val;
			as_object* global = get_global();
			if (global) global->get_member("traceTCP", &val);
			if (val.to_bool())
			{
				data[bytes_read] = 0; myprintf("\n*** TCP READ BEGIN ***\n%s\n*** TCP READ END ***\n", data);
			}
			return bytes_read;
		}
		else
		if (bytes_read == 0)
		{
			// Socket must close
			return -1;
		}
		else
		{
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK || err == WSAENOTCONN)
			{
				// No data ready.
				return 0;
			}

			// Presumably something bad.
			myprintf("net_socket_tcp::read() error in recv, error code = %d\n", err);
			return -1;
		}
	}

	int tcp::write(const char* data, int bytes)
	{
		// first try to read 
		read_data();

		if (get_socket_state() != 0)
		{
			close();
      return -1;
		}
		
		int bytes_sent = 0;
		if (m_port == SSL_PORT)
		{
#if TU_CONFIG_LINK_TO_SSL == 1
			bytes_sent = SSL_write(m_ssl_connection, data, bytes);
			int rc = SSL_get_error(m_ssl_connection, bytes_sent);
			if (rc != SSL_ERROR_NONE)
			{
				int err = WSAGetLastError();
				myprintf("SSL_write failed %d\n", err);
			}
#else
			myprintf("HTTPS requires SSL library\n");
#endif
		}
		else
		{
			bytes_sent = send(m_socket, data, bytes, 0);
		}

		as_value val;
		as_object* global = get_global();
		if (global) global->get_member("traceTCP", &val);
		if (val.to_bool())
		{
			myprintf("\n*** TCP WRITE BEGIN ***\n");
			while (bytes > 0)
			{
				char buf[256];
				int n = bytes > 200 ? 200 : bytes;
				memcpy(buf, data, n);
				buf[n] = 0;
				myprintf("%s\n", buf);
				data += n;
				bytes -= n;
			}
			myprintf("\n*** TCP WRITE END ***\n");
		}
		return bytes_sent;
	}

	bool tcp::connect(const tu_string& host, int port)
	{
		m_host = host;
		m_port = port;
		return connect();
	}

	bool tcp::connect(const tu_string& urlx)
	{
		tu_string url;
		for (Uint32 i = 0, n = urlx.size(); i < n; )
		{
			// convert \/ ==> /
			if (i < n - 1 && urlx[i] == '\\' && urlx[i + 1] == '/')
			{
				url += '/';
				i += 2;
			}
			else
			if (i < n - 8 && strncasecmp(&urlx[i], "\\u00253A", 8) == 0)
			{
				url += ':';
				i += 8;
			}
			else
			if (i < n - 8 && strncasecmp(&urlx[i], "\\u00252F", 8) == 0)
			{
				url += '/';
				i += 8;
			}
			else
			{
				url += urlx[i];
				i++;
			}
		}

		// skip 'http://'
		Uint32 i = 0;
		if (strncasecmp(url, "http://", 7) == 0)
		{
			i = 7;
			m_port = 80;	// default
		}
		else
		if (strncasecmp(url, "https://", 8) == 0)
		{
			i = 8;
			m_port = SSL_PORT;	// default
		}

		if (i == 0)
		{
			//myprintf("invalid URI: %s\n", url.c_str());
			return false;
		}

		// check proxy addr
		// http://IP-address-of-proxy:port/http://IP-address-of-site :port/folder-chain/page.htm
		//todo
//		int proxy_port = get_proxy_port();
//		tu_string proxy = get_proxy_addr();

		// get host name from url, find the first '/' or ':'
		Uint32 n = url.size();
		const char* p1 = url.c_str() + i;
		const char* p2 = p1;
		for (; url[i] != '/' && url[i] != ':' && i < n; i++) p2++;
		if (i == n)
		{
			// '/' is not found
			myprintf("strange url '%s'\n", url.c_str());
		}

		tu_string old_host = m_host;
		m_host = tu_string(p1, (int) (p2 - p1));

		// get port if any
		if (url[i] == ':')
		{
			p1 = url.c_str() + i + 1;
			p2 = p1;
			for (; url[i] != '/' && i < n; i++) p2++;
			tu_string s(p1, (int) (p2 - p1 - 1));
			m_port = atoi(s.c_str());
		}

		m_uri = url.c_str() + i;

		// reconnect ?
		if (old_host == m_host && m_host.size() > 0)
		{
			return true;
		}

		connect();
		return true;
	}

	bool tcp::connect()
	{
		// net_init()
#ifdef _WIN32
		WORD version_requested = MAKEWORD(1, 1);
		WSADATA wsa;
		WSAStartup(version_requested, &wsa);
		if (wsa.wVersion != version_requested)
		{	
			myprintf("Bad Winsock version %d\n", wsa.wVersion);
			return false;
		}
#endif

		// Open a socket to receive connections on.

		if (m_host.size() == 0)
		{
			myprintf("NULL url\n");
			return false;
		}
		if (!(m_port > 0))
		{
			myprintf("invalid tcp port %d\n", m_port);
			return false;
		}
    
		// get server address
    // IP4 ==>IP6 addr emulator
		char ipv4_str_buf[INET_ADDRSTRLEN] = { 0 };
//		const char *ipv4_str = NULL;
		int a[4] = {-1, -1, -1, -1};
		Uint8 ipv4[4] = {};
		sscanf(m_host, "%d.%d.%d.%d", a, a+1, a+2, a+3);
		if (a[0] >= 0 && a[1] >= 0 && a[2] >= 0 && a[3] >= 0)	// absolue ip4 address ?
		{
			ipv4[0] = a[0];
			ipv4[1] = a[1];
			ipv4[2] = a[2];
			ipv4[3] = a[3];

			// convert ip4 to presentation format.
      // IP4 ==>IP6 addr emulator
			m_host = inet_ntop(AF_INET, &ipv4, ipv4_str_buf, sizeof(ipv4_str_buf));
		}

		struct addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = PF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
#ifdef iOS
    hints.ai_flags = AI_DEFAULT;
#else
    hints.ai_flags = AI_PASSIVE;
#endif
    
		tu_string port(m_port);
    int rc = getaddrinfo(m_host, port.c_str(), &hints, &m_addrinfo_list);
    if (rc != 0)
		{
			//NOTREACHED
			myprintf("getaddrinfo failed: %s\n", gai_strerror(rc));
			return false;
		}

		m_socket = INVALID_SOCKET;
		m_addrinfo_current = m_addrinfo_list;
		connect_curent();

		return is_writeable();
	}

	void tcp::connect_curent()
	{
		for (; m_addrinfo_current; m_addrinfo_current = m_addrinfo_current->ai_next)
		{
      // iOS 9 bug fixing
      if (m_addrinfo_current->ai_family == AF_INET6)
      {
        sockaddr_in6* sockaddr_v6 = (struct sockaddr_in6*) m_addrinfo_current->ai_addr;
        if (sockaddr_v6 && sockaddr_v6->sin6_port == 0)
        {
          sockaddr_v6->sin6_port = htons(m_port);
        }
      }
            
			m_socket = socket(m_addrinfo_current->ai_family, m_addrinfo_current->ai_socktype, m_addrinfo_current->ai_protocol);
			if (m_socket >= 0)
			{
#ifdef iOS
        int val = 1;
        setsockopt(m_socket, SOL_SOCKET, SO_NOSIGPIPE, &val, sizeof(val));
#endif
				break;
			}
		}

		if (m_socket != INVALID_SOCKET)
		{
			if (m_port == SSL_PORT)
			{
				// create an SSL connection and attach it to the socket
	#if TU_CONFIG_LINK_TO_SSL == 1
				m_ssl_connection = SSL_new(get_ssl_context());
				int rc = SSL_set_fd(m_ssl_connection, m_socket);
				if (rc != 1)
				{
					myprintf("SSL_set_fd failed\n");
				}
	#else
				myprintf("HTTPS requires SSL library\n");
	#endif
			}
		
			set_nonblock();
			::connect(m_socket, m_addrinfo_current->ai_addr, m_addrinfo_current->ai_addrlen);
		}
	}


	void tcp::set_nonblock()
	{
		// Set non-blocking mode for the socket, so that
		// accept() doesn't block if there's no pending
		// connection.
#ifdef WIN32
		int mode = 1;
		ioctlsocket(m_socket, FIONBIO, (u_long FAR*) &mode);
#else
		int mode = fcntl(m_socket, F_GETFL, 0);
		mode |= O_NONBLOCK;
		fcntl(m_socket, F_SETFL, mode);
#endif
	}

	int tcp::get_socket_state() const
	// check socket state
	{
		int error = 0;
		socklen_t len = sizeof (error);
		int rc = getsockopt(m_socket, SOL_SOCKET, SO_ERROR, (char*) (&error), &len);
		return error;
	}

	bool tcp::is_connected()
	{
		int err = get_socket_state();
		if (err != 0)
		{
			printf("socket err: %d\n", err);

			if (m_ssl_connection)
			{
				// SSL_shutdown/SSL_free before close()
	#if TU_CONFIG_LINK_TO_SSL == 1
				SSL_shutdown(m_ssl_connection);
				SSL_free(m_ssl_connection);
	#endif
				m_ssl_connection = NULL;
			}

			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
			m_addrinfo_current = m_addrinfo_current->ai_next;
			connect_curent();
		}

		bool connected = is_writeable();

		if (m_port == SSL_PORT && connected)
		{
#if TU_CONFIG_LINK_TO_SSL == 1

			// connected ?
			int retcode = SSL_get_state(m_ssl_connection);
			//			printf("%X\n", retcode);
			if (retcode == SSL_ST_OK)
			{
				connected = true;
			}
			else
			{
				// Initiate SSL handshake
				retcode = SSL_connect(m_ssl_connection);
				int rc = SSL_get_error(m_ssl_connection, retcode);
				switch (rc)
				{
				case 	SSL_ERROR_WANT_READ:
				case SSL_ERROR_WANT_WRITE:
					connected = false;
					break;

				case SSL_ERROR_NONE:
					// connected
					break;

				default:
					int err = WSAGetLastError();
					myprintf("SSL_connect failed retcode=%d\n", retcode);
					connected = false;
				}
			}
#else
			myprintf("HTTPS requires SSL library\n");
			return false;
#endif
		}
		return connected;
	}

	bool tcp::write_http(const tu_string& data)
	{
		m_membuf->resize(0);

		// request
		tu_string r;

		// We use HTTP/1.0 because we do not wont get chunked encoding data
		r += m_method.size() > 0 ? m_method.utf8_to_upper() : tu_string(data.size() > 0 ? "POST" : "GET");
		r += tu_string(" ") + m_uri + tu_string(" HTTP/1.1\r\n");
		r += tu_string("Host: ") + m_host + tu_string("\r\n");
		r += "Cache-Control: no-cache\r\n";
		r += "User-Agent: Shockwave Flash\r\n";
//		Cookie: csrftoken=GL9bzUXcceoPZasie3GTy5vIKF8uNOmE; mid=WHZAfQAEAAE2SoLWap3AcUsTVlC9; s_network=""


		for (string_hash<as_value>::const_iterator it = m_headers.begin(); it != m_headers.end(); ++it)
		{
			if (it->second.to_tu_string().size() > 0)
			{
				r += it->first;
				r += ": ";
				r += it->second.to_tu_string();
				//	if (m_trace) myprintf("write header ==> '%s':'%s'\n", it->first.c_str(), it->second.to_string());
				r += "\r\n";
			}
		}


	//	r += data.size() > 0 ? "Connection: keep-alive\r\n" : "Connection: close\r\n";

		if (data.size() > 0)
		{
		//	r += "Content-Type: text/xml\r\n";
			char buf[256];
			snprintf(buf, 256, "Content-Length: %d\r\n", data.size());
			r += buf;
			r += "\r\n";
			r += data;
		}
		else
		{
			r += "\r\n";
		}
		return write(r.c_str(), r.size()) > 0 ? true : false;
	}

	bool tcp::write_http(const membuf& data)
	{
		m_membuf->resize(0);

		// request
		membuf r;

		// We use HTTP/1.0 because we do not wont get chunked encoding data
		r.append(m_method.size() > 0 ? m_method.utf8_to_upper() : tu_string(data.size() > 0 ? "POST" : "GET"));
		r.append(tu_string(" ") + m_uri + tu_string(" HTTP/1.1\r\n"));
		r.append(tu_string("Host: ") + m_host + tu_string("\r\n"));

		// write headers
		for (string_hash<as_value>::const_iterator it = m_headers.begin(); it != m_headers.end(); ++it)
		{
			if (it->second.to_tu_string().size() > 0)
			{
				r.append(it->first);
				r.append(": ");
				r.append(it->second.to_tu_string());
				//	if (m_trace) myprintf("write header ==> '%s':'%s'\n", it->first.c_str(), it->second.to_string());
				r.append("\r\n");
			}
		}


	//	r += data.size() > 0 ? "Connection: keep-alive\r\n" : "Connection: close\r\n";

		if (data.size() > 0)
		{
		//	r += "Content-Type: text/xml\r\n";
			char buf[256];
			snprintf(buf, 256, "Content-Length: %d\r\n", data.size());
			r.append(buf);
			r.append("\r\n");
			r.append(data);
		}
		else
		{
			r.append("\r\n");
		}
		return write((const char*) r.data(), r.size()) > 0 ? true : false;
	}


	int tcp::read_data()
	{
		// read data
		int n = read(m_readbuf, m_readbuf_size);
		if (n > 0)
		{
			m_membuf->append(m_readbuf, n);
		}
		else
		if (n == -1)
		{
			close();
		}
		return n;
	}

	int tcp::get_status() const
	{
		int k = 0;
		if (m_status.size() >= 3)
		{
			char* p = strdup(m_status.c_str());
			p[3] = 0;
			k = atoi(p);
			free(p);
		}
		return k;
	}


	void tcp::read_line(const char* p, const char* end, tu_string* str)
	{
		str->clear();
		for (; *p != '\r' && *(p + 1) != '\n' && p < end; p++)
		{
			*str += *p;
		}
	}

	bool tcp::read_http(const void** http_data, int* http_size, string_hash<as_value>* headers)
	{
		// read data
		int n = read(m_readbuf, m_readbuf_size);
		if (n > 0)
		{
			m_membuf->append(m_readbuf, n);
		}
		else
		{
			// try to parse
			const char* data = (const char*) m_membuf->data();
			if (data == NULL)
			{
				return false;
			}

			const char* body = NULL;
			const char* end = data + m_membuf->size();
			bool is_server = false;

			// server
			if (strncmp(data, "GET", 3) == 0)
			{
				is_server = true;
				headers->clear();

				tu_string s;
				array<tu_string> a;
				read_line(data, end, &s);	// HTTP/1.1 200 OK
				s.split(' ', &a);
				m_method = a[0];
				m_resource = a[1];
				data += m_method.size() + 1 + m_resource.size() + 1;
			}

			if (strncmp(data, "HTTP/", 5) == 0)
			{
				// уже почищено
				if (is_server == false)
				{
					headers->clear();
				}

				tu_string s;
				array<tu_string> a;
				read_line(data, end, &s);	// HTTP/1.1 200 OK
				s.split(' ', &a);
				m_status = "900";	// bad
				if (a.size() > 1)
				{
					m_status = a[1];	// 200
				}

				const char* p = data + s.size() + 2;
				while (p < end)
				{
					read_line(p, end, &s);
					if (s.size() == 0)
					{
						break;
					}
					
					//s.split(':', &a);
					tu_string first;
					const char* second = NULL;
					for (Uint32 i = 0, n = s.size(); i < n; i++)
					{
						if (s[i] == ':')
						{
							first = s.utf8_substring(0, i);
							second = &s[i + 1];
							break;
						}
					}

					// tp uppercase
					if (strncasecmp(first.c_str(), "transfer-encoding", 17) == 0)
					{
						first = "Transfer-Encoding";
					}

					// есть пара
					if (first.size() > 0 && second)
					{
						const char* newval = *second == ' ' ? second + 1 : second;
						as_value val;
						if (headers->get(first, &val))
						{
							// same input headers/cookies
							as_array* a = cast_to<as_array>(val.to_object());
							if (a)
							{
								// add new item in array
								a->push(newval);
							}
							else
							{
								// convert to array
								a = new as_array();
								a->push(val);
								a->push(newval);
								headers->set(first, a);
							}
						}
						else
						{
							headers->set(first, newval);
						}
						//if (m_trace) myprintf("read header ==> '%s': '%s'\n", first.c_str(), newval);
					}
					p += s.size() + 2;
				}

				if (is_server)
				{
					if (m_method == "GET")
					{
						if (strncmp(p, "\r\n", 2) == 0)
						{
							return true;
						}
					}
				}

				// sanity check for end 
				if (strncmp(p, "\r\n", 2) != 0)
				{
					// not enough data
					return false;
				}

				body = p + 2;

				if (m_method == "HEAD")
				{
					*http_data = NULL;
					*http_size = 0;
					return true;
				}

				//Content-Encoding: gzip
				as_value val;
				if (headers->get("Content-Encoding", &val) && val.to_tu_string() == "deflate")
				{
					int	buffer_bytes = 1024 * 1024;	// 1M.. hack
					Uint8*	buffer = new Uint8[buffer_bytes];
					int unzipped_len = (int) unzip(buffer, buffer_bytes, (const Uint8*) body, end - body);
					if (unzipped_len >= 0)
					{
						m_membuf->resize(0);
						m_membuf->append(buffer, unzipped_len);
						data = (const char*) m_membuf->data();
						end = data + m_membuf->size();
						body = data;

						if (headers->get("Content-Length", NULL))
						{
							headers->set("Content-Length", unzipped_len);
						}

					}
					delete buffer;
				}
				 
				if (headers->get("Transfer-Encoding", &val) && val.to_tu_string() == "chunked")
				{
					// chunked 
					const char* p = body;
					tu_string s;
					membuf* buf = new membuf();
					while (p < end)
					{
						// chunk size in HEX
						read_line(p, end, &s);
						char* e;
						long k = strtol(s.c_str(), &e, 16);
						if (s == "0")
						{
							// end of chinked data
							delete m_membuf;
							m_membuf = buf;

							*http_data = m_membuf->data();
							*http_size = m_membuf->size();
							return true;
						}

						p += s.size() + 2;	// chunk
                        
            int maxsize = end - p;
            k = imin(k, maxsize);
						buf->append(p, k);
						p += k + 2;	// skip \r\n at end of chunk.. so next chunk
					}
					// not enough data
					delete buf;
				}
				else
				{
					if (headers->get("Content-Length", &val))
					{
						int len = val.to_int();
						int size = end - body;	// + \r\n at end
						if (len == size)
						{
							if (len == 0)
							{
								// empty body
								*http_data = "";
								*http_size = 0;
							}
							else
							{
								*http_data = body;
								*http_size = size;
							}
							return true;			
						}
						else
						{
							// not enough data
						}
					}
					else
					{
						int size = end - body;	// + \r\n at end
						if (size > 0)
						{
							*http_data = body;
							*http_size = size;
							return true;			
						}
						else
						{
							int k=1;
						}
					}
				}
			}
		}
		return false;
	}

	bool tcp::get_string(tu_string* str)
	{
		// search ZERO-ended string
		if (m_membuf->size() > 0)
		{
			const char* p = NULL;
			const char* p1 = (const char*) m_membuf->data();
			const char* p2 = (const char*) m_membuf->data() + m_membuf->size();
			for (p = p1; p < p2 && *p != 0; p++);
			if (*p == 0 && p < p2)
			{
				*str = p1;
				p++;
				int n = p2 - p;
				membuf* mb = n > 0 ? new membuf(p, n) : new membuf();
				delete m_membuf;
				m_membuf = mb;

				return true;
			}
		}
		return false;
	}


	int tcp::start_server(int port)
	{
		m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_socket == INVALID_SOCKET)
		{
			fprintf(stderr, "can't open listen socket\n");
			return -1;
		}

		// Set the address.
		SOCKADDR_IN saddr;
		saddr.sin_family = AF_INET;
		saddr.sin_addr.s_addr = INADDR_ANY;
		saddr.sin_port = htons(port);

		// bind the address
		int ret = bind(m_socket, (LPSOCKADDR) &saddr, sizeof(saddr));
		if (ret == SOCKET_ERROR)
		{
			fprintf(stderr, "bind failed\n");
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
			return -1;
		}

		// gethostname

		// Start listening.
		ret = listen(m_socket, SOMAXCONN);
		if (ret == SOCKET_ERROR)
		{
			fprintf(stderr, "listen() failed\n");
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
			return -1;
		}

		// Set non-blocking mode for the socket, so that
		// accept() doesn't block if there's no pending
		// connection.
		set_nonblock();
		return 0;
	}

	SOCKET tcp::accept(tu_string* ip)
	{
		// Accept an incoming request.
		SOCKET	remote_socket = INVALID_SOCKET;
		struct sockaddr_in client;
#if defined(ANDROID)
		socklen_t ln;
#elif defined(__GNUC__)
		Uint32 ln;
#else
		int ln;
#endif
		ln = sizeof(sockaddr_in);
		remote_socket = ::accept(m_socket, (sockaddr*) &client, &ln);
		if (remote_socket != INVALID_SOCKET && ip)
		{
			// linux compilation error
		//vv	char buf[16];
		//vv	snprintf(buf, 16, "%d.%d.%d.%d", client.sin_addr.S_un.S_un_b.s_b1, client.sin_addr.S_un.S_un_b.s_b2, client.sin_addr.S_un.S_un_b.s_b3, client.sin_addr.S_un.S_un_b.s_b4);
		//vv	*ip = buf;
		}
		m_accept_time = tu_timer::get_ticks();
		return remote_socket;
	}

}

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:


