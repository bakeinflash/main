// as_itableview.cpp
// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "bakeinflash/bakeinflash_as_classes/as_itableview.h"
#include "bakeinflash/bakeinflash_root.h"
//#include "as_itableview.h"  

#ifdef iOS
	#import <MapKit/MapKit.h>  
	#import "as_itableview_.h"

	@interface EAGLView : UIView <UITextFieldDelegate> @end
	extern EAGLView* s_view;
	extern float s_scale;
	extern int s_x0;
	extern int s_y0;
	extern float s_retina;

#endif

namespace bakeinflash
{
	void	as_global_itableview_ctor(const fn_call& fn)
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
		
		// get screen coords
		
		rect b;
		ch->get_bound(&b);
		
		matrix m;
		m.set_inverse(ch->get_matrix());
		m.transform(&b);
		
//		m;
//		ch->get_world_matrix(&m);
//		m.transform(&b);
		
		int x = s_x0 + (int) TWIPS_TO_PIXELS(b.m_x_min * s_scale / s_retina);
		int y = s_y0 + (int) TWIPS_TO_PIXELS(b.m_y_min * s_scale / s_retina);
		int w = (int) TWIPS_TO_PIXELS(b.width() * s_scale / s_retina);
		int h = (int) TWIPS_TO_PIXELS(b.height() * s_scale / s_retina);
		
		fn.result->set_as_object(new as_itableview(x, y, w, h));
	}
	
}

//

#ifdef iOS


namespace bakeinflash
{
	as_itableview::as_itableview(int x, int y, int w, int h) :
		m_edit(false),
		m_delete_title("Delete")
	{
		m_selection_enabled = true;
		m_text_color.set(0, 0, 0, 255);	// black
		m_bk_color.set(0, 0, 0, 0);	// transparent
		CGRect rect = CGRectMake(x, y, w, h);
		myTableView* tv = [[myTableView alloc] initWithFrame:rect style:UITableViewStylePlain];
		tv.dataSource = tv;
		tv.delegate = tv;
		tv.swipeRow = -1;
		tv.parent = this;
		tv.backgroundColor = [UIColor colorWithRed:m_bk_color.m_r/255.0 green:m_bk_color.m_g/255.0 blue:m_bk_color.m_b/255.0 alpha:m_bk_color.m_a/255.0];

		tv.allowsSelectionDuringEditing = YES;
		tv.allowsSelection = YES;

		[s_view addSubview:tv];
		m_tableview = tv;
		
		UISwipeGestureRecognizer *gesture = [[UISwipeGestureRecognizer alloc] initWithTarget:tv action:@selector(handleSwipeFrom:)];
	//	[gesture setDirection: (UISwipeGestureRecognizerDirectionLeft	| UISwipeGestureRecognizerDirectionRight)];
		[tv addGestureRecognizer:gesture];
		[gesture release];
		
	}
	
	as_itableview::~as_itableview()
	{
		myTableView* tv = (myTableView*) m_tableview;
		if (tv)
		{
			[tv removeFromSuperview];
			[tv release];  
		}
	}
	
	bool	as_itableview::get_member(const tu_string& name, as_value* val)
	{
		myTableView* tv = (myTableView*) m_tableview;
		if (name == "edit")
		{
			val->set_bool(m_edit);
			return true;
		}
		else
		if (name == "editStyle")
		{
			val->set_tu_string(m_edit_style);
			return true;
		}
		else
		if (name == "_visible")
		{
			val->set_bool(!tv.hidden);
			return true;
		}
		else
		if (name == "items")
		{
			val->set_as_object(m_items);
			return true;
		}
		else
		if (name == "textColor")
		{
			//TODO
			return true;
		}
		else
		if (name == "selectionEnabled")
		{
			val->set_bool(m_selection_enabled);
			return true;
		}
		return as_object::get_member(name, val);
	}
	
