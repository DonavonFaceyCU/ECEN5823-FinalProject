/*
   gpio.h
  
    Created on: Dec 12, 2018
        Author: Dan Walkes

    Updated by Dave Sluiter Sept 7, 2020. moved #defines from .c to .h file.
    Updated by Dave Sluiter Dec 31, 2020. Minor edits with #defines.

    Editor: Feb 26, 2022, Dave Sluiter
    Change: Added comment about use of .h files.

 *
 * @student    Donavon Facey, Dofa5688@Colorado.edu
 *
 
 */


// Students: Remember, a header file (a .h file) generally defines an interface
//           for functions defined within an implementation file (a .c file).
//           The .h file defines what a caller (a user) of a .c file requires.
//           At a minimum, the .h file should define the publicly callable
//           functions, i.e. define the function prototypes. #define and type
//           definitions can be added if the caller requires theses.


#ifndef SRC_GPIO_H_
#define SRC_GPIO_H_

#include "em_gpio.h"

#define LED_port   (gpioPortF)
#define LED0_pin   (4)
#define LED1_pin   (5)

#define DISPLAY_EXTCOMIN_port (gpioPortD)
#define DISPLAY_EXTCOMIN_pin  (13)

#define Button_port   (gpioPortF)
#define B0_pin    (6)
#define B1_pin    (7)

// Function prototypes
void gpioInit();
void gpioLed0SetOn();
void gpioLed0SetOff();
void gpioLed1SetOn();
void gpioLed1SetOff();

void gpioSetDisplayExtcomin(bool state);



#endif /* SRC_GPIO_H_ */
