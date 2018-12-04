// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "bakeinflash/bakeinflash_as_classes/as_iOS.h"
#include "bakeinflash/bakeinflash_root.h"
#include "base/tu_file.h"

// for open file
#include <fcntl.h>
#ifndef WINPHONE
	#include <unistd.h>
#endif

#if ANDROID == 1
extern JNIEnv*  s_jenv;
extern jobject javaSettings;
#endif

#ifdef iOS
#import "as_iOS_.h"
#import <AudioToolbox/AudioToolbox.h> // for vibrate
#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>	// for sound
#import <QuartzCore/QuartzCore.h>
#import <Social/Social.h>
#import <GameKit/GameKit.h>
#import <Accounts/Accounts.h>
#import <Security/Security.h>
#import "KeychainItemWrapper.h"
//#import <FBSDKShareKit/FBSDKShareKit.h>

// for getfreemem
#import <mach/mach.h>
#import <mach/mach_host.h>

@interface EAGLView : UIView  <UITextFieldDelegate, MFMailComposeViewControllerDelegate, GKLeaderboardViewControllerDelegate>
- (void)startAnimation;
- (void)stopAnimation;
- (void) sendMessage :(const char*) name :(const char*) arg;
@end

extern EAGLView* s_view;
extern int s_x0;
extern int s_y0;
extern float s_scale;
extern float s_retina;

@interface myViewController : UIViewController @end
extern myViewController* s_myController;

@interface bakeinFlash : NSObject <UIApplicationDelegate>
+ (void) setAutoOrientation :(bool) val;
@end

#endif

void sendMessage(const char* name, const bakeinflash::as_value& arg);

namespace bakeinflash
{
  
  void	as_systemGetUUID(const fn_call& fn)
  {
    // arg1: app name
    if (fn.nargs == 0)
    {
      return;
    }
    
#ifdef iOS
    
    const tu_string& appname = fn.arg(0).to_tu_string();
    KeychainItemWrapper *keychain = [[KeychainItemWrapper alloc] initWithIdentifier: [NSString stringWithUTF8String: appname.c_str()]  accessGroup:nil];
    
    NSString* uuid  = [keychain objectForKey:(__bridge id)(kSecAttrAccount)];
    if (uuid == NULL || uuid.length == 0)
    {
      CFUUIDRef udid = CFUUIDCreate(NULL);
      uuid = (NSString*) CFUUIDCreateString(NULL, udid);
      CFRelease(udid);
      
      [keychain setObject:uuid forKey:(__bridge id)(kSecAttrAccount)];
    }

//    NSLog(@"%@", uuid);
    
    
    fn.result->set_string([uuid UTF8String]);
    
#else
    fn.result->set_string("static-uuid");
#endif
  }
  
  void	as_systemOpenFile(const fn_call& fn)
  {
    // arg1: filename
    if (fn.nargs >= 1)
    {
      tu_string url = get_workdir();
      url += fn.arg(0).to_tu_string();
      int fd = -1;
#ifndef WINPHONE
      fd = open(url.c_str(), O_RDONLY);
#endif
      fn.result->set_int(fd);
    }
  }
  
  void	as_systemCloseFile(const fn_call& fn)
  {
    // arg1: filename
    if (fn.nargs >= 1)
    {
      int fd = fn.arg(0).to_int();
      if (fd < 0)
      {
        return;
      }
#ifndef WINPHONE
      close(fd);
#endif
    }
  }
  
  void	as_systemGameCenter(const fn_call& fn);
  
  void	as_systemIsAppInstalled(const fn_call& fn)
  {
    bool myAppInstalled = false;
    if (fn.nargs > 0)
    {
#ifdef iOS
      NSString* appName = [NSString stringWithUTF8String: fn.arg(0).to_string()];
      myAppInstalled = [[UIApplication sharedApplication] canOpenURL:[NSURL URLWithString: appName]]; // @"myapp:"
#endif
    }
    fn.result->set_bool(myAppInstalled);
  }
  
  void	as_system_setCloud(const fn_call& fn)
  {
   	fn.result->set_bool(false);
    if (fn.nargs >= 2)
    {
      const tu_string& key = fn.arg(0).to_tu_string();
      const tu_string& value = fn.arg(1).to_tu_string();
      
      //NSUbiquitousKeyValueStore* keyStore = [[NSUbiquitousKeyValueStore alloc] init];
      //[keyStore setString: [NSString stringWithUTF8String: value.c_str()]  forKey: [NSString stringWithUTF8String: key.c_str()] ];
      //[keyStore synchronize];
      //[keyStore release];
      //fn.result->set_bool(true);
      

      
    }
  }
  
