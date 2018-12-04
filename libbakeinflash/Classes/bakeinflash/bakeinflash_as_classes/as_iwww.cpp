// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "bakeinflash/bakeinflash_as_classes/as_iwww.h"
#include "bakeinflash/bakeinflash_root.h"

#if ANDROID == 1
	//#include "base/tu_jni.h"

	extern JNIEnv*  s_jenv;
	extern float s_scale;
	extern int s_x0;
	extern int s_y0;
	extern tu_string s_pkgName;
	static float s_retina = 1;
#endif

#ifdef iOS
	#import <MapKit/MapKit.h>  
	#import "as_iwww_.h"

	@interface EAGLView : UIView <UITextFieldDelegate>
		- (void) sendMessage :(const char*) name :(const char*) arg;
	@end
		extern EAGLView* s_view;
	extern float s_scale;
	extern int s_x0;
	extern int s_y0;
	extern float s_retina;
#else
  #if !defined(ANDROID) && (defined(WIN32) || defined(__GNUC__))
    static int s_x0 = 0;
    static int s_y0 = 0;
    static float s_retina = 1;
    static float s_scale = 1;
  #endif
#endif

namespace bakeinflash
{
	void	as_global_iwebview_ctor(const fn_call& fn)
	{
		if (fn.nargs < 1)
		{
			myprintf("www ctor error, no args, needs target movie\n");
			return;
		}

		character* ch = cast_to<character>(fn.arg(0).to_object());
		if (ch == NULL)
		{
			return;
		}

		// get actual size of characters in pixels
		matrix m;
		ch->get_world_matrix(&m);

		float xscale = m.get_x_scale() * s_scale / s_retina;
		float yscale = m.get_y_scale() * s_scale / s_retina;

		int x = (int) (s_x0 / s_retina + TWIPS_TO_PIXELS(m.m_[0][2] * s_scale / s_retina));
		int y = (int) (s_y0 / s_retina + TWIPS_TO_PIXELS(m.m_[1][2] * s_scale / s_retina));
		int w = (int) TWIPS_TO_PIXELS(ch->get_width() * xscale);
		int h = (int) TWIPS_TO_PIXELS(ch->get_height() * yscale);

		fn.result->set_as_object(new as_iwebview(x, y, w, h));
	}

}

namespace bakeinflash
{
	as_iwebview::as_iwebview(int x, int y, int w, int h) :
		m_iwebview(NULL)
	{
#if iOS == 1
		CGRect rect = CGRectMake(x, y, w, h);
		myWebView* www = [[myWebView alloc] initWithFrame: rect];
		www.autoresizesSubviews = YES;
		www.autoresizingMask = (UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth);
		[www setBackgroundColor:[UIColor lightGrayColor]];
		[www setDelegate :www];

		[s_view addSubview :www];

		m_iwebview = www;
#elif ANDROID == 1
		// new myWebView();
		jclass cls = s_jenv->FindClass(s_pkgName + tu_string("myWebView"));
		jmethodID mid = s_jenv->GetMethodID(cls, "<init>", "(IIII)V");
		m_iwebview = s_jenv->NewObject(cls, mid, w, h, x, y);
		m_iwebview = s_jenv->NewGlobalRef(m_iwebview);
#else
		printf("webview class is not supported\n");
#endif
	}

	as_iwebview::~as_iwebview()
	{

#if iOS == 1
		myWebView* www = (myWebView*) m_iwebview;
		if (www)
		{
			[www removeFromSuperview];
			[www release]; 
		}
#elif ANDROID == 1
		// delete myWebView();
		jclass cls = s_jenv->FindClass(s_pkgName + tu_string("myWebView"));
		jmethodID mid = s_jenv->GetMethodID(cls, "remove", "()V");
		s_jenv->CallVoidMethod(m_iwebview, mid);
		s_jenv->DeleteGlobalRef((jobject) m_iwebview);
#else
		printf("webview class is not supported\n");
#endif
	}

	bool	as_iwebview::get_member(const tu_string& name, as_value* val)
	{
		//myWebView* www = (myWebView*) m_iwebview;
		return as_object::get_member(name, val);
	}

	bool	as_iwebview::set_member(const tu_string& name, const as_value& val)
	{
		if (name == "url")
		{
#ifdef iOS
			myWebView* www = (myWebView*) m_iwebview;
			NSString *urlAddress = [NSString stringWithUTF8String: val.to_string()];
			NSURL* url = [NSURL URLWithString:urlAddress];
			NSURLRequest* requestObj = [NSURLRequest requestWithURL:url];
			[www loadRequest:requestObj];
#endif

#if ANDROID == 1
			if (m_iwebview)
			{
				jclass cls = s_jenv->FindClass(s_pkgName + tu_string("myWebView"));
				jmethodID mid = s_jenv->GetMethodID(cls, "loadUrl", "(Ljava/lang/String;)V");
				const char* url = val.to_string();
				jstring jurl = s_jenv->NewStringUTF(url);
				s_jenv->CallVoidMethod(m_iwebview, mid, jurl);
			}
#endif
		}
		else
		if (name == "_visible")
		{
			if (m_iwebview)
			{
#ifdef iOS
				myWebView* www = (myWebView*) m_iwebview;
				www.hidden = !val.to_bool();
#endif

#if ANDROID == 1
				// setVisibility(int visibility)
				// 0 = visible
				// 4 = invisible
				// 8 = gone
				jclass cls = s_jenv->FindClass(s_pkgName + tu_string("myWebView"));
				jmethodID mid = s_jenv->GetMethodID(cls, "setVisible", "(I)V");
				s_jenv->CallVoidMethod(m_iwebview, mid,val.to_bool() ? 0 : 8);
#endif
			}
		}
		return as_object::set_member(name, val);
	}

}

//
//
//
#ifdef iOS

@implementation myWebView
	@synthesize parent;

- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
	NSURL* url = [request URL];
	if ([[url scheme] isEqualToString:@"callback"])
	{
		// Do something interesting...
		NSString* s = [url absoluteString ];
		[s_view sendMessage :"onCallback" :[s UTF8String]];
		return NO;
	}
	return YES;
}

- (void)webViewDidStartLoad:(UIWebView *)webView
{
}

- (void)webViewDidFinishLoad:(UIWebView *)webView
{

}

- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error
{
	NSLog(@"myWebView : %@",error);
}

@end

#endif
