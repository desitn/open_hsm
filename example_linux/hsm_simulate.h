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
    SIMULATE_SIG_RECV , // 4
    SIMULATE_SIG_SEND ,
    SIMULATE_SIG_MAX  ,
};

int em_simulate_create(void);

int em_simulate_event_cfg(void);

#ifdef __cplusplus
}
#endif

#endif // __HSM_SIMULATE_H
