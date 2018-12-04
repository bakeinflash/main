//
//  ES1Renderer.mm
//
#import "ES1Renderer.h"
#import "EAGLView.h"

#include "base/container.h"
#include "base/tu_timer.h"
#include "bakeinflash/bakeinflash_value.h"
#include "bakeinflash/bakeinflash_root.h"

//#ifdef fDash
//	#include "FutureDashApp.h"
//#endif

// embeded fonts
//#include "myriadproregular_ttf.h"
//#include "myriadprobold_ttf.h"
//#include "mshei_ttf.h"
//#include "arial_ttf.h"
//#include "ariali_ttf.h"
//#include "arialbd_ttf.h"

void bakeinflash_advance(bool profile);
void bakeinflash_init(const char* workdir, const char* infile, int backingWidth, int backingHeight, float retina, bool gles2);
void bakeinflash_add_class(const tu_string& name, const bakeinflash::as_value& ctor);
void bakeinflash_setfile(const tu_string& name, const unsigned char* buf, int bufsize);
void bakeinflash_set_fontface(const tu_string& name);
void bakeinflash_keep_bitmaps_alive(bool alive);

typedef void (*bakeinflash_fscommand_callback)(const char* command, const char* arg);
void bakeinflash_register_fscommand_callback(bakeinflash_fscommand_callback handler);

static int s_initPhase = 0;
static bool s_has_stencil = false;
static bool s_has_msaa = false;
extern EAGLView* s_view;

BOOL hasExtension(NSString *searchName)
{
	// For performance, the array can be created once and cached.
	NSString *extensionsString = [NSString stringWithCString: (const char*) glGetString(GL_EXTENSIONS) encoding: NSASCIIStringEncoding];
	NSArray *extensionsNames = [extensionsString componentsSeparatedByString:@" "];
	return [extensionsNames containsObject: searchName];
}

@interface bakeinFlash : NSObject <UIApplicationDelegate>
+ (const char*) getWorkdir;
+ (const char*) getInfile;
+ (float) getActivityPos;
+ (int) getActivityStyle;
+ (bool) getBitmapAlive;
+ (void) setAutoOrientation :(bool) val;
+ (bool) getAutoOrientation;
+ (bool) isPortrait;
+ (const char*) getMemorySWF :(const unsigned char**) ptr :(int*) len;
+ (bool) getFont :(int) i :(const char**) fontname :(const char**) filename :(const unsigned char**) ptr :(int*) len;
+ (bool) getClass :(int) i :(const char**) classname : (bakeinflash::as_c_function_ptr*) ctor;
+ (bool) getMSAA;
+ (void) setMSAA :(bool) val;
+ (bool) getShowAdvance;
+ (const char*) getFlashVars;
+ (void) setFlashVars :(NSString*) str;
@end

static tu_string s_fontface;
static string_hash<tu_string> s_fontmap; 
void bakeinflash_set_fontface(const tu_string& name)
{
	s_fontface = name;
}

// "Myriad Pro"
void get_fontface(const tu_string& fontname, bool is_bold, bool is_italic, tu_string* key)
{
	if (s_fontface == "jp" || s_fontface == "cn")
	{
		*key = "jp-cn"; //is_bold ? "jpB" : "jp";
	}
	else
	{
		if (is_bold && is_italic)
		{
			*key = fontname + " Bold Italic";
		}
		else
		if (is_bold)
		{
			*key = fontname + " Bold";
		}
		else
		if (is_italic)
		{
			*key = fontname + " Italic";
		}
		else
		{
			*key = fontname;
		}
	}
	//printf("get_fontface %s %d %d %s %s\n", fontname.c_str(), is_bold, is_italic, key->c_str(),s_fontface.c_str());
}
	
bool get_fontfile(const tu_string& key, tu_string* fontfile)
{
	if (s_fontmap.get(key, fontfile))
	{
//		printf("get_fontfile: %s=%s\n", key.c_str(), fontfile->c_str());
//		*fontfile = tu_string([bakeinFlash getWorkdir]) + (*fontfile);
		return true;
	}
	*fontfile = "myriadproregular.ttf";
	return true;
}

