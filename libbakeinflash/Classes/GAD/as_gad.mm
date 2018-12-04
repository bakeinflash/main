//
//
//

#include <time.h>
#include "as_gad.h"

extern "C"
{
    #import "GADBannerView.h"
    #import "GADRequest.h"
}

@interface myViewController : UIViewController<GADBannerViewDelegate> @end
extern myViewController* s_myController;

@interface gadHandler : NSObject<GADBannerViewDelegate> @end
@implementation gadHandler

    // We've received an ad successfully.
    - (void)adViewDidReceiveAd:(GADBannerView *)adView
    {
      //  NSLog(@"Received ad successfully");
    }

    - (void)adView:(GADBannerView *)view didFailToReceiveAdWithError:(GADRequestError *)error
    {
        NSLog(@"Failed to receive ad with error: %@", [error localizedFailureReason]);
    }

@end

namespace bakeinflash
{
    static GADBannerView* adBanner = NULL;
    
	void	as_gad_ctor(const fn_call& fn)
	// Constructor for ActionScript class
	{
		as_object* obj = new as_gad(fn.get_player());
		fn.result->set_as_object(obj);
	}
	
	void	as_gad_get_visible(const fn_call& fn)
	{
        as_gad* obj = cast_to<as_gad>(fn.this_ptr);
        if (adBanner && obj)
        {
            fn.result->set_bool(adBanner.hidden);
        }
    }
    
    void	as_gad_set_visible(const fn_call& fn)
    {
        as_gad* obj = cast_to<as_gad>(fn.this_ptr);
        if (adBanner && obj && fn.nargs > 0)
        {
                adBanner.hidden = !fn.arg(0).to_bool();
        }
    }
    
    void	as_gad_start(const fn_call& fn)
    {
        static gadHandler* gh = NULL;
		if (gh == NULL && fn.nargs > 0)
		{
            bool transform = false;
            GADAdSize gs;
            CGPoint origin;
           
            if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
            {
                gs = kGADAdSizeFullBanner;
                float w = CGSizeFromGADAdSize(gs).width;
                float h = CGSizeFromGADAdSize(gs).height;
                float xy = (w - h) / 2;
                xy = (w - h) / 2;
       //         origin = CGPointMake(-xy, xy);
      //          transform = true;
                
                origin = CGPointMake((s_myController.view.frame.size.width - w) / 2, s_myController.view.frame.size.height - h);
            }
            else
            {
                gs = kGADAdSizeBanner;
                float w = CGSizeFromGADAdSize(gs).width;
                float h = CGSizeFromGADAdSize(gs).height;
                
                CGSize result = [[UIScreen mainScreen] bounds].size;
                if (result.height == 480)
                {
                    // iPhone Classic
                    // Initialize the banner at the bottom of the screen.
                    origin = CGPointMake((s_myController.view.frame.size.width - w) / 2, s_myController.view.frame.size.height - h);
                }
                else
                {
                    // iPhone 5
                    // Initialize the banner at the bottom of the screen.
                    origin = CGPointMake((s_myController.view.frame.size.width - w) / 2, s_myController.view.frame.size.height - h);
                }
            }
            
            gh = [gadHandler alloc];
            adBanner = [[GADBannerView alloc] initWithAdSize:gs origin:origin];
            
            // Note: Edit SampleConstants.h to provide a definition for kSampleAdUnitID before compiling.
            adBanner.adUnitID = [NSString stringWithUTF8String: fn.arg(0).to_string()];
            adBanner.delegate = gh;
            adBanner.rootViewController = s_myController;
            if (transform)
            {
                adBanner.transform = CGAffineTransformMakeRotation(-M_PI / 2.0);
            }
            [s_myController.view addSubview :adBanner];
            
            static GADRequest *request = [GADRequest request];
           // request.testDevices = @[GAD_SIMULATOR_ID];
           // request.testing = YES;
            [adBanner loadRequest:request];
            
			printf("GAD started with ID: %s\n", fn.arg(0).to_string());
		}
	}

	

	as_gad::as_gad(player* player) :
		as_object(player)
	{
		builtin_member("start",as_gad_start);
		builtin_member("visible", as_value(as_gad_get_visible, as_gad_set_visible));
	}

}



