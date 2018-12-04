// container.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some non-inline implementation help for generic containers.


#include "base/container.h"
#include "base/utf8.h"
#include "base/tu_random.h"
#include <stdarg.h>
#include <locale.h>

//
// tu_string
//


#if _TU_USE_STL == 1

tu_string::tu_string() :
	m_flags(0)
{
}

tu_string::tu_string(char ch) :
	std::string(1, ch),
	m_flags(0)
{
}

tu_string::tu_string(Uint16 ch) :
	m_flags(0)
{
	if (ch <= 0x7F)
	{
		// Plain single-byte ASCII.
		*this = (char) ch;
	}
	else
	{
		char m_local[19];
		Uint32 m_size;
		utf8::encode_unicode_character(m_local, &m_size, (uint32) ch);
		m_local[m_size] = 0;	// terminate.
		*this = m_local;
	}
}

tu_string::tu_string(int val) :
	m_flags(0)
{
	char m_local[19];
	int m_size = snprintf(m_local, sizeof(m_local), "%d", val);
	*this = m_local;
}

tu_string::tu_string(double val) :
	m_flags(0)
{
	char m_local[19];
	int m_size = snprintf(m_local, sizeof(m_local), "%.14g", val);
	*this = m_local;
}

tu_string::tu_string(const char* str) :
	std::string(str),
	m_flags(0)
{
}

tu_string::tu_string(const char* buf, int buflen) :
	std::string(buf, buflen),
	m_flags(0)
{
}

tu_string::tu_string(const tu_string& str) :
	std::string(str),
	m_flags(0)
{
}

Uint32  tu_string::compute_hash() const
{
	if (!get_hashed_flag())
	{
		//set_hashed_flag();
		m_flags |= 2;
		m_hash_value = bernstein_hash(c_str(), size());
	}
	return m_hash_value;
}

tu_string::operator const char*() const
{
	return c_str();
}

char&	tu_string::operator[](Uint32 index)
{
	clear_flags();
	char* buf = (char*) c_str();
	return buf[index];
}

const char&	tu_string::operator[](Uint32 index) const
{
	return c_str()[index];
}

void	tu_string::operator+=(const char* str)
{
	clear_flags();
	std::string::operator+=(str);
}

void	tu_string::operator+=(char ch)
{
	if (ch != 0)
	{
		clear_flags();
		std::string::operator+=(ch);
	}
}

void	tu_string::operator+=(Uint16 ch)
{
	if (ch != 0)
	{
		clear_flags();
		if (ch <= 0x7F)
		{
			// Plain single-byte ASCII.
			std::string::operator+=((char) ch);
		}
		else
		{
			append_wide_char(ch);
		}
	}
}

void	tu_string::operator+=(int val)
{
	operator+=((double) val);
}

void	tu_string::operator+=(float val)
{
	operator+=((double) val);
}

void	tu_string::operator+=(double val)
{
	char str[50];
	snprintf(str, 50, "%.14g", val);
	std::string::operator+=(str);
}

void	tu_string::operator+=(const tu_string& str)
{
	clear_flags();
	std::string::operator+=(str);
}

tu_string	tu_string::operator+(const tu_string& str) const
{
	tu_string	new_string(*this);
	new_string += str;
	return new_string;
}

tu_string	tu_string::operator+(const char* str) const
	// NOT EFFICIENT!  But convenient.
{
	tu_string	new_string(*this);
	new_string += str;
	return new_string;
}

bool	tu_string::operator<(const char* str) const
{
	return strcmp(c_str(), str) < 0;
}

bool tu_string::operator<(const tu_string& str) const
{
	const char* p0 = c_str();
	const char* p1 = str.c_str();
	Uint32 min_len = size();
	bool str_longer = true;
	if (str.size() < min_len)
	{
		min_len = str.size();
		str_longer = false;
	}
	int cmp = memcmp(p0, p1, min_len);
	if (cmp < 0)
	{
		return true;
	}
	else 
	if (cmp > 0)
	{
		return false;
	}
	return str_longer;
}

