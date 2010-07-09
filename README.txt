KitServer 4                                                    July 8, 2010
===========================================================================
Version 4.3.11


1. INTRODUCTION
---------------

KitServer 4 is an "add-on" program for Pro Evolution Soccer 4 (PC) and
Winning Eleven 8 International (PC).  It enhances the "Strip Selection" 
menu of the game by allowing to select additional uniforms for Home and Away 
teams (for players and goalkeepers), and a game ball from an external list
of "extra" balls.
Configurable hot-keys are used to trigger selections.

This makes it easy to use "3rd" a.k.a. "alternative" a.k.a. "European" or 
"Champions League" (or whatever you want to call them) kits for specific 
matches, without having to edit the AFS-file and the executable.  Also, 
sometimes, teams use different shorts or socks with standard colored
shirts. This too is easily done with KitServer: all you have to do is 
just add extra uniforms with new combinations. 

KitServer can also replace default kits (players and goalkeepers): 
player 1st strip, player 2nd strip, goalkeeper 1st, goalkeeper 2nd.

Another usage for KitServer is testing of new kits/balls.


2. SUPPORTED VERSIONS OF THE GAME
---------------------------------

PES4 PC 1.0, PES4 PC 1.10, WE8I PC
PES4 PC Demo, PES4 PC Demo 2.
("3rd"-kit selector and ball selector are not implemented in demos)


3. INSTALL/UNINSTALL
--------------------

(Note: the following instructions use PES4 as an example, but the same
applies to WE8, with "WE8.exe" used instead of "PES4.exe", and etc.)

STEP 1. Copy the entire "kitserver" folder into your PES4 PC folder.
So that you directory structure looks like this:

dat/
kitserver/
PES4.exe
readme.htm
settings.dat
settings.exe

STEP 2. Go to "kitserver" folder, and run setup.exe. 
If STEP 1 was done correctly, you should see your "PES4.exe"
in the dropdown list. If KitServer hasn't been already installed 
for this executable, the "Install" button should become enabled. 
Press "Install" button. The installation should happen pretty quickly - 
in a matter of seconds. Once, it is complete, the popup window will 
display "SUCCESS!" message, or report an error if one 
occured.

If an error occurs, check if your PES4.exe is not currently in use
(i.e. exit the game, if it is currently running). Also, check that 
PES4.exe is not marked as read-only file.

To uninstall KitServer, launch the setup.exe again, select PES4.exe,
and press "Remove" button. After that, you can safely delete the 
whole "kitserver" folder, if you want.


4. HOW TO USE KDB
-----------------

The kits database ("KDB") has a folder "uni", which is where you
place additional folders. Each folder corresponds to a certain team,
and must be named accordingly: for England, you need to create 
folder called "006", for Newcastle - "078", for FC Barcelona -
it would be "165".
Place your kits in those folders, with these special names:

texga.bmp   -  will be used instead of GK 1st kit.
texgb.bmp   -  will be used instead of GK 2nd kit.
texpa.bmp   -  will be used instead of PLAYER 1st kit.
texpb.bmp   -  will be used instead of PLAYER 2nd kit.
gloves.bmp  -  will be used instead of GK gloves

Extra kits go to specially named folders:
"gx" - goalkeeper kits
"px" - player kits

In those folders, BMP files can be named arbitrarily.
For example: arsenal_third.bmp


DEFINING KIT ATTRIBUTES
-----------------------

When using extra kits, you will usually need to define one or more
of the following kit attributes: color of player's name on the shirt, 
color of player's number on the shirt, color of player's number on
shorts, collar of the shirt, 3D-model of the shirt.
(Example: for an "all-white" Holland kit, you will need to define
that numbers on shirt and shorts, as well as name on the back of the
shirt - all need to be orange.) 

All these attributes can be set in "attrib.cfg" file, which should
be placed into corresponding team's folder. For instance, for Holland,
the attrib.cfg will go into KDB/uni/015 folder.

