//
//
//

#include <time.h>
#include "as_GAI.h"

namespace bakeinflash
{

	void	as_ga_ctor(const fn_call& fn)
	// Constructor for ActionScript class
	{
		as_object* obj = new as_ga(fn.get_player());
		fn.result->set_as_object(obj);
	}
	
	static id<GAITracker> s_tracker = NULL;
	void	as_ga_start(const fn_call& fn)
	{
		if (fn.nargs > 0)
		{
			// GAI tracker
			// Optional: automatically send uncaught exceptions to Google Analytics.
			[GAI sharedInstance].trackUncaughtExceptions = YES;
				 
			// Optional: set Google Analytics dispatch interval to e.g. 20 seconds.
			[GAI sharedInstance].dispatchInterval = 20;
				 
			// Optional: set debug to YES for extra debugging information.
			[GAI sharedInstance].debug = YES;
				 
			// Create tracker instance.
			s_tracker = [[GAI sharedInstance] trackerWithTrackingId: [NSString stringWithUTF8String: fn.arg(0).to_string()]];
			printf("Google started with ID: %s\n", fn.arg(0).to_string());
		}
	}
	
	as_ga::as_ga(player* player) :
		as_object(player)
	{
		builtin_member("start",as_ga_start);
	}

}