  void	as_system_getCloud(const fn_call& fn)
  {
    if (fn.nargs >= 1)
    {
      const tu_string& key = fn.arg(0).to_tu_string();
      
     // NSUbiquitousKeyValueStore* keyStore = [[NSUbiquitousKeyValueStore alloc] init];
      //NSString *storedString = [keyStore stringForKey: [NSString stringWithUTF8String: key.c_str()]];
      //[keyStore release];
      //fn.result->set_string([storedString UTF8String]);
#ifdef iOS
      NSURL* ubiquitousURL = [[NSFileManager defaultManager] URLForUbiquityContainerIdentifier: nil];
      if (ubiquitousURL)
      {
        printf("iCloud disabled\n");
        return;
      }
#endif      
    }
  }
  
  /*
  void	as_systemInvite(const fn_call& fn)
  {
    if (fn.nargs > 0)
    {
    FBSDKGameRequestContent *gameRequestContent = [[FBSDKGameRequestContent alloc] init];
    
    // Look at FBSDKGameRequestContent for futher optional properties
    gameRequestContent.message = @"Let's play My brilliant game together";
    gameRequestContent.title = @"My custom invitation";
      const char* friend_id = fn.arg(0).to_string();
      gameRequestContent.to = @[[NSString stringWithUTF8String: friend_id]]; //@[contact.facebookID];
    gameRequestContent.actionType = FBSDKGameRequestActionTypeTurn;
    
    FBSDKGameRequestDialog* dialog = [[FBSDKGameRequestDialog alloc] init];
    dialog.frictionlessRequestsEnabled = YES;
    dialog.content = gameRequestContent;
    dialog.delegate = nil; //s_myController;
    
    // Assuming self implements <FBSDKGameRequestDialogDelegate>
    [dialog show];
    }
  }
  */
  