static void	fs_callback(const char* command, const char* args)
// For handling notification callbacks from ActionScript.
{
//	printf("fscommand: %s %s\n", command, args);
	tu_string cmd = command;
	if (cmd == "setIdleTimer")
	{
		int interval = atoi(args);
		[s_view resetIdleTimer :interval];
		if (interval > 0)
		{
			[s_view startBackgroundTask];
		}
		else
		{
			[s_view stopBackgroundTask];
		}
	}
	else
	if (cmd == "wakeUp")
	{
		tu_string msg(args);
		tu_string snd;
		const char* p = strchr(args, ',');
		if (p)
		{
			msg = tu_string(args, (int) (p - args));
			snd = p + 1;
		}
		[s_view wakeUp :[NSString stringWithUTF8String: (char*) msg.c_str()] :[NSString stringWithUTF8String: (char*) snd.c_str()]];
	}
	else
	if (cmd == "setInterval")
	{
		int interval = atoi(args);
		if (interval >= 1 && interval <= 60)
		{
			[s_view setAnimationFrameInterval: interval];
		}
	}
	else
	if (cmd == "alert")
	{
	//	UIAlertView *alert = [[UIAlertView alloc] initWithTitle: @"" message: [NSString stringWithUTF8String: args] destructiveButtonTitle:nil, nil];
//		[alert show];
	//	[alert release];
	}
	else
	if (cmd == "setFontFace")
	{
		bakeinflash_set_fontface(args);
	}
  else
  if (cmd == "set_workdir")
  {
    bakeinflash::set_workdir(args);
  }
  else
  if (cmd == "set_max_volume")
  {
    // set max sound volume in percent, [0..100]
    // usefull for embedded games
    bakeinflash::sound_handler* s = bakeinflash::get_sound_handler();
    if (s)
    {
      int vol = atoi(args);
      s->set_max_volume(vol);
    }
  }

}

@implementation ES1Renderer

// Create an OpenGL ES 1.1 context
- (id)init
{
	if ((self = [super init]))
	{
		printf("iOS version: %s\n", [[[UIDevice currentDevice] systemVersion] UTF8String]);
		
		[self createContext];
		
		for (int i = 0; ;i++)
		{
			const char* fontname = NULL;
			const char* filename = NULL;
			const unsigned char* ptr = NULL;
			int len = 0;
			bool rc = [bakeinFlash getFont :i :&fontname :&filename :&ptr :&len];
			if (rc)
			{
				assert(filename);
				assert(fontname);
				s_fontmap.add(fontname, filename);
				if (len > 0 && ptr)
				{
					bakeinflash_setfile(filename, ptr, len);
				}
			}
			else
			{
				break;
			}
		}
	}
	
	return self;
}

-(void) createContext
{
	//		context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
	//		if (context)
	//		{
	//			s_hasES2 = true;
	//			[context release];
	//			context = nil;
	//		}
	context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
	if (!context || ![EAGLContext setCurrentContext:context])
	{
		NSLog(@"can't create openGLES context");
		exit(1);
	}
	
	defaultFramebuffer = 0;
	colorRenderbuffer = 0;
	depthRenderbuffer = 0;
	msaaFramebuffer = 0;
	msaaRenderBuffer = 0;
	msaaDepthBuffer = 0;
	
	// Create default framebuffer object. The backing will be allocated for the current layer in -resizeFromLayer
	glGenFramebuffers(1, &defaultFramebuffer);
	glGenRenderbuffers(1, &colorRenderbuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderbuffer);
	
	//Generate our MSAA Frame and Render buffers
	glGenFramebuffers(1, &msaaFramebuffer);
	glGenRenderbuffers(1, &msaaRenderBuffer);
	glGenRenderbuffers(1, &msaaDepthBuffer);
	
	// add DEPTH buffer
	glGenRenderbuffers(1, &depthRenderbuffer);
	
}

-(void) refresh
{
	bool val = [bakeinFlash getMSAA];
	val = !val;
	printf("MSAA %s\n", val ? "on" : "off");
	[bakeinFlash setMSAA :val];
}

bool exist(const char* file);

