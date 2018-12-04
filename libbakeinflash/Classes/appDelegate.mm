//
//  appDelegate.m
//  Copyright BakeInFlash 2011. All rights reserved.
//

#import "appDelegate.h"

#include "base/container.h"
#include "bakeinflash/bakeinflash_value.h"
#include "bakeinflash/bakeinflash_root.h"
#include <stdio.h>
#include <unistd.h>

char s_infile[64];
char s_workdir[1024];
tu_string s_flash_vars;

@implementation bakeinFlash

@synthesize window;
@synthesize glView;
@synthesize glViewController;

+ (const char*) getFlashVars
{
  return s_flash_vars.c_str();
}
+ (void) setFlashVars :(NSString*) str
{
  s_flash_vars = [str UTF8String];
}

+ (const char*) getWorkdir
{
  return s_workdir;
}

+ (const char*) getInfile
{
  return s_infile;
}

static NSString* s_memswf = NULL;
static const unsigned char* s_memptr = NULL;
static int s_memlen = 0;
+ (const char*) getMemorySWF :(const unsigned char**) ptr :(int*) len
{
  *ptr = s_memptr;
  *len = s_memlen;
  return [s_memswf UTF8String];
}
+ (void) setMemorySWF :(NSString*) str :(unsigned char*) ptr :(int) len
{
  s_memswf = str;
  s_memptr = ptr;
  s_memlen = len;
}

static int s_fit_mode = 0;
+ (int) getFitMode
{
  return s_fit_mode;
}
+ (void) setFitMode :(int) mode
{
  s_fit_mode = mode;
}

static int s_interval = 2;
+ (int) getInterval
{
  return s_interval;
}
+ (void) setInterval :(int) val
{
  s_interval = val;
}

static bool s_zoom_gesture = false;
+ (bool) getZoomGesture
{
  return s_zoom_gesture;
}
+ (void) setZoomGesture :(bool) val
{
  s_zoom_gesture = val;
}

static bool s_show_adance = false;
+ (bool) getShowAdvance
{
  return s_show_adance;
}
+ (void) setShowAdvance :(bool) val
{
  s_show_adance = val;
}

static bool s_has_msaa = false;
+ (bool) getMSAA
{
  return s_has_msaa;
}
+ (void) setMSAA :(bool) val
{
  s_has_msaa = val;
}

static bool s_rotation_gesture = false;
+ (bool) getRotationGesture
{
  return s_rotation_gesture;
}
+ (void) setRotationGesture :(bool) val
{
  s_rotation_gesture = val;
}

+ (void) setData :(NSString*) str
{
  
  NSString* curPath = [[NSBundle mainBundle] bundlePath];
  snprintf(s_workdir, 1024, "%s/", [curPath UTF8String]);
  
  const unsigned char* ptr = NULL;
  int len = 0;
  const char* swf_filename = [bakeinFlash getMemorySWF :&ptr :&len];
  if (swf_filename)
  {
    //bakeinflash_setfile(swf_filename, ptr, len);
    //printf("memory input file: %s%s\n", s_workdir, swf_filename);
    assert(0);	// fixme
  }
  else
  {
    const char* s = [str UTF8String];
    char swf[1024];
    strncpy(swf, s, 1024);
    int n = strlen(swf);
    swf[n - 4] = 0;
    const char* ext = swf + n - 3;
    
    CGSize scr = [[UIScreen mainScreen] bounds].size;
    float scale = [[UIScreen mainScreen] scale];
    int w = (int) (scr.width * scale);
    int h = (int) (scr.height * scale);
    
    char url[1024];
    snprintf(s_infile, 64, "%s-%dx%d.%s", swf, w, h, ext);
    snprintf(url, 1024, "%s%s", s_workdir, s_infile);
    //	printf("trying %s\n", s_infile);
    if (exist(url))
    {
      printf("using %s\n", s_infile);
      return;
    }
    
    snprintf(s_infile, 64, "%s.%s", swf, ext);
    snprintf(url, 1024, "%s%s", s_workdir, s_infile);
    //	printf("trying %s\n", s_infile);
    if (exist(url))
    {
      printf("using %s\n", s_infile);
      return;
    }
    printf("no %s\n", s_infile);
    exit(0);
  }
}

static float s_activityPos = 0.5f;
+ (float) getActivityPos
{
  return s_activityPos;
}
+ (void) setActivityPos :(float) val
{
  s_activityPos = val;
}


static int s_activityStyle = 0;
+ (int) getActivityStyle
{
  return s_activityStyle;
}
+ (void) setActivityStyle :(int) val
{
  s_activityStyle = val;
}


static bool s_bitmapAlive = true;
+ (bool) getBitmapAlive
{
  return s_bitmapAlive;
}
+ (void) setBitmapAlive :(bool) val
{
  s_bitmapAlive = val;
}

static bool s_auto_orientation = true;
+ (bool) getAutoOrientation
{
  return s_auto_orientation;
}
+ (void) setAutoOrientation :(bool) val
{
  s_auto_orientation = val;
}

