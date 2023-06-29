#ifndef TRANSPORT_WRAPPER_H
#define TRANSPORT_WRAPPER_H

#include "transport_interface.h"

void transport_tlsInit( TransportInterface_t * transport );

bool transport_tlsConnect( const char * endpoint, size_t endpointLength );

void transport_tlsDisconnect( void );

#endif
