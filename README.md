# Dell Laptop Supply Spoofer
So I have this old Dell Inspiron 3593 (2020 model) laptop and it kind of showing its age, with the battery being slightly bloated and relatively high wear level of 40%. 😅

<img width="314" height="206" alt="image" src="https://github.com/user-attachments/assets/b2f79963-7d76-43f4-9f68-118938b4574d" />

I know! I know! I should replace the battery 😅, but I kind of thought "hey why not power the laptop with an external powerbank?" in that way I'm not limited to the laptop's internal battery and I could use my laptop anywhere for many hours without having to worry about a power source. 

I'm an outdoor person and sometimes I get claustrophobic inside coding all day, I would like to sometimes code outdoors, like by the beach, by the park or even in cafes. But with the current state of my laptops battery, I am not able to do those things and that's the main motivation behind this little project. 


# Defining the problem
So due to my laptops relatively old age (5 years is considered old in tech years 😂), it comes installed with a 4.5mm DC power jack. So challenge #1 is, how do I convert the Type-C port that comes with modern power banks into a 4.5mm DC power jack? My answer to that is with these little guys, USB PD trigger boards.  

<img width="250" height="250" alt="Screenshot2 2025-09-19 144130" src="https://github.com/user-attachments/assets/3652847a-56c4-43c3-a758-f07d894d5d77" />

These boards are designed to trigger a USB Power Delivery (USB PD) capable power bank to output a specific voltage level (i.e 5V, 9V, 12V, 15V & 20V) we can then solder on its output pins a 4.5mm DC Jack cable which we plug into our laptop. In our case, we need a power source capable of delivering 20V at 3.25A (65W) which is well within the limits of a USB PD capable 65W Type-C power bank. 

So by using a 65W USB PD capable power bank + USB PD Trigger module + 4.5mm DC Power jack cable and plugging it into the laptop would make this work right???

<img width="1834" height="500" alt="Illustration1" src="https://github.com/user-attachments/assets/d7da5f15-54ed-41e1-8b8b-be78dc088ceb" />


Well, not quite, because Dell laptop power supplies implement an ID chip on its output stage for authenticity checking, output voltage check & output power check. If you just plug in a random 20V supply into the laptop it would know that it's not an authentic dell laptop supply and it would severely limit the amount of current that it would draw from it (800mA in my experience) which results in an underpowered & underclocked laptop leading to poor performance. This is our challenge #2, how to bypass the dell proprietary handshake between supply and laptop. 

<img width="400" height="400" alt="image" src="https://github.com/user-attachments/assets/6b1a4680-bfd3-49b4-a303-d4e577e0b8b3" />

Notice from the schematic, Dell uses a DS2501 EEPROM IC for its ID chip, which is a 512bit 1-Wire prtocol EEPROM chip. This is where the serial number and power supply details are stored which is then read by the laptop when the power supply is first plugged. So in theory, if we read the data inside the DS2501 EEPROM, program it into our own ID chip and wire its output into the ID pin of our 4.5mm DC power jack, we should be able to bypass the authenticity check. 

# Probing the ID Chip
So I probed the ID pin of the supply with a logic analyzer to see the data being exchanged during handshake. Heres what I got. 

<img width="1874" height="600" alt="image" src="https://github.com/user-attachments/assets/eec96f0a-98f2-4da0-b544-0b802d40cf00" />

<img width="716" height="195" alt="image" src="https://github.com/user-attachments/assets/2dd3934f-3971-4673-8f36-9a5d6a463da2" />

RESET CONDITION: The laptop issues a ~500us pulse to check if a 1-Wire slave (DS2501) is present.

PRESENCE PULSE: The DS2501 responds with a presence pulse to let the laptop know its "presence" 

<img width="1141" height="184" alt="image" src="https://github.com/user-attachments/assets/53fd952c-dc4f-475e-8de7-f217ba14a6db" />

SKIP ROM COMMAND: This is a 1-Wire protocol command that basically tells the DS2501 IC to skip the slave identification process.

READ MEMORY COMMAND: This command (0xF0) is used to read data inside the DS2501 EPROM data field. 

<img width="1146" height="188" alt="image" src="https://github.com/user-attachments/assets/4bb8fa90-2dec-45ec-9b41-25ef0152c481" />

The bus master (the laptop) follows the read memory command byte with a 2-byte address TA1:(T7:T0) = 0x06 (lower nibble address), TA2:(T15:T8) = 0x00 (higher nibble address) that indicates a starting address location within the EEPROM data field.

*DATA SECTION*
<img width="1205" height="216" alt="image" src="https://github.com/user-attachments/assets/cc52cef8-a8d9-49a1-a25f-40379ddc11e3" />

DATA 0x27: This is an 8-bit CRC of the command byte and address bytes which is computed by the DS2501. (We can ignore this data)

DATA 0x41,'A': Data inside memory address 0x0006 which is the start address of the READ ROM command. DS2501 auto increments the address if the master issues more read time slots. 

DATA 0x43,'C': Data inside memory address 0x0007.

DATA 0x30,'0': Data inside memory address 0x0008.

DATA 0x36,'6': Data inside memory address 0x0009.

DATA 0x35,'5': Data inside memory address 0x000A.

So if you notice, the data reads "AC065" this basically tells the laptop that its an "AC" supply thats capable of outputting 65Watts! So if we just program these values on the same memory location in our own ID chip then we could essentially bypass the handshake. 

I tried reading the whole EEPROM memory of the ID chip and this is what i got. 

DELL00AC065195033CN0MGJN9LOC0001V8314A08

DELL: Power supply brand

AC: AC supply

65: 65 Watts capable

195: 19.5 Output Voltage

33CN0MGJN9LOC0001V8314A08: Laptop Supply Serial Number

Now we have the values to program into our ID chip! 

# Programming our ID Chip




