//
//  EAGLView.h
//  test
//
//  Created by Work on 04.07.10.
//  Copyright __MyCompanyName__ 2010. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <MapKit/MapKit.h>  
#import <MessageUI/MessageUI.h>	// mail
#import <GameKit/GameKit.h>

#import "ESRenderer.h"

// This class wraps the CAEAGLLayer from CoreAnimation into a convenient UIView subclass.
// The view content is basically an EAGL surface you render your OpenGL scene into.
// Note that setting the view non-opaque will only work if the EAGL surface has an alpha channel.

@interface myViewController : UIViewController 

	- (void)viewDidLoad;
	- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation;

	- (BOOL)shouldAutorotate NS_AVAILABLE_IOS(6_0);
	- (NSUInteger)supportedInterfaceOrientations NS_AVAILABLE_IOS(6_0);

	// Returns interface orientation masks.
	- (UIInterfaceOrientation)preferredInterfaceOrientationForPresentation NS_AVAILABLE_IOS(6_0);

@end


@interface EAGLView : UIView <UITextFieldDelegate, MFMailComposeViewControllerDelegate, UIGestureRecognizerDelegate, GKLeaderboardViewControllerDelegate>
{    
	UIImageView* myPreloader; 	
	
@private
	id <ESRenderer> renderer;
	
	BOOL animating;
	BOOL displayLinkSupported;
	NSInteger animationFrameInterval;
	// Use of the CADisplayLink class is the preferred method for controlling your animation timing.
	// CADisplayLink will link to the main display and fire every vsync when added to a given run-loop.
	// The NSTimer class is used only as fallback when running on a pre 3.1 device where CADisplayLink
	// isn't available.
	id displayLink;
	NSTimer *animationTimer;
	
	NSTimer *idleTimer;
	int idleInterval;
	
	UIBackgroundTaskIdentifier background_task; 
	bool stop_background_task;	
	
	UIPanGestureRecognizer * panGesture;
	UIPinchGestureRecognizer *pinchGesture;
	UITapGestureRecognizer *tapGesture;
	UIRotationGestureRecognizer *twoFingersRotate;
	UISwipeGestureRecognizer* swipeGesture;
	UILongPressGestureRecognizer *longGesture;
	
}

@property(nonatomic, retain) UIPanGestureRecognizer * panGesture;
@property(nonatomic, retain) UIPinchGestureRecognizer *pinchGesture;
@property(nonatomic, retain) UILongPressGestureRecognizer *longGesture;
@property (nonatomic, retain) UIImageView* myPreloader;
@property (readonly, nonatomic, getter=isAnimating) BOOL animating;
@property (nonatomic) NSInteger animationFrameInterval;

- (void)startAnimation;
- (void)stopAnimation;
- (void)drawView:(id)sender;
- (void) animateTextField: (UITextField*) textField up: (BOOL) up;
- (void)resetIdleTimer: (int) interval;
- (void)wakeUp :(NSString*) msg  :(NSString*) snd;

- (void)startBackgroundTask;
- (void)stopBackgroundTask;
- (UIImage*)snapshot;
- (void) sendMessage :(const char*) name :(const char*) arg;
//- (void)motionEnded:(UIEventSubtype)motion withEvent:(UIEvent *)event;
//- (BOOL)canBecomeFirstResponder;


@end
