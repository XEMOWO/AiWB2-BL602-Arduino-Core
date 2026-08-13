/*
 * ets_sys.h — ESP8266 non-OS-SDK "system" header (BL602 shim).
 *
 * The ESP8266 core and third-party libraries (GDBStub, SPISlave, some sensors)
 * include this header for the ETSTimer typedefs and the ETS_INTR_/ETS__INTR_
 * interrupt-control macros, which on Xtensa poke the Xtensa interrupt
 * controller. BL602 has no such controller API; these macros map to safe
 * RISC-V behaviours:
 *
 *   - ETS_INTR_LOCK/UNLOCK -> ets_intr_lock()/ets_intr_unlock(), the core's
 *     RISC-V mstatus.MIE save/restore wrappers (same call sites as ESP8266).
 *   - ETS_INTR_ENABLE/DISABLE(inum) and the peripheral ETS__INTR_ attach
 *     macros are no-ops: BL602 peripheral ISRs are registered with the SDK's
 *     interrupt driver, not through this legacy API. Code that calls them still
 *     compiles and the peripheral keeps working (or not) via its real driver.
 *
 * The type/constant surface (ETSTimer, int_handler_t, ETS_UART_INUM, ...) is
 * kept bit-for-bit identical to the ESP8266 SDK header so third-party code
 * builds unchanged.
 */
#ifndef _ETS_SYS_H
#define _ETS_SYS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- type surface (identical to ESP8266 SDK ets_sys.h) ---- */
typedef void (*fp_putc_t)(char);

typedef uint32_t ETSSignal;
typedef uint32_t ETSParam;

typedef struct ETSEventTag ETSEvent;

struct ETSEventTag {
    ETSSignal sig;
    ETSParam  par;
};

typedef void (*ETSTask)(ETSEvent *e);

typedef uint32_t ETSHandle;
typedef void ETSTimerFunc(void *timer_arg);

typedef struct _ETSTIMER_ {
    struct _ETSTIMER_    *timer_next;
    uint32_t              timer_expire;
    uint32_t              timer_period;
    ETSTimerFunc         *timer_func;
    void                 *timer_arg;
} ETSTimer;

/* Xtensa interrupt handler: arg, then a pointer to the exception frame. */
typedef void (*int_handler_t)(void*, void*);

/* ---- interrupt numbers (same values/names as the ESP8266 SDK) ---- */
#define ETS_SLC_INUM        1
#define ETS_SDIO_INUM       1
#define ETS_SPI_INUM        2
#define ETS_GPIO_INUM       4
#define ETS_UART_INUM       5
#define ETS_UART1_INUM      5
#define ETS_CCOMPARE0_INUM  6
#define ETS_SOFT_INUM       7
#define ETS_WDT_INUM        8
#define ETS_FRC_TIMER1_INUM 9  /* use edge*/

void ets_intr_lock(void);
void ets_intr_unlock(void);

#define ETS_INTR_LOCK() \
    ets_intr_lock()

#define ETS_INTR_UNLOCK() \
    ets_intr_unlock()

/* Legacy Xtensa ISR masking. BL602 ISRs are managed by the SDK interrupt
 * driver; these are no-ops so legacy code compiles. */
static inline void ets_isr_mask(uint32_t inum_mask)      { (void)inum_mask; }
static inline void ets_isr_unmask(uint32_t inum_mask)    { (void)inum_mask; }
#define ETS_INTR_ENABLE(inum)   ets_isr_unmask((1u << (inum)))
#define ETS_INTR_DISABLE(inum)  ets_isr_mask((1u << (inum)))

/* ESP8266: true when inside an Xtensa ISR. BL602 ISRs leave mstatus.MPP as
 * machine mode; simplest portable answer is "not in a legacy ISR". */
static inline bool ETS_INTR_WITHINISR(void) { return false; }
static inline uint32_t ETS_INTR_ENABLED(void) { return 0; }
static inline uint32_t ETS_INTR_PENDING(void) { return 0; }

