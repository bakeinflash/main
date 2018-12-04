// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "bakeinflash/bakeinflash_as_classes/as_inapp.h"
#include "bakeinflash/bakeinflash_root.h"

#ifdef iOS
#import "as_inapp_.h"

//@interface EAGLView : UIView <UITextFieldDelegate> @end
//extern EAGLView* s_view;
#endif

namespace bakeinflash
{

	void	as_inapp_buy(const fn_call& fn)
	{
		fn.result->set_bool(false);
		as_inapp* obj = cast_to<as_inapp>(fn.this_ptr);
		if (obj == NULL) 
		{
			return;
		}
		
		if (fn.nargs < 1)
		{
			return;
		}
		
		fn.result->set_bool(false);
#ifdef iOS 
		InAppPurchaseManager* inapp = (InAppPurchaseManager*) obj->m_inapp;
		if (inapp)
		{
			NSString* productID = [NSString stringWithUTF8String: fn.arg(0).to_string()];
			bool rc = [inapp purchase :productID];  
			fn.result->set_bool(rc);
		}
#endif
	}
	
	void	as_inapp_load(const fn_call& fn)
	{
		fn.result->set_bool(false);
		as_inapp* obj = cast_to<as_inapp>(fn.this_ptr);
		if (obj == NULL) 
		{
			return;
		}
			
		if (fn.nargs < 1)
		{
			return;
		}
		
		as_array* a = cast_to<as_array>(fn.arg(0).to_object());
		if (a == NULL)
		{
			return;
		}
		
#ifdef iOS 
		InAppPurchaseManager* inapp = (InAppPurchaseManager*) obj->m_inapp;
		if (inapp)
		{
			NSMutableSet* products = [[NSMutableSet alloc] init];
			for (int i = 0; i < a->size(); i++)
			{
				as_value val;
				if (a->get_member(i, &val))
				{
					NSString* productID = [NSString stringWithUTF8String: val.to_string()];
					[products addObject :productID];
				}
			}
			bool rc = [inapp loadStore :products];  
			fn.result->set_bool(rc);
		}
#endif
	}

	as_inapp* inapp_init()
	{
		// Create built-in object.
		as_inapp*	obj = new as_inapp();
		return obj;
	}

	as_inapp::as_inapp()
	{
#ifdef iOS 
		InAppPurchaseManager* inapp = [[InAppPurchaseManager alloc] init];
		inapp.m_parent = this;
		m_inapp = inapp;
#endif		
		builtin_member("buy", as_inapp_buy);
		builtin_member("load", as_inapp_load);	// loadStore
	}

	as_inapp::~as_inapp()
	{
	}
	
	void as_inapp::onload(void* xproducts, void* xinvalid_products)
	{
		bakeinflash::as_value function;
		if (get_member("onLoad", &function))
		{
			as_environment env;
			smart_ptr<as_array> a = new as_array();
			smart_ptr<as_array> b = new as_array();
			env.push(b.get());
			env.push(a.get());

#ifdef iOS 
			NSArray* products = (NSArray*) xproducts;
		 for (int i = 0; i < products.count; i++)
		 {
			 SKProduct* p = [products objectAtIndex: i];
//			 NSLog(@"Product title: %@" , p.localizedTitle);
			 const char* localizedTitle = [p.localizedTitle UTF8String];
//			 NSLog(@"Product description: %@" , p.localizedDescription);
			 const char* localizedDescription = [p.localizedDescription UTF8String];
//			 NSLog(@"Product price: %@" , p.price);
			 double price = [p.price doubleValue];
//			 NSLog(@"Product id: %@" , p.productIdentifier);
			 const char* productIdentifier = [p.productIdentifier UTF8String];
			 
			 smart_ptr<as_object> obj = new as_object();
			 obj->set_member("title", localizedTitle);
			 obj->set_member("desc", localizedDescription);
			 obj->set_member("id", productIdentifier);
			 obj->set_member("price", price);
			 a->push(obj.get());
		 }
			 
			NSArray* invalid_products = (NSArray*) xinvalid_products;
			for (int i = 0; i < invalid_products.count; i++)
			{
				NSString* invalidProductId = [invalid_products objectAtIndex: i];
//			 NSLog(@"Invalid product id: %@" , invalidProductId);
			 const char* productIdentifier = [invalidProductId UTF8String];
				b->push(productIdentifier);
		 }
#endif

			call_method(function, &env, this, 2, env.get_top_index());
		}
	}
	