The format of attrib.cfg file is simple. It consists of sections, where
each section describes one kit. A section has a header, which identifies
which file contains the kit. Like this:

[px/arsenal_third.bmp]

The header is followed by kit attribute definitions.
Attribute definition has the following format:

attribute = value

(Spaces on both sides of the "=" are important.)
The attrib.cfg file may also contain comments, which are solely for
user's convenience, and are ignored by KitServer. Comment starts with
a "#" character and continues until the end of line.

SUPPORTED ATTRIBUTES:

"shirt.name"
takes a value that defines color of player's name on the back of a
shirt. The value is a hexadecimal number RRGGBB. (Very similar to how
colors are defined in HTML, except there is no leading "#" character)
Also supported are values in format RRGGBBAA, where the last 2 digits
represent the alpha channel. This way you can control transparency:
00 - means transparent, FF - opaque. Values in between those boundaries 
will produce the translucent look: for example: FFFF0080 is a semi-
transparent yellow.
(NOTE: using transparent colors gives you an alternative way to turn off 
names, numbers of shirts/shorts.)

"shirt.number"
defines color of the player's number on the back of the shirt (and on
the front of the shirt - for national teams). Value has the same 
format as "shirt.name".

"shorts.number"
defines color of the player's number on shorts. Value has the same 
format as "shirt.name".

"collar"
Two possible values: yes or no. 
Defines whether the shirt has a collar or not. Note that not all 
shirt models support collars, so even if you specify this attribute
as "yes", it's not guaranteed that the shirt will appear with collar,
unless you also specify a 3D-model that supports collars.

"cuff"
Two possible values: yes or no.
Defines how the edges of long sleeves look. If set to "yes" the
sleeves will have cuffs, otherwise they will be just plain straight
sleeves. Note that some models may override this setting and force
special-looking cuffs.

