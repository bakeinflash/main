//
//  EAGLView.m
//  test
//
//  Created by Work on 04.07.10.
//  Copyright __MyCompanyName__ 2010. All rights reserved.
//

#import "EAGLView.h"
#import "ES1Renderer.h"

#include "base/container.h"
#include "base/tu_timer.h"
#include "bakeinflash/bakeinflash_value.h"
#include "bakeinflash/bakeinflash_root.h"
#include "bakeinflash/bakeinflash_text_.h"
#include "bakeinflash/bakeinflash_as_classes/as_iOS_.h"

EAGLView* s_view = NULL;
myViewController* s_myController = NULL;
static bool s_resetEScontext = false;

extern float s_scale;
extern int s_x0;
extern int s_y0;
extern float s_retina;

void onMouseDown(float x, float y);
void onMouseUp(float x, float y);
void onMouseMove(float x, float y, int flg);


@interface bakeinFlash : NSObject <UIApplicationDelegate>
+ (bool) getAutoOrientation;
+ (bool) isPortrait;
+ (bool) getZoomGesture;
+ (bool) getRotationGesture;
+ (int) getInterval;
+ (bool) getMSAA;
+ (void) setMSAA :(bool) val;
@end

//
// myViewController
//

bakeinflash::as_object* get_global();

void sendMessage(const char* name, const bakeinflash::as_value& arg)
{
		bakeinflash::as_value function;
        bakeinflash::as_object* obj = bakeinflash::get_global();
		if (obj && obj->get_member("sendMessage", &function))
		{
			bakeinflash::as_environment env;
			env.push(arg);
			env.push(name);
			bakeinflash::as_value val = bakeinflash::call_method(function, &env, obj, 2, env.get_top_index());
		}	

}

@implementation myViewController

- (void)didReceiveMemoryWarning
{ 
  // default behavior is to release the view if it doesn't have a superview.
	/*
	UIAlertView *alert = [[[UIAlertView alloc] initWithTitle:@"Warning"  message:@"App is running low on memory" delegate:nil cancelButtonTitle:@"Ok" otherButtonTitles:nil, nil] autorelease];
	
	[alert show];
	
	
  // remember to clean up anything outside of this view's scope, such as
  // data cached in the class instance and other global data.
  [super didReceiveMemoryWarning];
    */
    
    sendMessage("onLowMemory", 0);
}

- (id)init
{
	if ((self = [super init]))
	{
//		printf("myViewController\n");
		s_myController = self;
	}
	return self;
}

//
// iOS6
//
 - (BOOL)shouldAutorotate
{
	return [self shouldAutorotateToInterfaceOrientation:self.interfaceOrientation];
}

- (NSUInteger)supportedInterfaceOrientations
{
	//decide number of origination tob supported by Viewcontroller.
	return UIInterfaceOrientationMaskAll;
}

// Returns interface orientation masks.
//- (UIInterfaceOrientation)preferredInterfaceOrientationForPresentation
//{
//	return UIInterfaceOrientationPortraitUpsideDown;
//}

//
// iOS5
//
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
	if ([bakeinFlash getAutoOrientation] == false)
	{
		return false;
	}
	
	if ([bakeinFlash isPortrait])
	{
		return (interfaceOrientation == UIInterfaceOrientationPortrait ||	interfaceOrientation == UIInterfaceOrientationPortraitUpsideDown);
	}

	// Return YES for supported orientations
	return (interfaceOrientation == UIInterfaceOrientationLandscapeLeft || interfaceOrientation == UIInterfaceOrientationLandscapeRight);
}

- (void)viewDidLoad
{
	[super viewDidLoad];
	printf("viewDidLoad\n");
}

//- (void)motionEnded:(UIEventSubtype)motion withEvent:(UIEvent *)event
//{
//	NSLog(@"Shake happend …");
//}

