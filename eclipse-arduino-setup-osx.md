Steps for installing Eclipse with Arduino on OSx

1) Download and install eclipse indigo from here (install 32bit version for monotor view):

http://www.eclipse.org/downloads/download.php?file=/technology/epp/downloads/release/indigo/SR2/eclipse-cpp-indigo-SR2-incubation-macosx-cocoa.tar.gz&mirror_id=468

2) Install eclipse plugin from baeyens (use uncategorized):

http://www.baeyens.it/eclipse/update 

3)Set arduino path for :

Eclipse -> Preferences -> Arduino -> Arduino

/Applications/Arduino.app/Contents/Resources/Java/
/Applications/Arduino.app/Contents/Resources/Java/Libraries

4)Set paths for avrgcc,gnu make, ...
Eclipse -> Arduino -> Paths 

AVR-GCC,GNU make and AVRdude
/Applications/Arduino.app/Contents/Resources/Java/hardware/tools/avr/bin

AVR Header files
/Applications/Arduino.app/Contents/Resources/Java/hardware/tools/avr/avr/include

Set all to custom except for "Atmel Part Description Files"

5)Add arduino.h to files to index upfront add the followin complete path in :

Eclipse -> preferences -> C++ -> indexer
/Applications/Arduino.app/Contents/Resources/Java/hardware/arduino/cores/arduino/Arduino.h 

* if arduino files not found do the following *
Right click the project->index->Search for unresolved includes.
Right click the project->index->Freshen all Files.


6) Setting AVRDUDE config file to

/Applications/Arduino.app/Contents/Resources/Java/hardware/tools/avr/etc/avrdude.conf
in
Eclipse -> preferences -> Arduino -> AVRdude


7) For serial monitor install coolterm

http://www.macupdate.com/app/mac/31352/coolterm


8)To upload a sketch with eclipse:

Project -> properties -> Arduino -> AVRDUDE -> advanced
check :

Inhibit auto chip erase


9) for monotor view press the + right upside of the view and select baudrate and comport


build project then upload project

Note : 
	
	if trouble with locked com ports do the folowing :
	
	also from commandline execute following commands:
	sudo mkdir /var/lock 
	sudo chmod 777 /var/lock
	

Reference:
http://forum.arduino.cc/index.php?PHPSESSID=d2qk4ttielk6r3svcm81gd52f6&topic=79595.0
http://robots.dacloughb.com/project-1/setting-up-the-arduino-eclipse-environment-on-mac-os-x-lion/