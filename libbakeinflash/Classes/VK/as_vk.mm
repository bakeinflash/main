// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "as_vk.h"
#include "bakeinflash/bakeinflash_root.h"

#ifdef iOS
#import "as_vk_.h"
#import <UIKit/UIKit.h>

//@interface EAGLView : UIView <UITextFieldDelegate> @end
//extern EAGLView* s_view;
#endif

@interface EAGLView : UIView <UITextFieldDelegate> @end
extern EAGLView* s_view;

namespace bakeinflash
{

	void	as_vk_ctor(const fn_call& fn)
	{
		fn.result->set_as_object(new as_vk(fn.get_player()));
	}
	
	void	as_vk_post(const fn_call& fn)
	{
		bool rc = false;
		as_vk* vk = cast_to<as_vk>(fn.this_ptr);
		if (vk && fn.nargs > 0)
		{
			rc = vk->wall(fn.arg(0).to_string());
		}
		fn.result->set_bool(rc);
	}
	
	
	void	as_vk_appID_getter(const fn_call& fn)
	{
		as_vk* vk = cast_to<as_vk>(fn.this_ptr);
		assert(vk);
		fn.result->set_tu_string(vk->m_appID);
	}
	
	void	as_vk_appID_setter(const fn_call& fn)
	{
		as_vk* vk = cast_to<as_vk>(fn.this_ptr);
		assert(vk);
		vk->m_appID = fn.arg(0).to_tu_string();
	}

	
	void	as_vk_getToken(const fn_call& fn)
	{
		as_vk* vk = cast_to<as_vk>(fn.this_ptr);
		if (vk)
		{
			vk->getToken();
		}
	}
	
	as_vk::as_vk(player* player) :
		as_object(player),
		m_vk(NULL)
	{
		builtin_member("getToken", as_vk_getToken);
		builtin_member("post", as_vk_post);
		builtin_member("appID", as_value(as_vk_appID_getter, as_vk_appID_setter));
		
		CGRect  bounds = [[UIScreen mainScreen] bounds];
		vkWebView* webView = [[vkWebView alloc] initWithFrame:CGRectMake(0, 0, bounds.size.width, bounds.size.height)];
		webView.delegate = webView;
		webView.parent = this;
		m_vk = webView;
	}
	
	as_vk::~as_vk()
	{
		if (m_vk)
		{
			vkWebView* vk = (vkWebView*) m_vk;
			[vk release];
		}
	}
	
	void as_vk::onToken(const char* msg)
	{
		as_value function;
		if (get_member("onToken", &function))
		{
			as_environment env(get_player());
			env.push(msg);
			call_method(function, &env, this, 1, env.get_top_index());
		}
	}
	
	void as_vk::onPosted(const char* msg)
	{
		as_value function;
		if (get_member("onPosted", &function))
		{
			as_environment env(get_player());
			env.push(msg);
			call_method(function, &env, this, 1, env.get_top_index());
		}
	}

	void as_vk::getToken()
	{
		m_userID = [[[NSUserDefaults standardUserDefaults] objectForKey:@"myVKUserId"] UTF8String];
    m_accessToken = [[[NSUserDefaults standardUserDefaults] objectForKey:@"myVKToken"] UTF8String];
		printf("saved VK: %s, %s\n", m_userID.c_str(), m_accessToken.c_str());
		
		if (m_userID.size() == 0)
		{
			NSString* appID = [NSString stringWithUTF8String: m_appID.c_str()];
			NSString *authLink = [NSString stringWithFormat:@"http://api.vk.com/oauth/authorize?client_id=%@&scope=wall,photos,offline&redirect_uri=http://api.vk.com/blank.html&display=touch&response_type=token", appID];
			NSURL *url = [NSURL URLWithString:authLink];
			
			vkWebView* vk = (vkWebView*) m_vk;
			[vk indicator :true];
			[vk loadRequest:[NSURLRequest requestWithURL:url]];
			
			[s_view addSubview: vk];
		}
		else
		{
			onToken(NULL);
		}
	}
	
	bool as_vk::wall(const tu_string& msg)
	{
		m_userID = [[[NSUserDefaults standardUserDefaults] objectForKey:@"myVKUserId"] UTF8String];
    m_accessToken = [[[NSUserDefaults standardUserDefaults] objectForKey:@"myVKToken"] UTF8String];

		vkWebView* vk = (vkWebView*) m_vk;
		if (vk && m_userID.size() > 0)
		{
			[vk postText
			 :[NSString stringWithUTF8String: msg.c_str()]	// text
			 :[NSString stringWithUTF8String: m_userID.c_str()]	// userid
			 :[NSString stringWithUTF8String: m_accessToken.c_str()]	// token
			 ];
			return true;
		}
		return false;
	}

}

@implementation vkWebView

@synthesize parent;
@synthesize activityIndicatorView;

- (NSString*)stringBetweenString:(NSString*)start andString:(NSString*)end innerString:(NSString*)str
{
	NSScanner* scanner = [NSScanner scannerWithString:str];
	[scanner setCharactersToBeSkipped:nil];
	[scanner scanUpToString:start intoString:NULL];
	if ([scanner scanString:start intoString:NULL])
	{
		NSString* result = nil;
		if ([scanner scanUpToString:end intoString:&result])
		{
			return result;
		}
	}
	return nil;
}

