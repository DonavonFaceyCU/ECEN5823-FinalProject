Note: For all assignments and Energy Profiler measurements you’ll be taking this semester,  Peak measurements are instantaneous measurements taken at a specific point in time. In the Energy Profiler, this is accomplished by left-clicking at a location along the time axis.
Average measurements are measurements that are taken over a time-span. In the Energy Profiler, this is accomplished by left-clicking and dragging a region along the time axis.

Please include your answers to the questions below with your submission, entering into the space below each question
See [Mastering Markdown](https://guides.github.com/features/mastering-markdown/) for github markdown formatting if desired.

**1. How much current does the system draw (instantaneous measurement) when a single LED is on with the GPIO pin set to StrongAlternateStrong?**
   Answer: <I>5.53mA total system current</I>


**2. How much current does the system draw (instantaneous measurement) when a single LED is on with the GPIO pin set to WeakAlternateWeak?**
   Answer: <I>5.47mA total system current</I>


**3. Is there a meaningful difference in current between the answers for question 1 and 2? Please explain your answer, referencing the main board schematic, WSTK-Main-BRD4001A-A01-schematic.pdf or WSTK-Main-BRD4002A-A06-schematic.pdf, and AEM Accuracy in the ug279-brd4104a-user-guide.pdf. Both of these PDF files are available in the ECEN 5823 Student Public Folder in Google drive at: https://drive.google.com/drive/folders/1ACI8sUKakgpOLzwsGZkns3CQtc7r35bB?usp=sharing . Extra credit is available for this question and depends on your answer.**
   Answer: 
   
  <I>The difference is only increased by ~1%. This is not considered meaningful. The following data describes average current for StrongAltStrong and WeakAltWeak Drive Modes for each combination of LEDs.

  | LEDs   | Strong | Weak  |
  |--------|--------|-------|
  | 0 LEDs | 5.01 mA | 5.02 mA |
  | 1 LED  | 5.53 mA | 5.54 mA |
  | 2 LEDs | 6.05 mA | 6.05 mA |

  Based on the User Guide, for currents above 250 µA, the AEM is accurate to 100 µA. However, given this averaging over ~1 sec, this averages ~10k samples, meaning the greater level of precision is likely justified.
  Averaging many more data points show that the drive characteristics are nearly identical. Differences in the instantaneous measurements can be partially chalked up to the noticeable current ripple at approximately 50Hz.
  Given the data, it can be concluded the each LED draws approximately 520uA. The schematic shows each LED in series with a 2.7k resistor, giving a forward LED voltage of 1.4V.
  Given the series resistance and calculated forward voltage, the pin will not drive more than 1mA, so the drive characteristics between each setting should be negligible below 1mA.</I>

**4. With the WeakAlternateWeak drive strength setting, what is the average current for 1 complete on-off cycle for 1 LED with an on-off duty cycle of 50% (approximately 1 sec on, 1 sec off)?**
   Answer: <I>5.24mA total system current</I>


**5. With the WeakAlternateWeak drive strength setting, what is the average current for 1 complete on-off cycle for 2 LEDs (both on at the time same and both off at the same time) with an on-off duty cycle of 50% (approximately 1 sec on, 1 sec off)?**
   Answer: <I>5.49mA total system current</I>

<B><I>Note: This was taken on the newer BRD4002A Rev A06 Starter Kit Board. This may account for any small discrepancies with the older kits signed out from the lab.</B></I>
