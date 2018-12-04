// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "bakeinflash/bakeinflash_as_classes/as_igamecenter.h"
#include "bakeinflash/bakeinflash_root.h"

#ifdef iOS

	#import <GameKit/GameKit.h>

	@interface EAGLView : UIView  <GKLeaderboardViewControllerDelegate>
	- (void)startAnimation;
	- (void)stopAnimation;
	- (void) sendMessage :(const char*) name :(const char*) arg;
	@end

	extern EAGLView* s_view;
	extern float s_scale;
	extern int s_x0;
	extern int s_y0;
	extern float s_retina;

	@interface myViewController : UIViewController @end
	extern myViewController* s_myController;

#endif

namespace bakeinflash
{
	void	as_global_igamecenter_ctor(const fn_call& fn)
	{
		fn.result->set_as_object(new as_igamecenter());
	}
	
	void	as_igamecenter_getEnabled(const fn_call& fn)
	{
		as_igamecenter* gc = cast_to<as_igamecenter>(fn.this_ptr);
		if (gc)
		{
			GKLocalPlayer* lp = [GKLocalPlayer localPlayer];
			fn.result->set_bool(lp.isAuthenticated);
		}
	}
	
	void	as_igamecenter_login(const fn_call& fn)
	{
		as_igamecenter* gc = cast_to<as_igamecenter>(fn.this_ptr);
		if (gc)
		{
			GKLocalPlayer* lp = [GKLocalPlayer localPlayer];
			if (lp.isAuthenticated == false)
			{
				[lp authenticateWithCompletionHandler:^(NSError *err)
				 {
					 if (err == nil)
					 {
						 [s_view sendMessage :"onGameCenterLoginDone" :[lp.playerID UTF8String]];
					 }
					 else
					 {
						 NSLog(@"%@", err);
						 [s_view sendMessage :"onGameCenterLoginError" :[err.localizedDescription UTF8String]];
					 }
				 }];
			}
		}
	}
	
	void showLeaderBoard()
	{
		GKLeaderboardViewController *leaderboardController = [[[GKLeaderboardViewController alloc] init] autorelease];
		if (leaderboardController != nil)
		{
			leaderboardController.leaderboardDelegate = s_view;
			leaderboardController.timeScope = GKLeaderboardTimeScopeAllTime;
//			leaderboardController.category = @"bestfisher"; //(char*) leaderboard.c_str()];
			[s_myController presentViewController: leaderboardController animated: YES completion:nil];
		}
	}
	
	void	as_igamecenter_showLeaders(const fn_call& fn)
	{
		as_igamecenter* gc = cast_to<as_igamecenter>(fn.this_ptr);
		if (gc)
		{
			GKLocalPlayer* lp = [GKLocalPlayer localPlayer];
			if (lp.isAuthenticated)
			{
				showLeaderBoard();
			}
			else
			{
				[lp authenticateWithCompletionHandler:^(NSError *err)
				 {
					 if (err == nil)
					 {
						 showLeaderBoard();
					 }
					 else
					 {
						 NSLog(@"%@", err);
						// [[UIApplication sharedApplication] openURL:[NSURL URLWithString:@"gamecenter:"]];
						 [s_view sendMessage :"onGameCenterDisabled" :[err.localizedDescription UTF8String]];
					 }
				 }];
			}
		}
	}
	
