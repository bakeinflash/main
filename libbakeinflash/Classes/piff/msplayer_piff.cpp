//
//
//

#include "bakeinflash/bakeinflash_log.h"
#include "msplayer_piff.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "bakeinflash/bakeinflash_tcp.h"
#include "base/tu_timer.h"
#include "base/tu_file.h"

#if TU_CONFIG_LINK_TO_PLAYREADY == 1

namespace bakeinflash
{

	static string_hash<bool>	s_folder_name;

	atom::atom() :
		m_data(NULL),
		m_size(0)
	{
		if (s_folder_name.size() == 0)
		{
			s_folder_name.add("moov", true);
			s_folder_name.add("trak", true);
			s_folder_name.add("mdia", true);
			s_folder_name.add("minf", true);
			s_folder_name.add("dinf", true);
			s_folder_name.add("stbl", true);
			s_folder_name.add("mvex", true);

			s_folder_name.add("moof", true);
			s_folder_name.add("traf", true);

			s_folder_name.add("mfra", true);

		}
	}

	atom::~atom()
	{
		delete m_data;
	}

	int atom::write(tu_file& out)
	{
		out.write_be32((uint32) m_size);
		out.write_bytes(m_name.c_str(), 4);
		int written = 8;

		if (m_items.size() > 0)
		{
			// container
			for (int i = 0; i < m_items.size(); i++)
			{
				written += m_items[i]->write(out);
			}
		}
		else
		{
			// generic
			written += out.write_bytes(m_data, m_size - 8);
		}
		assert(m_size == written);
		return m_size;
	}

	int atom_generic::read(tu_file& in, const char* name, int size, int level)
	{
		m_size = size;
		m_name = name;

		m_data = (char*) malloc(size - 8);
		int n = in.read_bytes(m_data, size - 8);
		assert(n == size - 8);
		return size;
	}

	atom* atom_container::read_atom(tu_file& in, const char* parent, int total_size, int level)
	{
		int size = in.read_be32();
		char name[5];
		int n = in.read_bytes(name, 4);
		name[4] = 0;

		if (n == 4 && size > 0)
		{
			//char b[256]; snprintf(b, 256, "%s %d\n", name, size); tu_string bb(b); for (int i=0; i<level; i++) bb = tu_string(" ") + bb; printf(bb.c_str());

			atom* a = NULL;
			bool container = false;
			if (s_folder_name.get(name, &container))
			{
				// folder
				a = new atom_container();
				a->read(in, name, size, level + 1);
			}
			else
			{
				// generic
				a = new atom_generic();
				a->read(in, name, size, level);
			}
			return a;
		}
		return NULL;
	}

	int atom_container::read(tu_file& in, const char* parent, int total_size, int level)
	{
		m_size = total_size;
		m_name = parent;
		while (total_size > 8)
		{
			atom* a = read_atom(in, parent, total_size, level);
			total_size -= a->size();

			m_items.push_back(a);
		}
		//	assert(total_size == 0);
		return total_size;
	}

	const atom* atom_container::find(const char* name, int index) const
	{
		for (int i = 0; i < m_items.size(); i++)
		{
			if (m_items[i]->name() == name)
			{
				if (index > 0)
				{
					index--;
					continue;
				}
				return m_items[i].get();
			}
		}
		return NULL;
	}

	const atom* atom_container::find_recursive(const char* path) const
	{
		assert(strlen(path) >= 4);

		for (int i = 0; i < m_items.size(); i++)
		{
			if (memcmp(m_items[i]->name().c_str(), path, 4) == 0)
			{
				if (path[4] == 0)
				{
					return this;
				}
				else
					if (path[4] == '.')
					{
						return m_items[i]->find_recursive(path + 5);
					}
			}
		}
		return NULL;
	}

	const atom* atom_generic::find_recursive(const char* path) const
	{
		if (name() == path)
		{
			return this;
		}
		return NULL;
	}

