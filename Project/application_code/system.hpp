#ifndef SYSTEM_H
#define SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ctl_api.h>
#include <stdint.h>

typedef enum {
  eMainEvents_10ms = 0x01,
  eMainEvents_CanMessaged = 0x04,
} eMainEvents;

extern CTL_EVENT_SET_t gMainEvent;

// Reason for the alive/info response from the Extension/Device.
// These only seem to exist for logging purposes. The Miniserver does
// not seem to do anything special with them.
typedef enum /*: uint8_t*/ {
  eAliveReason_t_unknown = 0x00,
  eAliveReason_t_miniserver_start = 0x01, // unused?
  eAliveReason_t_pairing = 0x02,          // sent after a NAT Offer, if there is other reason (reset reasons)
  // Reasons from this point on are logged by the Miniserver.
  // The ones before are not
  eAliveReason_t_alive_Requested = 0x03,     // unused?
  eAliveReason_t_reconnected = 0x04,         // unused?
  eAliveReason_t_alive_packet = 0x05,        // Alive, sent during normal operation
  eAliveReason_t_reconnect_broadcast = 0x06, // unused?
  // different reset reasons from the ARM CPU
  eAliveReason_t_power_on_reset = 0x20,        // PORRSTF: POR/PDR flag
  eAliveReason_t_standby_reset = 0x21,         // PDDS bit => Powerdown deep sleep
  eAliveReason_t_watchdog_reset = 0x22,        // IWDGRSTF: Independent window watchdog reset flag
  eAliveReason_t_software_reset = 0x23,        // SFTRSTF: Software reset flag
  eAliveReason_t_pin_reset = 0x24,             // PINRSTF: PIN reset flag
  eAliveReason_t_window_watchdog_reset = 0x25, // WWDGRSTF: Window watchdog reset flag
  eAliveReason_t_low_power_reset = 0x26,       // LPWRSTF: Low-power reset flag
} eAliveReason_t;

extern eAliveReason_t gResetReason;

void system_init(void);
uint32_t serialnumber_24bit(void);
#if DEBUG
void MX_print_cpu_info(void);
#endif
float MX_read_temperature(void);
void SystemClock_Config(void);

#ifdef __cplusplus
}
#endif

#endif