- (void) indicator :(BOOL) show
{
	if (show && activityIndicatorView == NULL)
	{
		CGSize viewSize = s_view.bounds.size;
	
		// create new dialog box view and components
		activityIndicatorView = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray];
		
		// display it in the center of your view
		activityIndicatorView.center = CGPointMake(viewSize.width * 0.5, viewSize.height * 0.5);	// hack
		[self addSubview:activityIndicatorView];
		[activityIndicatorView startAnimating];
	}
	
	if (!show && activityIndicatorView)
	{
		[activityIndicatorView removeFromSuperview];
		[activityIndicatorView release];
		activityIndicatorView = NULL;
	}
}

-(void)webViewDidFinishLoad:(UIWebView *)webView
{
	[self indicator :false];

	NSString* reply = self.request.URL.absoluteString;
	NSString *body = [self stringByEvaluatingJavaScriptFromString:@"document.body.innerHTML"];
	//NSLog(@"vkWebView response: %@ %@", reply, body);

	bakeinflash::as_vk* vk = bakeinflash::cast_to<bakeinflash::as_vk>((bakeinflash::as_vk*) parent);
	
	// Если есть токен сохраняем его
	if ([self.request.URL.absoluteString rangeOfString:@"access_token"].location != NSNotFound)
	{
		// Получаем id пользователя, пригодится нам позднее
		NSArray *userAr = [reply componentsSeparatedByString:@"&user_id="];
		NSString * userID = [userAr lastObject];
		NSString * accessToken = [self stringBetweenString:@"access_token=" andString:@"&" innerString:reply];
		NSLog(@"UserID: %@, Token: %@", userID, accessToken);
		
		[[NSUserDefaults standardUserDefaults] setObject:accessToken forKey:@"myVKToken"];
		[[NSUserDefaults standardUserDefaults] setObject:userID forKey:@"myVKUserId"];
			
		// Сохраняем дату получения токена. Параметр expires_in=86400 в ответе ВКонтакта, говорит сколько будет действовать токен.
		// В данном случае, это для примера, мы можем проверять позднее истек ли токен или нет
		[[NSUserDefaults standardUserDefaults] setObject:[NSDate date] forKey:@"VKTokenDate"];
		[[NSUserDefaults standardUserDefaults] synchronize];

		vk->onToken(NULL);
		[self removeFromSuperview];
	}
	
	if ([self.request.URL.absoluteString rangeOfString:@"error"].location != NSNotFound ||
		[body rangeOfString:@"error"].location != NSNotFound ||
			body == @"")
	{
		vk->onToken("Не удалось соединиться");
		[self removeFromSuperview];
	}
}

- (NSString *)URLEncodedString:(NSString *)str
{
	NSString *result = (NSString *)CFURLCreateStringByAddingPercentEscapes(kCFAllocatorDefault,
																																				 (CFStringRef)str,
																																				 NULL,
																																				 CFSTR("!*'();:@&=+$,/?%#[]"),
																																				 kCFStringEncodingUTF8);
	[result autorelease];
	return result;
}

- (NSDictionary *) sendRequest:(NSString *)reqURl withCaptcha:(BOOL)captcha
{
	NSLog(@"Sending request: %@", reqURl);
	NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:reqURl]
																												 cachePolicy:NSURLRequestReloadIgnoringLocalCacheData
																										 timeoutInterval:60.0];
	
	// Для простоты используется обычный запрос NSURLConnection, ответ сервера сохраняем в NSData
	NSData *responseData = [NSURLConnection sendSynchronousRequest:request returningResponse:nil error:nil];
	
	// Если ответ получен успешно, можем его посмотреть и заодно с помощью JSONKit получить NSDictionary
	if (responseData)
	{
		//NSDictionary *dict = [[JSONDecoder decoder] parseJSONData:responseData];
		id jsonObjects = [NSJSONSerialization JSONObjectWithData:responseData options:NSJSONReadingMutableContainers error:nil];
		
		// Если есть описание ошибки в ответе
		NSString *errorMsg = [[jsonObjects objectForKey:@"error"] objectForKey:@"error_msg"];
		NSLog(@"Server response: %@ \nError: %@", jsonObjects, errorMsg);
		return jsonObjects;
	}
	return nil;
}

- (void) postText :(NSString*) text :(NSString*) user_id :(NSString*) accessToken
{
	// Создаем запрос на размещение текста на стене
   NSString *sendTextMessage = [NSString stringWithFormat:@"https://api.vk.com/method/wall.post?owner_id=%@&access_token=%@&message=%@", user_id, accessToken, [self URLEncodedString:text]];
	
	//NSLog(@"sendTextMessage: %@", sendTextMessage);
	
	// Если запрос более сложный мы можем работать дальше с полученным ответом
	NSDictionary *result = [self sendRequest:sendTextMessage withCaptcha:NO];
	
	// Если есть описание ошибки в ответе
	NSString *errorMsg = [[result objectForKey:@"error"] objectForKey:@"error_msg"];
	NSLog(@"%@", errorMsg ? errorMsg : @"Текст размещен на стене!");
	
	bakeinflash::as_vk* vk = bakeinflash::cast_to<bakeinflash::as_vk>((bakeinflash::as_vk*) parent);
	vk->onPosted(errorMsg ? [errorMsg UTF8String] : NULL);
}

@end
