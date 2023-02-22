/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-28     tyustli      first version
 * 2019-07-15     Magicoe      The first version for LPC55S6x, we can also use SCT as PWM
 *
 */

#include <rtthread.h>

#ifdef RT_USING_PWM
#if !defined(BSP_USING_CTIMER0_MAT0) && !defined(BSP_USING_CTIMER0_MAT1) && \
    !defined(BSP_USING_CTIMER0_MAT2)
#else
    #define BSP_USING_CTIMER0
#endif

#if !defined(BSP_USING_CTIMER1_MAT0) && !defined(BSP_USING_CTIMER1_MAT1) && \
    !defined(BSP_USING_CTIMER1_MAT2)
#else
    #define BSP_USING_CTIMER1
#endif

#if !defined(BSP_USING_CTIMER2_MAT0) && !defined(BSP_USING_CTIMER2_MAT1) && \
    !defined(BSP_USING_CTIMER2_MAT2)
#else
    #define BSP_USING_CTIMER2
#endif

#if !defined(BSP_USING_CTIMER3_MAT0) && !defined(BSP_USING_CTIMER3_MAT1) && \
    !defined(BSP_USING_CTIMER3_MAT2)
#else
    #define BSP_USING_CTIMER3
#endif

#if !defined(BSP_USING_CTIMER0) && !defined(BSP_USING_CTIMER1) && \
    !defined(BSP_USING_CTIMER2) && !defined(BSP_USING_CTIMER3)
#error "Please define at least one BSP_USING_CTIMERx_MATx"
#endif

#define LOG_TAG             "drv.pwm"
#include <drv_log.h>

#include <rtdevice.h>
#include "fsl_ctimer.h"
#include "drv_pwm.h"

#define DEFAULT_DUTY                  50
#define DEFAULT_FREQ                  1000

static rt_err_t lpc_drv_pwm_control(struct rt_device_pwm *device, int cmd, void *arg);

static struct rt_pwm_ops lpc_drv_ops =
{
    .control = lpc_drv_pwm_control
};

static rt_err_t lpc_drv_pwm_enable(struct rt_device_pwm *device, struct rt_pwm_configuration *configuration, rt_bool_t enable)
{
    CTIMER_Type *base;

    base = (CTIMER_Type *)device->parent.user_data;

    if (!enable)
    {
        /* Stop the timer */
        CTIMER_StopTimer(base);
    }
    else
    {
        /* Start the timer */
        CTIMER_StartTimer(base);
    }

    return RT_EOK;
}

static rt_err_t lpc_drv_pwm_get(struct rt_device_pwm *device, struct rt_pwm_configuration *configuration)
{
    uint8_t get_duty;
    uint32_t get_frequence;
    uint32_t pwmClock = 0;
    CTIMER_Type *base;

    base = (CTIMER_Type *)device->parent.user_data;

#ifdef BSP_USING_CTIMER0
    /* get frequence */
    pwmClock = CLOCK_GetFreq(kCLOCK_Timer0) ;
#endif

#ifdef BSP_USING_CTIMER1
    /* get frequence */
    pwmClock = CLOCK_GetFreq(kCLOCK_Timer1) ;
#endif

#ifdef BSP_USING_CTIMER2
    /* get frequence */
    pwmClock = CLOCK_GetFreq(kCLOCK_Timer2) ;
#endif

#ifdef BSP_USING_CTIMER3
    /* get frequence */
    pwmClock = CLOCK_GetFreq(kCLOCK_Timer3) ;
#endif

    get_frequence = pwmClock / (base->MR[kCTIMER_Match_3] + 1);

    if(configuration->channel == 1)
    {
        /* get dutycycle */
        get_duty = (100*(base->MR[kCTIMER_Match_3] + 1 - base->MR[kCTIMER_Match_1]))/(base->MR[kCTIMER_Match_3] + 1);
    }

    /* get dutycycle */
    /* conversion */
    configuration->period = 1000000000 / get_frequence;
    configuration->pulse = get_duty * configuration->period / 100;

    rt_kprintf("*** PWM period %d, pulse %d\r\n", configuration->period, configuration->pulse);

    return RT_EOK;
}

