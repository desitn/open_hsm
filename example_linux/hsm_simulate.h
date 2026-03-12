#ifndef __HSM_SIMULATE_H
#define __HSM_SIMULATE_H

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>

#include "../core/osa_hsm.h"

struct em_simulate;

enum em_simulate_signal_e
{
    SIMULATE_SIG_NONE = HSM_USER_SIG,
    SIMULATE_SIG_SEND ,
    SIMULATE_SIG_RECV , 
    SIMULATE_SIG_IDLE ,
    SIMULATE_SIG_MAX  ,
};


#ifdef __cplusplus
}
#endif

#endif // __HSM_SIMULATE_H