static UIActivityIndicatorView* activityIndicatorView = NULL;
-(void) showIndicator :(bool) show
{
	//
	// launch image
	//
/*
	if (show)
	{
		
		CGSize scr = [[UIScreen mainScreen] bounds].size;
		float scale = [[UIScreen mainScreen] scale];
		int w = (int) (scr.width * scale);
		int h = (int) (scr.height * scale);
		
		char loading[64];
		
		// try .png
		snprintf(loading, 64, "loading-%dx%d.png", w, h);
		printf("trying %s\n", loading);
		if (exist(loading) == false)
		{
			snprintf(loading, 64, "loading.png");
		}
		
		// try .jpg
		if (exist(loading) == false)
		{
			snprintf(loading, 64, "loading-%dx%d.jpg", w, h);
			printf("trying %s\n", loading);
			if (exist(loading) == false)
			{
				snprintf(loading, 64, "loading.jpg");
			}
		}

		if (exist(loading) == false)
		{
			// no image
            printf("no launch image %s\n", loading);
			return;
		}
        printf("using launch image %s\n", loading);

		s_view.myPreloader = [[UIImageView alloc] initWithImage: [UIImage imageNamed :[NSString stringWithUTF8String: loading] ]];
		s_view.myPreloader.contentMode = UIViewContentModeScaleAspectFit;
		bool isPortrait = [bakeinFlash isPortrait];
		if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
		{
			s_view.myPreloader.frame = isPortrait ? CGRectMake(0, 0, 768, 1024) : CGRectMake(0, 0, 1024, 768);
			
		}
		else
		{
//			s_view.myPreloader.frame = isPortrait ? CGRectMake(0, 0, 320, 480) : CGRectMake(0, 0, 480, 320);
            CGSize result = [[UIScreen mainScreen] bounds].size;
            if (result.height == 480)
            {
                // iPhone Classic
                s_view.myPreloader.frame = isPortrait ? CGRectMake(0, 0, 320, 480) : CGRectMake(0, 0, 480, 320);
            }
            else
            if (result.height == 568)
            {
                // iPhone 5
                s_view.myPreloader.frame = isPortrait ? CGRectMake(0, 0, 320, 1136/2) : CGRectMake(0, 0, 1136/2, 320);
            }
		}
		[s_view addSubview :s_view.myPreloader];
		
	}
	else
	{
		if (s_view.myPreloader)
		{
			[s_view.myPreloader removeFromSuperview];
			s_view.myPreloader = NULL;
		}
	
	}
	*/
  
	//
	// activity indicator
	//
	if ([bakeinFlash getActivityStyle] > 0)
	{
		if (show)
		{
			if (activityIndicatorView == NULL)
			{
				// create new dialog box view and components
				switch ([bakeinFlash getActivityStyle])
				{
					case 3:
						activityIndicatorView = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle: UIActivityIndicatorViewStyleWhiteLarge];
						break;
					case 1:
						activityIndicatorView = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle: UIActivityIndicatorViewStyleWhite];
						break;
					case 2:
						activityIndicatorView = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle: UIActivityIndicatorViewStyleGray];
						break;
				}
				
				float y = [bakeinFlash getActivityPos]; 
				
				// display it in the center of your view
				CGSize viewSize = s_view.bounds.size;
				activityIndicatorView.center = CGPointMake(viewSize.width * 0.5, viewSize.height * y);
				[s_view addSubview:activityIndicatorView];
				[activityIndicatorView startAnimating];
			}
		}
		else
		{
			if (activityIndicatorView != NULL)
			{
				[activityIndicatorView removeFromSuperview];
		//		[activityIndicatorView release];
				activityIndicatorView = NULL;
			}
		}
	}
}

- (BOOL)resizeFromLayer:(CAEAGLLayer *)layer
{
//#ifdef RETINA
	layer.contentsScale = [[UIScreen mainScreen] scale];
//#endif

	// Allocate color buffer backing based on the current layer size
	glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
	[context renderbufferStorage:GL_RENDERBUFFER fromDrawable:layer];
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &backingWidth);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backingHeight);
	
	//printf("opengl screen: %d %d\n", backingWidth, backingHeight);
	// add depth buffer
	
	s_has_stencil = hasExtension(@"GL_OES_packed_depth_stencil") & ![bakeinFlash getMSAA];
	if (s_has_stencil)
	{
		glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES, backingWidth, backingHeight);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
	}
	else
	{
		glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, backingWidth, backingHeight);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
	}

	
	// MSAA =============
	if ([bakeinFlash getMSAA])
	{
		s_has_msaa = true;
		
		//Bind our MSAA buffers
		glBindFramebuffer(GL_FRAMEBUFFER, msaaFramebuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, msaaRenderBuffer);
		
		// Generate the msaaDepthBuffer.
		// 4 will be the number of pixels that the MSAA buffer will use in order to make one pixel on the render buffer.
		glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER, 4, GL_RGBA8_OES, backingWidth, backingHeight);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaaRenderBuffer);
		
		//Bind the msaa depth buffer.
		glBindRenderbuffer(GL_RENDERBUFFER, msaaDepthBuffer);
		glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT16, backingWidth , backingHeight);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, msaaDepthBuffer);
	}
	
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		NSLog(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
		assert(0);
	}
	
	// render to
	[EAGLContext setCurrentContext:context];

	//	NSLog(@"screen size: %dx%d", backingWidth, backingHeight);
	return YES;
}