	//
	//
	//
	piff::piff() :
		m_protected_stream(false),
		poAppContext(NULL),
		pbRevocationBuffer(NULL),
		pbOpaqueBuffer(NULL),
		m_sample_data_offset(0),
		m_default_AlgorithmID_audio(0),
		m_default_IV_size_audio(8),
		m_default_AlgorithmID_video(0),
		m_default_IV_size_video(8)
	{
		memset(&oDecryptContext, 0, sizeof(oDecryptContext) );
		memset(m_default_KID_audio, 0, sizeof(m_default_KID_audio));
		memset(m_default_KID_video, 0, sizeof(m_default_KID_video));
	}

	piff::~piff()
	{
		close();
	}
	smart_ptr<atom> piff::get_frame(tu_file& in)
	{
		// first take ftyp & moov
		if (m_ftyp)
		{
			smart_ptr<atom> a = m_ftyp;
			m_ftyp = NULL;
			return a;
		}
		if (m_moov)
		{
			smart_ptr<atom> a = m_moov;
			m_moov = NULL;
			return a;
		}

		smart_ptr<atom> a = read_atom(in, "file", 0, 0);
		if (a == NULL)
		{
			return NULL;
		}

		if (m_protected_stream)
		{
			if (a->name() == "moof")
			{
				const atom* traf = a->find("traf");

				const atom* trun = traf->find("trun");
				parse_trun(trun);

				const atom* uuid = traf->find("uuid");
				parse_uuid(uuid);
			}
			else
				if (a->name() == "mdat")
				{
					decode(a);
				}
		}
		return a;
	}

	long piff::decode(atom* a)
	{
		unsigned long dr = 0;
		uint8* tag = (uint8*) a->data();	// mdat + 8
		int offset = 0;
		int len = a->size() - 8; //thisAtom->AtomicLength - offset;

		int sum = 0;
		for (int i = 0; i < m_sample_encription_box.size(); i++)
		{
			sum += m_sample_sizes[i];
		}
		assert(sum == len);

		//			sum += sample_data_offset;

		int sample_count = m_sample_encription_box.size();
		assert(m_sample_encription_box.size() == m_sample_sizes.size());
		uint8* sample = tag + offset; // + sample_data_offset;
		const uint8* end = tag + len;

		for (int i = 0; i < sample_count; i++)
		{
			union
			{
				uint64 val;
				uint8 aval[8];
			} initializationVector;

			for (int k = 0; k < 8; k++)
			{
				initializationVector.aval[7 - k] = m_sample_encription_box[i].InitializationVector[k];
			}

			unsigned long map[2] = {0};
			const array<entries_t>& entries = m_sample_encription_box[i].entries;
			if (entries.size() == 0)
			{
				map[0] = 0;
				map[1] = m_sample_sizes[i];
				dr = decode_mdat2((uint8*) sample, m_sample_sizes[i], initializationVector.val);
			}
			else
			{
				map[0] = m_sample_sizes[i];
				map[1] = 0;
				int NumberOfEntries = entries.size() ;
				for (int j = 0; j < NumberOfEntries; j++)
				{
					assert(NumberOfEntries==1);
					int BytesOfClearData = m_sample_encription_box[i].entries[j].BytesOfClearData;
					int BytesOfEncryptedData = m_sample_encription_box[i].entries[j].BytesOfEncryptedData;
					assert(m_sample_sizes[i] == BytesOfClearData + BytesOfEncryptedData );
					if (BytesOfEncryptedData > 0)
					{
						// sample ==> NAL layer
						// The length field is a variable length field. It can be 1, 2, or 4 bytes long and is specified in
						// the SampleEntry for the track (it can be found at
						// AVCSampleEntry.AVCConfigurationBox.
						// AVCDecoderConfigurationRecord.lengthSizeMinusOne)

						// The application encrypting the content MUST choose a clear prefix length that leaves at least the
						// nalLength and the nal_unit_type fields in the clear. 
						dr = decode_mdat2((uint8*) sample + BytesOfClearData, BytesOfEncryptedData, initializationVector.val);
					}
				}
			}
			sample += m_sample_sizes[i];		// next sample
			assert(sample <= end);
		}
		//	printf("decoded %d\n", n);
		return dr;
	}