bool	tu_string::operator>(const char* str) const
{
	return strcmp(c_str(), str) > 0;
}

bool	tu_string::operator>(const tu_string& str) const
{
	return str < *this;
}

void tu_string::clear()
{
	clear_flags();
	resize(0, false);
}

int	tu_string::utf8_length() const
{
	return utf8_char_count(c_str(), size()); 
}

void tu_string::split(const tu_string& delimiter, array<double>* res) const
{
	const char*	item = c_str();
	const char*	end = item + size();
	const char* p = item;
	int len;

	assert(res);
	res->clear();

	for (; p < end; p++)
	{
		if (*p == delimiter)
		{
			len = int(p - item);
			res->push_back(atof(tu_string(item, len).c_str()));
			item = p + 1;
		}
	}

	// last and maybe the only item
	len = int(p - item);
	res->push_back(atof(tu_string(item, int(p - item)).c_str()));
}

void tu_string::split(const tu_string& delimiter, array<tu_string>* res) const
{
	const char*	item = c_str();
	const char*	end = item + size();
	const char* p = item;
	int len;

	assert(res);
	res->clear();

	for (; p < end; p++)
	{
		if (*p == delimiter)
		{
			len = int(p - item);
			res->push_back(tu_string(item, len));
			item = p + 1;
		}
	}

	// last and maybe the only item
	len = int(p - item);
	res->push_back(tu_string(item, len));
}
/*
void tu_string::split(const char* delimiter, array<tu_string>* res) const
{
	const char*	item = get_buffer();
	const char*	end = item + size();
	const char* p = item;
	int len;
	int delimiter_len = strlen(delimiter);

	assert(res);
	res->clear();

	for (; p <= end - delimiter_len; )
	{
		if (strncmp(p, delimiter, delimiter_len) == 0)
		{
			len = int(p - item);
			res->push_back(tu_string(item, len));
			item = p + delimiter_len;
			p += delimiter_len;
		}
		else
		{
			p++;
		}
	}

	// last and maybe the only item
	len = int(p - item);
	res->push_back(tu_string(item, len));
}
*/
void tu_string::append_wide_char(uint16 c)
{
	char buf[8];
	Uint32 index = 0;
	utf8::encode_unicode_character(buf, &index, (uint32) c);
	buf[index] = 0;

	*this += buf;
}


void tu_string::append_wide_char(uint32 c)
{
	char buf[8];
	Uint32 index = 0;
	utf8::encode_unicode_character(buf, &index, c);
	buf[index] = 0;

	*this += buf;
}


void	tu_string::resize(int new_size, bool do_copy)
{
	clear_flags();
	std::string::resize(new_size);
}



#else

tu_string::tu_string() :
	m_flags(0),
	m_size(0),
	m_buffer(NULL)
{
	*m_local = 0;
}

tu_string::tu_string(char ch) :
	m_flags(0),
	m_size(1),
	m_buffer(NULL)
{
	m_local[0] = ch;
	m_local[1] = 0;	// terminate
}

tu_string::tu_string(Uint16 ch) :
	m_flags(0),
	m_size(0),
	m_buffer(NULL)
{
	if (ch <= 0x7F)
	{
		// Plain single-byte ASCII.
		m_size = 1;
		m_local[0] = (char) ch;
		m_local[1] = 0;	// terminate
	}
	else
	{
		utf8::encode_unicode_character(m_local, &m_size, (uint32) ch);
		m_local[m_size] = 0;	// terminate.
	}
}

tu_string::tu_string(int val) :
	m_flags(0),
	m_buffer(NULL)
{
	m_size = snprintf(m_local, sizeof(m_local), "%d", val);
}

tu_string::tu_string(double val) :
	m_flags(0),
	m_buffer(NULL)
{
	m_size = snprintf(m_local, sizeof(m_local), "%.14g", val);
}

