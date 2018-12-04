// as_imap.cpp
// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "bakeinflash/bakeinflash_as_classes/as_imap.h"
#include "bakeinflash/bakeinflash_root.h"
#include "base/tu_queue.h"
//#include "as_imap.h"  

#ifdef iOS
	#import <MapKit/MapKit.h>  
	#import "as_imap_.h"

	@interface EAGLView : UIView <UITextFieldDelegate> @end
	extern EAGLView* s_view;
#endif

namespace bakeinflash
{
	void	as_global_imap_ctor(const fn_call& fn)
	// Constructor for ActionScript class XMLSocket
	{
		if (fn.nargs < 1)
		{
			return;
		}
		
		character* ch = cast_to<character>(fn.arg(0).to_object());
		if (ch == NULL)
		{
			return;
		}
		
		rect b;
		ch->get_bound(&b);
		int x = (int) TWIPS_TO_PIXELS(b.m_x_min);
		int y = (int) TWIPS_TO_PIXELS(b.m_y_min);
		int w = (int) TWIPS_TO_PIXELS(b.width());
		int h = (int) TWIPS_TO_PIXELS(b.height());
		
		as_array* latlon = NULL;
		if (fn.nargs > 1)
		{
			latlon = cast_to<as_array>(fn.arg(1).to_object()); 
		}
		fn.result->set_as_object(new as_imap(x, y, w, h, latlon));
	}
}

#ifdef WIN32

namespace bakeinflash
{
	as_imap::as_imap(int x, int y, int w, int h, as_array* latlon)
	{
	}

	as_imap::~as_imap()
	{
	}
}

#endif

//

#ifdef iOS

namespace bakeinflash
{
	as_imap::as_imap(int x, int y, int w, int h, as_array* latlon)
	{
		//     builtin_member("setTextFormat", map_scale);
		
		CLLocationManager* locationManager = [[CLLocationManager alloc] init];
	//vv	locationManager.delegate = s_view;
		//  locationManager.desiredAccuracy = kCLLocationAccuracyKilometer;
		//   locationManager.distanceFilter = 10; // or whatever
		[locationManager startUpdatingLocation];
		m_location_manager = locationManager;
		
		
		CGRect rect = CGRectMake(x, y, w, h);
		myMap* map = [[myMap alloc] initWithFrame:rect];
		map.showsUserLocation = YES;    // starts updating user location
		map.delegate = map;
		map.parent = this;
		
		if (latlon)
		{
			for (int i = 0; i < latlon->size(); i++)
			{
				CLLocationCoordinate2D coo;
				
				as_object* item = (*latlon)[i].to_object();
				if (item == NULL)
				{
					continue;
				}
				as_value val;
				if (item->get_member("latitude", &val) == false)
				{
					continue;
				}
				coo.latitude = val.to_number();
				
				if (item->get_member("longitude", &val) == false)
				{
					continue;
				}
				coo.longitude = val.to_number();
				
				if (item->get_member("title", &val) == false)
				{
					continue;
				}
				tu_string title  = val.to_tu_string();
				
				MapAnnotation* newAnnotation = [[MapAnnotation alloc] initWithCoordinate:coo];
				newAnnotation.title = [NSString stringWithUTF8String: (char*) title.c_str()];
				[map addAnnotation:newAnnotation];
			}
		}
		
		[s_view addSubview:map];
		m_map = map;
	}
	
	as_imap::~as_imap()
	{
		
		myMap* map = (myMap*) m_map;
		[map removeFromSuperview];
		[map release];  
		
		CLLocationManager* locationManager = (CLLocationManager*) m_location_manager;
		[locationManager release];  
	}
	
	void as_imap::notifySelect(const char* title)
	{
		bakeinflash::as_value function;
		if (get_member("onSelect", &function))
		{
			as_environment env;
			env.push(title);
			call_method(function, &env, this, 1, env.get_top_index());
		}
		myprintf("select=%s\n", title);
	}
	