	void piff::parse_uuid(const atom* a)
	{
		// uuid = "a2394f52-5a9b-4f14-a244-6c427c648df4"
		const uint8* tag = (const uint8*) a->data();	// ==> uuid name
		assert(memcmp(tag, "\xA2\x39\x4F\x52\x5A\x9B\x4F\x14\xA2\x44\x6C\x42\x7C\x64\x8D\xF4", 16) == 0);

		uint32 ver_flags = read_be32(tag + 16); 
		if (ver_flags & 1)
		{
			//unsigned int(24) AlgorithmID;
			//unsigned int(8) IV_size;
			//unsigned int(8)[16] KID;
			assert(0);
		}

		int sample_count = read_be32(tag + 20);
	//	printf("sample_count=%d\n", sample_count);

		int offset = 24;
		//assert(sizeof(seb_t.InitializationVector) == default_IV_size_video);
		m_sample_encription_box.resize(0);
		m_sample_encription_box.resize(sample_count);
		for (int i = 0; i < sample_count; i++)
		{
			memcpy(m_sample_encription_box[i].InitializationVector, tag + offset, m_default_IV_size_video);
			offset += m_default_IV_size_video;

			if (ver_flags & 2)
			{
				uint16 NumberOfEntries = read_be16(tag + offset);
				offset += 2;

				m_sample_encription_box[i].entries.resize(NumberOfEntries);
				for (int j = 0; j < NumberOfEntries; j++)
				{
					uint16 BytesOfClearData = read_be16(tag + offset);
					m_sample_encription_box[i].entries[j].BytesOfClearData = BytesOfClearData;
					offset += 2;

					uint32 BytesOfEncryptedData = read_be32(tag + offset);
					m_sample_encription_box[i].entries[j].BytesOfEncryptedData = BytesOfEncryptedData;
					offset += 4;
				}
			}
		}
	}

	void piff::parse_enca(const atom* a)
	{
		// track encryption box, Container Scheme Information Box („schi‟)
		// The Track Encryption box contains default values for the AlgorithmID, IV_size, and KID for theentire track.
		/*
		char* tag = thisAtom->AtomicData + 4;	// ==>'uuid'
		const uint8* tag = (const uint8*) a->data();
		int tagsize =	read_be32(tag-4);

		unsigned char uuid[16];
		memcpy(uuid, tag + 4, 16);

		uint32_t ver_flags;
		memcpy(&ver_flags, tag + 20, 4);

		union{
		uint32_t dd32;	// big endian
		uint8_t dd8[4];	// big endian
		} dd;
		memcpy(&dd.dd32, tag + 24, 4);

		assert(default_IV_size_audio ==0);

		default_AlgorithmID_audio = dd.dd8[0]*256*256 +  dd.dd8[1]*256 + dd.dd8[2];
		default_IV_size_audio = dd.dd8[3];

		memcpy(&default_KID_audio, tag + 28, 16);
		assert(tagsize-4 == 44);*/
	}

	void piff::parse_encv(const atom* a)
	{
		// track encryption box, Container Scheme Information Box („schi‟)
		// The Track Encryption box contains default values for the AlgorithmID, IV_size, and KID for theentire track.
		/*	char* tag = thisAtom->AtomicData + 4;	// ==>'uuid'
		int tagsize = UInt32FromBigEndian(tag-4);

		unsigned char uuid[16];
		memcpy(uuid, tag + 4, 16);

		uint32_t ver_flags;
		memcpy(&ver_flags, tag + 20, 4);

		union{
		uint32_t dd32;	// big endian
		uint8_t dd8[4];	// big endian
		} dd;
		memcpy(&dd.dd32, tag + 24, 4);

		assert(default_IV_size_video ==0);
		default_AlgorithmID_video = dd.dd8[0]*256*256 +  dd.dd8[1]*256 + dd.dd8[2];
		default_IV_size_video = dd.dd8[3];

		memcpy(&default_KID_video, tag + 28, 16);
		assert(tagsize-4 == 44);*/
	}