	void	as_igamecenter_getTop(const fn_call& fn)
	{
		as_igamecenter* gc = cast_to<as_igamecenter>(fn.this_ptr);
		if (gc)
		{
			GKLeaderboard *leaderboardRequest = [[[GKLeaderboard alloc] init] autorelease];
			if (leaderboardRequest != nil)
			{
				leaderboardRequest.playerScope = GKLeaderboardPlayerScopeGlobal;
				leaderboardRequest.timeScope = GKLeaderboardTimeScopeAllTime;
//				leaderboardRequest.category = @"bestfisher"; //[NSString stringWithUTF8String: (char*) leaderboard.c_str()];
				leaderboardRequest.range = NSMakeRange(1, 3);
				[
				 leaderboardRequest loadScoresWithCompletionHandler: ^(NSArray *scores, NSError *err)
				 {
					 if (err != nil)
					 {
						 NSLog(@"%@", err);
						 [s_view sendMessage :"onGetTopError" :[err.localizedDescription UTF8String]];
					 }
					 
					 if (scores != nil)
					 {
						 // Process the score information.
						 NSLog(@"%@", scores);
						 char str[256];
						 snprintf(str, 256, "%s,%s", [[[scores objectAtIndex :0] formattedValue] UTF8String], [[[scores objectAtIndex :0] playerID] UTF8String]);
						 [s_view sendMessage :"onGetTopDone" :str];
					 }
				 }];
			}
		}
	}
		
	void	as_igamecenter_getScore(const fn_call& fn)
	{
		as_igamecenter* gc = cast_to<as_igamecenter>(fn.this_ptr);
		if (gc)
		{
			if (fn.nargs > 0)
			{
				NSArray* playerIDs = [NSArray arrayWithObject:[NSString stringWithUTF8String: (char*) fn.arg(0).to_string()]];
				GKLeaderboard *leaderboardRequest = [[[GKLeaderboard alloc] initWithPlayerIDs: playerIDs] autorelease];
				leaderboardRequest.timeScope = GKLeaderboardTimeScopeAllTime;
//				leaderboardRequest.category = @"bestfisher";  //[NSString stringWithUTF8String: (char*) leaderboard.c_str()];
				leaderboardRequest.range = NSMakeRange(1, 3);
				leaderboardRequest.playerScope = GKLeaderboardPlayerScopeGlobal;
				[leaderboardRequest loadScoresWithCompletionHandler: ^(NSArray *scores, NSError *err)
				 {
					 if (err != nil)
					 {
						 NSLog(@"%@", err);
						 [s_view sendMessage :"onGetScoreError" :[err.localizedDescription UTF8String]];
					 }
					 else
					 {
						 [s_view sendMessage :"onGetScoreDone" : (scores ? [[[scores objectAtIndex :0] formattedValue] UTF8String] : "0")];
					 }
				 }];
			}
		}
	}
	
	
	