	bool	as_itableview::set_member(const tu_string& name, const as_value& val)
	{
		myTableView* tv = (myTableView*) m_tableview;
		if (name == "edit")
		{
			m_edit = val.to_bool();
			[tv setEditing: m_edit];
			return true;
		}
		else
		if (name == "editStyle")
		{
			m_edit_style = val.to_tu_string();
			return true;
		}
		else
		if (name == "_visible")
		{
			tv.hidden = !val.to_bool();
			return true;
		}
		else
		if (name == "items")
		{
			m_items = cast_to<as_array>(val.to_object());
			if (m_items)
			{
				[tv reloadData];
			}
			return true;
		}
		else
		if (name == "textColor")
		{
			as_object* obj = val.to_object();
			if (obj)
			{
				as_value r, g, b, a;
				obj->get_member("r", &r);
				obj->get_member("g", &g);
				obj->get_member("b", &b);
				obj->get_member("a", &a);
				m_text_color.set(r.to_int(), g.to_int(), b.to_int(), a.to_int());
				[tv reloadData];
			}
			return true;
		}
		else
		if (name == "selectionEnabled")
		{
			m_selection_enabled = val.to_bool();
			[tv reloadData];
			return true;
		}
		else
		if (name == "deleteTitle")
		{
			m_delete_title = val.to_tu_string();
			return true;
		}
		return as_object::set_member(name, val);
	}

}

#endif

//
//
//

@implementation CustomCell

@synthesize primaryLabel,secondaryLabel,myImageView;

- (id)initWithFrame:(CGRect)frame reuseIdentifier:(NSString *)reuseIdentifier style:(int)style
{
	if (self = [super initWithFrame:frame reuseIdentifier:reuseIdentifier])
	{
		m_style = style;
		// Initialization code
		switch (m_style)
		{
			case 0:
				break;
			case 1:
				primaryLabel = [[UILabel alloc]init];
				primaryLabel.textAlignment = UITextAlignmentLeft;
			//	primaryLabel.font = [UIFont systemFontOfSize:22];
				[primaryLabel setFont:[UIFont boldSystemFontOfSize:18]];
				primaryLabel.backgroundColor = [UIColor clearColor];
				
				secondaryLabel = [[UILabel alloc]init];
				secondaryLabel.textAlignment = UITextAlignmentLeft;
			//	secondaryLabel.font = [UIFont systemFontOfSize:22];
				[secondaryLabel setFont:[UIFont boldSystemFontOfSize:18]];
				secondaryLabel.backgroundColor = [UIColor clearColor];
				
				myImageView = [[UIImageView alloc]init];
				[self.contentView addSubview:primaryLabel];
				[self.contentView addSubview:secondaryLabel];
				[self.contentView addSubview:myImageView];
				break;
			case 2:
				primaryLabel = [[UILabel alloc]init];
				primaryLabel.textAlignment = UITextAlignmentLeft;
				primaryLabel.font = [UIFont systemFontOfSize:14];
				primaryLabel.backgroundColor = [UIColor clearColor];
				secondaryLabel = [[UILabel alloc]init];
				secondaryLabel.textAlignment = UITextAlignmentLeft;
				secondaryLabel.font = [UIFont systemFontOfSize:8];
				secondaryLabel.backgroundColor = [UIColor clearColor];
				myImageView = [[UIImageView alloc]init];
				[self.contentView addSubview:primaryLabel];
				[self.contentView addSubview:secondaryLabel];
				[self.contentView addSubview:myImageView];
				break;
		}
	}
	return self;
}

- (void)layoutSubviews
{
	[super layoutSubviews];
	
	switch (m_style)
	{
		case 0:
			break;
		case 1:
		{
			CGRect contentRect = self.contentView.bounds;
			CGFloat boundsX = contentRect.origin.x;
			CGRect frame;
			frame = CGRectMake(boundsX + 5 ,0, CELLSIZE, CELLSIZE);
			myImageView.frame = frame;
			frame = CGRectMake(boundsX + 55 ,9, 100, 25);
			primaryLabel.frame = frame;
			frame = UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone ? CGRectMake(boundsX + 200 ,9, 70, 25) :  CGRectMake(boundsX + 470 ,9, 70, 25);
			secondaryLabel.frame = frame;
			break;
		}
		case 2:
		{
			CGRect contentRect = self.contentView.bounds;
			CGFloat boundsX = contentRect.origin.x;
			CGRect frame;
			frame = CGRectMake(boundsX + 10 ,0, CELLSIZE, CELLSIZE);
			myImageView.frame = frame;
			frame= CGRectMake(boundsX + 70 ,5, 200, 25);
			primaryLabel.frame = frame;
			frame= CGRectMake(boundsX + 70 ,30, 100, 15);
			secondaryLabel.frame = frame;
			break;
		}
	}
	
}

