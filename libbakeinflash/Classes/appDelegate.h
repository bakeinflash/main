//
//  bakeinFlash.h
//  Copyright BakeInFlash 2011. All rights reserved.
//

#import <UIKit/UIKit.h>

@class EAGLView;
@class myViewController;

@interface bakeinFlash : NSObject <UIApplicationDelegate>
{
    UIWindow *window;
    EAGLView *glView;
		UIViewController* glViewController;
}
@property (nonatomic, retain) UIWindow* window;
@property (nonatomic, retain) EAGLView* glView;
@property (nonatomic, retain) UIViewController* glViewController;

+ (bool) getZoomGesture;
+ (void) setZoomGesture :(bool) val;
+ (bool) getRotationGesture;
+ (void) setRotationGesture :(bool) val;
+ (int) getInterval;
+ (void) setInterval :(int) val;

+ (void) setData :(NSString*) str;
+ (float) getActivityPos;
+ (void) setActivityPos :(float) val;
+ (int) getActivityStyle;
+ (void) setActivityStyle :(int) val;
+ (bool) getAutoOrientation;
+ (void) setAutoOrientation :(bool) val;
+ (bool) getMSAA;
+ (void) setMSAA :(bool) val;
+ (bool) isPortrait;
+ (const char*) getWorkdir;
+ (const char*) getInfile;
+ (bool) getBitmapAlive;
+ (void) setBitmapAlive :(bool) val;
+ (void) addFont :(NSString*) fontname :(NSString*) filename :(const unsigned char*) ptr :(int) size;
+ (bool) getFont :(int) i :(const char**) fontname :(const char**) filename :(const unsigned char**) ptr :(int*) len;


@end

@interface myViewController : UIViewController 

- (id) init;

@end


@interface EAGLView : UIView 

- (void)startAnimation;
- (void)stopAnimation;
- (void) sendMessage :(const char*) name :(const char*) arg;

@end
