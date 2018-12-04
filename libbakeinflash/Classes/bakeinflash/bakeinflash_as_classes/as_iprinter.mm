// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "bakeinflash/bakeinflash_as_classes/as_iprinter.h"
#include "bakeinflash/bakeinflash_root.h"

#ifdef iOS
#import "as_iprinter_.h"
#import "as_iprinter.h"
#import <UIKit/UIKit.h>

@interface EAGLView : UIView
- (void)startAnimation;
- (void)stopAnimation;
- (void) sendMessage :(const char*) name :(const char*) arg;
@end

extern EAGLView* s_view;

#endif



namespace bakeinflash
{
  void	as_global_iprinter_ctor(const fn_call& fn)
  {
    fn.result->set_as_object(new as_iprinter());
  }
  
  void	as_iprinter_print(const fn_call& fn)
  {
    fn.result->set_bool(false);
    
    as_iprinter* obj = cast_to<as_iprinter>(fn.this_ptr);
    if (obj == NULL)
    {
      return;
    }
    
    obj->m_printJob = cast_to<as_array>(fn.arg(0).to_object());
    if (fn.nargs < 1 || obj->m_printJob == NULL || obj->m_printJob->size() == 0)
    {
      printf("invalid args\n");
      return;
    }
    
    if ([UIPrintInteractionController isPrintingAvailable] == false)
    {
      printf("No printers available\n");
      return;
    }
    
    // Obtain the shared UIPrintInteractionController
    UIPrintInteractionController *controller = [UIPrintInteractionController sharedPrintController];
    if(controller == NULL)
    {
      printf("Couldn't get shared UIPrintInteractionController!");
      return;
    }
    
    // We need a completion handler block for printing.
    UIPrintInteractionCompletionHandler completionHandler = ^(UIPrintInteractionController *printController, BOOL completed, NSError *error)
    {
      //printf("print job completed %d\n", completed);
      if (completed)
      {
        [s_view sendMessage :"onPrintCompleted" : error ? [error.domain UTF8String] : ""];
      }
    };
    
    
    // Obtain a printInfo so that we can set our printing defaults.
    UIPrintInfo *printInfo = [UIPrintInfo printInfo];
    
    // This application prints photos. UIKit will pick a paper size and print
    // quality appropriate for this content type.
    printInfo.outputType = UIPrintInfoOutputPhoto;
    
    printInfo.jobName = @"myPhotoPrinter"; //[[self.imageURL path] lastPathComponent];
    
    //   printInfo.orientation = UIPrintInfoOrientationLandscape;
    
    // Use this printInfo for this print job.
    controller.printInfo = printInfo;
    
    PrintPhotoPageRenderer *pageRenderer = [[PrintPhotoPageRenderer alloc]init];
    pageRenderer.parent = obj;
    
    controller.printPageRenderer = pageRenderer;
    [pageRenderer release];
    
    if([UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPad)
    {
      CGSize scr = [[UIScreen mainScreen] bounds].size;
      [controller presentFromRect:CGRectMake(scr.width / 2, scr.height / 2, 0, 0) inView:s_view animated:YES completionHandler:completionHandler];  // iPad
    }
    else
    {
      [controller presentAnimated:YES completionHandler:completionHandler];  // iPhone
    }
    
    fn.result->set_bool(true);
  }
  
  
  as_iprinter::as_iprinter()
  {
    builtin_member("print", as_iprinter_print);
  }
  
  as_iprinter::~as_iprinter()
  {
  }
  
}


@implementation PrintPhotoPageRenderer

@synthesize parent;

// This code always draws one image at print time.
-(NSInteger)numberOfPages
{
  int n = 0;
  bakeinflash::as_iprinter* obj = bakeinflash::cast_to<bakeinflash::as_iprinter>((bakeinflash::as_iprinter*) parent);
  if (obj && obj->m_printJob)
  {
    n = obj->m_printJob->size();
  }
  printf("print pages: %d\n", n);
  return n;
}

- (void)drawPageAtIndex:(NSInteger)pageIndex inRect:(CGRect)printableRect
{
  bakeinflash::as_iprinter* obj = bakeinflash::cast_to<bakeinflash::as_iprinter>((bakeinflash::as_iprinter*) parent);
  if (obj && obj->m_printJob && pageIndex < obj->m_printJob->size())
  {
    bakeinflash::as_object* item = obj->m_printJob->operator[](pageIndex).to_object();
    if (item)
    {
      bakeinflash::as_value urlb;
      item->get_member("url", &urlb);
      bakeinflash::as_value label;
      item->get_member("label", &label);
      //  bakeinflash::as_value datetime;
      //  item->get_member("datetime", &datetime);
      
      // printf("printing '%s\n%s\n%s'\n",  urlb.to_string(), datetime.to_string(), label.to_string());
      NSString* path = [NSString stringWithUTF8String: urlb.to_string()];
      NSURL *url = [NSURL URLWithString:path];
      NSData *data = [[NSData alloc] initWithContentsOfURL:url];
      UIImage *image = [[UIImage alloc] initWithData:data];
      
      
      CGRect destRect;
      
      // When drawPageAtIndex:inRect: paperRect reflects the size of
      // the paper we are printing on and printableRect reflects the rectangle
      // describing the imageable area of the page, that is the portion of the page
      // that the printer can mark without clipping.
      CGSize paperSize = self.paperRect.size;
      CGSize imageableAreaSize = self.printableRect.size;
      
      // If the paperRect and printableRect have the same size, the sheet is borderless and we will use
      // the fill algorithm. Otherwise we will uniformly scale the image to fit the imageable area as close
      // as is possible without clipping.
      BOOL fillSheet = paperSize.width == imageableAreaSize.width && paperSize.height == imageableAreaSize.height;
      CGSize imageSize = [image size];
      if(fillSheet)
      {
        destRect = CGRectMake(0, 0, paperSize.width, paperSize.height);
      }
      else
      {
        destRect = self.printableRect;
      }
      
      // Calculate the ratios of the destination rectangle width and height to the image width and height.
      float width_scale = destRect.size.width / imageSize.width;
      float height_scale = destRect.size.height / imageSize.height;
      
      float scale = fminf(width_scale, height_scale) * 0.90f;
      
      /*   if(fillSheet)
       {
       scale = width_scale > height_scale ? width_scale : height_scale;	  // This produces a fill to the entire sheet and clips content.
       }
       else
       {
       scale = width_scale < height_scale ? width_scale : height_scale;	  // This shows all the content at the expense of additional white space.
       }*/
      
      if (image != nil)
      {
        //  UIFont *font = [UIFont fontWithName:@"Helvetica-Bold" size: 8];
        //  NSString* dt = [NSString stringWithUTF8String: datetime.to_string()];
        //  CGSize dtSize = [dt sizeWithFont:font];
        
        // to center
        float x0 = (paperSize.width - imageSize.width * scale) / 2;
        float y0 = 20; // * dtSize.height; // (paperSize.height - imageSize.height * scale) / 2,
        destRect = CGRectMake(x0, y0, imageSize.width * scale, imageSize.height * scale);
        
        [image drawInRect:destRect];
        
        
        
        if (label.is_undefined() == false)
        {
          UIFont *font2 = [UIFont fontWithName:@"Helvetica-Bold" size: 11];
          NSString* jobTitle = [NSString stringWithUTF8String: label.to_string()];
          CGSize titleSize = [jobTitle sizeWithFont:font2];
          
          CGRect textRect = CGRectMake(x0, y0 + imageSize.height * scale + titleSize.height / 2,
                                       imageSize.width * scale, imageSize.height * scale);
          
          //              NSDictionary *attributes = [NSDictionary dictionaryWithObjectsAndKeys: font2, NSFontAttributeName,
          //             [NSNumber numberWithFloat:1.0], NSBaselineOffsetAttributeName, nil];
          
          NSMutableParagraphStyle *style = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
          [style setLineBreakMode: NSLineBreakByWordWrapping];
          
          NSDictionary *attributes = @{NSFontAttributeName: font2, NSParagraphStyleAttributeName: style};
          
          [jobTitle drawInRect:textRect withAttributes:attributes];
        }
        
        tu_string s((int) pageIndex);
        [s_view sendMessage :"onPrintPageCompleted" : s.c_str()];
        
      }
      else
      {
        NSLog(@"%s No image to draw!", __func__);
      }
      
    }
    else
    {
      NSLog(@"%s No print index", __func__);
    }
  }
  else
  {
    NSLog(@"%s No print job", __func__);
  }
  
}

@end