tu_string::tu_string(const char* str) :
	m_flags(0),
	m_size(0),
	m_buffer(NULL)
{
	if (str)
	{
		m_size = strlen(str);
		if (m_size < sizeof(m_local))
		{
			strcpy(m_local, str);
		}
		else
		{
			m_buffer = strdup(str);
		}
	}
}

tu_string::tu_string(const char* buf, int buflen) :
	m_flags(0),
	m_size(0),
	m_buffer(NULL)
{
	if (buf && buflen >= 0)
	{
		m_size = buflen;
		if (m_size < sizeof(m_local))
		{
			memcpy(m_local, buf, m_size);
			m_local[m_size] = 0;	// terminate.
		}
		else
		{
			m_buffer = (char*) malloc(m_size + 1);
			memcpy(m_buffer, buf, m_size);
			m_buffer[m_size] = 0;	// terminate.
		}
	}
}

tu_string::tu_string(const tu_string& str) :
	m_flags(0),
	m_size(str.size()),
	m_buffer(NULL)
{
	if (m_size < sizeof(m_local))
	{
		strcpy(m_local, str.get_buffer());
	}
	else
	{
		m_buffer = (char*) malloc(m_size + 1);
		strcpy(m_buffer, str.get_buffer());
	}
}

tu_string::~tu_string()
{
	if (m_buffer) free(m_buffer);
}

Uint32  tu_string::compute_hash() const
{
	if (!get_hashed_flag())
	{
		//set_hashed_flag();
		m_flags |= 2;
		m_hash_value = bernstein_hash(get_buffer(), m_size);
	}
	return m_hash_value;
}

tu_string::operator const char*() const
{
	return get_buffer();
}

const char*	tu_string::c_str() const
{
	return get_buffer();
}

// operator= returns void; if you want to know why, ask Charles Bloom :)
// (executive summary: a = b = c is an invitation to bad code)
void	tu_string::operator=(const char* str)
{
	if (str && 	get_buffer() != str)
	{
		clear_flags();
		resize((int) strlen(str), false);
		strcpy(get_buffer(), str);
	}
}

void	tu_string::operator=(const tu_string& str)
{
	operator=(str.c_str());
}

bool	tu_string::operator==(const tu_string& str) const
{
	if (m_size != str.size())
	{
		return false;
	}

	if (this == &str || m_size == 0)
	{
		return true;
	}

	return strcmp(get_buffer(), str.get_buffer()) == 0;
}

bool	tu_string::operator==(const char* str) const
{
//	myprintf("operator== ");
	return strcmp(get_buffer(), str) == 0;
}

bool	tu_string::operator!=(const char* str) const
{
	return strcmp(get_buffer(), str) != 0;
}

bool	tu_string::operator!=(const tu_string& str) const
{
	if (this != &str && m_size == str.size())
	{
		return strcmp(get_buffer(), str.get_buffer()) != 0;
	}
	return true;
}

// index >=0
void tu_string::erase(Uint32 index, int count)
{
	if (index >= 0 && index + count <= m_size)
	{
		bool copytolocal = m_size >= sizeof(m_local) && m_size - count < sizeof(m_local);
		clear_flags();
		char* p = get_buffer();
		memmove(p + index, p + index + count,	m_size - index - count);
		m_size -= count;
		p[m_size] = 0;	// terminate

		if (copytolocal)
		{
			strcpy(m_local, p);
			free(m_buffer);
			m_buffer = NULL;
		}
	}
}

// insert char before index
void tu_string::insert(Uint32 index, char ch)
{
	if (index >= 0 && index <= m_size)
	{
		clear_flags();
		int len = size();
		resize(len + 1, true);
		char* buf = get_buffer();

		// actual memory size of the empty string is 1 (zero at the end)
		memmove(buf + index + 1, buf + index, len + 1 - index);
		*(buf + index) = ch;
	}
}

char&	tu_string::operator[](Uint32 index)
{
	assert(index >= 0 && index <= m_size);
	clear_flags();
	return get_buffer()[index];
}

