//
//
//

#ifndef MSPLAYER_piff_H
#define MSPLAYER_piff_H

#if TU_CONFIG_LINK_TO_PLAYREADY == 1

#include <assert.h>
#include "base/container.h"
#include "base/utility.h"
#include "base/smart_ptr.h"
#include "base/tu_file.h"
//#include "base/membuf.h"
#include "bakeinflash/bakeinflash_value.h"

#include <drmcrt.h>
#include <drmcontextsizes.h>
#include <drmmanager.h>
#include <drmrevocation.h>
#include <drmcmdlnpars.h>
#include <drmtoolsutils.h>
#include <drmtoolsinit.h>
#include <drmcmdlnpars.h>
#include <drmtoolsutils.h>
#include <drmplayreadyobj.h>
#include <drmtoolsmediafile.h>
#include <drmtoolsconstants.h>
#include <drmxmlparser.h>
#include <drmtoolsinit.h>
#include <drmconstants.h>

namespace bakeinflash
{
	// forward decl
	struct atom;

	struct atom : public ref_counted
	{
		atom();
		virtual ~atom();

		virtual int read(tu_file& in, const char* parent, int size, int level) = 0;
		int write(tu_file& outfile);
		const char* data() const { return m_data; }
		virtual bool parse() { return false; };
		int size() const { return m_size; }
		tu_string name() const { return m_name; }
		virtual const atom* find(const char* name, int index = 0) const = 0;
		virtual const atom* find_recursive(const char* name) const = 0;
		virtual bool is_folder() const { return false; }

		smart_ptr<atom> m_parent;
		char* m_data;
		int m_size;
		tu_string m_name;
		array< smart_ptr<atom> > m_items;
	};

	struct atom_generic : public atom
	{
		virtual int read(tu_file& in, const char* parent, int size, int level);
		virtual const atom* find(const char* name, int index = 0) const { return NULL; };
		virtual const atom* find_recursive(const char* name) const;
	};

	struct atom_container : public atom
	{
		virtual int read(tu_file& in, const char* parent, int size, int level);
		virtual const atom* find(const char* name, int index = 0) const;
		virtual const atom* find_recursive(const char* name) const;
		virtual bool is_folder() const { return true; }

		atom* read_atom(tu_file& in, const char* parent, int total_size, int level);
	};

	struct piff : public atom_container
	{
		struct entries_t
		{
			uint16 BytesOfClearData;
			uint32 BytesOfEncryptedData;
		};
		struct seb_t
		{
			char InitializationVector[8];
			array<entries_t> entries;
		};

		piff();
		virtual ~piff();

		smart_ptr<atom> get_frame(tu_file& in);
		long open(tu_file& in, const char* hdsfile);
		long init(tu_file& in, const char* hdsfile);
		long request(const tu_string& url, const membuf& body, tu_string* response);

		void close();

		// DRM
		long initDRM(const uint8* pssh, int psshlen, const char* store);
		static short* charToWord(const char* a);
		long decode_mdat2(DRM_BYTE* buf, int len, uint64 f_ui64InitializationVector);
		long generate_license_request(tu_string* url, membuf* body);
		long process_response(const tu_string& body);
		long bind();

		bool m_protected_stream;

		smart_ptr<atom> m_ftyp;
		smart_ptr<atom> m_moov;

		smart_ptr<atom> m_license;
		smart_ptr<atom> m_mdat;
		smart_ptr<atom> m_enca;
		smart_ptr<atom> m_encv;

	private:

		void parse_trun(const atom* trun);
		void parse_uuid(const atom* trun);
		void parse_enca(const atom* a);
		void parse_encv(const atom* a);
		long decode(atom* a);

		DRM_DECRYPT_CONTEXT oDecryptContext;
		DRM_APP_CONTEXT* poAppContext;
		DRM_BYTE* pbRevocationBuffer;
		DRM_BYTE* pbOpaqueBuffer;

		// for decoder 
		int m_sample_data_offset;
		array<uint32> m_sample_sizes;
		array<seb_t> m_sample_encription_box;

		int m_default_AlgorithmID_audio;
		int m_default_IV_size_audio;
		uint8 m_default_KID_audio[16];
		int m_default_AlgorithmID_video;
		int m_default_IV_size_video;
		uint8 m_default_KID_video[16];
	};
}

#endif

#endif