static rt_err_t lpc_drv_pwm_set(struct rt_device_pwm *device, struct rt_pwm_configuration *configuration)
{
    RT_ASSERT(configuration->period > 0);
    RT_ASSERT(configuration->pulse <= configuration->period);

    ctimer_config_t config;
    CTIMER_Type *base;
    base = (CTIMER_Type *)device->parent.user_data;

    uint32_t pwmPeriod, pulsePeriod;
    /* Run as a timer */
    config.mode = kCTIMER_TimerMode;
    /* This field is ignored when mode is timer */
    config.input = kCTIMER_Capture_0;
    /* Timer counter is incremented on every APB bus clock */
    config.prescale = 0;

    if(configuration->channel == 1)
    {
        /* Get the PWM period match value and pulse width match value of DEFAULT_FREQ PWM signal with DEFAULT_DUTY dutycycle */
        /* Calculate PWM period match value */
        pwmPeriod = (( CLOCK_GetFreq(kCLOCK_Timer2) / (config.prescale + 1) ) / DEFAULT_FREQ) - 1;

        /* Calculate pulse width match value */
        if (DEFAULT_DUTY == 0)
        {
            pulsePeriod = pwmPeriod + 1;
        }
        else
        {
            pulsePeriod = (pwmPeriod * (100 - DEFAULT_DUTY)) / 100;
        }
        /* Match on channel 3 will define the PWM period */
        base->MR[kCTIMER_Match_3] = pwmPeriod;
        /* This will define the PWM pulse period */
        base->MR[kCTIMER_Match_1] = pulsePeriod;

    }

    return RT_EOK;
}

static rt_err_t lpc_drv_pwm_control(struct rt_device_pwm *device, int cmd, void *arg)
{
    struct rt_pwm_configuration *configuration = (struct rt_pwm_configuration *)arg;

    switch (cmd)
    {
    case PWM_CMD_ENABLE:
        return lpc_drv_pwm_enable(device, configuration, RT_TRUE);
    case PWM_CMD_DISABLE:
        return lpc_drv_pwm_enable(device, configuration, RT_FALSE);
    case PWM_CMD_SET:
        return lpc_drv_pwm_set(device, configuration);
    case PWM_CMD_GET:
        return lpc_drv_pwm_get(device, configuration);
    default:
        return RT_EINVAL;
    }
}