const char&	tu_string::operator[](Uint32 index) const
{
	assert(index >= 0 && index <= m_size);
	return get_buffer()[index];
}

void	tu_string::operator+=(const char* str)
{
	clear_flags();
	int	str_length = (int) strlen(str);
	int	old_length = size();
	resize(old_length + str_length, true);
	memcpy(get_buffer() + old_length, str, str_length + 1);
}

void	tu_string::operator+=(char ch)
{
	if (ch != 0)
	{
		clear_flags();
		int old_length = size();
		resize(old_length + 1, true);
		*(get_buffer() + old_length) = ch;
	}
}

void	tu_string::operator+=(Uint16 ch)
{
	if (ch != 0)
	{
		clear_flags();
		int old_length = size();
		if (ch <= 0x7F)
		{
			// Plain single-byte ASCII.
			resize(old_length + 1, true);
			*(get_buffer() + old_length) = (char) ch;
		}
		else
		{
			append_wide_char(ch);
		}
	}
}

void	tu_string::operator+=(int val)
{
	operator+=((double) val);
}

void	tu_string::operator+=(float val)
{
	operator+=((double) val);
}

void	tu_string::operator+=(double val)
{
	char str[50];
	snprintf(str, 50, "%.14g", val);

	int	str_length = (int) strlen(str);
	int	old_length = size();
	resize(old_length + str_length, true);
	memcpy(get_buffer() + old_length, str, str_length + 1);
}

void	tu_string::operator+=(const tu_string& str)
{
	clear_flags();
	int str_length = str.size();
	int old_length = size();
	int new_length = old_length + str_length;
	resize(new_length, true);
	memcpy(get_buffer() + old_length, str.c_str(), str_length);
	get_buffer()[new_length] = 0;
}

tu_string	tu_string::operator+(const tu_string& str) const
{
	tu_string	new_string(*this);
	new_string += str;
	return new_string;
}

tu_string	tu_string::operator+(const char* str) const
	// NOT EFFICIENT!  But convenient.
{
	tu_string	new_string(*this);
	new_string += str;
	return new_string;
}

bool	tu_string::operator<(const char* str) const
{
	return strcmp(c_str(), str) < 0;
}

bool tu_string::operator<(const tu_string& str) const
{
	const char* p0 = get_buffer();
	const char* p1 = str.get_buffer();
	Uint32 min_len = size();
	bool str_longer = true;
	if (str.size() < min_len)
	{
		min_len = str.size();
		str_longer = false;
	}
	int cmp = memcmp(p0, p1, min_len);
	if (cmp < 0)
	{
		return true;
	}
	else 
	if (cmp > 0)
	{
		return false;
	}
	return str_longer;
}

bool	tu_string::operator>(const char* str) const
{
	return strcmp(c_str(), str) > 0;
}

bool	tu_string::operator>(const tu_string& str) const
{
	return str < *this;
}

void tu_string::clear()
{
	clear_flags();
	resize(0, false);
}

int	tu_string::utf8_length() const
{
	return utf8_char_count(get_buffer(), size()); 
}

void tu_string::split(const tu_string& delimiter, array<double>* res) const
{
	assert(res);
	res->clear();

	int delimiter_size = delimiter.size();
	if (delimiter_size == 0)
	{
		const char* p = get_buffer();
		char s[2];
		s[1] = 0;
		for (int i = 0; i < size(); i++)
		{
			*s = *p++;
			res->push_back(atof(s));
		}
	}
	else
	{
		const char*	item = get_buffer();
		const char*	end = item + size() - delimiter_size + 1;
		const char* p = item;
		int len;
		for (; p < end; p++)
		{
			if (memcmp(p, delimiter.c_str(), delimiter_size) == 0)
			{
				len = int(p - item);
				res->push_back(atof(tu_string(item, len).c_str()));
				item = p + delimiter_size;
			}
		}

		// move p to end
		p += delimiter_size - 1;
		// last and maybe the only item
		len = int(p - item);
		res->push_back(atof(tu_string(item, len).c_str()));
	}
}

