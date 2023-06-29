// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: MIT
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "transport/transport.h"

int main( int argc, char * argv[] )
{
    if( argc != 6 )
    {
        printf( "Usage: %s certificateFilePath privateKeyFilePath "
                "rootCAFilePath endpoint thingName\n",
                argv[ 0 ] );
        return 1;
    }

    char * certificateFilePath = argv[ 1 ];
    char * privateKeyFilePath = argv[ 2 ];
    char * rootCAFilePath = argv[ 3 ];
    char * endpoint = argv[ 4 ];
    char * thingName = argv[ 5 ];
    ( void ) thingName;

    bool result = transport_tlsConnect( certificateFilePath,
                                        privateKeyFilePath,
                                        rootCAFilePath,
                                        endpoint );
    assert( result );

    return 0;
}