	void as_inapp::onpurchase(const char* productIdentifier, int rc)
	{
		bakeinflash::as_value function;
		if (get_member("onPurchase", &function))
		{
			as_environment env;
			env.push(rc);
			env.push(productIdentifier);
			call_method(function, &env, this, 2, env.get_top_index());
		}
	}
	
}

#ifdef iOS

//
//
//
@implementation SKProduct (LocalizedPrice)

- (NSString *)localizedPrice
{
	NSNumberFormatter *numberFormatter = [[NSNumberFormatter alloc] init];
	[numberFormatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
	[numberFormatter setNumberStyle:NSNumberFormatterCurrencyStyle];
	[numberFormatter setLocale:self.priceLocale];
	NSString *formattedString = [numberFormatter stringFromNumber:self.price];
	[numberFormatter release];
	return formattedString;
}

@end 

//
//
//
@implementation InAppPurchaseManager
@synthesize m_parent;
//@synthesize m_product_id;

- (id)init
{
//	if ((self = [super init]))
//	{
//	  [self loadStore];
//	}
	m_request = nil;
	
	// restarts any purchases if they were interrupted last time the app was open
	[[SKPaymentQueue defaultQueue] addTransactionObserver:self];

	return self;
}

#pragma mark -
#pragma mark SKProductsRequestDelegate methods

- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response
{
	bakeinflash::as_inapp* inapp = bakeinflash::cast_to<bakeinflash::as_inapp>((bakeinflash::as_inapp*) m_parent);
	if (inapp && m_request)
	{
		inapp->onload(response.products, response.invalidProductIdentifiers);
	}
	
	// finally release the reqest we alloc/init’ed in requestProUpgradeProductData
	[m_request release];
	m_request = nil;
	
//	[[NSNotificationCenter defaultCenter] postNotificationName:kInAppPurchaseManagerProductsFetchedNotification object:self userInfo:nil];
}

#pragma -
#pragma Public methods

//
// call this method once on startup
//
- (BOOL)loadStore :(NSMutableSet*) products
{
	if (m_request == nil)
	{
		// get the product description (defined in early sections)
    m_request = [[SKProductsRequest alloc] initWithProductIdentifiers:products];
		m_request.delegate = self;
		[m_request start];
		return true;
	}		
	return false;
	// we will release the request object in the delegate callback
}

//
// call this before making a purchase
//
- (BOOL)canMakePurchases
{
	return [SKPaymentQueue canMakePayments];
}

//
// kick off the upgrade transaction
//
- (BOOL)purchase: (NSString*) product_id
{
	NSArray* t = [[SKPaymentQueue defaultQueue] transactions];
	int n = [t count];
	if (n == 0 && [self canMakePurchases])
	{
		SKPayment* payment = [SKPayment paymentWithProductIdentifier:product_id];
//		payment.quantity = 1; // number of "items" that user wishes to purchase.
		[[SKPaymentQueue defaultQueue] addPayment:payment];
		return YES;
	}
	return NO;
}

#pragma -
#pragma Purchase helpers

//
// saves a record of the transaction by storing the receipt to disk
//
- (void)recordTransaction:(SKPaymentTransaction *)transaction
{
//	myprintf("recordTransaction: %d\n", transaction.error.code);
//	if ([transaction.payment.productIdentifier isEqualToString:m_product_id])
//	{
		// save the transaction receipt to disk
//		[[NSUserDefaults standardUserDefaults] setValue:transaction.transactionReceipt forKey:@"proUpgradeTransactionReceipt" ];
//		[[NSUserDefaults standardUserDefaults] synchronize];
//	}
}

//
// enable pro features
//
- (void)provideContent:(NSString*) productId
{
	// enable the pro features
	NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];
	NSString* result = [userDefaults objectForKey: productId];
	int n = result ? [result intValue] : 0;
	n++;
	NSString* val = [NSString stringWithFormat: @"%d", n];
	[userDefaults setObject: val forKey: productId];
	[userDefaults synchronize];
	
//	NSLog(@"provideContent: '%@': %@", productId, val);
}

