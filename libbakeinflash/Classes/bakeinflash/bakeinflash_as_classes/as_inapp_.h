//
//  as_imap_.h
//  xxx
//
//  Created by Vitaly on 4/26/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <StoreKit/StoreKit.h>

//#define kInAppPurchaseManagerProductsFetchedNotification @"kInAppPurchaseManagerProductsFetchedNotification"
//#define kInAppPurchaseManagerTransactionFailedNotification @"kInAppPurchaseManagerTransactionFailedNotification"
//#define kInAppPurchaseManagerTransactionSucceededNotification @"kInAppPurchaseManagerTransactionSucceededNotification"


@interface InAppPurchaseManager : NSObject <SKProductsRequestDelegate, SKPaymentTransactionObserver>
{
	void* m_parent;
	SKProductsRequest* m_request;
} 

@property void* m_parent;

// public methods
- (void) restoreTransactions;
- (BOOL) loadStore :(NSMutableSet*) products;
-(BOOL) canMakePurchases;
-(BOOL) purchase :(NSString*) productId;

@end 

//
@interface SKProduct (LocalizedPrice)

@property (nonatomic, readonly) NSString *localizedPrice;

@end 