	void piff::parse_trun(const atom* a)
	{
		const uint8* tag = (const uint8*) a->data();
		uint32 ver_flags = read_be32(tag); 
		int sample_count = read_be32(tag + 4);
		m_sample_data_offset =  read_be32(tag + 8);
		//		if (ver_flags == 0xb05)	
		//		{
		//			uint32 first_sample_flags = read_be32(tag + 12);
		//		}

		const uint8* p = tag;
		m_sample_sizes.resize(sample_count);
		for (int i = 0; i < sample_count; i++)
		{
			if (ver_flags == 0x301)		// hack
			{
				int sample_duration = read_be32(p + 12);
				int sample_size = read_be32(p + 16);
				m_sample_sizes[i] = sample_size;
				p += 8;
			}
			else
				if (ver_flags == 0xb05)		// hack
				{
					uint32 sample_duration = read_be32(p + 16);
					uint32 sample_size = read_be32(p + 20);
					uint32 sample_composition_time_offset = read_be32(p + 24);
					m_sample_sizes[i] = sample_size;
					p += 12;
				}
				else
				{
					assert(0);
				}
		}
	}

	long piff::generate_license_request(tu_string* url, membuf* body)
	{
		const DRM_CONST_STRING* rgpdstrRights[1] = { NULL };
		rgpdstrRights [0] = (DRM_CONST_STRING *) &g_dstrWMDRM_RIGHT_PLAYBACK;
		DRM_DWORD  cchUrl = 0;
		DRM_DWORD cbChallenge = 0;
		DRM_RESULT dr = Drm_LicenseAcq_GenerateChallenge(poAppContext, rgpdstrRights,	DRM_NO_OF(rgpdstrRights),	NULL,	NULL,	0, NULL, &cchUrl, NULL,	NULL,	NULL,	&cbChallenge);
		if (dr == DRM_E_BUFFERTOOSMALL)
		{
			// success, we now have the required buffer lengths

			DRM_CHAR* purl = (DRM_CHAR*) malloc(cchUrl);
			DRM_BYTE* pbody = (DRM_BYTE*) malloc(cbChallenge);

			dr = Drm_LicenseAcq_GenerateChallenge(poAppContext,	rgpdstrRights, DRM_NO_OF(rgpdstrRights), NULL, NULL, 0, purl, &cchUrl, NULL, NULL, pbody,	&cbChallenge);
			if (dr == DRM_SUCCESS)
			{
				*url = purl;
				body->append(pbody, cbChallenge);
			}
			free(pbody);
			free(purl);
		}
		return dr;
	}


	long piff::open(tu_file& in, const char* hdsfile)
	{
		m_ftyp = read_atom(in, "file", 0, 0);
		if (m_ftyp == NULL)
		{
			return false;
		}

		m_moov = read_atom(in, "file", 0, 0);	// hack
		if (m_moov == NULL)
		{
			return false;
		}

		//		const atom* traka = m_moov->find("trak", 0);
		//		const atom* enca = traka->find_recursive("mdia.minf.stbl.stsd");

		//		const atom* trakv = m_moov->find("trak", 1);
		//		const atom* encv = traka->find_recursive("mdia.minf.stbl.stsd.encv");

		const atom* uuid = m_moov->find("uuid");
		m_protected_stream = (uuid != NULL);
		if (uuid == NULL)
		{
			// no uuid.. sttream is not protected
			return 0;
		}

		// unsigned int(8)[16]  extended_type=0xd08a4f18-10f3-4a82-b6c8-32d8aba183d3,
		// uint32 version=0 +  flags=0
		// unsigned int(8)[16] SystemID;
		// unsigned int(32) DataSize;
		// unsigned int(8)[DataSize] Data;

		const uint8* pssh = (const uint8*) uuid->data();	// pssh ==> uuid
		assert(memcmp(pssh, "\xd0\x8a\x4f\x18\x10\xf3\x4a\x82\xb6\xc8\x32\xd8\xab\xa1\x83\xd3", 16) == 0);
		pssh += 16;			// ==> ver_flags
		uint32 ver_flags = read_be32(pssh);
		pssh += 4;			// ==> SystemID
		pssh += 16;			// skip SystemID
		uint32 DataSize = read_be32(pssh);
		pssh += 4;			// ==> Data
	//	assert(DataSize == read_le32(pssh));

		long dr = initDRM(pssh, DataSize, hdsfile);
		if (dr == 0x8004C013) // no license
		{
			// try to get it
			// a license was not found in the license store.
			tu_string url;
			membuf body;
			dr = generate_license_request(&url, &body);
			if (dr == DRM_SUCCESS)
			{
				// send request to the url and get responce
				tu_string response;
				dr = request(url, body, &response);
				if (dr == 0)
				{
					dr = process_response(response);
					if (dr == 0)
					{
						myprintf("added license\n");

						// try to re-bind
						dr = bind();
					}
					else
					{
						myprintf("failed to get license, 0x%08X\n", dr);
					}
				}
			}
		}
		return dr;
	}