- (void)viewDidAppear:(BOOL)animated
{
	printf("viewDidAppear\n");
//	[self.view becomeFirstResponder];
//	[super viewDidAppear :animated];
}

//- (void)viewWillDisappear:(BOOL)animated
//{
//	[self.view resignFirstResponder];
//	[super viewWillDisappear :animated];
//}

@end

//
// EAGLView
//

@implementation EAGLView

@synthesize animating;
@synthesize myPreloader;
@synthesize panGesture;
@synthesize pinchGesture;
@synthesize longGesture;

@dynamic animationFrameInterval;


// You must implement this method
+ (Class)layerClass
{
	return [CAEAGLLayer class];
}

//The EAGL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:
//- (id)initWithCoder:(NSCoder*)coder
- (id)initWithFrame:(CGRect)frame; 
{    
	[super initWithFrame :frame];
	//	if ((self = [super initWithCoder:coder]))
	{
		// Get the layer
		CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
		
		eaglLayer.opaque = TRUE;
		eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
																		[NSNumber numberWithBool:FALSE], kEAGLDrawablePropertyRetainedBacking, kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
		
		
		
		renderer = [[ES1Renderer alloc] init];
		
		if (!renderer)
		{
		  [self release];
			return nil;
		}
		
		animating = FALSE;
		displayLinkSupported = FALSE;
		animationFrameInterval = [bakeinFlash getInterval];	// 60/12=5fps
		displayLink = nil;
		animationTimer = nil;
		
		// A system version of 3.1 or greater is required to use CADisplayLink. The NSTimer
		// class is used as fallback when it isn't available.
		NSString *reqSysVer = @"3.1";
		NSString *currSysVer = [[UIDevice currentDevice] systemVersion];
		if ([currSysVer compare:reqSysVer options:NSNumericSearch] != NSOrderedAscending)
			displayLinkSupported = TRUE;
	}

	//The tap gesture
/*	
	tapGesture = [[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTap:)] autorelease];
	[tapGesture setDelegate:self];
	[self addGestureRecognizer :tapGesture];
	
	
 //The pan gesture
	panGesture = [[[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(panGestureMoveAround:)] autorelease];
	[panGesture setMaximumNumberOfTouches :1];
	[panGesture setMinimumNumberOfTouches :1];
	[panGesture setDelegate:self];
	[self addGestureRecognizer :panGesture];
	*/
	
	// Two finger rotate  
	if ([bakeinFlash getRotationGesture] == true)
	{
		twoFingersRotate = [[[UIRotationGestureRecognizer alloc] initWithTarget:self action:@selector(twoFingersRotate:)] autorelease];
		[self addGestureRecognizer :twoFingersRotate];	
	}
	
	//The pinch gesture
	if ([bakeinFlash getZoomGesture] == true)
	{
		pinchGesture = [[[UIPinchGestureRecognizer alloc] initWithTarget :self action :@selector(pinchGesture:)] autorelease];
		[pinchGesture setDelegate:self];
		[self addGestureRecognizer :pinchGesture];
	}
	
//	longGesture = [[[UILongPressGestureRecognizer alloc] initWithTarget:self action: @selector(handleLongPress:)]  autorelease];
//	longGesture.minimumPressDuration = 2.0; //seconds
//	[self addGestureRecognizer :longGesture];
	
	return self;
}

