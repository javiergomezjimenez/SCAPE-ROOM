ECE382_Lab05
============

MSP430 IR remote reciever

###Pre-Lab
1) How long will it take the timer to roll over?

TACCR0 = 0xFFFF

TACTL = ID_3

0xFFFF µs/rollover


2) How long does each timer count last?

1 us



<table class="table table-striped table-bordered">
<thead>
<tr>
<th align="center">Pulse</th>
<th align="center">Duration (ms)</th>
<th align="center">Timer A counts</th>
</tr>
</thead>
<tbody>
<tr>
<td align="center" colspan="1">Start logic 0 half-pulse</td>
<td align="center" colspan="1">8.9556 ms</td>
<td align="center" colspan="1">8888</td>
</tr>
<tr>
<td align="center" colspan="1">Start logic 1 half-pulse</td>
<td align="center" colspan="1">4.4498 ms</td>
<td align="center" colspan="1">4412</td>
</tr>
<tr>
<td align="center" colspan="1">Data 1 logic 0 half-pulse</td>
<td align="center" colspan="1">592.8 us</td>
<td align="center" colspan="1">591</td>
</tr>
<tr>
<td align="center" colspan="1">Data 1 logic 1 half-pulse</td>
<td align="center" colspan="1">1.6312 ms</td>
<td align="center" colspan="1">1616</td>
</tr>
<tr>
<td align="center" colspan="1">Data 0 logic 0 half-pulse</td>
<td align="center" colspan="1">625.2 us</td>
<td align="center" colspan="1">620</td>
</tr>
<tr>
<td align="center" colspan="1">Data 0 logic 1 half-pulse</td>
<td align="center" colspan="1">474.6 us</td>
<td align="center" colspan="1">468</td>
</tr>
<tr>
<td align="center" colspan="1">Stop logic 0 half-pulse</td>
<td align="center" colspan="1">600 us</td>
<td align="center" colspan="1">593</td>
</tr>
<tr>
<td align="center" colspan="1">Stop logic 1 half-pulse</td>
<td align="center" colspan="1">40 ms</td>
<td align="center" colspan="1">39263</td>
</tr>
</tbody>
</table>

<table class="table table-striped table-bordered">
<thead>
<tr>
<th align="center">Button</th>
<th align="center">code (not including start and stop bits)</th>
</tr>
</thead>
<tbody>
<tr>
<td align="center" colspan="1">0</td>
<td align="center" colspan="1">00000000111111111101100000100111 0xffd827</td>
</tr>
<tr>
<td align="center" colspan="1">1</td>
<td align="center" colspan="1">00000000111111110000100011110111 0xff08f7</td>
</tr>
<tr>
<td align="center" colspan="1">2</td>
<td align="center" colspan="1">00000000111111111100000000111111 0xffc03f</td>
</tr>
<tr>
<td align="center" colspan="1">3</td>
<td align="center" colspan="1">00000000111111111000000001111111 0xff8074</td>
</tr>
<tr>
<td align="center" colspan="1">Enter</td>
<td align="center" colspan="1">00000000111111111010000001011111 0xffa05f</td>
</tr>
</tbody>
</table>

```C
while (1)  {

	while(IR_DECODER_PIN != 0);			// IR input is nominally logic 1

	for(i=0; i<SAMPLE_SIZE; i++) {

32		TAR = 0;						// reset timer and
33		while(IR_DECODER_PIN==0);		// wait while IR is logic 0
34		time0[i] = TAR;					// and store timer A
35
36		TAR = 0;						// reset timer and
37		while(IR_DECODER_PIN != 0);		// wait while IR is logic 1
38		time1[i] = TAR;					// and store timer A

	} 
} 
```
![alt tag](https://raw.githubusercontent.com/EricWardner/ECE382_Lab05/master/day1waveform.png)

###Setup/Goals
The main goal of the lab was to write a program that would allow the MSP430 to recieve and interpret data from an IR sensor and from that data turn an LED on and off. The IR data would come from a common remote control. 

The initial set up was fairly straight-forward. It involved attaching the pins from an IR sensor to the MSP430. 

A simple diagram for the IR sensor is shown here

![alt tag](http://i.imgur.com/TR597IB.png)

Here is the actual connection to the MSP430

![alt tag](http://i.imgur.com/4ht6UAH.jpg)



###Lab Functionality
The transition to functionality started with the start5.c file which contained an outline of the code pictured below

![alt tag](http://ece382.com/labs/lab5/schematic.jpg)

The init funciton was already complete so I began with the interrupts. 

#####Part2 Vector
First to check if it was rising or falling edge the switch on pin was created. Case 0 is falling edge and Case  1 is rising edge.

```C
case 0:						// !!!!!!!!!NEGATIVE EDGE!!!!!!!!!!
	pulseDuration = TAR;
	if((pulseDuration < maxLogic0Pulse) && (pulseDuration > minLogic0Pulse)){
		irPacket = (irPacket << 1) | 0;
	}
	if((pulseDuration < maxLogic1Pulse) && (pulseDuration > minLogic1Pulse)){
		irPacket = (irPacket << 1) | 1;
	}
	packetData[packetIndex++] = pulseDuration;
	TACTL = 0;				//turn off timer A e.w.
	LOW_2_HIGH; 				// Setup pin interrupr on positive edge
	break;

```
Case 0: The most important aspect of case 0 was building the IR data packet. This was done using known values for length of 1s and 0s from a given remote. The length was determined from initial waveform readings. 

```C
case 1:							// !!!!!!!!POSITIVE EDGE!!!!!!!!!!!
	TAR = 0x0000;						// time measurements are based at time 0
	TA0CCR0 = 0x2710;
	TACTL = ID_3 | TASSEL_2 | MC_1 | TAIE;
	HIGH_2_LOW; 						// Setup pin interrupr on positive edge
	break;
```

Case 1: On the positive edge, the timers and interrups had to be properly set. TACCR0 was calculated for the proper rollover time. 

#####Timer A Vector
The timer interrupt was a bit similar to the rising edge of the hardware interrupt in the sense that it managed settings of timers and flags. The following code accomplished what is in the picture under TimerA. It is really important to understand how the masks work and howthe XOR and bit manipulation gives the proper functionality. 

```C
#pragma vector = TIMER0_A1_VECTOR			// This is from the MSP430G2553.h file
__interrupt void timerOverflow (void) {

	TACTL = 0;
	TACTL ^= TAIE;
	packetIndex = 0;
	newPacket = TRUE;
	TACTL &= ~TAIFG;
}
```

#####Debugging/Problems
Even though the process for the code was laid out clearly in the given image, I still had trouble implementing it for a while. The hardest part was simply getting an understanding of how the timer worked into the overall scheme. It also took a while to realize how the masks worked and what something like ``` TACTL ^= TAIE; ``` would do. 
An important part of the debugging process was setting a breakpoint in the timerA interrupt to check the packets recived from the remote. If the packets were different than what was expected based off of the inital waveform readings, something was very wrong. 

####Functionality
I was able to get the required functionality working well and it can be seen in the below video

[![IMAGE ALT TEXT HERE](http://img.youtube.com/vi/P8m6kqYVIsk/0.jpg)](http://www.youtube.com/watch?v=P8m6kqYVIsk)

Some thoughts on A functionality: I would imagine A functionality wouldn't be extremely difficult, The hardest part would be getting all the different code to work together but the main premise would be that instead of my if statements looking for button presses, they would look for packet data. 


