#ifndef SYSTEM_H
#define SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

void SystemClock_Config(void);
void MX_print_cpu_info(void);
float MX_read_temperature(void);

#ifdef __cplusplus
}
#endif

#endif