//
// removes the transaction from the queue and posts a notification with the transaction result
//
- (void)finishTransaction:(SKPaymentTransaction *)transaction wasSuccessful:(BOOL)wasSuccessful
{
//	myprintf("finishTransaction: %d\n", transaction.error.code);
	// remove the transaction from the payment queue.
	[[SKPaymentQueue defaultQueue] finishTransaction:transaction];
	
//	NSDictionary *userInfo = [NSDictionary dictionaryWithObjectsAndKeys:transaction, @"transaction" , nil];
//	if (wasSuccessful)
//	{
		// send out a notification that we’ve finished the transaction
//		[[NSNotificationCenter defaultCenter] postNotificationName:kInAppPurchaseManagerTransactionSucceededNotification object:self userInfo:userInfo];
//	}
//	else
//	{
		// send out a notification for the failed transaction
//		[[NSNotificationCenter defaultCenter] postNotificationName:kInAppPurchaseManagerTransactionFailedNotification object:self userInfo:userInfo];
//	}
}

//
// called when the transaction was successful
//
- (void)completeTransaction:(SKPaymentTransaction *)transaction
{
//	myprintf("completeTransaction: %d\n", transaction.error.code);
	[self recordTransaction:transaction];
	[self provideContent:transaction.payment.productIdentifier];
	[self finishTransaction:transaction wasSuccessful:YES];
}

//
// called when a transaction has been restored and and successfully completed
//
- (void)restoreTransaction:(SKPaymentTransaction *)transaction
{
//	myprintf("restoreTransaction: %d\n", transaction.error.code);
	[self recordTransaction:transaction.originalTransaction];
	[self provideContent:transaction.originalTransaction.payment.productIdentifier];
	[self finishTransaction:transaction wasSuccessful:YES];
}

//
// called when a transaction has failed
//
- (void)failedTransaction:(SKPaymentTransaction *)transaction
{
//	myprintf("failedTransaction: %d\n", transaction.error.code);
	if (transaction.error.code != SKErrorPaymentCancelled)
	{
		// error!
		[self finishTransaction:transaction wasSuccessful:NO];
	}
	else
	{
		// this is fine, the user just cancelled, so don’t notify
		[[SKPaymentQueue defaultQueue] finishTransaction:transaction];
	}
}

#pragma mark -
#pragma mark SKPaymentTransactionObserver methods

//
// called when the transaction status is updated
//
- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions
{
	for (SKPaymentTransaction *transaction in transactions)
	{
		switch (transaction.transactionState)
		{
			case SKPaymentTransactionStatePurchased:	// 1
				[self completeTransaction:transaction];
				break;
			case SKPaymentTransactionStateFailed:	// 2
				[self failedTransaction:transaction];
				break;
			case SKPaymentTransactionStateRestored:
				[self restoreTransaction:transaction];
				break;
			default:
				break;
		}
		
//	myprintf("transactionState: %d\n", transaction.transactionState);
		bakeinflash::as_inapp* inapp = bakeinflash::cast_to<bakeinflash::as_inapp>((bakeinflash::as_inapp*) m_parent);
		if (inapp)
		{
			const char* productIdentifier = [transaction.payment.productIdentifier UTF8String];
			inapp->onpurchase(productIdentifier, transaction.transactionState);
		}
	}
} 

@end

#endif