"model"
Specifies which 3D-model should be used with this kit. This attribute
is only supported for player kits - not goalkeepers. Goalkeepers and
players of the same team share the same 3D-model (that appears to be
the limitation set by the game itself).
The value is a decimal number, specifying the 3D-model id. 
Hint: one way to learn about different 3D-models is to use Waterloo's 
tool ("WE Set Default Colour" v3.2.0) - it has explanation about almost 
any shirt model used in the game. Another way: visit Ajay's excellent
site ( http://pes4.edgegaming.com/ ) - it has an article about models 
with explanations and screenshots.  

"shorts.number.location"
Possible values: off, left, right, both
Specifies where should the number appear on the shorts.

"number.type"
Possible values: 0,1,2,3
Specifies which number type (from etc_ee_tex.bin) should be used 
with this kit.

"name.type"
Possible values: 0,1
Specifies which name font (from etc_ee_tex.bin) should be used
with this kit.

"name.shape"
Possible values: straight, curved
This allows to specify whether the name should have a curved shape,
or be straight.


EXAMPLE of a section in attrib.cfg:

# Arsenal third "yellow" kit
shirt.name = 000088  # dark blue
shirt.number = 000088 # dark blue
shorts.number = 888800 # yellow
collar = yes
cuff = yes
model = 4
shorts.number.location = left
number.type = 2
name.type = 0 # standard type
name.shape = straight


NOTE ABOUT EDIT MODE:
~~~~~~~~~~~~~~~~~~~~~
When using the game's Edit Mode to alter the way kits look, keep
in mind that kitserver always tries to enforce its setting and override
the game's default or edited values. For example, if you have a kit that
have a full set of attributes defined in attrib.cfg (like example above), 
then when you go to edit mode and try to modify the colors or short number
location, you will not see the expected change, because kitserver will
dynamically override your change with the value it got from attrib.cfg.
Of course, if the attribute was not defined in attrib.cfg, then the 
kitserver won't interfere with your editing.
Alternatively, you can choose not to define any attribues for a kit in 
attrib.cfg, but instead use the Edit Mode to set the colors, number-type, 
name-type, etc. This would work fine for 1st/2nd kit, but for 3rd(extra) 
kits you must define this attributes in attrib.cfg. I recommend always 
defining all attributes for all kits (1st,2nd,and extra) in attrib.cfg. 
This way everything is kept in one place and is easier to manage.


5. HOW TO PREPARE KITS
----------------------

Step #1 is to download a kit.
You can use pretty much any PES4 kit in 512x256 BMP format.
(Which is how all major kitmakers - Spark, kEL, biker_jim_uk, and others -
distribute the kits). 

Many kits are already prepared with correct transparency.
(For example, kits from kitpacks done by Spark are like this. 
Download them here: http://www.pespc-files.com)
If you downloaded such kit, you don't need to do anything extra.
Otherwise, make sure you follow Step #2:

Step #2 is to set proper transparency for the BMP palette.
For that, you can use obocaman's WE/PES Graphics Studio v04.04.24 
(or later). In the right section ("Bitmap Info"), open your BMP, then 
set all the colors to be opaque. After that, in palette, select the 
color(s) that you need to be transparent (usually some bright green 
or pink), and set its opacity to 0%. Save the kit, by right-clicking 
on it, and choosing "Save As" menu item.


6. BALL CONFIGURATION
---------------------

Inside KDB, there is also a folder called "balls". Place extra ball files
into that directory. For each extra ball you must add a section in 
attrib.cfg file, like this:

[name of the ball]
ball.model = <filename>
ball.texture = <filename>

where "name of the ball" is what will be displayed by KitServer as the
ball's label; ball.model specifies which file contains the 3d-model of the
ball, and ball.texture specifies the texture file.

For ball texture, you can either use BIN file (compressed texture) - which
is how the ball textures are usually distributed by ball-makers. Or you
can use a 256x256 8bit color BMP file (uncompressed texture). This feature
is something that may be useful to ball-makers as a quick-test tool for 
tweaking the ball texture.

See KDB/balls/attrib.cfg file for example ball configurations.


7. HOT-KEYS
-----------

To select extra kits or balls, you will need to press "hot-keys" 
on keyboard. Defaults are:

"1" - switches home player kit
"2" - switches away player kit
"3" - switches home goalkeeper kit
"4" - switches away goalkeeper kit
"V" - selects previous extra ball
"B" - selects next extra ball
"R" - selects random extra ball
"C" - resets ball selection back to "game choice".

The keys are defined in kserv.cfg file, which is a simple text file
that can be either modified manually (in Notepad, for example), or 
you can use a GUI program: kctrl.exe. Using kctrl.exe allows for easy 
re-mapping of hot-keys, but some (advanced) options cannot be modified 
with kctrl.exe. For example, to change the value of "kit.useLargeTexture"
you will need to manually edit kserv.cfg.


8. TROUBLESHOOTING
------------------

Sometimes, during Kitserver install, the "auto-detect" selection of 
operating system fails. This results in the following: the game crashes 
right at the beginning, when Kitserver is installed, and works fine, 
when Kitserver is not installed. 
Here is what you can try to fix this: 

1. Start a command-prompt window (Start->Run->"cmd")
Go to kitserver folder, and type in: setup -os
2. You should see the setup window with "Operating system" 
selector enabled. Press "Remove" button (if the button is enabled) 
- a message box saying that Kitserver was successfully uninstalled 
should appear.
3. From the drop-down list of operating systems, select the one that 
matches yours.
4. Press "Install" button (if the button is enabled) - a message
box saying that Kitserver was successfully installed should appear.
5. Exit setup.


9. CREDITS
----------

Programming: Juce
Testing: Jim (biker_jim_uk)
Sample kits: (c)Spark, (c)kEL
Sample balls: (c)Spark
Nike Aerow ball 3D-model: Ariel Santarelli

===========================================================================