  void	as_systemRenewCredentials(const fn_call& fn)
  {
#ifdef iOS
    ACAccountStore* accountStore = [[ACAccountStore alloc] init];
    ACAccountType* accountTypeFacebook = [accountStore accountTypeWithAccountTypeIdentifier: ACAccountTypeIdentifierFacebook];
    NSArray *accounts = [accountStore accountsWithAccountType:accountTypeFacebook];
    if (accounts && accounts.count > 0)
    {
      ACAccount* facebookAccount = [accounts lastObject];
      [accountStore renewCredentialsForAccount:(ACAccount *)facebookAccount completion:^(ACAccountCredentialRenewResult renewResult, NSError *error)
      {
        NSString* msg = @"Error from renew credentials";
        if (!error)
        {
          switch (renewResult)
          {
            case ACAccountCredentialRenewResultRenewed:
              msg = @""; // no errors
              break;
            case ACAccountCredentialRenewResultRejected:
              msg = @"User declined permission";
              break;
            case ACAccountCredentialRenewResultFailed:
              msg = @"Non-user-initiated cancel, you may attempt to retry";
              break;
            default:
              msg = @"Unhandled FB error";
              break;
          }
        }
       
       // multithread!!
       dispatch_async(dispatch_get_main_queue(), ^(void)
       {
         sendMessage("onRenewCredentials", [msg UTF8String]);
       });
     }];
    }
#endif
  }
  
  
  void	as_systemLoginWithSocial(const fn_call& fn)
  {
#ifdef iOS
    if (fn.nargs > 0)
    {
      as_object* arg = fn.arg(0).to_object();
      if (arg == NULL)
      {
        printf("system.loginWith: invalid arg, %s\n", fn.arg(0).to_string());
        return;
      }
      
      as_value network;
      arg->get_member("network", &network);
      as_value appID;
      arg->get_member("appID", &appID);
      as_value onError;
      arg->get_member("onError", &onError);
      as_value onLogin;
      arg->get_member("onLogin", &onLogin);
      
      Reachability *reachability = [Reachability reachabilityForInternetConnection];
      NetworkStatus networkStatus = [reachability currentReachabilityStatus];
      bool rc = !(networkStatus == NotReachable);
      if (rc == false)
      {
        sendMessage(onError.to_string(), "No Internet connection");
        return;
      }
      
      if (network.to_tu_string() == "facebook")
      {
        ACAccountStore* accountStore = [[ACAccountStore alloc] init];
        ACAccountType* accountTypeFacebook = [accountStore accountTypeWithAccountTypeIdentifier: ACAccountTypeIdentifierFacebook];
        
        NSDictionary *options = @{
                                  ACFacebookAppIdKey: [NSString stringWithUTF8String: appID.to_string()],  //]@"346634375517364",
//                                  ACFacebookPermissionsKey:  @[@"publish_actions", @"public_profile", @"user_friends", @"read_friendlists", @"read_custom_friendlist"],
                                  ACFacebookPermissionsKey:  @[@"email",  @"user_friends"],
                                  ACFacebookAudienceKey: ACFacebookAudienceFriends
                                  };

        [accountStore requestAccessToAccountsWithType:accountTypeFacebook options:options completion:^(BOOL granted, NSError *error)
        {
           if (granted)
           {
             NSArray *accounts = [accountStore accountsWithAccountType:accountTypeFacebook];
             if (accounts && accounts.count > 0)
             {
               ACAccount* facebookAccount = [accounts lastObject];
               
               NSURL *requestURL = [NSURL URLWithString: @"https://graph.facebook.com/me"];
               NSMutableDictionary* param = [[NSMutableDictionary alloc] init];
               param[@"fields"] = @"id,name,picture";
               SLRequest *request = [SLRequest requestForServiceType:SLServiceTypeFacebook requestMethod:SLRequestMethodGET URL:requestURL parameters:param];
               request.account = facebookAccount;
               [request performRequestWithHandler:^(NSData *data, NSHTTPURLResponse *response, NSError *error)
                {
                  NSLog(@"%@", [[NSString alloc] initWithData:data encoding: NSUTF8StringEncoding]);
                  if (!error)
                  {
                    as_object* obj = new as_object();
                    obj->set_member("token", [facebookAccount.credential.oauthToken UTF8String]);
                    //obj->set_member("username", [facebookAccount.username UTF8String]);
                    //obj->set_member("userFullName", [facebookAccount.userFullName UTF8String]);
                    //obj->set_member("identifier", [facebookAccount.identifier UTF8String]);
                    obj->set_member("type", "facebook");
                    
                    
                  //  as_object* fb = new as_facebook(accountStore);
                  //  obj->set_member("handler", fb);
                    
                    obj->set_member("data", [[[NSString alloc] initWithData:data encoding: NSUTF8StringEncoding] UTF8String]);
                    
                   // sendMessage(onLogin.to_string(), obj);
                   // dispatch_async(dispatch_get_main_queue(),^{ });
                    
                    dispatch_async(dispatch_get_main_queue(), ^(void)
                    {
                      //Run UI Updates
                      sendMessage(onLogin.to_string(), obj);
                    });
                    
                  }
                  else
                  {
                    sendMessage("onErrorFB", "Access denied");
                  }
                }];
             }
             else
             {
                sendMessage(onError.to_string(), "Please setup your Facebook account in settings");
             }
           }
           else
           {
             if (error != nil)
             {
               // Fail gracefully...
               if ([error code]== ACErrorAccountNotFound)
               {
                 sendMessage(onError.to_string(), "Please setup your Facebook account in settings");
               }
               else
               {
                 sendMessage(onError.to_string(), [[error localizedDescription] UTF8String]);
               }
             }
             else
             {
               sendMessage("onErrorFB", "Access denied");
             }

           }
         }
         ];
      }
      else
      if (network.to_tu_string() == "twitter")
      {
        ACAccountStore* accountStore = [[ACAccountStore alloc] init];
        ACAccountType* accountTypeTwitter = [accountStore accountTypeWithAccountTypeIdentifier: ACAccountTypeIdentifierTwitter];
        
        [accountStore requestAccessToAccountsWithType:accountTypeTwitter options:nil completion:^(BOOL granted, NSError *error)
         {
           if (granted)
           {
             NSArray *accounts = [accountStore accountsWithAccountType:accountTypeTwitter];
             if (accounts && accounts.count > 0)
             {
               ACAccount* facebookAccount = [accounts lastObject];
               as_object* obj = new as_object();
               obj->set_member("token", [facebookAccount.credential.oauthToken UTF8String]);
               obj->set_member("username", [facebookAccount.username UTF8String]);
               obj->set_member("userFullName", [facebookAccount.userFullName UTF8String]);
               obj->set_member("identifier", [facebookAccount.identifier UTF8String]);
               obj->set_member("id", [facebookAccount.identifier UTF8String]);
               obj->set_member("type", "twitter");
               
               sendMessage(onLogin.to_string(), obj);
             }
             else
             {
               sendMessage(onError.to_string(), "Please setup your Twitter account in settings");
             }
           }
           else
           {
             //NSLog(@"Error: [%@]",[error localizedDescription]);
             
             // Fail gracefully...
             if ([error code]== ACErrorAccountNotFound)
             {
               sendMessage(onError.to_string(), "Please setup your Twitter account in settings");
             }
             else
             {
               sendMessage(onError.to_string(), "Twitter account access denied");
             }
             
             
           }
         }
         ];
      }
      else
      {
        printf("system.loginWith: invalid network\n");
      }
    }
#endif
  }
  