	long piff::request(const tu_string& url, const membuf& body, tu_string* response)
	{
		Uint32 m_start = tu_timer::get_ticks();
		tcp m_http;

		bool rc = m_http.connect(url);
		if (rc == false)
		{
			// invalid URL
			return -1;
		}

		// connection timeout = 10sec = 10ms*1000
		for (int i = 0; i < 1000; i++)
		{
			if (m_http.is_connected())
			{
				// established connection.. send request
				m_http.m_headers.add("Content-type", "text/xml;charset=utf-8");
				m_http.write_http(body);

				// read timeout = 10sec = 10ms*1000
				const void* data = NULL;
				int size = 0;
				string_hash<as_value> headers;
				for (int j = 0; j < 1000; j++)
				{
					// read file
					bool rc = m_http.read_http(&data, &size, &headers);
					if (rc)
					{
						if  (m_http.get_status() == 200)
						{
//							response->resize(0);
	//						response->append(data, size);
							*response = (const char*) data;
							assert(response->size() == size);
							m_http.close();
							return 0;
						}
						else
						{
							// 404 - not found.. read error
							myprintf("piff: http error %d\n", m_http.get_status());
							return -404;
						}
					}

					// wait data
					tu_timer::sleep(10);
				}

				// read timeout
				return -405;
			}

			// wait connection
			tu_timer::sleep(10);
		}
		// connection timeout
		return -406;
	}


	void piff::close()
	{
		Drm_Reader_Close(&oDecryptContext);
		Drm_Uninitialize (poAppContext);
		if (pbRevocationBuffer) Oem_MemFree(pbRevocationBuffer);
		if (pbOpaqueBuffer) Oem_MemFree(pbOpaqueBuffer);
		if (poAppContext) Oem_MemFree(poAppContext);
	}

	// Handle callbacks from Drm_Reader_Bind
	DRM_RESULT DRM_CALL BindCallback(
		__in     const DRM_VOID                 *f_pvPolicyCallbackData,
		__in           DRM_POLICY_CALLBACK_TYPE  f_dwCallbackType,
		__in_opt const DRM_KID                  *f_pKID,
		__in_opt const DRM_LID                  *f_pLID,
		__in_opt const DRM_VOID                 *f_pv )
	{
//		return DRMTOOLS_PrintPolicyCallback( f_pvPolicyCallbackData, f_dwCallbackType );
		return 0; 
	}


	short* piff::charToWord(const char* a)
	{
		size_t n = strlen(a) + 1;
		short* res = (short*) malloc(n * 2);
		short* p = res;
		while (*a)
		{
			*p++ = (short) (*a++);
		}
		*p = 0;
		return res;
	}