static int s_appwidth = 0;
static int s_appheight = 0;
static int s_fps = 0;
+ (bool) isPortrait
{
  // first time ?
  if (s_fps == 0)
  {
    char url[512];
    snprintf(url, 512, "%s%s", s_workdir, s_infile);
    bakeinflash::get_fileinfo(url, &s_appwidth, &s_appheight, &s_fps);
    s_interval = int(60.0 / s_fps);
    printf("%s: w=%d h=%d fps=%d, interval=%d\n", url, s_appwidth, s_appheight, s_fps, s_interval);
  }
  return s_appwidth <= s_appheight;
}

// hack
static int s_class_count = 0;
static  NSString* s_class_name[100];
static  bakeinflash::as_c_function_ptr s_class_ctor[100];
+ (void) addClass :(NSString*) classname :(bakeinflash::as_c_function_ptr) ctor
{
  if (s_class_count < 100)
  {
    s_class_name[s_class_count] = classname;
    s_class_ctor[s_class_count] = ctor;
    s_class_count++;
  }
}
+ (bool) getClass :(int) i :(const char**) classname : (bakeinflash::as_c_function_ptr*) ctor
{
  if (i < s_class_count)
  {
    *classname = [s_class_name[i] UTF8String];
    *ctor = s_class_ctor[i];
    return true;
  }
  return false;
}

// hack
static int s_font_count = 0;
static  NSString* s_font_name[10];
static  NSString* s_font_file[10];
static  const unsigned char* s_font_ptr[10];
static  int s_font_size[10];
+ (void) addFont :(NSString*) fontname :(NSString*) filename :(const unsigned char*) ptr :(int) size
{
  if (s_font_count < 10)
  {
    s_font_name[s_font_count] = fontname;
    s_font_file[s_font_count] = filename;
    s_font_ptr[s_font_count] = ptr;
    s_font_size[s_font_count] = size;
    s_font_count++;
  }
}
+ (bool) getFont :(int) i :(const char**) fontname :(const char**) filename :(const unsigned char**) ptr :(int*) len
{
  if (i < s_font_count)
  {
    *fontname = [s_font_name[i] UTF8String];
    *filename = [s_font_file[i] UTF8String];
    *ptr = s_font_ptr[i];
    *len = s_font_size[i];
    return true;
  }
  return false;
}


- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
		[UIApplication sharedApplication].statusBarHidden = YES;
  CGRect b = [[UIScreen mainScreen] bounds];
		window = [[UIWindow alloc] initWithFrame: b];
  
		glViewController = [[myViewController alloc] init];
		window.rootViewController = glViewController;
  
		/*bool isPortrait = [bakeinFlash isPortrait];
		CGRect frame;
		if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
    {
      frame = isPortrait ? CGRectMake(0, 0, 768, 1024) : CGRectMake(0, 0, 1024, 768);
    }
    else
    {
      frame = isPortrait ? CGRectMake(0, 0, 320, 480) : CGRectMake(0, 0, 1000, 300);
    }
     */
		glView = [[EAGLView alloc] initWithFrame: b];
		glViewController.view = glView;
  
		[window addSubview :glView];
		[window makeKeyAndVisible];
  
  [glView startAnimation];
  
  /*
   NSLog(@"Registering for push notifications...");
   [[UIApplication sharedApplication]
   registerForRemoteNotificationTypes:
   (UIRemoteNotificationTypeAlert |
   UIRemoteNotificationTypeBadge |
   UIRemoteNotificationTypeSound)];
   */
  
  // register to observe notifications from the store
  [[NSNotificationCenter defaultCenter]
   addObserver: self
   selector: @selector (storeDidChange:)
   name: NSUbiquitousKeyValueStoreDidChangeExternallyNotification
   object: [NSUbiquitousKeyValueStore defaultStore]];
  
  // get changes that might have happened while this
  // instance of your app wasn't running
  [[NSUbiquitousKeyValueStore defaultStore] synchronize];
  
  
  return YES;
}

//- (void)application:(UIApplication *)application didReceiveLocalNotification:(UILocalNotification *)notification
//{
//  printf("didReceiveLocalNotification\n");
//}

//- (void)application:(UIApplication *)app didRegisterForRemoteNotificationsWithDeviceToken:(NSData *)deviceToken
//{
//  NSString *str = [NSString stringWithFormat:@"Device Token=%@",deviceToken];
//  NSLog(@"%@", str);
//}

//- (void)application:(UIApplication *)app didFailToRegisterForRemoteNotificationsWithError:(NSError *)err {
  
  //	NSString *str = [NSString stringWithFormat: @"Error: %@", err];
  //	NSLog(str);
  
//}

//- (void)application:(UIApplication *)application didReceiveRemoteNotification:(NSDictionary *)userInfo
//{
 // for (id key in userInfo)
 // {
 //   NSLog(@"key: %@, value: %@", key, [userInfo objectForKey:key]);
 // }
  
//}


- (void)applicationDidEnterBackground:(UIApplication *)application
{
  [glView stopAnimation];
}


- (void)applicationDidBecomeActive:(UIApplication *)application
{
  [glView startAnimation];
}

- (void)applicationWillTerminate:(UIApplication *)application
{
  [glView stopAnimation];
}

- (void)dealloc
{
  [window release];
  [glView release];
  
  [super dealloc];
}


+ (void)initialize
{
  // Initialize your static variables here...
  if (self == [bakeinFlash class])
  {
  }
  
}

@end