- (void)setSelected:(BOOL)selected animated:(BOOL)animated
{
	[super setSelected:selected animated:animated];
	// Configure the view for the selected state
}


@end


@implementation myTableView
@synthesize parent;
@synthesize swipeRow;

- (void) setEdit: (BOOL) enabled 
{
	bakeinflash::as_itableview* obj = bakeinflash::cast_to<bakeinflash::as_itableview>((bakeinflash::as_itableview*) parent);
	obj->m_edit = enabled;
	obj->m_edit_style = enabled ? "delete" : "";
	[self setEditing: enabled];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView 
{
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section 
{
	bakeinflash::as_itableview* obj = bakeinflash::cast_to<bakeinflash::as_itableview>((bakeinflash::as_itableview*) parent);
	if (obj && obj->m_items) 
	{
		return  obj->m_items->size();		//[your array count];
	}
	return 0;
	
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
	return CELLSIZE;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
	bakeinflash::as_itableview* obj = bakeinflash::cast_to<bakeinflash::as_itableview>((bakeinflash::as_itableview*) parent);
	static NSString *CellIdentifier = @"Cell";
	
//	UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
//	if (cell == nil)
//	{
//		cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier] autorelease];
//	}
	
	int i = indexPath.row;
	if (obj && obj->m_items && i < obj->m_items->size()) 
	{
		const bakeinflash::as_array& a = *obj->m_items;
		int style = 0;	// default string
		bakeinflash::as_object* o = a[i].to_object();
		if (o)
		{
			style = 1;	// hack
		}
	
		CustomCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
		if (cell == nil)
		{
			cell = [[[CustomCell alloc] initWithFrame:CGRectZero reuseIdentifier:CellIdentifier style:style] autorelease];
		}
		
		// Configure the cell...
		if (style == 0)
		{
			cell.textLabel.text = [NSString stringWithUTF8String: a[i].to_string()];
			cell.textLabel.textColor = [UIColor colorWithRed:obj->m_text_color.m_r/255.0 green:obj->m_text_color.m_g/255.0 blue:obj->m_text_color.m_b/255.0 alpha:obj->m_text_color.m_a/255.0];
		}
		else
		{
			cell.textLabel.text = @""; 
			bakeinflash::as_value val;
			
			o->get_member("image", &val);
			cell.myImageView.image = [UIImage imageNamed:[NSString stringWithUTF8String: val.to_string()] ];
			printf("image=%s\n", val.to_string());
			o->get_member("label1", &val);
			cell.primaryLabel.text = [NSString stringWithUTF8String: val.to_string()];
			
			o->get_member("label2", &val);
			cell.secondaryLabel.text = [NSString stringWithUTF8String: val.to_string()];
		}
		
		cell.selectionStyle = obj->m_selection_enabled ? UITableViewCellSelectionStyleBlue : UITableViewCellSelectionStyleNone;
		return cell;
	}
	return nil;
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath 
{    
//	printf("canEditRowAtIndexPath=%d\n", indexPath.row);
//	return indexPath.row == swipeRow ? YES : NO;  
	return YES;
}

-(void)tableView:(UITableView*)tableView willBeginEditingRowAtIndexPath:(NSIndexPath *)indexPath
{
}

- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath 
{
//	return indexPath.row == swipeRow ? UITableViewCellEditingStyleDelete : UITableViewCellEditingStyleNone;  
	
	bakeinflash::as_itableview* obj = bakeinflash::cast_to<bakeinflash::as_itableview>((bakeinflash::as_itableview*) parent);
//	int i = indexPath.row;
	if (obj->m_edit_style == "delete")
	{
		return UITableViewCellEditingStyleDelete;
	}
	else
	if (obj->m_edit_style == "insert")
	{
		return UITableViewCellEditingStyleInsert;
	}
	return UITableViewCellEditingStyleNone;
}


- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
	bakeinflash::as_itableview* obj = bakeinflash::cast_to<bakeinflash::as_itableview>((bakeinflash::as_itableview*) parent);
	int i = indexPath.row;
	if (editingStyle == UITableViewCellEditingStyleDelete)
	{
		if (obj && obj->m_items && i < obj->m_items->size()) 
		{
			bool doDelete = true;
			bakeinflash::as_value function;
			if (obj->get_member("onBeforeDelete", &function))
			{
				bakeinflash::as_environment env;
				env.push(i);
				bakeinflash::as_value val = bakeinflash::call_method(function, &env, obj, 1, env.get_top_index());
				doDelete = val.to_bool();
			}
			
			if (doDelete)
			{
				obj->m_items->remove(i); 
				[self reloadData];
				
				if (obj->get_member("onAfterDelete", &function))
				{
					bakeinflash::as_environment env;
					bakeinflash::as_value val = bakeinflash::call_method(function, &env, obj, 0, env.get_top_index());
				}
				
			}
			[self setEdit: false];
		}
	} 
	else
	if (editingStyle == UITableViewCellEditingStyleInsert)
	{
		if (obj && obj->m_items && i < obj->m_items->size()) 
		{
			bakeinflash::as_value newval("");
			bakeinflash::as_value function;
			if (obj->get_member("onBeforeInsert", &function))
			{
				bakeinflash::as_environment env;
				env.push(i);
				newval = bakeinflash::call_method(function, &env, obj, 1, env.get_top_index());
			}
			
			obj->m_items->insert(i, newval.to_string());
			[self reloadData];
			[self setEdit: false];
		}
	}
	
}

- (void)handleSwipeFrom:(UISwipeGestureRecognizer *)recognizer 
{
	bakeinflash::as_itableview* obj = bakeinflash::cast_to<bakeinflash::as_itableview>((bakeinflash::as_itableview*) parent);
	if (obj && obj->m_selection_enabled)
	{
		CGPoint location = [recognizer locationInView:self];
		NSIndexPath* indexPath = [self indexPathForRowAtPoint:location];
		//printf("swipe=%d\n", indexPath.row);
		swipeRow = indexPath.row;
	
		[self setEdit: true];
	}
}

-(NSIndexPath*) tableView: (UITableView*) tableView willSelectRowAtIndexPath: (NSIndexPath *)indexPath
{
//	return nil;
	return indexPath;
}

- (void)tableView: (UITableView *)tableView didSelectRowAtIndexPath: (NSIndexPath *)indexPath 
{
	int idx = indexPath.row;
//	printf("didSelectRowAtIndexPath: %d\n", idx);
	
	bakeinflash::as_itableview* obj = bakeinflash::cast_to<bakeinflash::as_itableview>((bakeinflash::as_itableview*) parent);
	if (obj && obj->m_selection_enabled)
	{
		bakeinflash::as_value function;
		if (obj->get_member("onSelected", &function))
		{
			bakeinflash::as_environment env;
			env.push(idx);
			bakeinflash::call_method(function, &env, obj, 1, env.get_top_index());
		}
	}

		
	[self setEdit: false];
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	[self setEdit: false];
	[super touchesBegan:touches withEvent:event];
}

- (NSString *)tableView:(UITableView *)tableView titleForDeleteConfirmationButtonForRowAtIndexPath:(NSIndexPath *)indexPath
{
	bakeinflash::as_itableview* obj = bakeinflash::cast_to<bakeinflash::as_itableview>((bakeinflash::as_itableview*) parent);
	if (obj)
	{
		return [NSString stringWithUTF8String: (char*) obj->m_delete_title.c_str()];
	}
	return @"Delete";
}


@end

