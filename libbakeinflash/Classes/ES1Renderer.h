//
//  ES1Renderer.h
//  test
//
//  Created by Work on 04.07.10.
//  Copyright __MyCompanyName__ 2010. All rights reserved.
//

#import "ESRenderer.h"

#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>

@interface ES1Renderer : NSObject <ESRenderer>
{
	
@private
    EAGLContext *context;

	// The pixel dimensions of the CAEAGLLayer
    GLint backingWidth;
    GLint backingHeight;

  // The OpenGL ES names for the framebuffer and renderbuffer used to render to this view
	GLuint defaultFramebuffer;
	GLuint colorRenderbuffer;
	GLuint depthRenderbuffer;
	GLuint msaaFramebuffer;
	GLuint msaaRenderBuffer;
	GLuint msaaDepthBuffer;
}

- (void) render;
- (void) refresh;
- (void) createContext;
- (void) freeContext;
- (BOOL)resizeFromLayer:(CAEAGLLayer *)layer;

@end
