//
//  as_imap_.h
//  xxx
//
//  Created by Vitaly on 4/26/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <MapKit/MapKit.h>

@interface myAccel : NSObject <UIAccelerometerDelegate>
{
	void* parent;
	UIAccelerometer* accel;
	BOOL histeresisExcited;
	UIAcceleration* lastAcceleration;
}

@property void* parent;
@property (nonatomic, retain) UIAccelerometer* accel;
@property(retain) UIAcceleration* lastAcceleration;

- (void)accelerometer:(UIAccelerometer *)accelerometer didAccelerate:(UIAcceleration *)acceleration;

@end

