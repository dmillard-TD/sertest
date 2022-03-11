
sertest version 1.0
usage: ././sertest [-v] [-d device] [-b baud] -t|-r
  -v enables verbose mode
  -d <devicename> sets the serial device
  -b <baud> sets the baud rate
  -t sets mode to transmit
  -r sets mode to receive
  -s turns on single character mode


sertest is designed to pass data from one serial port to another
while hooked together in a crossover configuration
i.e. COM1 TX -> COM2 -> RX
     COM1 RX -> COM2 -> TX

One port is designated the "transmitter" and the other port
is designated the "receiver".

First, start up the "receiver" side in it's own shell:

./sertest -d /dev/ttyHS1 -b 115200 -r

Next, start up the "transmitter" side in it's own shell:

./sertest -d /dev/tthHS2 -b 115200 -t

The transmitter will send the characters A-Z and repeat
The recevier will receive each letter and print if the 
letter matches what it expects.

sertest can be run at any standard baud rate up to 4 Mb/s

The '-s' option can be used to enable single character mode.
In this mode, the only character that gets transmitted 
is a 'U' which translates to 0x55.  This is very convenient 
to view on an oscilloscope.

