//
//  initialization.h
//  RPC
//
//  Created by Haochen Ding & Shisong Tang on 2013-06-26.
//  Copyright (c) 2013 Haochen Ding & Shisong Tang. All rights reserved.
//

#ifndef __RPC__initialization__
#define __RPC__initialization__

struct Info {
    char *address;
    char *port;
    int sockfd;
};

int Init(struct Info **info);
void *get_in_addr(struct sockaddr *sa);

#endif /* defined(__RPC__initialization__) */
