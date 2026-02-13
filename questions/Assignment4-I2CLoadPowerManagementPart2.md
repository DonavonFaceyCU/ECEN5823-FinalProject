Please include your answers to the questions below with your submission, entering into the space below each question
See [Mastering Markdown](https://guides.github.com/features/mastering-markdown/) for github markdown formatting if desired.

*Be sure to take measurements with logging disabled to ensure your logging logic is not impacting current/time measurements.*

*Please include screenshots of the profiler window detailing each current measurement captured.  See the file Instructions to add screenshots in assignment.docx in the ECEN 5823 Student Public Folder.* 

1. What is the average current per period? (Remember, once you have measured your average current, average current is average current over all time. Average current doesn’t carry with it the units of the timespan over which it was measured).
   Answer: 32.28uA
   <br>Screenshot:  
   ![Avg_current_per_period](screenshots/assignment4/avg_current_per_period_irq.png)  

2. What is the ave current from the time we sleep the MCU to EM3 until we power-on the 7021 in response to the LETIMER0 UF IRQ?
   Answer: 1.83uA
   <br>Screenshot:  
   ![Avg_current_LPM_Off](screenshots/assignment4/avg_current_lpm_off_irq.png)  

3. What is the ave current from the time we power-on the 7021 until we get the COMP1 IRQ indicating that the 7021's maximum time for conversion (measurement) has expired.
   Answer: 922.85uA
   <br>Screenshot:  
   ![Avg_current_LPM_On](screenshots/assignment4/avg_current_lpm_on_irq.png)  

4. How long is the Si7021 Powered On for 1 temperature reading?
   Answer: 99.94ms
   <br>Screenshot:  
   ![duration_lpm_on](screenshots/assignment4/avg_current_lpm_on_irq.png)  

5. Given the average current per period from Q1, calculate the operating time of the system for a 1000mAh battery? - ignoring battery manufacturers, efficiencies and battery derating - just a first-order calculation.
   Answer (in hours): 1000mAh / 32.28uA = 30,979 hours = 3.54 years
   
6. How has the power consumption performance of your design changed since the previous assignment?
   Answer: The power consumption when sleeping is effectively the same. However, by sleeping while powering on the SI7021, the on-time current was reduced by ~82%. This translated to a ~80% reduction in average current, as the idle current did not change at all.
   


