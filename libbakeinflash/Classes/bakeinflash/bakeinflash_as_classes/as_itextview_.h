//
//
//  Created by Vitaly on 4/26/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface myTextView : UITextView <UITextViewDelegate, UIScrollViewDelegate>
{
	void* parent;
}
@property void* parent;

@end
