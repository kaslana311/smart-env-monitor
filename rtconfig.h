#ifndef RT_CONFIG_H__
#define RT_CONFIG_H__

/* Automatically generated file; DO NOT EDIT. */
/* RT-Thread Configuration */

/* RT-Thread Kernel */

#define RT_NAME_MAX 8
#define RT_ALIGN_SIZE 4
#define RT_THREAD_PRIORITY_32
#define RT_THREAD_PRIORITY_MAX 32
#define RT_TICK_PER_SECOND 1000
#define RT_USING_OVERFLOW_CHECK
#define RT_USING_HOOK
#define RT_USING_IDLE_HOOK
#define RT_IDLE_HOOK_LIST_SIZE 4
#define IDLE_THREAD_STACK_SIZE 1024
#define RT_DEBUG
#define RT_DEBUG_COLOR

/* Inter-Thread communication */

#define RT_USING_SEMAPHORE
#define RT_USING_MUTEX
#define RT_USING_EVENT
#define RT_USING_MAILBOX
#define RT_USING_MESSAGEQUEUE

/* Memory Management */

#define RT_USING_MEMPOOL
#define RT_USING_SMALL_MEM
#define RT_USING_HEAP

/* Kernel Device Object */

#define RT_USING_DEVICE
#define RT_VER_NUM 0x40002
#define ARCH_ARM
#define RT_USING_CPU_FFS
#define ARCH_ARM_CORTEX_M
#define ARCH_ARM_CORTEX_M4

/* RT-Thread Components */

#define RT_USING_COMPONENTS_INIT
#define RT_USING_USER_MAIN
#define RT_MAIN_THREAD_STACK_SIZE 2048
#define RT_MAIN_THREAD_PRIORITY 10

/* C++ features */


/* Command shell */


/* Device virtual file system */


/* Device Drivers */

#define RT_USING_DEVICE_IPC
#define RT_PIPE_BUFSZ 512
#define RT_USING_SERIAL
#define RT_SERIAL_USING_DMA
#define RT_SERIAL_RB_BUFSZ 64
#define RT_USING_CAN
#define RT_USING_I2C
#define RT_USING_I2C_BITOPS
#define RT_USING_PIN
#define RT_USING_PWM
#define RT_USING_SPI
#define RT_USING_WDT
#define RT_USING_HWCRYPTO
#define RT_HWCRYPTO_DEFAULT_NAME "hwcryto"
#define RT_HWCRYPTO_IV_MAX_SIZE 16
#define RT_HWCRYPTO_KEYBIT_MAX_SIZE 256
#define RT_HWCRYPTO_USING_RNG

/* Using USB */


/* POSIX layer and C standard library */

#define RT_LIBC_USING_TIME

/* Network */

/* Socket abstraction layer */


/* Network interface device */


/* light weight TCP/IP stack */


/* AT commands */


/* VBUS(Virtual Software BUS) */


/* Utilities */


/* RT-Thread online packages */

/* IoT - internet of things */


/* Wi-Fi */

/* Marvell WiFi */


/* Wiced WiFi */


/* IoT Cloud */


/* security packages */


/* language packages */


/* multimedia packages */


/* tools packages */

#define PKG_USING_SYSTEMVIEW
#define PKG_SYSVIEW_APP_NAME "RT-Thread Trace"
#define PKG_SYSVIEW_DEVICE_NAME "Cortex-M4"
#define PKG_SYSVIEW_TIMESTAMP_FREQ 0
#define PKG_SYSVIEW_CPU_FREQ 0
#define PKG_SYSVIEW_RAM_BASE 0x20000000
#define PKG_SYSVIEW_EVENTID_OFFSET 32
#define PKG_SYSVIEW_USE_CYCCNT_TIMESTAMP
#define PKG_SYSVIEW_SYSDESC0 "I#15=SysTick"
#define PKG_SYSVIEW_SYSDESC1 ""
#define PKG_SYSVIEW_SYSDESC2 ""

/* Segger RTT configuration */

#define PKG_SEGGER_RTT_MAX_NUM_UP_BUFFERS 3
#define PKG_SEGGER_RTT_MAX_NUM_DOWN_BUFFERS 3
#define PKG_SEGGER_RTT_BUFFER_SIZE_UP 1024
#define PKG_SEGGER_RTT_BUFFER_SIZE_DOWN 16
#define PKG_SEGGER_RTT_PRINTF_BUFFER_SIZE 64
#define PKG_SEGGER_RTT_AS_SERIAL_DEVICE
#define PKG_SERIAL_DEVICE_NAME "segger"
#define PKG_SEGGER_RTT_MODE_ENABLE_NO_BLOCK_SKIP
#define PKG_SEGGER_RTT_MAX_INTERRUPT_PRIORITY 0x20

/* SystemView buffer configuration */

#define PKG_SEGGER_SYSVIEW_RTT_BUFFER_SIZE 1024
#define PKG_SEGGER_SYSVIEW_RTT_CHANNEL 1
#define PKG_SEGGER_SYSVIEW_USE_STATIC_BUFFER

/* SystemView Id configuration */

#define PKG_SEGGER_SYSVIEW_ID_BASE 0x10000000
#define PKG_SEGGER_SYSVIEW_ID_SHIFT 2
#define PKG_USING_SYSTEMVIEW_LATEST_VERSION

/* system packages */


/* peripheral libraries and drivers */


/* miscellaneous packages */


/* samples: kernel and components samples */

#define SOC_FAMILY_STM32
#define SOC_SERIES_STM32F4

/* Hardware Drivers Config */

#define SOC_STM32F407IG

/* Onboard Peripheral Drivers */

#define BSP_USING_CAN168M
#define CORE_USING_MONITOR
#define BSP_USING_WDT

/* On-chip Peripheral Drivers */

#define BSP_USING_ON_CHIP_FLASH
#define BSP_USING_GPIO
#define BSP_USING_UART
#define BSP_USING_UART1
#define BSP_UART1_RX_USING_DMA
#define BSP_USING_UART3
#define BSP_UART3_RX_USING_DMA
#define BSP_USING_UART6
#define BSP_UART6_RX_USING_DMA
#define BSP_UART6_TX_USING_DMA
#define BSP_USING_PWM
#define BSP_USING_PWM1
#define BSP_USING_PWM1_CH1
#define BSP_USING_PWM4
#define BSP_USING_PWM4_CH3
#define BSP_USING_PWM5
#define BSP_USING_PWM5_CH1
#define BSP_USING_PWM5_CH2
#define BSP_USING_PWM5_CH3
#define BSP_USING_PWM10
#define BSP_USING_PWM10_CH1
#define BSP_USING_CAN
#define BSP_USING_CAN1
#define BSP_USING_CAN2
#define BSP_USING_SPI
#define BSP_USING_SPI1
#define BSP_SPI1_TX_USING_DMA
#define BSP_SPI1_RX_USING_DMA
#define BSP_USING_SPI2
#define BSP_SPI2_TX_USING_DMA
#define BSP_SPI2_RX_USING_DMA
#define BSP_USING_I2C
#define BSP_USING_I2C2
#define BSP_I2C2_SCL_PIN 81
#define BSP_I2C2_SDA_PIN 80
#define BSP_USING_DSP

/* Board extended module Drivers */

/* Choose Car model Config */

#define CORE_USING_INFANTRY
#define CORE_USING_TAN1_INFANTRY

#endif
