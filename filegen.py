#!/usr/bin/python

import binascii, random, os, sys, time

directory = None
s = -1
g = -1
gm = -1
keystypes = { "sensorname" : None,
              "firmware" : "rand_firmware",
              "timestamp" : None,
              "unixtimestamp" : None,
              "bytecount" : int,
              "packetcount" : int,
              "malwarecount" : int,
              "uptime" : int,
              "machineip" : "rand_machineip",
              "language" : "rand_language",
              "ipv4count" : int,
              "ipv6count" : int,
              "threatlevel" : int }
sensors = [1, 2, 11, 12, 17, 18, 21, 23, 24, 27, 28, 40, 41, 42, 44, 45, 46, 48,
    50, 54, 56, 57, 60, 61, 63, 64]
sensornames = ["Alfa", "Bravo", "Charlie", "Delta", "Echo", "Foxtrot", "Golf",
    "Hotel", "India", "Juliett", "Kilo", "Lima", "Mike", "November", "Oscar",
    "Papa", "Quebec", "Romeo", "Sierra", "Tango", "Uniform", "Victor", "Whiskey",
    "Xray", "Zulu"]
languages = ["English", "Spanish", "French", "Italian", "Afrikaans", "Korean",
    "Chinese", "Japanese", "Dutch", "German", "Greek", "Portuguese", "Russian"]
    
counter = 0

def rand_firmware():
    f1 = str(random.randrange(1, 7))
    f2 = str(random.randrange(11, 99))
    f3 = str(random.randrange(1111, 9999))
    return f1 + "." + f2 + "." + f3

def timestamp(unixtime):
	return time.strftime("%Y-%m-%d-%H-%M-%S", unixtime)

def rand_unixtime():
    currenttime = time.time()
    randtime = random.randrange(int(round(currenttime / 1000)), int(currenttime))
    return randtime

def rand_machineip():
    ip1 = str(random.randrange(255))
    ip2 = str(random.randrange(255))
    ip3 = str(random.randrange(255))
    ip4 = str(random.randrange(255))
    return ip1 + "." + ip2 + "." + ip3 + "." + ip4

def rand_language():
    return languages[random.randrange(len(languages) - 1)]

def get_garbage_kv():
	k = binascii.b2a_hex(os.urandom(4))
	v = binascii.b2a_hex(os.urandom(4))
	return (str(k) + "=" + str(v))

def generate_file():
    global counter
	
    sensor = random.randrange(len(sensors) - 1)
    rand_time = rand_unixtime()
    rand_time_struct = time.localtime(rand_time)
    
    contents = ""
    keys = keystypes.keys()
    random.shuffle(keys)
    
    for key in keys:
    	if 0 == random.randrange(0, s):
    		continue
    		
        contents += (key + '=')
        
        if keystypes[key] is int:
            contents += str(random.randrange(sys.maxint))
        else:
            if key == "sensorname":
                contents += sensornames[sensor]
            elif key == "unixtimestamp":
            	contents += str(rand_time)
            elif key == "timestamp":
            	contents += timestamp(rand_time_struct)
            else:
                contents += globals()[keystypes[key]]()
        
        contents += '\n'
        
        if 0 == random.randrange(0, g):
        	for i in range(0, random.randrange(1, gm)):
        		contents += (get_garbage_kv() + "\n")
    
    timesecs = int(round(time.time()))
    timesecs -= counter
    counter += 1
    filestr = directory + str(sensors[sensor]) + '_' + str(timesecs) + ".sen"
    to_write = open(filestr, "w")
    to_write.write(contents)
    print "Wrote contents: " + filestr
    to_write.close()

def print_usage():
	print "MATH 4777 Project File Generator\nUsage: ./filegen.py <#files> <directory> [-s <%chance of key skipping>] [-g <%chance of garbage> [-gm <max number of garbage lines>]]\n"

def main(argv):
    global directory, s, g, gm
    
    if len(argv) < 2:
        print_usage()
        return -1
    elif len(argv) >= 3:
    	for i in range(2, len(argv)):
    		try:
    			float(argv[i])
    			continue
    		except:
				if argv[i] == "-s":
					s = int(1 / float(argv[i + 1]))
				elif argv[i] == "-g":
					g = int(1 / float(argv[i + 1]))
				elif argv[i] == "-gm":
					gm = int(argv[i + 1])
				else:
					print_usage()
					return -1
    
    if gm > -1 and g == -1:
    	print_usage()
    	return -1
    
    if gm == -1:
    	gm = 10
    
    directory = argv[1]
    
    if not directory.endswith("/"):
    	directory += "/"
    
    random.seed()
    
    for i in range(0, int(argv[0])):
    	print i
        generate_file()
    
    return 0

if __name__ == "__main__":
    main(sys.argv[1:])