void tu_string::split(const tu_string& delimiter, array<tu_string>* res) const
{
	assert(res);
	res->clear();

	int delimiter_size = delimiter.size();
	if (delimiter_size == 0)
	{
		const char* p = get_buffer();
		for (int i = 0; i < size(); i++)
		{
			res->push_back(tu_string(*p++));
		}
	}
	else
	{
		const char*	item = get_buffer();
		const char*	end = item + size() - delimiter_size + 1;
		const char* p = item;
		int len;
		for (; p < end; p++)
		{
			if (memcmp(p, delimiter.c_str(), delimiter_size) == 0)
			{
				len = int(p - item);
				res->push_back(tu_string(item, len));
				item = p + delimiter_size;
			}
		}

		// move p to end
		p += delimiter_size - 1;
		// last and maybe the only item
		len = int(p - item);
		res->push_back(tu_string(item, len));
	}
}

void tu_string::append_wide_char(uint16 c)
{
	char buf[8];
	Uint32 index = 0;
	utf8::encode_unicode_character(buf, &index, (uint32) c);
	buf[index] = 0;

	*this += buf;
}


void tu_string::append_wide_char(uint32 c)
{
	char buf[8];
	Uint32 index = 0;
	utf8::encode_unicode_character(buf, &index, c);
	buf[index] = 0;

	*this += buf;
}


void	tu_string::resize(int new_size, bool do_copy)
{
	if (new_size >= 0 && new_size != m_size)
	{
		if (m_size < sizeof(m_local))
		{
			if (new_size < sizeof(m_local))
			{
				// Stay with internal storage.
				m_local[new_size] = 0;	// terminate
			}
			else
			{
				// need to allocate heap buffer.
				m_buffer = (char*) malloc(new_size + 1);

				// Copy existing data.
				if (do_copy && m_size > 0)
				{
					memcpy(m_buffer, m_local, m_size + 1);		// +1 means last ZERO byte
				}
				m_buffer[new_size] = 0;	// terminate
			}
		}
		else
		{
			// Currently using heap storage.
			if (new_size < sizeof(m_local))
			{
				// Switch to local storage.

				// Copy existing string info.
				if (do_copy && m_size > 0)
				{
					memcpy(m_local, m_buffer, new_size);
				}
				m_local[new_size] = 0;	// terminate
				free(m_buffer);
				m_buffer = NULL;
			}
			else
			{
				m_buffer = (char*) realloc(m_buffer, new_size + 1);
				m_buffer[new_size] = 0;	// terminate
			}
		}
		m_size = new_size;
		clear_flags();
	}
}

#endif

//
//
//

template<class char_type>
/*static*/ void	encode_utf8_from_wchar_generic(tu_string* result, const char_type* wstr)
{
	const char_type*	in = wstr;

	// First pass: compute the necessary string length.
	Uint32	bytes_needed = 0;
	char	dummy[10];
	Uint32	offset;
	for (;;)
	{
		Uint32	uc = *in++;
		offset = 0;
		utf8::encode_unicode_character(dummy, &offset, uc);
		bytes_needed += offset;

		assert(offset <= 6);

		if (uc == 0)
		{
			break;
		}
	}

	// Second pass: transfer the data.
	result->resize(bytes_needed - 1, true);	// resize() adds 1 for the \0 terminator
	in = wstr;

	// UNSAFE !!!
	char*	out = (char*) result->c_str();

	offset = 0;
	for (;;)
	{
		assert(offset < bytes_needed);

		Uint32	uc = *in++;
		utf8::encode_unicode_character(out, &offset, uc);

		assert(offset <= bytes_needed);

		if (uc == 0)
		{
			break;
		}
	}

	assert(offset == bytes_needed);
	assert((*result)[offset - 1] == 0);
	assert(result->size() == (int) strlen(result->c_str()));
}


