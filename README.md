1. Data Exchange

UpStream: esp32- jetson nano
all the sesnor data, current posoiton of servos, ball
DownStream: jetson nano - esp32
the commands for walk,crouch,stand tall, balance,

2. Ecoding
DownStream:
it works by assinging every byte of 1 packet to 1 task
1st byte has special combination of numbers that is very unlikely to popup randomly,followed by message, the checksum

example
byte 0     = 0x05 Header 1
byte 1     = 0x01 Header 2
byte 2     = mode
byte 3     = speed
byte 4     = turn
byte 5     = other info
byte 6     = sequence
byte 7     = checksum

3.Checksum
a checksum works agreiing on a fucniton (like XOR )by putting the message through a fucntion or operation, outputting a 1 byte output, which is appended along with the message, the reciever does same math operation , then comapres it with the checksum

4. Example

lets say we use a XOR fucntion,lets calcualte a checksum using a jason command for dog to mode: walk ,speed 50%, no turn, and sequence number 0x05

1we XOR the bytes mode^speed^turn^sequence- 0x01 ^ 0x32 ^ 0x00 ^0x05


after calculating xor we get 0x36 as the 8th byte, we append it, esp does the same math and should get identical data


5.Errors handling
If the esp32's checksum fails to match, it knows its garbage data, it completely drops the current packet and waits for the next starting symbol to appear,


ENERGY CALC==============================================================

Normal Trotting Current: During a dynamic trot, 4 of the servos (the hip abductors) simply hold the legs straight, while the other 8 servos (hips and knees) are actively sweeping back and forth. This averages out to approximately 0.6A per servo.
Total CUrrent = 12*0.65 = 7.8A
TOtal wattage = 6*7.8 = 46.8 watts


Peak / Stall Current: An MG996R servo will stall (draw maximum current) at roughly 2.5A  when running at 6V .
Ttoal Current = 12*2.5A = 30A
Total watts = 180 W


Boards
JEtson Nano depending on mode will consume between 7W and 15W ,
Esp32+Mpu6050 will consume ab 1W

for external batter to power the motors,
BEtween a 3s and a 2s battery ,
we will select a 3s battaery,
2s battery has 3.7*2 = 7.4v , but it dips down to 6.3 volts dischargin, which reduces the total headroom for the motors,

a 3s battery has 3.7*3 = 11.2 v and it dips down to 9 v which is safe, and jetson nano accepts volatage between 9-19 volts .


is we use a 3300 mAh battry, its rated 35 c, 3300mah*35c = 115A
it can provide 115 amps of coninous current

assume normal walking,
dog cosumes 50W(servos) + 15 W(jetson+esp+mpu6050)

total current draw =  65W/ 11.2 = 5.8A

RUntime = 3300mAh /5.8A* 60 /100 = 27.3 Minuts of continos walking

POWER REGULATOR
FOr the servos: we use 2 6V 15 AMpDc-Dc buck converter ensures constant volatage to the servos. on either side each handling 6 servos.

For Microcontrollers we can use a 5-3.3V buck converter




