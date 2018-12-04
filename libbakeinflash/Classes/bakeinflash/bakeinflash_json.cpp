#include "bakeinflash/bakeinflash_as_classes/as_array.h"
#include "bakeinflash/bakeinflash_log.h"

namespace bakeinflash
{
	static const char* json_text;
	static int json_at = 0;
	static char json_ch = ' ';

	char json_next();
	void json_white();
	as_value json_value();

	void json_dbg()
	{
		dump(json_text + json_at - 1, 20);
	}

	void json_error(const char* m)
	{
//		myprintf("JSONError: %s\n%s\n", m, json_text);
		myprintf("JSON parse error\n");
	}

	tu_string& json_string()
	{
		static tu_string s;
		s.clear();
		bool outer = false;

		if (json_ch == '"')
		{
			while (json_next())
			{
				if (json_ch == '"')
				{
					json_next();
					return s;
				}
				else 
				if (json_ch == '\\')
				{
					switch (json_next())
					{
					case 'b' :
						s += '\b';
						break;
					case 'f' :
						s += '\f';
						break;
					case 'n' :
						s += '\n';
						break;
					case 'r' :
						s += '\r';
						break;
					case 't' :
						s += '\t';
						break;
					case 'u' :
					{
						uint32 u = 0;
						char zero = '0';
						for (int i = 0; i < 4; i++)
						{
							char ch = json_next();
							int t = ch - zero;
							u = u * 16 + t;
						}
						s.append_wide_char(u);
						break;
					}
					default :
						s += json_ch;
					}
				}
				else
				{
					s += json_ch;
				}
			}
		}
		json_error("Bad string");
		return s;
	}

	as_array* json_array()
	{
		as_array* a = new as_array();
		if (json_ch == '[')
		{
			json_next();
			json_white();
			if (json_ch == ']')
			{
				json_next();
				return a;
			}
			while (json_ch)
			{
				a->push(json_value());
				json_white();
				if (json_ch == ']')
				{
					json_next();
					return a;
				}
				else 
				if (json_ch != ',')
				{
					break;
				}
				json_next();
				json_white();
			}
		}
		json_error("Bad array");
		return a;
	}

	as_object* json_object()
	{
		tu_string k;
		as_object* o = new as_object();
		if (json_ch == '{')
		{
			json_next();
			json_white();
			if (json_ch == '}')
			{
				json_next();
				return o;
			}
			while (json_ch)
			{
				k = json_string();
				json_white();
				if (json_ch != ':')
				{
					break;
				}
				json_next();
				o->set_member(k, json_value());
				json_white();
				if (json_ch == '}')
				{
					json_next();
					return o;
				}
				else
				if (json_ch != ',')
				{
					break;
				}
				json_next();
				json_white();
			}
		}
		json_error("Bad object");
		return o;
	}

	double json_number()
	{
		static tu_string n;
		n.clear();

		if (json_ch == '-')
		{
			n = '-';
			json_next();
		}
		while (json_ch >= '0' && json_ch <= '9')
		{
			n +=json_ch;
			json_next();
		}
		if (json_ch == '.')
		{
			n += '.';
			while (json_next() && json_ch >= '0' && json_ch <= '9')
			{
				n += json_ch;
			}
		}

		double v;
		if (string_to_number(&v, n.c_str()))
		{
			return v;
		}
		json_error("Bad number");
		return get_nan();
	}

	bool json_word()
	{
		switch (json_ch)
		{
			case 't' :
				// 'true'
				if (json_next() == 'r' && json_next() == 'u' && json_next() == 'e')
				{
					json_next();
					return true;
				}
				break;
			case 'f' :
				// 'false'
				if (json_next() == 'a' && json_next() == 'l' && json_next() == 's' && json_next() == 'e')
				{
					json_next();
					return false;
				}
				break;
			case 'n' :
				// 'null'
				if (json_next() == 'u' && json_next() == 'l' && json_next() == 'l')
				{
					json_next();
					return 0;
				}
				break;
		}
		json_error("json_word Syntax error");
		return false;
	}

	void json_white()
	{
		while (json_ch)
		{
			if ((Sint8) json_ch <= ' ')
			{
				json_next();
			}
			else
				if (json_ch == '/')
				{
					switch (json_next())
					{
						case '/' :
							while (json_next() && json_ch != '\n' && json_ch != '\r') {}
							break;
						case '*' :
							json_next();
							for (int i = 0; i < 256; i++)
							{
								if (json_ch)
								{
									if (json_ch == '*')
									{
										if (json_next() == '/')
										{
											json_next();
											break;
										}
									}
									else
									{
										json_next();
									}
								}
								else
								{
									json_error("Unterminated comment");
								}
							}
							break;
						default :
							json_error("json_white Syntax error");
						}
				}
				else
				{
					break;
				}
		}
	}

	char json_next()
	{
		json_ch = json_text[json_at];
		json_at++;
		return json_ch;
	}

	as_value json_value()
	{
		json_white();
		switch (json_ch)
		{
			case '{' :
				return json_object();
			case '[' :
				return json_array();
			case '"' :
				return json_string().c_str();
			case '-' :
				return json_number();
			default :
				return json_ch >= '0' && json_ch <= '9' ? json_number() : json_word();
			}
	}


	void	json_parse(const fn_call& fn)
	{
		json_text = fn.arg(0).to_string();
		json_at = 0;
		json_ch = ' ';
		*fn.result = json_value();
	}

	as_object* json_init()
	{
		// Create built-in math object.
		as_object*	obj = new as_object();
		obj->builtin_member("parse", json_parse);
		return obj;
	}

}
