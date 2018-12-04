
//  Created by Vitaly on 4/26/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface PrintPhotoPageRenderer : UIPrintPageRenderer
{
    void* parent;
}
@property void* parent;

@end
