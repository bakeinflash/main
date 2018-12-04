// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#if TU_CONFIG_LINK_TO_LIBHPDF == 1

#include "base/container.h"
#include "as_print.h"
#include <setjmp.h>
#include <math.h>

#ifdef WIN32
#include "libharu/hpdf.h"
#include <windows.h>
#else
//#include <cups/cups.h>
#endif

namespace bakeinflash
{

	int print_text(const tu_string& s, const tu_string& dev)
	{
#ifdef WIN32
		TCHAR   szDriver[16] = "WINSPOOL";
		TCHAR   szPrinter[256];
		DWORD   cchBuffer = 255;
		HDC     hdcPrint = NULL;
		HDC     hdcPrintImg = NULL;
		HANDLE  hPrinter = NULL;
		PRINTER_INFO_2  *pPrinterData;
		BYTE    pdBuffer[16384];
		BOOL    bReturn = FALSE;

		DWORD   cbBuf = sizeof (pdBuffer);
		DWORD   cbNeeded = 0;
		pPrinterData = (PRINTER_INFO_2 *)&pdBuffer[0];

		// get the default printer name
		bReturn = GetDefaultPrinter(szPrinter, &cchBuffer);
		if (!bReturn)
		{
			printf("print_image: no default printers\n");
			return -1;
		}

		// open the default printer
		bReturn = OpenPrinter(szPrinter, &hPrinter, NULL);
		if (!bReturn)
		{
			printf("print_image: cann't open printer %s\n", szPrinter);
			return -1;
		}

		// get the printer port name
		bReturn =  GetPrinter(hPrinter,	2, &pdBuffer[0], cbBuf, &cbNeeded);
		// this handle is no longer needed
		ClosePrinter(hPrinter);

		if (!bReturn)
		{
			printf("print_image: cann't get printer p;ort name for %s\n", szPrinter);
			return -1;
		}

		// create the Print DC
		hdcPrint = CreateDC(szDriver, szPrinter, pPrinterData->pPortName, NULL); 
		if (!hdcPrint)
		{
			printf("print_image: cann't create HDC for %s\n", szPrinter);
			return -1;
		}

		// Print a test page that contains the string  
		Escape(hdcPrint, STARTDOC, 0, NULL, NULL); 

		// print line
		char* ss = (char*) malloc(s.size() + sizeof(WORD) + 1);	// WORD + EOL
		memcpy(ss + sizeof(WORD), s.c_str(), s.size() + 1);
		*((WORD*) ss) = s.size();
		Escape(hdcPrint, PASSTHROUGH, s.size() + sizeof(WORD) + 1, ss, NULL); 
		free(ss);

		Escape(hdcPrint, ENDDOC, 0, NULL, NULL); 
		DeleteDC(hdcPrint); 
		return 0;
#else
		FILE* prn = fopen(dev.c_str(), "w");	// "/dev/usb/lp0"
		if (prn == NULL)
		{
			printf("can't open %s\n", dev.c_str());
			return -1;
		}
		fprintf(prn, "%s", s.c_str());
		fclose(prn);
		return 0;
#endif
	}