	void as_imap::notifyDeselect(const char* title)
	{
		bakeinflash::as_value function;
		if (get_member("onSelect", &function))
		{
			as_environment env;
			env.push(title);
			call_method(function, &env, this, 1, env.get_top_index());
		}
		myprintf("deselect=%s\n", title);
	}
	
}



@implementation MapAnnotation
@synthesize coordinate;
@synthesize title;
@synthesize subtitle;

- (id) initWithCoordinate:(CLLocationCoordinate2D)coord
{
	coordinate = coord;
	return self;
}

- (void) dealloc
{
	[title release];
	[subtitle release];
	[super dealloc];
}

@end

@implementation myMap
@synthesize parent;

static NSString* const ANNOTATION_SELECTED_DESELECTED = @"mapAnnotationSelectedOrDeselected";

- (void)mapView:(MKMapView* )mapView didAddAnnotationViews:(NSArray* )views
{
	for (MKAnnotationView* anAnnotationView in views)
	{
		[anAnnotationView setCanShowCallout:YES];
		
		[anAnnotationView addObserver:self
											 forKeyPath:@"selected"
													options:NSKeyValueObservingOptionNew
													context:ANNOTATION_SELECTED_DESELECTED];
	}
	[self zoomToFitMapAnnotations:self] ;
}


//for (MKAnnotationView* anAnnotationView in views)
//{
//    [anAnnotationView removeObserver:annotation forKeyPath:@"selected"];
//}

- (void)observeValueForKeyPath:(NSString* )keyPath ofObject:(id)object change:(NSDictionary* )change context:(void* )context
{
	// bake = selected item
	MapAnnotation* bike = [object annotation];
	NSString* action = (NSString* )context;
	if ([action isEqualToString:ANNOTATION_SELECTED_DESELECTED]) 
	{
		BOOL annotationSelected = [[change valueForKey:@"new"] boolValue];
		bakeinflash::as_imap* map = (bakeinflash::as_imap*) parent;
		if (annotationSelected) 
		{
			map->notifySelect([bike.title UTF8String]);
		}
		else
		{
			map->notifyDeselect([bike.title UTF8String]);
		}
	}
}

-(void)zoomToFitMapAnnotations:(MKMapView*)mapView
{
	if([mapView.annotations count] == 0)
		return;
	
	CLLocationCoordinate2D topLeftCoord;
	topLeftCoord.latitude = -90;
	topLeftCoord.longitude = 180;
	
	CLLocationCoordinate2D bottomRightCoord;
	bottomRightCoord.latitude = 90;
	bottomRightCoord.longitude = -180;
	
	for(MapAnnotation* annotation in mapView.annotations)
	{
		topLeftCoord.longitude = fmin(topLeftCoord.longitude, annotation.coordinate.longitude);
		topLeftCoord.latitude = fmax(topLeftCoord.latitude, annotation.coordinate.latitude);
		
		bottomRightCoord.longitude = fmax(bottomRightCoord.longitude, annotation.coordinate.longitude);
		bottomRightCoord.latitude = fmin(bottomRightCoord.latitude, annotation.coordinate.latitude);
	}
	
	MKCoordinateRegion region;
	region.center.latitude = topLeftCoord.latitude - (topLeftCoord.latitude - bottomRightCoord.latitude)*  0.5;
	region.center.longitude = topLeftCoord.longitude + (bottomRightCoord.longitude - topLeftCoord.longitude)*  0.5;
	region.span.latitudeDelta = fabs(topLeftCoord.latitude - bottomRightCoord.latitude)*  1.1; // Add a little extra space on the sides
	region.span.longitudeDelta = fabs(bottomRightCoord.longitude - topLeftCoord.longitude)*  1.1; // Add a little extra space on the sides
	
	region = [mapView regionThatFits:region];
	[mapView setRegion:region animated:YES];
}

@end

#endif		// iOS

//

#ifdef ANDROID

namespace bakeinflash
{
	as_imap::as_imap(int x, int y, int w, int h, as_array* latlon)
	{
	}

	as_imap::~as_imap()
	{
	}
}

#endif
