//
//  appletrace.h
//  appletrace
//
//  Created by everettjf on 2017/9/12.
//  Copyright © 2017年 everettjf. All rights reserved.
//

#import <Foundation/Foundation.h>

FOUNDATION_EXPORT void _kk_APTBeginSection(const char *name);
FOUNDATION_EXPORT void _kk_APTEndSection(const char *name);
FOUNDATION_EXPORT void _kk_APTSyncWait(void);

// Objective C class method
#define APTBegin _kk_APTBeginSection([NSString stringWithFormat:@"[%@]%@", self, NSStringFromSelector(_cmd)].UTF8String)
#define APTEnd _kk_APTEndSection([NSString stringWithFormat:@"[%@]%@", self, NSStringFromSelector(_cmd)].UTF8String)

