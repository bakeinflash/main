//
//
//

#include <time.h>
#include "as_flurry.h"
#import "Flurry.h"
#import "FlurryAdDelegate.h"
#import "FlurryAds.h"

@interface myViewController : UIViewController @end
extern myViewController* s_myController;

@interface flurryHandler : NSObject @end
@implementation flurryHandler

- (void)spaceDidReceiveAd:(NSString *)adSpace
{
    NSLog(@"Ad Space [%@] Did Receive Ad", adSpace);
}

- (void)spaceDidFailToReceiveAd:(NSString *)adSpace error:(NSError *)error
{
    NSLog(@"Ad Space [%@] Did Fail to Receive Ad with error [%@]", adSpace, error);
}

- (BOOL) spaceShouldDisplay:(NSString*)adSpace interstitial:(BOOL) interstitial
{
    NSLog(@"spaceShouldDisplay: %@", adSpace);
    return true;
}

@end

namespace bakeinflash
{

	void	as_flurry_ctor(const fn_call& fn)
	// Constructor for ActionScript class
	{
		as_object* obj = new as_flurry(fn.get_player());
		fn.result->set_as_object(obj);
	}
	
	void	as_flurry_start(const fn_call& fn)
	{
		if (fn.nargs > 0)
		{
		//	[Flurry setDebugLogEnabled:YES];
		//	[Flurry setShowErrorInLogEnabled:YES];
			[Flurry setSessionReportsOnPauseEnabled:YES];
			[Flurry startSession:  [NSString stringWithUTF8String: fn.arg(0).to_string()] ];
			printf("flurry started with ID: %s\n", fn.arg(0).to_string());
		}
	}

    void	as_flurry_banner(const fn_call& fn)
	{
        static NSObject* flurry_handler = NULL;
        if (flurry_handler == NULL && fn.nargs > 0)
        {
            [FlurryAds initialize :s_myController];
            [FlurryAds enableTestAds:YES];
            
            flurry_handler = [flurryHandler alloc];
            
            // Optional step: Register yourself as a delegate for ad callbacks
            [FlurryAds setAdDelegate :flurry_handler];
            
            // Fetch and display banner ad
            [FlurryAds fetchAndDisplayAdForSpace :[NSString stringWithUTF8String: fn.arg(0).to_string()]  view:s_myController.view size:BANNER_BOTTOM];
        }
    }
    
	void	as_flurry_startEvent(const fn_call& fn)
	{
		if (fn.nargs >= 2)
		{
			NSDictionary *params = [NSDictionary dictionaryWithObjectsAndKeys:
			 @"Bottle", [NSString stringWithUTF8String: fn.arg(1).to_string()],
//			 @"Registered", @"User_Status", // Capture user status
			 nil];
			
			[Flurry logEvent: [NSString stringWithUTF8String: fn.arg(0).to_string()] withParameters: params timed:YES];

		}
	}
	
	void	as_flurry_endEvent(const fn_call& fn)
	{
		if (fn.nargs >= 2)
		{
			NSDictionary *params = [NSDictionary dictionaryWithObjectsAndKeys:
															@"Bottle", [NSString stringWithUTF8String: fn.arg(1).to_string()],
															nil];

			[Flurry endTimedEvent: [NSString stringWithUTF8String: fn.arg(0).to_string()] withParameters: params];
		}
	}
	
	as_flurry::as_flurry(player* player) :
		as_object(player)
	{
		builtin_member("start",as_flurry_start);
		builtin_member("startEvent",as_flurry_startEvent);
		builtin_member("endEvent",as_flurry_endEvent);
		builtin_member("banner",as_flurry_banner);
	}

}

