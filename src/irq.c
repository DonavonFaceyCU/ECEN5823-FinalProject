/*
 * irq.c
 *
 *  Created on: Jan 28, 2026
 *      Author: donav
 */

#include "irq.h"

void LETIMER0_IRQHandler(){
  //Disable IRQ
  NVIC_DisableIRQ(LETIMER0_IRQn);

  //Read LETIMER IFLAG
  uint32_t intflags = LETIMER_IntGet(LETIMER0);

  //Clear and handle Interrupt
  if(intflags & LETIMER_IF_COMP1){
      LETIMER_IntClear(LETIMER0, LETIMER_IF_COMP1);
      gpioLed0SetOn();
      //gpioLed1SetOff();
  }

  if(intflags & LETIMER_IF_UF){
      LETIMER_IntClear(LETIMER0, LETIMER_IF_UF);
      gpioLed0SetOff();
      //gpioLed1SetOn();
  }



  //Enable IRQ
  NVIC_EnableIRQ(LETIMER0_IRQn);
}
