
#import <Foundation/Foundation.h>

#define objc_dynamic_cast(TYPE, object) \
({ \
TYPE *dyn_cast_object = (TYPE*)(object); \
[dyn_cast_object isKindOfClass:[TYPE class]] ? dyn_cast_object : nil; \
})

@interface myTextField : UITextField 
{
	void* parent;
}

@property void* parent;

- (void) onFocus;
- (int) getMaxLength;


@end
