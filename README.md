# Dell Laptop Supply Spoofer
So I have this old Dell Inspiron 3593 (2020 model) laptop and it's of showing its age, with the battery being slightly bloated and relatively high wear level of 40%. 😅

<img width="314" height="206" alt="image" src="https://github.com/user-attachments/assets/b2f79963-7d76-43f4-9f68-118938b4574d" />

I know! I know! I should replace the battery 😅, but I kind of thought "hey why not power the laptop with an external powerbank?" in that way I'm not limited to the laptops internal battery. I could use my laptop anywhere for many hours without having to worry about a power source. 

I'm an outdoor person and sometimes I get claustrophobic inside coding all day, I would like to sometimes code outdoors, like by the beach, by the park or even in cafes. But with the current state of my laptops battery, I am not able to do those things and that's the main motivation behind this little project. 


# Defining the problem
So due to my laptops relatively old age (5 years is considered old in tech years 😂), it comes installed with a 4.5mm DC power jack. So challenge #1 is, how do I convert the Type-C port that comes with modern power banks into a 4.5mm DC power jack? My answer to that is with these little guys, USB PD trigger boards.  

<img width="250" height="250" alt="Screenshot2 2025-09-19 144130" src="https://github.com/user-attachments/assets/3652847a-56c4-43c3-a758-f07d894d5d77" />

These boards are designed to trigger a USB Power Delivery (USB PD) capable power bank to output a specific voltage level (i.e 5V, 9V, 12V, 15V & 20V) we can then solder on its output pins a 4.5mm DC Jack cable which we can plug into the laptop. As for the power source, we will be using a 65Watt USB PD capable power bank that can output 20V and source up to 3.25A. 

So by using a 65W USB PD capable power bank + USB PD Trigger module + 4.5mm DC Power jack cable and plugging it into the laptop would make this work right???

<img width="1834" height="500" alt="Illustration1" src="https://github.com/user-attachments/assets/d7da5f15-54ed-41e1-8b8b-be78dc088ceb" />


Well, not quite, because Dell laptop power supplies implement an ID chip on its output stage for authenticity checking, output voltage check & output power check. If you just plug in a random 20V supply into the laptop it would know that it's not an authentic dell laptop supply and it would severely limit the amount of current that it would draw from it (800mA in my experience) which results in an underpowered & underclocked laptop leading to poor performance. This is our challenge #2, how to bypass the dell proprietary handshake between supply and laptop. 

<img width="400" height="400" alt="image" src="https://github.com/user-attachments/assets/6b1a4680-bfd3-49b4-a303-d4e577e0b8b3" />

Notice from the schematic, Dell uses a DS2501 EEPROM IC for its ID chip, which is a 512bit 1-Wire protocol EEPROM chip. This is where the serial number and power supply details are stored, which is then read by the laptop when the power supply is first plugged in. So in theory, if we read the data inside the DS2501 EEPROM, program it into our own ID chip and wire the data pin into the ID pin of our 4.5mm DC power jack, we should be able to bypass the authenticity check. 

So now, the whole system should look like this: 

<img width="2459" height="492" alt="Illustration2" src="https://github.com/user-attachments/assets/9c9b66fa-e823-4a8a-97ff-b42679d37232" />

# Probing the ID Chip
So I probed the ID pin of the supply with a logic analyzer to see the data being exchanged during handshake. Heres what I got. 

<img width="1874" height="600" alt="image" src="https://github.com/user-attachments/assets/eec96f0a-98f2-4da0-b544-0b802d40cf00" />

<img width="716" height="195" alt="image" src="https://github.com/user-attachments/assets/2dd3934f-3971-4673-8f36-9a5d6a463da2" />

RESET CONDITION: The laptop issues a ~500us pulse to check if a 1-Wire slave (DS2501) is present.

PRESENCE PULSE: The DS2501 responds with a presence pulse to let the laptop know its "presence" 

<img width="1141" height="184" alt="image" src="https://github.com/user-attachments/assets/53fd952c-dc4f-475e-8de7-f217ba14a6db" />

SKIP ROM COMMAND: This is a 1-Wire protocol command that basically tells the DS2501 IC to skip the slave identification process.

READ MEMORY COMMAND: This command (0xF0) is used to read data inside the DS2501 EEPROM data field. 

<img width="1146" height="188" alt="image" src="https://github.com/user-attachments/assets/4bb8fa90-2dec-45ec-9b41-25ef0152c481" />

The bus master (the laptop) follows the read memory command byte with a 2-byte address TA1:(T7:T0) = 0x06 (lower nibble address), TA2:(T15:T8) = 0x00 (higher nibble address) that indicates a starting address location within the EEPROM data field.

**DATA SECTION**
<img width="1205" height="216" alt="image" src="https://github.com/user-attachments/assets/cc52cef8-a8d9-49a1-a25f-40379ddc11e3" />

DATA 0x27: This is an 8-bit CRC of the command byte and address bytes which is computed by the DS2501.

