# Dell Laptop Supply Spoofer
So I have this old Dell Inspiron 3593 (2020 model) laptop and it kind of showing its age, with the battery being slightly bloated and relatively high wear level of 40%. 😅

<img width="314" height="206" alt="image" src="https://github.com/user-attachments/assets/b2f79963-7d76-43f4-9f68-118938b4574d" />

I know! I know! I should replace the battery 😅, but I kind of thought "hey why not power the laptop with an external powerbank?" in that way I'm not limited to the laptop's internal battery and I could use my laptop anywhere for many hours without having to worry about a power source. 

I'm an outdoor person and sometimes I get claustrophobic inside coding all day, I would like to sometimes code outdoors, like by the beach, by the park or even in cafes. But with the current state of my laptops battery, I am not able to do those things and that's the main motivation behind this little project. 


# Defining the problem (define the challenges for this project to work)
So due to my laptops relatively old age (5 years is considered old in tech years 😂), it comes installed with a 4.5mm DC power jack. So challenge #1 is, how do I convert the Type-C port that comes with modern power banks into a 4.5mm DC power jack? My answer to that is with these little guys, USB PD trigger boards.  

<img width="250" height="250" alt="Screenshot2 2025-09-19 144130" src="https://github.com/user-attachments/assets/3652847a-56c4-43c3-a758-f07d894d5d77" />

These boards are designed to trigger a USB Power Delivery (USB PD) capable power bank to output a specific voltage level (i.e 5V, 9V, 12V, 15V & 20V). In our case, we need a power source capable of delivering 20V at 3.25A (65W) which is well within the limits of a USB PD capable 65W Type-C power bank. 

So by using a 65W USB PD capable power bank + USB PD Trigger module + 4.5mm DC Power jack cable and plugging it into the laptop would make this work right???

<img width="1834" height="500" alt="Illustration1" src="https://github.com/user-attachments/assets/d7da5f15-54ed-41e1-8b8b-be78dc088ceb" />


Well, not quite, because Dell laptop power supplies implement an ID chip on its output stage for authenticity checking, output voltage check & output power check. If you just plug in a random 20V supply into the laptop it would know that it's not an authentic dell laptop supply and it would severely limit the amount of current that it would draw from it (800mA in my experience) which results in an underpowered & underclocked laptop leading to poor performance. This is our challenge #2, how to bypass the dell proprietary handshaker between supply and laptop. 

<img width="628" height="447" alt="image" src="https://github.com/user-attachments/assets/6b1a4680-bfd3-49b4-a303-d4e577e0b8b3" />