/*
- (void)handleTap:(UITapGestureRecognizer*) gestureRecognizer
{    
	CGPoint pp = [gestureRecognizer locationInView :self];
	printf("xxxx %f %f\n", pp.x, pp.y);
	if (gestureRecognizer.state == UIGestureRecognizerStateEnded)    
	{ 
		// handling code  
		
	} 
}

-(void)panGestureMoveAround:(UIPanGestureRecognizer*) gestureRecognizer
{
	if (myPreloader == NULL)
	{
		float scale = [[UIScreen mainScreen] scale];
		CGPoint pp = [gestureRecognizer locationInView :self];
		switch ([gestureRecognizer state])
		{
			case UIGestureRecognizerStateBegan:
			case UIGestureRecognizerStateChanged:
			{
				onMouseMove(pp.x * scale, pp.y * scale, 1);
				[renderer render];
				//	printf("move x=%f, y=%f\n", touchPos.x * scale, touchPos.y * scale);
				break;
			}
			case UIGestureRecognizerStateEnded:
			{
				onMouseMove(pp.x * scale, pp.y * scale, 1);
				[renderer render];
				onMouseUp(pp.x * scale, pp.y * scale);
				[renderer render];
				break;
			}
			case UIGestureRecognizerStatePossible:
			case UIGestureRecognizerStateCancelled:
			case UIGestureRecognizerStateFailed:
				break;
		}
	}
}
*/

-(void)handleLongPress:(UILongPressGestureRecognizer *)gestureRecognizer
{
	switch ([gestureRecognizer state])
	{
		case UIGestureRecognizerStateBegan:
		{
			s_resetEScontext = true;
			break;
		}
		case UIGestureRecognizerStateChanged:
		case UIGestureRecognizerStateEnded:
		case UIGestureRecognizerStatePossible:
		case UIGestureRecognizerStateCancelled:
		case UIGestureRecognizerStateFailed:
			break;
	}

}


- (void) pinchGesture:(UIPinchGestureRecognizer*) gestureRecognizer
{
	static float s_prev_zoom = 1.0f;
	float z = [gestureRecognizer scale];
	
	CGPoint pp = [gestureRecognizer locationInView :self];
	float x = pp.x * s_retina;
	float y = pp.y * s_retina;
	
	switch ([gestureRecognizer state])
	{
		case UIGestureRecognizerStateBegan:
		{
			s_prev_zoom = z;
			
			bakeinflash::as_object* obj = new bakeinflash::as_object();
			obj->set_member("x", (x - s_x0) / s_scale);
			obj->set_member("y", (y - s_y0) / s_scale);
			obj->set_member("z", z);
			
			sendMessage("onZoomBegan", obj);
		//	[gestureRecognizer setScale:1];
			break;
		}
		case UIGestureRecognizerStateChanged:
		{
			bakeinflash::as_object* obj = new bakeinflash::as_object();
			obj->set_member("x", (x - s_x0) / s_scale);
			obj->set_member("y", (y - s_y0) / s_scale);
			obj->set_member("z", z - s_prev_zoom);
			sendMessage("onZoom", obj);
			
			[renderer render];
			s_prev_zoom = z;
			
			break;
		}
		case UIGestureRecognizerStateEnded:
		{
			bakeinflash::as_object* obj = new bakeinflash::as_object();
			obj->set_member("x", (x - s_x0) / s_scale);
			obj->set_member("y", (y - s_y0) / s_scale);
			obj->set_member("z", s_prev_zoom);
			sendMessage("onZoomEnded", obj);
			break;
		}
		case UIGestureRecognizerStatePossible:
		case UIGestureRecognizerStateCancelled:
		case UIGestureRecognizerStateFailed:
			break;
	}
}

// Two finger rotate   
- (void) twoFingersRotate:(UIRotationGestureRecognizer*) gestureRecognizer 
{
  // Convert the radian value to show the degree of rotation
	static float s_prev_angle = 0.0f;
	float a = [gestureRecognizer rotation] * (180 / M_PI);
	switch ([gestureRecognizer state])
	{
		case UIGestureRecognizerStateBegan:
			s_prev_angle = a;
			sendMessage("onRotationBegan", a);
			break;
		case UIGestureRecognizerStateChanged:
			sendMessage("onRotation",  a - s_prev_angle);
			s_prev_angle = a;
			break;
		case UIGestureRecognizerStateEnded:
			sendMessage("onRotationEnded", s_prev_angle);
			break;
		case UIGestureRecognizerStatePossible:
		case UIGestureRecognizerStateCancelled:
		case UIGestureRecognizerStateFailed:
			break;
	}
}

