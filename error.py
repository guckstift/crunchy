#!/usr/bin/env python3

count = 0

def error(line, *args):
	global count
	
	if line > 0:
		print("error @ line", line, ":", *args)
	else:
		print("error :", *args)
	
	count += 1

def fatal(line, *args):
	if line > 0:
		print("fatal @ line", line, ":", *args)
	else:
		print("fatal :", *args)
	
	exit(1)
