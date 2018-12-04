//
//  as_imap_.h
//  xxx
//
//  Created by Vitaly on 4/26/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <MapKit/MapKit.h>

@interface myCamera : NSObject <UINavigationControllerDelegate, UIImagePickerControllerDelegate>
{
	void* m_parent;
	UIImagePickerController* m_picker;
	UIImage* m_image;
	NSData* m_imageData;
}

@property void* m_parent;
@property (nonatomic, retain) UIImagePickerController* m_picker;
@property (nonatomic, retain) UIImage* m_image;
@property (nonatomic, retain) NSData* m_imageData;

- (void) getJpeg :(float) quality :(int*) size :(const void**) data;
- (void) show;
- (void) hide;

- (id)init;
- (void)dealloc;
- (void)imagePickerController:(UIImagePickerController *)picker	didFinishPickingImage:(UIImage *)image editingInfo:(NSDictionary *)editingInfo;
- (void)imagePickerControllerDidCancel:(UIImagePickerController *)picker;
//- (void)imagePickerController:(UIImagePickerController *)picker didFinishPickingMediaWithInfo:(NSDictionary *)info;

@end

