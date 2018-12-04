//
//  as_imap_.h
//  xxx
//
//  Created by Vitaly on 4/26/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <MapKit/MapKit.h>

@interface MapAnnotation : NSObject <MKAnnotation> 
{
	NSString *title;
	NSString *subtitle;
	CLLocationCoordinate2D coordinate;
}

@property (nonatomic, readonly) CLLocationCoordinate2D coordinate;
@property (nonatomic, retain) NSString* title;
@property (nonatomic, retain) NSString* subtitle;

// add an init method so you can set the coordinate property on startup
- (id) initWithCoordinate:(CLLocationCoordinate2D) coord;


@end

@interface myMap : MKMapView <MKMapViewDelegate>
{
	void* parent;
}

@property void* parent;
-(void)zoomToFitMapAnnotations:(MKMapView*) mapView;

@end

