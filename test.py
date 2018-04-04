import argparse, json, struct
from pprint import pprint
from struct import *
parser = argparse.ArgumentParser()
parser.add_argument("jsonFile")
args = parser.parse_args()
print(args.jsonFile)

data = json.load(open(args.jsonFile))
pprint(data)

#
#              id      fsize   x       y       width   height  r   g   b
# echo -e 'dtbx\x01\x00\x20\x00\x60\x00\x50\x00\x60\x00\x24\x00\x11\x80\x13\x05\x00Hello' > /tmp/kedei_lcd_in
fifo_write = open('/tmp/kedei_lcd_in', 'wb')
for ctrl in data["controls"]:
	#pprint(ctrl)
	if ctrl["type"] == "textBox":
		print("Text Box:")
		fifo_write.write(bytearray(b'dtbx'))
		slen = len(ctrl["text"]);
		binary_data = struct.pack("=HHHHHHBBBH", ctrl["id"], ctrl["fontSize"], ctrl["x"], ctrl["y"], ctrl["width"], ctrl["height"], ctrl["r"], ctrl["g"], ctrl["b"], slen)#, len(ctrl["text"])
		fifo_write.write(binary_data)
		fifo_write.write(bytearray(ctrl["text"], encoding="utf-8"))
	elif ctrl["type"] == "label":
		print("Label:")
		fifo_write.write(bytearray(b'dlbl'))
		slen = len(ctrl["text"]);
		binary_data = struct.pack("=HHHHHHBBBH", ctrl["id"], ctrl["fontSize"], ctrl["x"], ctrl["y"], ctrl["width"], ctrl["height"], ctrl["r"], ctrl["g"], ctrl["b"], slen)#, len(ctrl["text"])
		fifo_write.write(binary_data)
		fifo_write.write(bytearray(ctrl["text"], encoding="utf-8"))
fifo_write.close()
	
