#!/usr/bin/env python3

import error

def load_source(src_name):
	fs = open(src_name, "rb")
	src = fs.read()
	src_txt = ""
	
	for char_code in src:
		char = chr(char_code)
		
		if char_code > 0x7f:
			error.error(0, "character code", char_code, "is not an ascii code")
		elif char_code < 32 and char not in ["\n", "\t"]:
			error.error(0, "character code", char_code, "is not allowed")
		
		src_txt += char
	
	return src_txt