  // facebook
/*
  void	as_facebookSendRequest(const fn_call& fn)
  {
#ifdef iOS
     as_facebook* fb = cast_to<as_facebook>(fn.this_ptr);
    if (fb == NULL || fn.nargs < 2)
    {
      return;
    }
   
    ACAccountStore* accountStore = (ACAccountStore*) fb->m_handler;
    ACAccountType* accountTypeFacebook = [accountStore accountTypeWithAccountTypeIdentifier: ACAccountTypeIdentifierFacebook];
    NSArray *accounts = [accountStore accountsWithAccountType:accountTypeFacebook];
    assert(accounts && accounts.count > 0);
    ACAccount* facebookAccount = [accounts lastObject];
    
    NSURL *requestURL = [NSURL URLWithString: [NSString stringWithUTF8String: fn.arg(0).to_string()]];
    NSMutableDictionary* param = [[NSMutableDictionary alloc] init];
    const tu_string& arg = fn.arg(1).to_tu_string();
    if (arg.size() > 0)
    {
      // fields=picture,id,name&a=c,d,e
      array<tu_string> a;
      arg.split('&', &a);
      for (int i = 0; i < a.size(); i++)
      {
        array<tu_string> b;
        a[i].split('=', &b);
        if (b.size() == 2)
        {
          param[[NSString stringWithUTF8String: b[0].c_str()]] = [NSString stringWithUTF8String: b[1].c_str()];
        }
        
      }
    }
    
    SLRequest *request = [SLRequest requestForServiceType:SLServiceTypeFacebook requestMethod:SLRequestMethodGET URL:requestURL parameters:param];
    request.account = facebookAccount;
    [request performRequestWithHandler:^(NSData *data, NSHTTPURLResponse *response, NSError *error)
    {
      if (!error)
      {
        sendMessage("onReplyFacebook", [[[NSString alloc] initWithData:data encoding: NSUTF8StringEncoding] UTF8String]);
        dispatch_async(dispatch_get_main_queue(),^{ });
      }
      else
      {
        sendMessage("onReplyFacebook", [error.localizedDescription UTF8String]);
      }
    }];

#endif
  }
 
  as_facebook::as_facebook(void* handler):
    m_handler(handler)
  {
    builtin_member("send", as_facebookSendRequest);
  }
 */
  
  void	as_systemPostToWall(const fn_call& fn)
  {
#ifdef iOS
    float val = [[[UIDevice currentDevice] systemVersion] floatValue];
    int major = int(val);
    if (fn.nargs > 0 && major >= 6)
    {
      const tu_string& fb = fn.arg(0).to_tu_string();
      SLComposeViewController* fbController = NULL;
      if (fb == "facebook")
      {
        fbController = [SLComposeViewController composeViewControllerForServiceType:SLServiceTypeFacebook];
      }
      else
        if (fb == "twitter")
        {
          fbController = [SLComposeViewController composeViewControllerForServiceType:SLServiceTypeTwitter];
        }
      
      if (fbController)
      {
        SLComposeViewControllerCompletionHandler __block completionHandler = ^(SLComposeViewControllerResult result)
        {
          [fbController dismissViewControllerAnimated:YES completion:nil];
          switch(result)
          {
            case SLComposeViewControllerResultCancelled:
            default:
              NSLog(@"Post Cancelled.....");
              break;
            case SLComposeViewControllerResultDone:
              NSLog(@"Post Succesfull....");
              break;
          }
        };
        //					[fbController addImage:[UIImage imageNamed:@"1.jpg"]];
        //					[fbController setInitialText:@"Check out this article."];
        //					[fbController addURL:[NSURL URLWithString:@"http://soulwithmobiletechnology.blogspot.com/"]];
        [fbController setCompletionHandler:completionHandler];
        [s_myController presentViewController:fbController animated:YES completion:nil];
        return;
      }
    }
    
    const char* msg = "Can't post to wall";
    UIAlertView *alert = [[[UIAlertView alloc] initWithTitle:[NSString stringWithUTF8String: fn.arg(0).to_string()]  message:[NSString stringWithUTF8String: msg] delegate:nil cancelButtonTitle:@"Ok" otherButtonTitles:nil, nil] autorelease];
    [alert show];
#endif    
  }
  
  void	as_systemSetAutoOrientation(const fn_call& fn)
  {
    if (fn.nargs > 0)
    {
      bool flag = fn.arg(0).to_bool();
#ifdef iOS
      [bakeinFlash setAutoOrientation : flag];
#endif
    }
  }
  