- (void)stopBackgroundTask
{
	stop_background_task = true;
}

- (void)startBackgroundTask
{
	//	printf("applicationDidEnterBackground\n");
	
	//Check if our iOS version supports multitasking I.E iOS 4
	if ([[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)]) 
	{ 
		//Check if device supports mulitasking
    if ([[UIDevice currentDevice] isMultitaskingSupported]) 
		{
			//Get the shared application instance
			UIApplication *application = [UIApplication sharedApplication];
			
			background_task = [application beginBackgroundTaskWithExpirationHandler: ^
												 {
													 //Tell the system that we are done with the tasks
													 [application endBackgroundTask: background_task]; 
													 
													 //Set the task to be invalid
													 background_task = UIBackgroundTaskInvalid;
													 
													 //System will be shutting down the app at any point in time now
												 }];
			
			//Background tasks require you to use asyncrous tasks
			dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^
										 {
											 //Perform your tasks that your application requires
											 //NSLog(@"Running in the background!\n");
											 stop_background_task = false;
											 while (stop_background_task == false)
											 {
												 [NSThread sleepForTimeInterval: 1.0];	// sec
											 }
											 
											// NSLog(@"stop background\n");
											 
											 
											 //End the task so the system knows that you are done with what you need to perform
											 [application endBackgroundTask: background_task]; 
											 
											 //Invalidate the background_task
											 background_task = UIBackgroundTaskInvalid;
										 });
    }
	}
}


- (void)drawView:(id)sender
{
	s_view = self;
	
	if (s_resetEScontext)
	{
		s_resetEScontext = false;
		[renderer refresh];
	}
	[renderer render];
}

- (void)layoutSubviews
{
	[renderer resizeFromLayer:(CAEAGLLayer*)self.layer];
	[self drawView:nil];
	
	[[UIApplication sharedApplication] setStatusBarHidden:YES animated:NO];
}

- (NSInteger)animationFrameInterval
{
	return animationFrameInterval;
}

- (void)setAnimationFrameInterval:(NSInteger)frameInterval
{
	// Frame interval defines how many display frames must pass between each time the
	// display link fires. The display link will only fire 30 times a second when the
	// frame internal is two on a display that refreshes 60 times a second. The default
	// frame interval setting of one will fire 60 times a second when the display refreshes
	// at 60 times a second. A frame interval setting of less than one results in undefined
	// behavior.
	if (frameInterval >= 1)
	{
		animationFrameInterval = frameInterval;
		
		if (animating)
		{
			[self stopAnimation];
			[self startAnimation];
		}
	}
}

- (void)startAnimation
{
	if (!animating)
	{
		if (displayLinkSupported)
		{
			// CADisplayLink is API new to iPhone SDK 3.1. Compiling against earlier versions will result in a warning, but can be dismissed
			// if the system version runtime check for CADisplayLink exists in -initWithCoder:. The runtime check ensures this code will
			// not be called in system versions earlier than 3.1.
			
			displayLink = [NSClassFromString(@"CADisplayLink") displayLinkWithTarget:self selector:@selector(drawView:)];
			[displayLink setFrameInterval:animationFrameInterval];
			[displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
		}
		else
			animationTimer = [NSTimer scheduledTimerWithTimeInterval:(NSTimeInterval)((1.0 / 60.0) * animationFrameInterval) target:self selector:@selector(drawView:) userInfo:nil repeats:TRUE];
		
		animating = TRUE;
    sendMessage("onResume", bakeinflash::as_value());
	}
}

- (void)stopAnimation
{
	if (animating)
	{
		if (displayLinkSupported)
		{
			[displayLink invalidate];
			displayLink = nil;
		}
		else
		{
			[animationTimer invalidate];
			animationTimer = nil;
		}
		
		animating = FALSE;
    sendMessage("onPause", bakeinflash::as_value());
	}
}

- (void)dealloc
{
	[renderer release];
	[super dealloc];
}


- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	if (myPreloader == NULL)
	{
		float scale = [[UIScreen mainScreen] scale];
//		NSSet *allTouches = [event allTouches];
//		if ([allTouches count] == 1)
		{
			UITouch *t = [[touches allObjects] objectAtIndex:0];
			CGPoint touchPos = [t locationInView:t.view];
			onMouseMove(touchPos.x * scale, touchPos.y * scale, 0);
			[renderer render];
			onMouseDown(touchPos.x * scale, touchPos.y * scale);
	//		[renderer render];
			//printf("began x=%f, y=%f\n", touchPos.x * scale, touchPos.y * scale);
		}
	}
}


- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event 
{
	if (myPreloader == NULL)
	{
		float scale = [[UIScreen mainScreen] scale];
		NSSet *allTouches = [event allTouches];
		if ([allTouches count] == 1)
		{
			UITouch *t = [[touches allObjects] objectAtIndex:0];
			CGPoint touchPos = [t locationInView:t.view];
			onMouseMove(touchPos.x * scale, touchPos.y * scale, 1);
		//	[renderer render];
		//	printf("move x=%f, y=%f\n", touchPos.x * scale, touchPos.y * scale);
		}
	}
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	if (myPreloader == NULL)
	{
		float scale = [[UIScreen mainScreen] scale];
//		NSSet *allTouches = [event allTouches];
//		if ([allTouches count] == 1)
		{
			UITouch *t = [[touches allObjects] objectAtIndex:0];
			CGPoint touchPos = [t locationInView:t.view];
			
			onMouseMove(touchPos.x * scale, touchPos.y * scale, 1);
			[renderer render];
			onMouseUp(touchPos.x * scale, touchPos.y * scale);
			//[renderer render];
		//	printf("end x=%f, y=%f\n", touchPos.x * scale, touchPos.y * scale);
		}
	}
}


- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
	myTextField* fld	= objc_dynamic_cast(myTextField, textField);
	if (fld)
	{
		bakeinflash::character* obj = (bakeinflash::character*) fld.parent;	// hack, unsafe
		bakeinflash::as_value function;
		if (obj->get_member("onChanged", &function))
		{
			bakeinflash::as_environment env;
			env.push(fld.parent);
			bakeinflash::as_value val = bakeinflash::call_method(function, &env, obj, 1, env.get_top_index());
		}
		
		[renderer render];
	}
	
	[textField resignFirstResponder];
	return NO;
}

int movementDistance = 0;
- (void)textFieldDidBeginEditing:(UITextField *)textField
{
  
  myTextField* fld	= objc_dynamic_cast(myTextField, textField);
	if (fld)
	{
		[fld onFocus];
	}
	
  
	int kbd_origin = UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone ? 265 : 600;
	movementDistance = textField.frame.origin.y - kbd_origin + textField.frame.size.height; // tweak as needed
	if (movementDistance > 0)
	{
		[self animateTextField: textField up: YES];
	}
}


- (void)textFieldDidEndEditing:(UITextField *)textField
{
	if (movementDistance > 0)
	{
		[self animateTextField: textField up: NO];
	}
}

- (void) animateTextField: (UITextField*) textField up: (BOOL) up
{
	const float movementDuration = 0.3f; // tweak as needed
	int movement = (up ? -movementDistance : movementDistance);
	[UIView beginAnimations: @"anim" context: nil];
	[UIView setAnimationBeginsFromCurrentState: YES];
	[UIView setAnimationDuration: movementDuration];
	textField.frame = CGRectOffset(textField.frame, 0, movement);
	[UIView commitAnimations];
}