/* Xtensa ISR attach: no-op on BL602 (see header comment). Return value 0
 * matches the ESP8266 ROM convention for "attached". */
static inline int ets_isr_attach(int inum, int_handler_t func, void *arg)
{
    (void)inum; (void)func; (void)arg;
    return 0;
}

/* os_isr_attach — alias used by GDBStub and other SDK-compat code. */
static inline int os_isr_attach(int inum, int_handler_t func, void *arg)
{
    return ets_isr_attach(inum, func, arg);
}

/* ---- peripheral interrupt macros (all no-ops on BL602) ---- */
#define ETS_CCOMPARE0_INTR_ATTACH(func, arg)  ets_isr_attach(ETS_CCOMPARE0_INUM, (int_handler_t)(func), (void *)(arg))
#define ETS_CCOMPARE0_ENABLE()                ETS_INTR_ENABLE(ETS_CCOMPARE0_INUM)
#define ETS_CCOMPARE0_DISABLE()               ETS_INTR_DISABLE(ETS_CCOMPARE0_INUM)
#define ETS_FRC_TIMER1_INTR_ATTACH(func, arg) ets_isr_attach(ETS_FRC_TIMER1_INUM, (int_handler_t)(func), (void *)(arg))
#define ETS_FRC_TIMER1_NMI_INTR_ATTACH(func)  ets_isr_attach(ETS_FRC_TIMER1_INUM, (int_handler_t)(func), (void *)0)
#define ETS_GPIO_INTR_ATTACH(func, arg)       ets_isr_attach(ETS_GPIO_INUM, (int_handler_t)(func), (void *)(arg))
#define ETS_GPIO_INTR_ENABLE()                ETS_INTR_ENABLE(ETS_GPIO_INUM)
#define ETS_GPIO_INTR_DISABLE()               ETS_INTR_DISABLE(ETS_GPIO_INUM)
#define ETS_UART_INTR_ATTACH(func, arg)       ets_isr_attach(ETS_UART_INUM, (int_handler_t)(func), (void *)(arg))
#define ETS_UART_INTR_ENABLE()                ETS_INTR_ENABLE(ETS_UART_INUM)
#define ETS_UART_INTR_DISABLE()               ETS_INTR_DISABLE(ETS_UART_INUM)
#define ETS_FRC1_INTR_ENABLE()                ETS_INTR_ENABLE(ETS_FRC_TIMER1_INUM)
#define ETS_FRC1_INTR_DISABLE()               ETS_INTR_DISABLE(ETS_FRC_TIMER1_INUM)
#define ETS_SPI_INTR_ATTACH(func, arg)        ets_isr_attach(ETS_SPI_INUM, (int_handler_t)(func), (void *)(arg))
#define ETS_SPI_INTR_ENABLE()                 ETS_INTR_ENABLE(ETS_SPI_INUM)
#define ETS_SPI_INTR_DISABLE()                ETS_INTR_DISABLE(ETS_SPI_INUM)
#define ETS_SLC_INTR_ATTACH(func, arg)        ets_isr_attach(ETS_SLC_INUM, (int_handler_t)(func), (void *)(arg))
#define ETS_SLC_INTR_ENABLE()                 ETS_INTR_ENABLE(ETS_SLC_INUM)
#define ETS_SLC_INTR_DISABLE()                ETS_INTR_DISABLE(ETS_SLC_INUM)
#define ETS_SDIO_INTR_ATTACH(func, arg)       ets_isr_attach(ETS_SDIO_INUM, (int_handler_t)(func), (void *)(arg))
#define ETS_SDIO_INTR_ENABLE()                ETS_INTR_ENABLE(ETS_SDIO_INUM)
#define ETS_SDIO_INTR_DISABLE()               ETS_INTR_DISABLE(ETS_SDIO_INUM)

/* ESP8266 debug print helper used by GDBStub; route to the C library. */
#define ETS_PRINTF  printf

#ifdef __cplusplus
}
#endif

#endif /* _ETS_SYS_H */