int rt_hw_pwm_init(void)
{
    rt_err_t ret = RT_EOK;

#ifdef BSP_USING_CTIMER0

    static struct rt_device_pwm pwm0_device;
    ctimer_config_t config0;
    uint32_t pwmPeriod0, pulsePeriod0;

    /* Use 12 MHz clock for some of the Ctimers */
    CLOCK_AttachClk(kMAIN_CLK_to_CTIMER0);

    /* Run as a timer */
    config0.mode = kCTIMER_TimerMode;
    /* This field is ignored when mode is timer */
    config0.input = kCTIMER_Capture_0;
    /* Timer counter is incremented on every APB bus clock */
    config0.prescale = 0;

    CTIMER_Init(CTIMER0, &config0);

#ifdef BSP_USING_CTIMER0_MAT3
    /* Get the PWM period match value and pulse width match value of DEFAULT_FREQ PWM signal with DEFAULT_DUTY dutycycle */
    /* Calculate PWM period match value */
    pwmPeriod0 = (( CLOCK_GetFreq(kCLOCK_Timer0) / (config0.prescale + 1) ) / DEFAULT_FREQ) - 1;

    /* Calculate pulse width match value */
    if (DEFAULT_DUTY == 0)
    {
        pulsePeriod0 = pwmPeriod0 + 1;
    }
    else
    {
        pulsePeriod0 = (pwmPeriod0 * (100 - DEFAULT_DUTY)) / 100;
    }
    CTIMER_SetupPwmPeriod(CTIMER0, kCTIMER_Match_3 , kCTIMER_Match_3, pwmPeriod0, pulsePeriod0, false);
#endif

    ret = rt_device_pwm_register(&pwm0_device, "pwm0", &lpc_drv_ops, CTIMER0);

    if (ret != RT_EOK)
    {
        LOG_E("%s register failed", "pwm0");
    }

#endif /* BSP_USING_CTIMER0 */

#ifdef BSP_USING_CTIMER1

    static struct rt_device_pwm pwm1_device;
    ctimer_config_t config1;
    uint32_t pwmPeriod1, pulsePeriod1;

    /* Use 12 MHz clock for some of the Ctimers */
    CLOCK_AttachClk(kMAIN_CLK_to_CTIMER1);

    /* Run as a timer */
    config1.mode = kCTIMER_TimerMode;
    /* This field is ignored when mode is timer */
    config1.input = kCTIMER_Capture_0;
    /* Timer counter is incremented on every APB bus clock */
    config1.prescale = 0;

    CTIMER_Init(CTIMER1, &config1);

#ifdef BSP_USING_CTIMER1_MAT3
    /* Get the PWM period match value and pulse width match value of DEFAULT_FREQ PWM signal with DEFAULT_DUTY dutycycle */
    /* Calculate PWM period match value */
    pwmPeriod1 = (( CLOCK_GetFreq(kCLOCK_Timer1) / (config1.prescale + 1) ) / DEFAULT_FREQ) - 1;

    /* Calculate pulse width match value */
    if (DEFAULT_DUTY == 0)
    {
        pulsePeriod1 = pwmPeriod1 + 1;
    }
    else
    {
        pulsePeriod1 = (pwmPeriod1 * (100 - DEFAULT_DUTY)) / 100;
    }
    CTIMER_SetupPwmPeriod(CTIMER1, kCTIMER_Match_3 , kCTIMER_Match_3, pwmPeriod1, pulsePeriod1, false);
#endif

    ret = rt_device_pwm_register(&pwm1_device, "pwm1", &lpc_drv_ops, CTIMER1);

    if (ret != RT_EOK)
    {
        LOG_E("%s register failed", "pwm1");
    }

#endif /* BSP_USING_CTIMER1 */

#ifdef BSP_USING_CTIMER2

    static struct rt_device_pwm pwm2_device;
    ctimer_config_t config2;
    uint32_t pwmPeriod2, pulsePeriod2;

    /* Use 12 MHz clock for some of the Ctimers */
    CLOCK_AttachClk(kMAIN_CLK_to_CTIMER2);

    /* Run as a timer */
    config2.mode = kCTIMER_TimerMode;
    /* This field is ignored when mode is timer */
    config2.input = kCTIMER_Capture_0;
    /* Timer counter is incremented on every APB bus clock */
    config2.prescale = 0;

    CTIMER_Init(CTIMER2, &config2);

#ifdef BSP_USING_CTIMER2_MAT1
    /* Get the PWM period match value and pulse width match value of DEFAULT_FREQ PWM signal with DEFAULT_DUTY dutycycle */
    /* Calculate PWM period match value */
    pwmPeriod2 = (( CLOCK_GetFreq(kCLOCK_Timer2) / (config2.prescale + 1) ) / DEFAULT_FREQ) - 1;

    /* Calculate pulse width match value */
    if (DEFAULT_DUTY == 0)
    {
        pulsePeriod2 = pwmPeriod2 + 1;
    }
    else
    {
        pulsePeriod2 = (pwmPeriod2 * (100 - DEFAULT_DUTY)) / 100;
    }
    CTIMER_SetupPwmPeriod(CTIMER2, kCTIMER_Match_3 , kCTIMER_Match_1, pwmPeriod2, pulsePeriod2, false);
#endif

#ifdef BSP_USING_CTIMER2_MAT2
    /* Get the PWM period match value and pulse width match value of DEFAULT_FREQ PWM signal with DEFAULT_DUTY dutycycle */
    /* Calculate PWM period match value */
    pwmPeriod2 = (( CLOCK_GetFreq(kCLOCK_Timer2) / (config2.prescale + 1) ) / DEFAULT_FREQ) - 1;

    /* Calculate pulse width match value */
    if (DEFAULT_DUTY == 0)
    {
        pulsePeriod2 = pwmPeriod2 + 1;
    }
    else
    {
        pulsePeriod2 = (pwmPeriod2 * (100 - DEFAULT_DUTY)) / 100;
    }
    CTIMER_SetupPwmPeriod(CTIMER2, kCTIMER_Match_3 , kCTIMER_Match_2, pwmPeriod2, pulsePeriod2, false);
#endif

    ret = rt_device_pwm_register(&pwm2_device, "pwm2", &lpc_drv_ops, CTIMER2);

    if (ret != RT_EOK)
    {
        LOG_E("%s register failed", "pwm2");
    }

#endif /* BSP_USING_CTIMER2 */

#ifdef BSP_USING_CTIMER3

    static struct rt_device_pwm pwm3_device;
    ctimer_config_t config3;
    uint32_t pwmPeriod3, pulsePeriod3;

    /* Use 12 MHz clock for some of the Ctimers */
    CLOCK_AttachClk(kMAIN_CLK_to_CTIMER3);

    /* Run as a timer */
    config3.mode = kCTIMER_TimerMode;
    /* This field is ignored when mode is timer */
    config3.input = kCTIMER_Capture_0;
    /* Timer counter is incremented on every APB bus clock */
    config3.prescale = 0;

    CTIMER_Init(CTIMER3, &config3);

#ifdef BSP_USING_CTIMER3_MAT1
    /* Get the PWM period match value and pulse width match value of DEFAULT_FREQ PWM signal with DEFAULT_DUTY dutycycle */
    /* Calculate PWM period match value */
    pwmPeriod3 = (( CLOCK_GetFreq(kCLOCK_Timer3) / (config3.prescale + 1) ) / DEFAULT_FREQ) - 1;

    /* Calculate pulse width match value */
    if (DEFAULT_DUTY == 0)
    {
        pulsePeriod3 = pwmPeriod3 + 1;
    }
    else
    {
        pulsePeriod3 = (pwmPeriod3 * (100 - DEFAULT_DUTY)) / 100;
    }
    CTIMER_SetupPwmPeriod(CTIMER3, kCTIMER_Match_3 , kCTIMER_Match_1, pwmPeriod3, pulsePeriod3, false);
#endif

#ifdef BSP_USING_CTIMER3_MAT2
    /* Get the PWM period match value and pulse width match value of DEFAULT_FREQ PWM signal with DEFAULT_DUTY dutycycle */
    /* Calculate PWM period match value */
    pwmPeriod3 = (( CLOCK_GetFreq(kCLOCK_Timer3) / (config3.prescale + 1) ) / DEFAULT_FREQ) - 1;

    /* Calculate pulse width match value */
    if (DEFAULT_DUTY == 0)
    {
        pulsePeriod3 = pwmPeriod3 + 1;
    }
    else
    {
        pulsePeriod3 = (pwmPeriod3 * (100 - DEFAULT_DUTY)) / 100;
    }
    CTIMER_SetupPwmPeriod(CTIMER3, kCTIMER_Match_3 , kCTIMER_Match_2, pwmPeriod3, pulsePeriod3, false);
#endif

    ret = rt_device_pwm_register(&pwm3_device, "pwm3", &lpc_drv_ops, CTIMER3);

    if (ret != RT_EOK)
    {
        LOG_E("%s register failed", "pwm3");
    }

#endif /* BSP_USING_CTIMER3 */

    return ret;
}

INIT_DEVICE_EXPORT(rt_hw_pwm_init);

#endif /* RT_USING_PWM */