	int print_image(Uint8* buf, int w, int h, const char* pdf_file)
	{
#ifdef WIN32
		TCHAR   szDriver[16] = "WINSPOOL";
		TCHAR   szPrinter[256];
		DWORD   cchBuffer = 255;
		HDC     hdcPrint = NULL;
		HDC     hdcPrintImg = NULL;
		HANDLE  hPrinter = NULL;
		PRINTER_INFO_2  *pPrinterData;
		BYTE    pdBuffer[16384];
		BOOL    bReturn = FALSE;

		DWORD   cbBuf = sizeof (pdBuffer);
		DWORD   cbNeeded = 0;
		pPrinterData = (PRINTER_INFO_2 *)&pdBuffer[0];

		// get the default printer name
		bReturn = GetDefaultPrinter(szPrinter, &cchBuffer);
		if (!bReturn)
		{
			printf("print_image: no default printers\n");
			return -1;
		}

		// open the default printer
		bReturn = OpenPrinter(szPrinter, &hPrinter, NULL);
		if (!bReturn)
		{
			printf("print_image: cann't open printer %s\n", szPrinter);
			return -1;
		}

		// get the printer port name
		bReturn =  GetPrinter(hPrinter,	2, &pdBuffer[0], cbBuf, &cbNeeded);
		// this handle is no longer needed
		ClosePrinter(hPrinter);

		if (!bReturn)
		{
			printf("print_image: cann't get printer p;ort name for %s\n", szPrinter);
			return -1;
		}

		// create the Print DC
		hdcPrint = CreateDC(szDriver, szPrinter, pPrinterData->pPortName, NULL); 
		if (!hdcPrint)
		{
			printf("print_image: cann't create HDC for %s\n", szPrinter);
			return -1;
		}

		// Print a test page that contains the string  
		Escape(hdcPrint, STARTDOC, 0, NULL, NULL); 

		//			HBITMAP bmp; // = (HBITMAP)LoadImage(0, L"print_file.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		//			HBITMAP CreatCompatibleBitmap(bmp);
		//hdcPrintImg = bmp;

		//BitBlt(hdcPrint, 0, 0, 3300, 2550, hdcPrintImg, 0, 0, SRCCOPY);

		//			Escape(hdcPrint, NEWFRAME, 0, NULL, NULL); 
		Escape(hdcPrint, ENDDOC, 0, NULL, NULL); 
		DeleteDC(hdcPrint); 
#else
		//  cupsPrintFile(cupsGetDefault(), tmpfilename, "cairo PS", 0, NULL);
		//	unlink(tmpfilename);
#endif
		return 0;
	}

#ifdef WIN32
	static jmp_buf env;
	void error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no,	void* user_data)
	{
		printf("ERROR: error_no=%04X, detail_no=%u\n", (HPDF_UINT) error_no,	(HPDF_UINT) detail_no);
		longjmp(env, 1);
	}

	int create_pdf(Uint8* buf, int w, int h, const char* fname)
	{
		HPDF_Doc pdf = HPDF_New(error_handler, NULL);
		if (!pdf)
		{
			printf("error: cannot create PdfDoc object\n");
			return 1;
		}

		// error-handler
		if (setjmp(env))
		{
			HPDF_Free(pdf);
			return 1;
		}

		HPDF_SetCompressionMode(pdf, HPDF_COMP_ALL);

		// add a new page object.
		HPDF_Page page = HPDF_AddPage(pdf);
		HPDF_Page_SetWidth(page, (HPDF_REAL) w);
		HPDF_Page_SetHeight(page, (HPDF_REAL) h);

		HPDF_Destination dst = HPDF_Page_CreateDestination(page);
		HPDF_Destination_SetXYZ(dst, 0, HPDF_Page_GetHeight(page), 1);
		HPDF_SetOpenAction(pdf, dst);

		// create default-font 
		//		HPDF_Font font = HPDF_GetFont(pdf, "Helvetica", NULL);
		//		HPDF_Page_BeginText(page);

		//		HPDF_Page_SetFontAndSize(page, font, 20);
		//		HPDF_Page_MoveTextPos(page, 220, HPDF_Page_GetHeight(page) - 70);
		//		HPDF_Page_ShowText(page, "ImageDemo");
		//		HPDF_Page_EndText(page);

		// load image file
		HPDF_Image image = HPDF_LoadRawImageFromMem(pdf, buf, w, h, HPDF_CS_DEVICE_RGB, 8);
		HPDF_UINT iw = HPDF_Image_GetWidth(image);
		HPDF_UINT ih = HPDF_Image_GetHeight(image);

		HPDF_Page_SetLineWidth(page, 0.5);

		HPDF_REAL x = 0;
		HPDF_REAL y = 0; //HPDF_Page_GetHeight(page) - 150;

		// Draw image to the canvas.(normal-mode with actual size.)
		HPDF_Page_DrawImage(page, image, x, y, iw, ih);

		//	show_description(page, x, y, "Actual Size");

		// save the document to a file and cleanup 
		HPDF_SaveToFile(pdf, fname);
		HPDF_Free(pdf);

		return 0;
	}
#else
	int create_pdf(Uint8* buf, int w, int h, const char* fname)
	{
		return 0;
	}
#endif

}	// namespace bakeinflash


#endif
