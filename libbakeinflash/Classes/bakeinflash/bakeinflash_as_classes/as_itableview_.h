//
//
//  Created by Vitaly on 4/26/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface CustomCell : UITableViewCell 
{
	UILabel *primaryLabel;
	UILabel *secondaryLabel;
	UIImageView *myImageView;
	int m_style;
}

@property(nonatomic,retain)UILabel *primaryLabel;
@property(nonatomic,retain)UILabel *secondaryLabel;
@property(nonatomic,retain)UIImageView *myImageView;
@end


@interface myTableView : UITableView <UITableViewDataSource, UITableViewDelegate, UIScrollViewDelegate>
{
	void* parent;
	int swipeRow;
}

@property void* parent;
@property int swipeRow;

@end