  // loadURL, open it in browser
  void	as_systemLaunchUrl(const fn_call& fn)
  {
#ifdef iOS
    NSString* launchUrl = [NSString stringWithUTF8String: fn.arg(0).to_string()];
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString: launchUrl]];
#endif
  }
  
  void	as_systemGetFreeMem(const fn_call& fn)
  {
#ifdef iOS
    mach_port_t host_port = mach_host_self();
    mach_msg_type_number_t host_size = sizeof(vm_statistics_data_t) / sizeof(integer_t);
    vm_size_t pagesize;
    vm_statistics_data_t vm_stat;
    
    host_page_size(host_port, &pagesize);
    
    if (host_statistics(host_port, HOST_VM_INFO, (host_info_t)&vm_stat, &host_size) != KERN_SUCCESS)
    {
      NSLog(@"Failed to fetch vm statistics");
    }
    else
    {
      as_object* obj = new as_object();
      int mem_used = (vm_stat.active_count + vm_stat.inactive_count + vm_stat.wire_count) * pagesize;
      int mem_free = vm_stat.free_count * pagesize;
      int mem_total = mem_used + mem_free;
      obj->set_member("used", mem_used / 1024.0 / 1024.0);	//MB
      obj->set_member("free", mem_free / 1024.0 / 1024.0);	//MB
      obj->set_member("total", mem_total / 1024.0 / 1024.0);	//MB
      fn.result->set_as_object(obj);
    }
#endif
  }
  
  
  // mail
  void	as_systemSendMail(const fn_call& fn)
  {
#ifdef iOS
    if (fn.nargs >= 3)
    {
      // try to create mail picker
      MFMailComposeViewController *picker = [[MFMailComposeViewController alloc] init];
      if (picker == nil)
      {
        return;
      }
      picker.mailComposeDelegate = s_view;
      
      // Set the subject of email
      [picker setSubject: [NSString stringWithUTF8String: fn.arg(1).to_string()]];
      
      // Add email addresses
      // Notice three sections: "to" "cc" and "bcc"
      //		[picker setToRecipients:[NSArray arrayWithObjects:@"TO mailID1",@"TO mailID2", nil]];
      //			[picker setCcRecipients:[NSArray arrayWithObject:@"CC MailID"]];
      //			[picker setBccRecipients:[NSArray arrayWithObject:@"BCC Mail ID"]];
      [picker setToRecipients:[NSArray arrayWithObjects: [NSString stringWithUTF8String: fn.arg(0).to_string()], nil]];
      
      
      // Fill out the email body text
      NSString *emailBody = [NSString stringWithUTF8String: fn.arg(2).to_string()];
      
      // This is not an HTML formatted email
      [picker setMessageBody:emailBody isHTML:NO];
      
      //			UIImage* screenshot = [s_view snapshot];
      
      // ATTACHING A SCREENSHOT
      //			NSData *myData = UIImagePNGRepresentation(screenshot);
      //			[picker addAttachmentData:myData mimeType:@"image/png" fileName:@"screenshot"];
      
      [s_myController presentModalViewController:picker animated:YES];
      [picker release];
    }
#endif
  }
  
  
#ifdef iOS
  static UIActivityIndicatorView* activityIndicatorView = NULL;