void tu_string::encode_utf8_from_uint32(tu_string* result, const uint32* wstr)
{
	encode_utf8_from_wchar_generic<uint32>(result, wstr);
}


// TODO: this currently treats each uint16 as a single character --
// need to change it to handle utf16!!!
void tu_string::encode_utf8_from_uint16(tu_string* result, const uint16* wstr)
{
	encode_utf8_from_wchar_generic<uint16>(result, wstr);
}

void tu_string::encode_utf8_from_wchar(tu_string* result, const wchar_t* wstr)
{
	if (sizeof(wchar_t) == sizeof(uint32)) {
		encode_utf8_from_uint32(result, (const uint32*) wstr);
	} else if (sizeof(wchar_t) == sizeof(uint16)) {
		encode_utf8_from_uint16(result, (const uint16*) wstr);
	} else {
		assert(0);  // Unexpected wchar_t size!
	}
}

/*static*/ int	tu_string::stricmp(const char* a, const char* b)
{
#ifdef _WIN32
	return _stricmp(a, b);
#else
	return strcasecmp(a, b);
#endif
}


uint32	tu_string::utf8_char_at(Uint32 index) const
{
	const char*	buf = c_str();
	uint32	c;

	do
	{
		c = utf8::decode_next_unicode_character(&buf);
		index--;

		if (c == 0)
		{
			// We've hit the end of the string; don't go further.
			assert(index == 0);
			return c;
		}
	}
	while (index >= 0);

	return c;
}


tu_string	tu_string::utf8_to_upper() const
{
	const char*	buf = c_str();
	tu_string str;
//	setlocale(LC_CTYPE, "Russian");
	for (;;)
	{
		uint32 c = utf8::decode_next_unicode_character(&buf);
          
		if (c == 0)
		{
			// We've hit the end of the string; don't go further.
			return str;
		}
//		str += (Uint16) towupper(c);
		str += (char) toupper(c);
	}
	return str;
}


tu_string	tu_string::utf8_to_lower() const
{
	const char*	buf = c_str();
	tu_string str;
//	setlocale(LC_CTYPE, "Russian");
	for (;;)
	{
		uint32 c = utf8::decode_next_unicode_character(&buf);
    
		if (c == 0)
		{
			// We've hit the end of the string; don't go further.
			return str;
		}
//		str += (Uint16) towlower(c);
		str += (char) tolower(c);
	}
	return str;
}


/*static*/ int	tu_string::utf8_char_count(const char* buf, int buflen)
{
	const char*	p = buf;
	int	length = 0;

	while (p - buf < buflen)
	{
		uint32	c = utf8::decode_next_unicode_character(&p);
		if (c == 0)
		{
			break;
		}

		length++;
	}

	return length;
}


tu_string	tu_string::utf8_substring(int start, int end) const
{
	assert(start <= end);

	if (start == end)
	{
		// Special case, always return empty string.
		return tu_string();
	}

	const char*	p = c_str();
	int	index = 0;
	const char*	start_pointer = p;
	const char*	end_pointer = p;

	for (;;)
	{
		if (index == start)
		{
			start_pointer = p;
		}

		uint32	c = utf8::decode_next_unicode_character(&p);
		index++;

		if (index == end)
		{
			end_pointer = p;
			break;
		}

		if (c == 0)
		{
			if (index < end)
			{
				assert(0);
				end_pointer = p;
			}
			break;
		}
	}

	if (end_pointer < start_pointer)
	{
		end_pointer = start_pointer;
	}

	return tu_string(start_pointer, int(end_pointer - start_pointer));
}


#ifdef _WIN32
#define vsnprintf	_vsnprintf
#endif // _WIN32

tu_string string_myprintf(const char* fmt, ...)
// Handy sprintf wrapper.
{
	static const int	BUFFER_SIZE = 500;
	char	s_buffer[BUFFER_SIZE];

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(s_buffer, BUFFER_SIZE, fmt, ap);
	va_end(ap);

	return s_buffer;
}



// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
