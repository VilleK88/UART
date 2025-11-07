# Exercise 1 – Read version info and DevEui from LoRa module - UART    
  
Implement a program that verifies that UART connection to LoRa module works and then reads and  
processes EUID from the module. The program should work as follows:  
  1. Program waits for user to press SW_0. When user presses the button then program starts
communication with the LoRa module.  
  2. Program sends command “AT” to module and waits for response for 500 ms. If no response is  
received or the response is not correct the program tries again up to five times. If no response is
received after five attempts program prints “module not responding” and goes back to step 1. If 
response is received program prints “Connected to LoRa module”.  
  3. Program reads firmware version of the module and prints the result. If no response is received in
500 ms program prints “Module stopped responding” and goes back to step 1.  
  4. Program reads DevEui from the device. If no response is received in 500 ms program prints
“Module stopped responding” and goes back to step 1. DevEui contains 8 bytes that the module
outputs in hexadecimal separated by colons. The program must remove the colons between the
bytes and convert the hexadecimal digits to lower case.  
5. Go to step 1  