#endif

  void	as_systemShowActivity(const fn_call& fn)
  {
    if (fn.nargs > 0)
    {
#ifdef iOS
      bool show = fn.arg(0).to_bool();
      if (show && activityIndicatorView == NULL)
      {
        CGSize viewSize = s_view.bounds.size;
        
        // create new dialog box view and components
        activityIndicatorView = [[UIActivityIndicatorView alloc]
                                 initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhiteLarge];
        
        // other size? change it
        //			activityIndicatorView.bounds = CGRectMake(0, 0, 65, 65);
        //			activityIndicatorView.hidesWhenStopped = YES;
        //			activityIndicatorView.alpha = 0.7f;
        //			activityIndicatorView.backgroundColor = [UIColor blackColor];
        //			activityIndicatorView.layer.cornerRadius = 10.0f;
        
        // display it in the center of your view
        activityIndicatorView.center = CGPointMake(viewSize.width * 0.5, viewSize.height * 0.5);	// hack
        [s_view addSubview:activityIndicatorView];
        [activityIndicatorView startAnimating];
      }
      
      if (!show && activityIndicatorView)
      {
        [activityIndicatorView removeFromSuperview];
        [activityIndicatorView release];
        activityIndicatorView = NULL;
      }
#endif 
    }
  }
  
  void	as_systemAlert(const fn_call& fn)
  {
    // arg1: filename
    if (fn.nargs >= 2)
    {
#ifdef iOS
      UIAlertView *alert = [[[UIAlertView alloc] initWithTitle:[NSString stringWithUTF8String: fn.arg(0).to_string()]  message:[NSString stringWithUTF8String: fn.arg(1).to_string()] delegate:nil cancelButtonTitle:@"Ok" otherButtonTitles:nil, nil] autorelease];
      [alert show];
#endif
    }
  }
  
  void	as_systemGetVersion(const fn_call& fn)
  {
    //	float val = [[[UIDevice currentDevice] systemVersion] floatValue];
    //	int major = int(val);
    //	int minor = int((val - major) * 10);
    //	fn.result->set_tu_string(tu_string(major) + tu_string(".") + tu_string(minor));
#ifdef iOS
    fn.result->set_tu_string([[[UIDevice currentDevice] systemVersion] UTF8String]);
#endif
  }
  
  void	as_systemGetModel(const fn_call& fn)
  {
#ifdef iOS
    NSString *deviceType = [UIDevice currentDevice].model;
    fn.result->set_string([deviceType UTF8String]);
#endif
  }
  
  void	as_systemGetLocalization(const fn_call& fn)
  {
    //Get the current user locale.
#ifdef iOS
    NSLocale *currentLocale = [NSLocale currentLocale];
    fn.result->set_string([[currentLocale localeIdentifier] UTF8String]);
#endif
  }
  
  void	as_systemReadFile(const fn_call& fn)
  {
    // arg1: filename
    if (fn.nargs >= 1)
    {
      tu_string file_name = get_workdir();
      file_name += fn.arg(0).to_tu_string();
      tu_file fi(file_name.c_str(), "r");
      if (fi.get_error() == TU_FILE_NO_ERROR)
      {
        int n = fi.size();
        char* buf = (char*) malloc(n + 1);
        memset(buf, 0, n + 1);
        fi.read_bytes(buf, n);
        fn.result->set_string(buf);
        free(buf);
      }
    }
  }
  
  void	as_systemWriteFile(const fn_call& fn)
  {
    // arg1: filename
    // arg2: string
    if (fn.nargs >= 2)
    {
      tu_string file_name = get_workdir();
      file_name += fn.arg(0).to_tu_string();
      tu_file fi(file_name.c_str(), "w");
      if (fi.get_error() == TU_FILE_NO_ERROR)
      {
        int n = fn.arg(1).to_tu_string().size();
        fi.write_bytes(fn.arg(1).to_string(), n);
      }
    }
  }
  
  void as_systemVibrate(const fn_call& fn)
  {
#ifdef iOS
    AudioServicesPlaySystemSound (kSystemSoundID_Vibrate);	// vibrate
#endif
  }
  
  void as_systemHasNetwork(const fn_call& fn)
  {
#ifdef iOS
		//		Reachability* reachability = [Reachability reachabilityWithHostName:@"www.apple.com"];
		//		NetworkStatus remoteHostStatus = [reachability currentReachabilityStatus];
		//		if(remoteHostStatus == NotReachable) { NSLog(@"not reachable");}
		//		else if (remoteHostStatus == ReachableViaWWAN) { NSLog(@"reachable via wwan");}
		//		else if (remoteHostStatus == ReachableViaWiFi) { NSLog(@"reachable via wifi");}

		Reachability *reachability = [Reachability reachabilityForInternetConnection];
		NetworkStatus networkStatus = [reachability currentReachabilityStatus];
		bool rc = !(networkStatus == NotReachable);
		fn.result->set_bool(rc);
#else
    fn.result->set_bool(true);
#endif
  }
  
  void	as_systemLoadURL(const fn_call& fn)
  {
#ifdef iOS
    NSURL *myURL = [NSURL URLWithString:[NSString stringWithUTF8String: (char*) fn.arg(0).to_string()] ];
    NSString *sqlResults = [[NSString alloc] initWithContentsOfURL:myURL];
    fn.result->set_string( [sqlResults UTF8String] );
#endif
  }
  
  void	as_systemGetVar(const fn_call& fn)
  {
    as_iOS* obj = cast_to<as_iOS>(fn.this_ptr);
    if (obj && fn.nargs > 0)
    {
#ifdef iOS
      NSString* key = [NSString stringWithUTF8String: fn.arg(0).to_string()];
      NSUserDefaults* userDefaults = [NSUserDefaults standardUserDefaults];
      NSString* result = [userDefaults objectForKey: key];
      if (result != nil)
      {
        fn.result->set_string( [result UTF8String] );
      }
#endif
#ifdef ANDROID
      // String silent = settings.getString("silentMode", "");
      jclass cls = s_jenv->FindClass("android/content/SharedPreferences");
      jmethodID mid = s_jenv->GetMethodID(cls, "getString", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
      jstring jkey = s_jenv->NewStringUTF(fn.arg(0).to_string());
      jstring jdefval = s_jenv->NewStringUTF("");
      jstring js = (jstring) s_jenv->CallObjectMethod(javaSettings, mid, jkey, jdefval);
      const char* str = s_jenv->GetStringUTFChars(js, NULL);
      fn.result->set_string(str);
      //alog("preferences: %s='%s'", name.c_str(), str);
#endif
    }
  }
  
  void	as_systemSetVar(const fn_call& fn)
  {
    as_iOS* obj = cast_to<as_iOS>(fn.this_ptr);
    if (obj && fn.nargs >= 2)
    {
#ifdef iOS
      NSString* key = [NSString stringWithUTF8String: fn.arg(0).to_string()];
      NSString* val = [NSString stringWithUTF8String: fn.arg(1).to_string()];
      NSUserDefaults* userDefaults = [NSUserDefaults standardUserDefaults];
      [userDefaults setObject: val forKey: key];
      [userDefaults synchronize];
#endif
#ifdef ANDROID
      
      //Editor  android.content.SharedPreferences.edit();
      jclass cls = s_jenv->FindClass("android/content/SharedPreferences");
      jmethodID mid = s_jenv->GetMethodID(cls, "edit", "()Landroid/content/SharedPreferences$Editor;");
      jobject jedit = s_jenv->CallObjectMethod(javaSettings, mid);
      
      jclass clsEditor = s_jenv->FindClass("android/content/SharedPreferences$Editor");
      jmethodID midPutString = s_jenv->GetMethodID(clsEditor, "putString", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/SharedPreferences$Editor;");
      jmethodID midCommit = s_jenv->GetMethodID(clsEditor, "commit", "()Z");
      
      string_hash<as_value>::const_iterator it = obj->m_members.begin();
      while (it != obj->m_members.end())
      {
        if (it->second.is_enum())
        {
          //	editor.putString(String  key, String  value)
          jstring jkey = s_jenv->NewStringUTF(it->first.c_str());
          jstring jval = s_jenv->NewStringUTF(it->second.to_string());
          s_jenv->CallObjectMethod(jedit, midPutString, jkey, jval);
          alog("commit: %s='%s'", it->first.c_str(), it->second.to_string());
        }
        ++it;
      }
      
      // editor.commit();
      bool rc = s_jenv->CallBooleanMethod(jedit, midCommit);
#endif
    }
  }
  
  as_iOS* iOS_init()
  {
    // Create built-in object.
    as_iOS*	obj = new as_iOS();
    return obj;
  }
  
  as_iOS::as_iOS()
  {
    builtin_member("getVar", as_systemGetVar);
    builtin_member("setVar", as_systemSetVar);
    builtin_member("loadURL", as_systemLoadURL);
    builtin_member("hasNetwork", as_systemHasNetwork);
    builtin_member("vibrate", as_systemVibrate);
    builtin_member("readFile", as_systemReadFile);
    builtin_member("writeFile", as_systemWriteFile);
    builtin_member("getLocalization", as_systemGetLocalization);
    builtin_member("alert", as_systemAlert);
    builtin_member("showActivity", as_systemShowActivity);
    builtin_member("sendMail", as_systemSendMail);
    builtin_member("getVersion",as_systemGetVersion);
    builtin_member("getModel",as_systemGetModel);
    builtin_member("getFreeMem",as_systemGetFreeMem);
    builtin_member("setAutoOrientation",as_systemSetAutoOrientation);
    builtin_member("loginWith",as_systemLoginWithSocial);
    builtin_member("renewCredentials",as_systemRenewCredentials);
    // builtin_member("invite",as_systemInvite);
    builtin_member("postToWall",as_systemPostToWall);
    builtin_member("isAppInstalled",as_systemIsAppInstalled);
    builtin_member("getCloud", as_system_getCloud);
    builtin_member("setCloud", as_system_setCloud);
    builtin_member("openFile", as_systemOpenFile);
    builtin_member("closeFile", as_systemCloseFile);
    
    builtin_member("gameCenter",as_systemGameCenter);
 		builtin_member("getUUID", as_systemGetUUID);
  }
  
  as_iOS::~as_iOS()
  {
#if ANDROID == 1
    s_jenv->DeleteGlobalRef(m_jPreferences);
#endif
  }
  
  
  void	as_systemGameCenter(const fn_call& fn)
  {
    // args; leaderboardname, request
    if (fn.nargs >= 2)
    {
#ifdef iOS
      const tu_string& leaderboard = fn.arg(0).to_tu_string();
      const tu_string& request = fn.arg(1).to_tu_string();
      if (request == "setScore" && fn.nargs >= 3)
      {
        GKScore *scoreReporter = [[GKScore alloc] initWithCategory: [NSString stringWithUTF8String: (char*) leaderboard.c_str()]];
        scoreReporter.value = fn.arg(2).to_int();
        scoreReporter.context = 0;
        [
         scoreReporter reportScoreWithCompletionHandler:^(NSError *err)
         {
           if (err != nil)
           {
             NSLog(@"%@", err);
             [s_view sendMessage :"onSetScore" :""];
           }
           else
           {
             [s_view sendMessage :"onSetScore" :"done"];
           }
         }
         ];
      }
      else
        if (request == "show")	// show leader board
        {
          GKLeaderboardViewController *leaderboardController = [[GKLeaderboardViewController alloc] init];
          if (leaderboardController != nil)
          {
            leaderboardController.leaderboardDelegate = s_view;
            leaderboardController.timeScope = GKLeaderboardTimeScopeToday;
            leaderboardController.category = [NSString stringWithUTF8String: (char*) leaderboard.c_str()];
            [s_myController presentViewController: leaderboardController animated: YES completion:nil];
          }
        }
        else
          if (request == "getTop")
          {
            GKLeaderboard *leaderboardRequest = [[[GKLeaderboard alloc] init] autorelease];
            if (leaderboardRequest != nil)
            {
              leaderboardRequest.playerScope = GKLeaderboardPlayerScopeGlobal;
              leaderboardRequest.timeScope = GKLeaderboardTimeScopeAllTime;
              leaderboardRequest.category = [NSString stringWithUTF8String: (char*) leaderboard.c_str()];
              leaderboardRequest.range = NSMakeRange(1, 10);
              [
               leaderboardRequest loadScoresWithCompletionHandler: ^(NSArray *scores, NSError *err)
               {
                 if (err != nil)
                 {
                   NSLog(@"%@", err);
                   [s_view sendMessage :"onGetTop" :""];
                 }
                 
                 if (scores != nil)
                 {
                   // Process the score information.
                   NSLog(@"%@", scores);
                   char str[256];
                   snprintf(str, 256, "%s,%s", [[[scores objectAtIndex :0] formattedValue] UTF8String], [[[scores objectAtIndex :0] playerID] UTF8String]);
                   [s_view sendMessage :"onGetTop" :str];
                 }
               }
               ];
            }
          }
          else
            if (request == "getScore" && fn.nargs >= 3)
            {
              NSArray* playerIDs = [NSArray arrayWithObject:[NSString stringWithUTF8String: (char*) fn.arg(2).to_string()]];
              GKLeaderboard *leaderboardRequest = [[[GKLeaderboard alloc] initWithPlayerIDs: playerIDs] autorelease];
              leaderboardRequest.timeScope = GKLeaderboardTimeScopeAllTime;
              leaderboardRequest.category = [NSString stringWithUTF8String: (char*) leaderboard.c_str()];
              leaderboardRequest.range = NSMakeRange(1,10);
              leaderboardRequest.playerScope = GKLeaderboardPlayerScopeGlobal;
              [leaderboardRequest loadScoresWithCompletionHandler: ^(NSArray *scores, NSError *err)
               {
                 if (err != nil)
                 {
                   NSLog(@"%@", err);
                   [s_view sendMessage :"onGetScore" :""];
                 }
                 else
                 {
                   [s_view sendMessage :"onGetScore" : (scores ? [[[scores objectAtIndex :0] formattedValue] UTF8String] : "0")];
                 }
               }];
            }
            else
              if (request == "init")
              {
                static GKLocalPlayer* lp = [GKLocalPlayer localPlayer];
                [lp authenticateWithCompletionHandler:^(NSError *err)
                 {
                   if (err == nil)
                   {
                     [s_view sendMessage :"onGameCenter" :[lp.playerID UTF8String]];
                   }
                   else
                   {
                     NSLog(@"%@", err);
                     [s_view sendMessage :"onGameCenter" :""];
                   }
                 }];
              }
              else
                if (request == "getInfo" && fn.nargs >= 4)
                {
                  
                  character* ch = cast_to<character>(fn.arg(3).to_object());
                  if (ch == NULL)
                  {
                    return;
                  }
                  
                  // get screen coords
                  
                  rect b;
                  ch->get_bound(&b);
                  
                  matrix m;
                  m.set_inverse(ch->get_matrix());
                  m.transform(&b);
                  
                  ch->get_world_matrix(&m);
                  m.transform(&b);
                  
                  // hack
                  static int x = s_x0 + (int) TWIPS_TO_PIXELS(b.m_x_min * s_scale / s_retina);
                  static int y = s_y0 + (int) TWIPS_TO_PIXELS(b.m_y_min * s_scale / s_retina);
                  static int w = (int) TWIPS_TO_PIXELS(b.width() * s_scale / s_retina);
                  static int h = (int) TWIPS_TO_PIXELS(b.height() * s_scale / s_retina);
                  
                  NSArray* playerIDsArray = [NSArray arrayWithObject:[NSString stringWithUTF8String: (char*) fn.arg(2).to_string()]];
                  [GKPlayer loadPlayersForIdentifiers:playerIDsArray withCompletionHandler:^(NSArray *players, NSError *err)
                   {
                     if (err != nil)
                     {
                       // Handle the error.
                       NSLog(@"%@", err);
                       [s_view sendMessage :"onGetInfo" :""];
                     }
                     else
                     {
                       NSLog(@"%@", players);
                       [s_view sendMessage :"onGetInfo" :[[[players objectAtIndex :0] alias] UTF8String]];
                       
                       [[players objectAtIndex:0] loadPhotoForSize:GKPhotoSizeSmall withCompletionHandler:^(UIImage *photo, NSError *error)
                        {
                          if (photo != nil)
                          {
                            // [self storePhoto: photo forPlayer: player];
                            [s_view sendMessage :"onGetImage" :"done"];
                            
                            UIImageView* im = [[[UIImageView alloc] initWithImage: photo] autorelease];
                            im.contentMode = UIViewContentModeScaleAspectFit;
                            im.frame = CGRectMake(x, y, w, h);
                            [s_view addSubview :im];
                            
                            
                          }
                          if (error != nil)
                          {
                            // Handle the error if necessary.
                            NSLog(@"%@", error);
                            [s_view sendMessage :"onGetImage" :"no"];
                          }
                        }];
                       
                     }
                   }];
                }
#endif
    }
    
  }
  
}