	// apply accepted license, write it into HDS
	long piff::process_response(const tu_string& body)
	{
		DRM_RESULT dr = DRM_SUCCESS;
		if (true )
		{
			DRM_LICENSE_RESPONSE oLicenseResponse = { eUnknownProtocol, 0 };
			dr = Drm_LicenseAcq_ProcessResponse( poAppContext, DRM_PROCESS_LIC_RESPONSE_SIGNATURE_NOT_REQUIRED,	(const DRM_BYTE*) body.c_str(),	(DRM_DWORD) body.size(),	&oLicenseResponse);
			if (dr == 0)
			{
				DRM_DWORD cbChallenge = 0;
				dr = Drm_LicenseAcq_GenerateAck(poAppContext, &oLicenseResponse, NULL, &cbChallenge);
				if (dr == DRM_E_BUFFERTOOSMALL)
				{
					// result is ok
					DRM_BYTE* pbChallenge = (DRM_BYTE*) Oem_MemAlloc(cbChallenge);
					dr = Drm_LicenseAcq_GenerateAck(poAppContext, &oLicenseResponse, pbChallenge, &cbChallenge);
					Oem_MemFree(pbChallenge);
				}
			}
		}
		else
		{
			DRM_RESULT dr1 = DRM_SUCCESS;
			dr = Drm_LicenseAcq_ProcessAckResponse( poAppContext, (DRM_BYTE*) body.c_str(), (DRM_DWORD) body.size(),	&dr1);
		}
		return dr;
	}

	long piff::initDRM(const uint8* pssh, int psshlen, const char* store)
	{
		DRM_CONST_STRING dstrStoreName;
		dstrStoreName.pwszString = (const DRM_WCHAR*) charToWord(store);
		dstrStoreName.cchString =  strlen(store);

		poAppContext = (DRM_APP_CONTEXT *) Oem_MemAlloc( sizeof(DRM_APP_CONTEXT) );
		pbOpaqueBuffer = (DRM_BYTE *) Oem_MemAlloc(MINIMUM_APPCONTEXT_OPAQUE_BUFFER_SIZE);
		DRM_DWORD cbOpaqueBuffer = MINIMUM_APPCONTEXT_OPAQUE_BUFFER_SIZE;
		DRM_RESULT dr = DRM_SUCCESS;
		ChkDR(Drm_Initialize(poAppContext, NULL, pbOpaqueBuffer, cbOpaqueBuffer, &dstrStoreName));

		if(DRM_REVOCATION_IsRevocationSupported())
		{
			assert(pbRevocationBuffer == NULL);
			ChkMem( pbRevocationBuffer = (DRM_BYTE*) Oem_MemAlloc(REVOCATION_BUFFER_SIZE));
			ChkDR(Drm_Revocation_SetBuffer(poAppContext, pbRevocationBuffer, REVOCATION_BUFFER_SIZE));
		}

		ChkDR(Drm_Content_SetProperty (poAppContext, DRM_CSP_AUTODETECT_HEADER, (const DRM_BYTE*) pssh, (DRM_DWORD) psshlen));

		ChkDR(bind());

ErrorExit:
		free((void*) dstrStoreName.pwszString);
		return dr;
	}

	long piff::bind()
	{
		DRM_RESULT dr = DRM_SUCCESS;
		const DRM_CONST_STRING* apdcsRights[1] = { NULL };
		apdcsRights[0] = (DRM_CONST_STRING *) &g_dstrWMDRM_RIGHT_PLAYBACK;
		ChkDR(Drm_Reader_Bind(poAppContext, apdcsRights, 1, (DRMPFNPOLICYCALLBACK)BindCallback,	NULL,	&oDecryptContext));
		ChkDR(Drm_Reader_Commit( poAppContext, NULL, NULL));
ErrorExit:
		return dr;
	}

	long piff::decode_mdat2(DRM_BYTE* buf, int len, DRM_UINT64 f_ui64InitializationVector)
	{
		DRM_AES_COUNTER_MODE_CONTEXT f_pCtrContext;
		f_pCtrContext.bByteOffset = 0;
		f_pCtrContext.qwBlockOffset = 0;
		f_pCtrContext.qwInitializationVector = f_ui64InitializationVector;
		return Drm_Reader_DecryptLegacy (&oDecryptContext, &f_pCtrContext, (DRM_BYTE*) buf, (DRM_DWORD) len);
	}

}

#endif
