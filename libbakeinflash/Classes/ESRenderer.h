//
//  ESRenderer.h
//  test
//
//  Created by Work on 04.07.10.
//  Copyright __MyCompanyName__ 2010. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIImageView.h>

#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>

@protocol ESRenderer <NSObject>

- (void)render;
- (BOOL)resizeFromLayer:(CAEAGLLayer *)layer;


@end