DATA 0x41,'A': Data inside memory address 0x0006 which is the start address of the READ ROM command. DS2501 auto increments the address if the master issues more read time slots. 

DATA 0x43,'C': Data inside memory address 0x0007.

DATA 0x30,'0': Data inside memory address 0x0008.

DATA 0x36,'6': Data inside memory address 0x0009.

DATA 0x35,'5': Data inside memory address 0x000A.

So if you notice, the data reads "AC065" this basically tells the laptop that its an "AC" supply thats capable of outputting 65Watts! So if we just program these values on the same memory location in our own ID chip then we could essentially bypass the handshake. 

I tried reading the whole EEPROM of the ID chip and this is what i got. 

DELL00AC065195033CN0MGJN9LOC0001V8314A08

DELL: Power supply manufacturer

AC: AC supply

65: 65 Watts capable

195: 19.5 Output Voltage

33CN0MGJN9LOC0001V8314A08: Laptop Supply Serial Number

Now we have the values to program into our ID chip! 

# Programming the ID Chip

The dell laptop supplies use a DS2501 ID Chip, I have a few problems on using this chip:

1. Its a one time programmable chip.
2. You need a 12V pulse applied to the data pins to program the EEPROM. Which we would need a separate circuit for precise 12V pulse generation.
3. Lastly, IT'S OBSOLETE! 😭

So instead, I opted for a newer chip in the same family, the DS2431 1024bit 1-Wire EEPROM IC. I think this is an improvement over the now obsolete DS2501 for the following reasons:

1. You could program the EEPROM multiple times.
2. You do not need a 12V pulse to program the chip.
3. Same EEPROM addresses and memory structure as DS2501.
4. Same ROM commands as DS2501
5. Larger memory.
6. Lastly, ITS NOT OBSOLETE! 👌

I've developed a driver for programming and reading into the EEPROM of these chips. You can download and refer to the Repo link given below on how to use the driver.

Repo Link: https://github.com/eldonjakee08/DS2431_1WIRE_EEPROM_DRIVER

<img width="571" height="316" alt="image" src="https://github.com/user-attachments/assets/a17aa85a-c4ab-4d42-9d8f-86aa5feede38" />

I made a simple STM32 program to write into the EEPROM. Let me explain how it works. 
1. declare and intialize uint8_t pdata[] array, this will hold the first 8 bytes of data to be written into EEPROM memory location 0x0000 to 0x0007
2. call OneWire_WriteMemory() function (from the driver) to write the contents of pdata array into DS2431s EEPROM
3. declare and intialize another uint8_t pdata2[] array, this will hold the 2nd batch of 8 byte data to be written into memory location 0x0008 to 0x000F
4. call OneWire_WriteMemory() function again to write the contents of pdata2 array into DS2431s EEPROM

If you notice, in pdata[] array, theres this "0x27" byte. This data is actually the 8bit CRC generated by the DS2501 after master issues the SKIP ROM, READ MEMORY & MEMORY LOCATION commands. 

The problem with DS2431 is that, it does not generate an 8Bit CRC after master issues the previously mentioned commands. The laptop expects to get the 0x27 CRC from slave, so to solve this, we hard coded the 0x27 8bit CRC into the memory location 0x0006 of DS2431 EEPROM. 

This particular memory location is important as this is where the laptop starts to read from the ID chips EEPROM. So by the time the laptop expects an 8Bit CRC from the slave, what the DS2431 essentially does is sending the hard coded 0x27 CRC data inside the memory location 0x0006, thereby replicating the DS2501 behavior. 👌

After programming the chip we should get a message in the Serial Wire Viewer. (Shown below)

<img width="465" height="890" alt="Screenshot 2025-07-26 013716" src="https://github.com/user-attachments/assets/65a82975-156f-4c5b-8d1f-67bd8ddf90db" />


Now that we have the programmed chip, we can now proceed in assembling everything together. 

# Assembling the Hardware

The hardware for this project is pretty simple. See schematic below. 

<img width="884" height="369" alt="image" src="https://github.com/user-attachments/assets/04f0b716-b396-4e2d-ac99-6bf7bdb13a54" />

The output of the PD trigger is directly connected to the power lines of the 4.5mm DC jack cable. The data line of the DS2431 is directly connected to the ID line of the cable. 

If you notice, the DS2431 does not have any power rails, this is because the IC can operate in "parasitic power mode" this essentially means that it draws its power from the data lines during the read/write process. 

**END PRODUCT**

<img width="550" height="550" alt="image" src="https://github.com/user-attachments/assets/f11efeaf-0275-48ba-9d0a-c0103ba2d1e3" />

<img width="550" height="550" alt="image" src="https://github.com/user-attachments/assets/b29ef2d5-ccbe-4c6f-b3ce-696ca427efab" />

**TESTING**

Heres a youtube video with me testing the Supply Spoofer. 

[![Youtube](https://img.youtube.com/vi/CXdM-Me_jxc/maxresdefault.jpg)](https://www.youtube.com/watch?v=CXdM-Me_jxc)
