/*
 * irq.c
 *
 *  Created on: Jan 28, 2026
 *      Author: donav
 */

#include "irq.h"

// Include logging specifically for this .c file
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

static uint32_t timer_counts;
static uint32_t freq;

static void Time_incrementUnderflowCounter();
static volatile uint32_t underflowCounter;

static void Time_incrementUnderflowCounter(){
  underflowCounter++;
}

static uint8_t timerWaitUs_irq(uint32_t delay_usec);

void Time_Init(uint32_t timerCounts, uint32_t frequency){
  timer_counts = timerCounts;
  freq = frequency;

}

uint64_t Time_getTime(){
  uint64_t time;
  CORE_ATOMIC_SECTION
  (
      uint32_t current_counts = LETIMER_CounterGet(LETIMER0);
      time = ((underflowCounter + 1) * timer_counts) - current_counts;
  )

  return time;
}

uint64_t letimerMilliseconds(){
  return underflowCounter * LETIMER_PERIOD_MS;
}

//The fundamental time unit is LETIMER0 timer ticks. At max frequency (~32kHz), the total counter still wouldn't overflow for ~17 million years. The 64 bit accesses are made atomic with a critical section.
//The underflow increment operation is handled in the ISR handler to prevent race conditions involving the timer continuing to run without the underflow counted. This does allow for a practically infinite delay time.
uint8_t timerWaitUs(uint32_t delay_usec){
  return timerWaitUs_irq(delay_usec);
}

uint8_t timerWaitUs_polled(uint32_t delay_usec){
  uint64_t currentTime = Time_getTime();

  //Range Checking
  //Any value of delay_usec is valid since the function does not have any valid range. As such, returning -1 (error) never happens.
  if(delay_usec == 0){
      return 1;
  }

  uint32_t delayTime = (((uint64_t)delay_usec * freq) / 1000000) + 1;
  uint64_t targetTime = currentTime + delayTime;

  uint64_t countTime = Time_getTime();
  while(countTime < targetTime){
      countTime = Time_getTime();
  }

  return 1;
}

uint32_t targetUnderflow;
uint32_t targetCounts;
static uint8_t timerWaitUs_irq(uint32_t delay_usec){
  uint64_t currentTime = Time_getTime();

  //Range Checking
  //Any value of delay_usec is valid since the function does not have any valid range. As such, returning -1 (error) never happens.
  if(delay_usec == 0){
      return 1;
  }

  uint32_t delayTime = (((uint64_t)delay_usec * freq) / 1000000) + 1;
  uint64_t targetTime = currentTime + delayTime;
  targetUnderflow = targetTime / timer_counts;
  targetCounts = (timer_counts - 1) - (targetTime % timer_counts);

  CORE_ATOMIC_SECTION
  (
    LETIMER_CompareSet(LETIMER0, 1, targetCounts);
    LETIMER_IntClear(LETIMER0, LETIMER_IF_COMP1);
    LETIMER_IntEnable(LETIMER0, LETIMER_IF_COMP1);
  )

  return 1;
}

///////////////////////////// IRQ Handlers are below ///////////////////////////////////

void LETIMER0_IRQHandler(void){
  //Disable IRQ
  NVIC_DisableIRQ(LETIMER0_IRQn);

  //Read LETIMER IFLAG
  uint32_t intflags = LETIMER_IntGet(LETIMER0);

  //Clear and handle Interrupt
  if(intflags & LETIMER_IF_COMP1){
      if(underflowCounter >= targetUnderflow){
          Scheduler_Set_COMP1();
          LETIMER_IntDisable(LETIMER0, LETIMER_IF_COMP1);
      }
      LETIMER_IntClear(LETIMER0, LETIMER_IF_COMP1);
  }

  if(intflags & LETIMER_IF_UF){
      LETIMER_IntClear(LETIMER0, LETIMER_IF_UF);
      Time_incrementUnderflowCounter();
      Scheduler_Set_UF();
  }



  //Enable IRQ
  NVIC_EnableIRQ(LETIMER0_IRQn);
}

void I2C0_IRQHandler(void) {
 I2C_TransferReturn_TypeDef transferStatus;
 transferStatus = I2C_Transfer(I2C0);
 if (transferStatus == i2cTransferDone) {
   Scheduler_Set_TXC();
 }
 if (transferStatus < 0) {
 LOG_ERROR("%d", transferStatus);
 }
} // I2C0_IRQHandler()

void GPIO_EVEN_IRQHandler(void)
{
  NVIC_DisableIRQ(GPIO_EVEN_IRQn);
  uint32_t flags = GPIO_IntGet();
  GPIO_IntClear(flags);

  if (flags & (1 << B0_pin)){
    if (GPIO_PinInGet(B0_port, B0_pin)){ //Rising Edge - DISABLED
      Scheduler_Set_PB0_released();
    } else { //Falling Edge - ENABLED
      Scheduler_Set_PB0_pressed();
    }
  }
  NVIC_EnableIRQ(GPIO_EVEN_IRQn);
}