// limit input length
- (BOOL)textField:(UITextField *) textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
	int maxLength = -1;
	myTextField* fld	= objc_dynamic_cast(myTextField, textField);
	if (fld)
	{
		maxLength = [fld getMaxLength];
	}
	
	if (maxLength < 0)
	{
		return true;
	}
	
	NSUInteger oldLength = [textField.text length];
	NSUInteger replacementLength = [string length];
	NSUInteger rangeLength = range.length;
	NSUInteger newLength = oldLength - rangeLength + replacementLength;
	return newLength <= maxLength;
}


- (void)resetIdleTimer: (int) interval
{
	idleInterval = interval;
//	printf("resetIdleTimer idleTimer %d\n", interval);
	
	//	UIApplication *app = [UIApplication sharedApplication];
	//	UIApplicationState state = [app applicationState];
	//	if (state == UIApplicationStateActive)
	//	{
	//	}
	
	if (idleInterval == 0)
	{
		if (idleTimer)
		{
			[idleTimer release];
			idleTimer = nil;
		}
		return;
	}
	
	if (!idleTimer) 
	{
		idleTimer = [[NSTimer scheduledTimerWithTimeInterval: idleInterval
																									target:self
																								selector:@selector(idleTimerExceeded)
																								userInfo:nil
																								 repeats:NO] retain];
	}
	else
	{
		if (fabs([idleTimer.fireDate timeIntervalSinceNow]) < idleInterval - 1.0) 
		{
			[idleTimer setFireDate:[NSDate dateWithTimeIntervalSinceNow: idleInterval]];
		}
	}
}

- (void)idleTimerExceeded 
{
//	printf("idleTimerExceeded\n");
	[idleTimer release];
	idleTimer = nil;
	
	if (idleInterval > 0)
	{
    sendMessage("onIdleTimer", bakeinflash::as_value());
		[self resetIdleTimer :idleInterval];
	}
}

//- (UIResponder *)nextResponder 
//{
//	[self resetIdleTimer];
//	return [super nextResponder];
//}

- (void)wakeUp :(NSString*) msg :(NSString*) snd
{ 
	[[UIApplication sharedApplication] cancelAllLocalNotifications];

	UILocalNotification* alarm = [[[UILocalNotification alloc] init] autorelease];
	alarm.fireDate = [[NSDate date] dateByAddingTimeInterval: 0.0];
	alarm.timeZone = [NSTimeZone defaultTimeZone];
	alarm.repeatInterval = 0;
	alarm.alertBody = msg;
	alarm.soundName =  snd;
	UIApplication *app = [UIApplication sharedApplication];
	[app scheduleLocalNotification:alarm];
}

- (void)leaderboardViewControllerDidFinish:(GKLeaderboardViewController *)viewController
{
	[viewController dismissModalViewControllerAnimated:YES];
	sendMessage("onHideLeaderBoard", "");
}