	void	as_igamecenter_setScore2(int val)
	{
		printf("as_igamecenter_setScore2: %d\n", val);
		GKScore *scoreReporter = [[GKScore alloc] init]; // initWithCategory: @"bestfisher"]; // [NSString stringWithUTF8String: (char*) leaderboard.c_str()]];
		scoreReporter.value = val;
		scoreReporter.context = 0;
//		scoreReporter.shouldSetDefaultLeaderboard = YES;
		[
		 scoreReporter reportScoreWithCompletionHandler:^(NSError *err)
		 {
			 if (err != nil)
			 {
				 NSLog(@"%@", err);
				 [s_view sendMessage :"onSetScoreError" :[err.localizedDescription UTF8String]];
			 }
			 else
			 {
				 [s_view sendMessage :"onSetScoreDone" :""];
			 }
		 }];
	}
	void	as_igamecenter_setScore(const fn_call& fn)
	{
		as_igamecenter* gc = cast_to<as_igamecenter>(fn.this_ptr);
		if (gc)
		{
			static int val;
			val = fn.arg(0).to_int();
			printf("as_igamecenter_setScore: %d\n", val);
			
			GKLocalPlayer* lp = [GKLocalPlayer localPlayer];
			if (lp.isAuthenticated)
			{
				as_igamecenter_setScore2(val);
			}
			else
			{
				[lp authenticateWithCompletionHandler:^(NSError *err)
				 {
					 if (err == nil)
					 {
						 as_igamecenter_setScore2(val);
					 }
					 else
					 {
						 NSLog(@"%@", err);
						 // [[UIApplication sharedApplication] openURL:[NSURL URLWithString:@"gamecenter:"]];
						 [s_view sendMessage :"onGameCenterDisabled" :[err.localizedDescription UTF8String]];
					 }
				 }];
			}
		}
	}
		
		
	void	as_igamecenter_getInfo(const fn_call& fn)
	{
		as_igamecenter* gc = cast_to<as_igamecenter>(fn.this_ptr);
		if (gc)
		{
			// playerID, movieclip
			if (fn.nargs >= 2)
			{
				character* ch = cast_to<character>(fn.arg(1).to_object());
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
				
		//		m;
		//		ch->get_world_matrix(&m);
		//		m.transform(&b);
				
				// hack
				static int x;
				x = s_x0 + (int) TWIPS_TO_PIXELS(b.m_x_min * s_scale / s_retina);
				static int y;
				y = s_y0 + (int) TWIPS_TO_PIXELS(b.m_y_min * s_scale / s_retina);
				static int w;
				w = (int) TWIPS_TO_PIXELS(b.width() * s_scale / s_retina);
				static int h;
				h = (int) TWIPS_TO_PIXELS(b.height() * s_scale / s_retina);

				// hack, static
				static UIImageView* im;
				
				NSArray* playerIDsArray = [NSArray arrayWithObject:[NSString stringWithUTF8String: (char*) fn.arg(0).to_string()]];
				[GKPlayer loadPlayersForIdentifiers:playerIDsArray withCompletionHandler:^(NSArray *players, NSError *err)
				 {
					 if (err != nil)
					 {
						 if (im)
						 {
							 [im removeFromSuperview];
							 im = NULL;
						 }
						 
						 // Handle the error.
						 NSLog(@"%@", err);
						 [s_view sendMessage :"onGetInfoError" :[err.localizedDescription UTF8String]];
					 }
					 else
					 {
						 NSLog(@"%@", players);
						 [s_view sendMessage :"onGetInfoDone" :[[[players objectAtIndex :0] alias] UTF8String]];
						 
						 [[players objectAtIndex:0] loadPhotoForSize:GKPhotoSizeSmall withCompletionHandler:^(UIImage *photo, NSError *error)
							{
								
								if (im)
								{
									[im removeFromSuperview];
									im = NULL;
								}
								
								if (photo != nil)
								{
									[s_view sendMessage :"onGetImageDone" :""];
									
									im = [[[UIImageView alloc] initWithImage: photo] autorelease];
									im.contentMode = UIViewContentModeScaleAspectFit;
									im.frame = CGRectMake(x, y, w, h);
									[s_view addSubview :im];
								}
								if (error != nil)
								{
									// Handle the error if necessary.
									NSLog(@"%@", error);
									[s_view sendMessage :"onGetImageError" :[err.localizedDescription UTF8String]];
								}
							}];
						 
					 }
				 }];
			}
		}
	}
	
}

//

#ifdef iOS


namespace bakeinflash
{
	as_igamecenter::as_igamecenter()
	{
		builtin_member("login", as_igamecenter_login);
		builtin_member("showLeaders", as_igamecenter_showLeaders);
		builtin_member("getTop", as_igamecenter_getTop);
		builtin_member("getScore", as_igamecenter_getScore);
		builtin_member("setScore", as_igamecenter_setScore);
		builtin_member("getInfo", as_igamecenter_getInfo);
		builtin_member("enabled", as_value(as_igamecenter_getEnabled, as_value()));
	}
	
	as_igamecenter::~as_igamecenter()
	{
	}
	
	bool	as_igamecenter::get_member(const tu_string& name, as_value* val)
	{
		//myWebView* www = (myWebView*) m_iwebview;
		if (name == "isAuthenticated")
		{
			GKLocalPlayer* lp = [GKLocalPlayer localPlayer];
			val->set_bool(lp.isAuthenticated);
			return true;
		}
		return as_object::get_member(name, val);
	}
	
	bool	as_igamecenter::set_member(const tu_string& name, const as_value& val)
	{
		if (name == "leaderBoard")
		{
			m_leader_board = val.to_tu_string();
			return true;
		}
		else
		if (name == "topImage")
		{
			m_top_image = cast_to<character>(val.to_object());
			return true;
		}
		return as_object::set_member(name, val);
	}

}

#endif

