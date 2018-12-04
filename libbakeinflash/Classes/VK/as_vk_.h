//
//  as_imap_.h
//  xxx
//
//  Created by Vitaly on 4/26/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIWebView.h>
#import <UIKit/UIActivityIndicatorView.h>

@interface vkWebView : UIWebView <UIWebViewDelegate>
{
	void* parent;
	UIActivityIndicatorView* activityIndicatorView;
}
@property void* parent;
@property (nonatomic, retain) UIActivityIndicatorView* activityIndicatorView;

- (NSString*)stringBetweenString:(NSString*)start andString:(NSString*)end innerString:(NSString*)str;
- (NSDictionary*) sendRequest:(NSString *)reqURl withCaptcha:(BOOL)captcha;
- (void) postText :(NSString*) text :(NSString*) user_id :(NSString*) accessToken;
- (void) indicator :(BOOL) show;

@end
