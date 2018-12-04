// sysinfo.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// bakeinflash plugin, some useful misc programs

#include <stdio.h>
#include <dirent.h>

#include "base/tu_file.h"
#include "sysinfo.h"
#include "bakeinflash/bakeinflash_root.h"

#ifndef WIN32
#include <sys/stat.h>
#include "unistd.h"
#include "sys/ioctl.h"
#include "fcntl.h"
#include "linux/hdreg.h"
#endif

// methods that is called from Action Scirpt
namespace bakeinflash
{

	// Executes the given command using CreateProcess() and WaitForSingleObject().
	// Returns FALSE if the command could not be executed or if the exit code could not be determined.
	int execute(const char* cmd, const char* arg)
	{
		int result = 0;		// false
#ifdef WIN32
		PROCESS_INFORMATION processInformation = {0};
		STARTUPINFO startupInfo                = {0};
		startupInfo.cb                         = sizeof(startupInfo);

		// Create the process
		result = CreateProcess((LPSTR) cmd, (LPSTR) arg, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL, NULL, &startupInfo, &processInformation);
		if (result)
		{
			// Successfully created the process.  Wait for it to finish.
			WaitForSingleObject( processInformation.hProcess, INFINITE );

			// Get the exit code.
			DWORD exitCode = 0;
			result = GetExitCodeProcess(processInformation.hProcess, &exitCode);

			// Close the handles.
			CloseHandle(processInformation.hProcess);
			CloseHandle(processInformation.hThread);
		}
#endif
		return result;
	}

	void	as_sysinfo_ctor(const fn_call& fn)
	{
		sysinfo*	obj = new sysinfo();
		fn.result->set_as_object(obj);
	}

	// gets dir entity. It is useful for a preloading of the SWF files
	void	getDir(const fn_call& fn)
	{
		sysinfo* si = cast_to<sysinfo>(fn.this_ptr);
		if (si)
		{
			if (fn.nargs > 0)
			{
				as_object* dir = new as_object();
				bool recursive = fn.nargs >= 2 ? fn.arg(1).to_bool() : false;
				si->get_dir(dir, fn.arg(0).to_string(), recursive);
				fn.result->set_as_object(dir);
			}
		}

	}
	// gets HDD serial NO. It is useful for a binding the program to HDD
	void	getHDDSerNo(const fn_call& fn)
	{
		sysinfo* si = cast_to<sysinfo>(fn.this_ptr);
		if (si)
		{
			if (fn.nargs > 0)
			{
				tu_string sn;
				si->get_hdd_serno(&sn, fn.arg(0).to_string());
				fn.result->set_tu_string(sn);
			}
		}
	}

	// gets available free memory
	void	getFreeMem(const fn_call& fn)
	{
		sysinfo* si = cast_to<sysinfo>(fn.this_ptr);
		if (si)
		{
			fn.result->set_int(si->get_freemem());
		}
	}

	void	sysinfo_exec(const fn_call& fn)
	{
		sysinfo* si = cast_to<sysinfo>(fn.this_ptr);
		if (si && fn.nargs >= 2)
		{
			const char* cmd = fn.arg(0).to_string();
			const char* arg = fn.arg(1).to_string();
//			system(arg); //"\"d:some path\\program.exe\" \"d:\\other path\\file name.ext\"");
			execute(cmd, arg);
		}
	}

	// DLL interface

//	extern "C"
//	{
//		exported_module as_object* bakeinflash_module_init(const array<as_value>& params)
//		{
//			return new sysinfo();
//		}
//	}

	sysinfo::sysinfo()
	{
		builtin_member("getDir", getDir);
		builtin_member("getHDDSerNo", getHDDSerNo);
		builtin_member("getFreeMem", getFreeMem);
		builtin_member("exec", sysinfo_exec);
	}

	void sysinfo::get_dir(as_object* info, const tu_string& path, bool recursive)
	{
		DIR* dir = opendir(path.c_str());
		if (dir == NULL)
		{
			return;
		}

		//	as_object* info = new as_object();
		while (dirent* de = readdir(dir))
		{
			tu_string name = de->d_name;
			if (*name.c_str() == '.' || name == "..")
			{
				continue;
			}

#ifdef WIN32
			bool is_readonly = de->data.dwFileAttributes & 0x01;
			bool is_hidden = de->data.dwFileAttributes & 0x02;
			//??		bool b2 = de->data.dwFileAttributes & 0x04;
			//??		bool b3 = de->data.dwFileAttributes & 0x08;
			bool is_dir = de->data.dwFileAttributes & 0x10;
#else
			struct stat buf;
			stat(tu_string(path + de->d_name).c_str(), &buf);
			bool is_readonly = false;	//TODO
			bool is_hidden = false; //TODO
			bool is_dir = S_ISDIR(buf.st_mode);
#endif

			// printf("%s, %d\n", name.c_str(), is_dir);

			as_object* item = new as_object();
			info->set_member(name, item);

			item->set_member("is_dir", is_dir);
			item->set_member("is_readonly", is_readonly);
			item->set_member("is_hidden", is_hidden);
			if (is_dir && recursive)
			{
				as_object* info = new as_object();
				item->set_member("dirinfo", info);
				get_dir(info, path + de->d_name + "/", recursive);
			}

		}
		closedir(dir);
	}

	bool sysinfo::get_hdd_serno(tu_string* sn, const char* dev)
	{
		assert(sn);

		if (dev == NULL)
		{
			return false;
		}

#ifdef WIN32
		//TODO
		return false;
#else

		struct hd_driveid id;
		int fd = open(dev, O_RDONLY | O_NONBLOCK);
		if (fd < 0)
		{
			//		printf("can't get info about '%s'\n", dev);
			return false;
		}

		int rc = ioctl(fd, HDIO_GET_IDENTITY, &id);
		close(fd);

		if (rc != 0)
		{
			//		printf("can't get info about '%s'\n", dev);
			return false;
		}


		//  printf("serial no %s\n", id.serial_no);
		//  printf("model number %s\n", id.model);
		//  printf("firmware revision %s\n", id.fw_rev);
		//  printf("cylinders %s\n", id.cyls);
		//  printf("heads %s\n", id.heads);
		//  printf("sectors per track %s\n", id.sectors);

		*sn = (char*) id.serial_no;

#endif

	}

	// gets free memory size in KB
	int sysinfo::get_freemem()
	{
#ifdef WIN32
		return 0;		//TODO
#else

		int free_mem = 0;
		FILE* fi = fopen("/proc/meminfo", "r");
		if (fi)
		{
			char s[256];
			while (true)
			{
				// read one line
				int i = 0;
				while (i < 255)
				{
					int n = fread(s + i, 1, 1, fi);
					if (n == 0 || s[i] == 0x0A)	// eol ? eof ?
					{
						break;
					}
					i++;
				}
				s[i] = 0;

				const char* p = strstr(s, "MemFree:");
				if (p)
				{
					p = p + 8;
					free_mem = atoi(p);
					break;
				}
			}
			fclose(fi);
		}
		return free_mem;

#endif
	}
}