// Dismisses the email composition interface when users tap Cancel or Send. Proceeds to update the message field with the result of the operation.
- (void)mailComposeController:(MFMailComposeViewController*)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError*)error
{ 
	const char* str = "Unknown Error";
	switch (result)
	{
		case MFMailComposeResultCancelled:
			str = "Cancelled";
			break;
		case MFMailComposeResultSaved:
			str = "Saved";
			break;
		case MFMailComposeResultSent:
			str = "Sent";
			break;
		case MFMailComposeResultFailed:
			str = "Failed";
			break;
			
		default:
		{
			UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Email" message:@"Sending Failed - Unknown Error :-("
																										 delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
			[alert show];
			[alert release];
			
			break;
		}
	}
	[controller dismissModalViewControllerAnimated:YES];
	sendMessage("onMailDidFinish", str);
}

// OpenGL ES view snapshot. Call this method after you draw and before -presentRenderbuffer:.
- (UIImage*)snapshot
{
	GLint backingWidth, backingHeight;
	
	// Bind the color renderbuffer used to render the OpenGL ES view
	// If your application only creates a single color renderbuffer which is already bound at this point, 
	// this call is redundant, but it is needed if you're dealing with multiple renderbuffers.
	// Note, replace "_colorRenderbuffer" with the actual name of the renderbuffer object defined in your class.
//	glBindRenderbuffer(GL_RENDERBUFFER, _colorRenderbuffer);
	
	// Get the size of the backing CAEAGLLayer
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &backingWidth);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backingHeight);
	
	NSInteger x = 0, y = 0, width = backingWidth, height = backingHeight;
	NSInteger dataLength = width * height * 4;
	GLubyte *data = (GLubyte*)malloc(dataLength * sizeof(GLubyte));
	
	// Read pixel data from the framebuffer
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
	
	// Create a CGImage with the pixel data
	// If your OpenGL ES content is opaque, use kCGImageAlphaNoneSkipLast to ignore the alpha channel
	// otherwise, use kCGImageAlphaPremultipliedLast
	CGDataProviderRef ref = CGDataProviderCreateWithData(NULL, data, dataLength, NULL);
	CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();
	CGImageRef iref = CGImageCreate(width, height, 8, 32, width * 4, colorspace, kCGBitmapByteOrder32Big | kCGImageAlphaPremultipliedLast, ref, NULL, true, kCGRenderingIntentDefault);
	
	// OpenGL ES measures data in PIXELS
	// Create a graphics context with the target size measured in POINTS
	NSInteger widthInPoints, heightInPoints;
	if (NULL != UIGraphicsBeginImageContextWithOptions) 
	{
		// On iOS 4 and later, use UIGraphicsBeginImageContextWithOptions to take the scale into consideration
		// Set the scale parameter to your OpenGL ES view's contentScaleFactor
		// so that you get a high-resolution snapshot when its value is greater than 1.0
		CGFloat scale = self.contentScaleFactor;
		widthInPoints = width / scale;
		heightInPoints = height / scale;
		UIGraphicsBeginImageContextWithOptions(CGSizeMake(widthInPoints, heightInPoints), NO, scale);
	}
	else
	{
		// On iOS prior to 4, fall back to use UIGraphicsBeginImageContext
		widthInPoints = width;
		heightInPoints = height;
		UIGraphicsBeginImageContext(CGSizeMake(widthInPoints, heightInPoints));
	}
	
	CGContextRef cgcontext = UIGraphicsGetCurrentContext();
	
	// UIKit coordinate system is upside down to GL/Quartz coordinate system
	// Flip the CGImage by rendering it to the flipped bitmap context
	// The size of the destination area is measured in POINTS
	CGContextSetBlendMode(cgcontext, kCGBlendModeCopy);
	CGContextDrawImage(cgcontext, CGRectMake(0.0, 0.0, widthInPoints, heightInPoints), iref);
	
	// Retrieve the UIImage from the current context
	UIImage *image = UIGraphicsGetImageFromCurrentImageContext();
	
	UIGraphicsEndImageContext();
	
	// Clean up
	free(data);
	CFRelease(ref);
	CFRelease(colorspace);
	CGImageRelease(iref);
	
	return image;
}

- (void) sendMessage :(const char*) name :(const char*) arg
{
	sendMessage(name, arg);
}

/*
 - (BOOL) canBecomeFirstResponder 
 {
	 return YES;
 }
 
 - (void)motionEnded:(UIEventSubtype)motion withEvent:(UIEvent *)event
 {
	 if ( event.subtype == UIEventSubtypeMotionShake ) 
	 {
		 NSLog(@"shake");
		 sendMessage("onShake", 0);
	 }    
//	 if ( [super respondsToSelector:@selector(motionEnded:withEvent:)] ) 
//	 {
//		 [super motionEnded:motion withEvent:event];
//	 }
}
 
// - (void)motionBegan:(UIEventSubtype)motion withEvent:(UIEvent *)event
// {
//	 NSLog(@"motionBegan");
//	 if ( event.subtype == UIEventSubtypeMotionShake )
//	 {
//		 NSLog(@"shake");
//	 }
// }
*/

@end