- (void)render
{
	int t_advance = tu_timer::get_ticks();
	
	// MSAA =============
	if (s_has_msaa && [bakeinFlash getMSAA])
	{
		glBindFramebuffer(GL_FRAMEBUFFER, msaaFramebuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, msaaRenderBuffer);
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
	}
	
	//  must be иначе блики и полсы бегут на айпеде с айфон приложением
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	if (s_initPhase == 0)
	{
		[self showIndicator :true];
		s_initPhase++;
	}
	else
		if (s_initPhase == 1)
		{
			s_initPhase++;
			
			for (int i = 0; ;i++)
			{
				const char* classname = NULL;
				bakeinflash::as_c_function_ptr ctor;
				bool rc = [bakeinFlash getClass :i :&classname :&ctor];
				if (rc)
				{
					assert(classname);
					assert(ctor);
					bakeinflash_add_class(classname, ctor);
				}
				else
				{
					break;
				}
			}
			
			bakeinflash_register_fscommand_callback(fs_callback);
			
			bool val = [bakeinFlash getBitmapAlive];
			bakeinflash_keep_bitmaps_alive(val);
			
			float scale = [[UIScreen mainScreen] scale];
			bakeinflash_init([bakeinFlash getWorkdir], [bakeinFlash getInfile], backingWidth, backingHeight, scale, s_has_stencil);
            bakeinflash::set_flash_vars([bakeinFlash getFlashVars]);
		}
		else
		{
			bakeinflash_advance([bakeinFlash getShowAdvance]);
			[self showIndicator :false];
		}
	
	t_advance = tu_timer::get_ticks();
	//	glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
	if (s_view.isAnimating)
	{
		
		// MSAA =============
		if (s_has_msaa && [bakeinFlash getMSAA])
		{
			// Apple (and the khronos group) encourages you to discard depth
			// render buffer contents whenever is possible
			GLenum attachments[] = {GL_DEPTH_ATTACHMENT};
			glDiscardFramebufferEXT(GL_READ_FRAMEBUFFER_APPLE, 1, attachments);
			
			//Bind both MSAA and View FrameBuffers.
			glBindFramebuffer(GL_READ_FRAMEBUFFER_APPLE, msaaFramebuffer);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER_APPLE, defaultFramebuffer);
			
			// Call a resolve to combine both buffers
			glResolveMultisampleFramebufferAPPLE();
			
			// Present final image to screen
			glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
		}
		
		[context presentRenderbuffer:GL_RENDERBUFFER];
	}
	//	printf("presentRenderbuffer time: %d\n", tu_timer::get_ticks() - t_advance);
}

- (void) freeContext
{
	// Tear down GL
	if (defaultFramebuffer)
	{
		glDeleteFramebuffers(1, &defaultFramebuffer);
		defaultFramebuffer = 0;
	}
	if (colorRenderbuffer)
	{
		glDeleteRenderbuffers(1, &colorRenderbuffer);
		colorRenderbuffer = 0;
	}
	if (depthRenderbuffer)
	{
		glDeleteRenderbuffers(1, &depthRenderbuffer);
		depthRenderbuffer = 0;
	}
	if (msaaFramebuffer)
	{
		glDeleteRenderbuffers(1, &msaaFramebuffer);
		msaaFramebuffer = 0;
	}
	if (msaaRenderBuffer)
	{
		glDeleteRenderbuffers(1, &msaaRenderBuffer);
		msaaRenderBuffer = 0;
	}
	if (msaaDepthBuffer)
	{
		glDeleteRenderbuffers(1, &msaaDepthBuffer);
		msaaDepthBuffer = 0;
	}
	
	// Tear down context
	if ([EAGLContext currentContext] == context)
	{
		[EAGLContext setCurrentContext:nil];
	}
	
	//	[context release];
	context = nil;
	
}

- (void)dealloc
{
	[self freeContext];
	[super dealloc];
}

@end
