#!/usr/bin/env python3

import error

class Token:
	def __init__(self, line, column, value = None):
		self.line = line
		self.column = column
		self.value = value
		
	def __repr__(self):
		return f"{self.value}"
	
	def __eq__(self, other):
		return type(self) is type(other) and self.value == other.value

class Ident(Token):
	pass

class Keyword(Token):
	pass

class Int(Token):
	pass

class String(Token):
	def __repr__(self):
		return f'"{self.value}"'

class Bool(Token):
	def __repr__(self):
		return "true" if self.value else "false"

class Special(Token):
	def __repr__(self):
		return f'"{self.value}"'

class End(Token):
	def __repr__(self):
		return "END"

specials = [
	"==",
	"!=",
	"<=",
	">=",
	":",
	";",
	",",
	"=",
	"+",
	"-",
	"*",
	"<",
	">",
	"{",
	"}",
	"(",
	")",
]

special_chars = []

for special in specials:
	for char in special:
		if char not in special_chars:
			special_chars.append(char)

keywords = [
	"bool",
	"else",
	"false",
	"func",
	"if",
	"int",
	"print",
	"return",
	"string",
	"true",
	"var",
	"while",
]

def lex(src):
	tokens = []
	line = 1
	column = 1
	
	def record(kind, value = None):
		tokens.append(kind(line, column, value))
	
	while len(src) > 0:
		char = src[:1]
		
		if isAlpha(char):
			text = ""
			
			while len(src) > 0 and isAlDec(char):
				text += char
				src = src[1:]
				char = src[:1]
			
			if text in ["true", "false"]:
				record(Bool, text == "true")
			elif text in keywords:
				record(Keyword, text)
			else:
				record(Ident, text)
		
		elif isDec(char):
			text = ""
			
			while len(src) > 0 and isDec(char):
				text += char
				src = src[1:]
				char = src[:1]
			
			val = parse_int(text, line)
			record(Int, val)
		
		elif isWhite(char):
			src = src[1:]
		
		elif char == "\n":
			src = src[1:]
			line += 1
		
		elif isSpecial(char):
			found = False
			
			for special in specials:
				extract = src[:len(special)]
				
				if extract == special:
					found = True
					break
			
			if found:
				record(Special, extract)
				src = src[len(extract):]
			else:
				error.error(line, "unrecognized special", char)
				src = src[1:]
		
		elif char == '"':
			src = src[1:]
			char = src[:1]
			text = ""
			
			while len(src) > 0 and char >= " " and char != '"':
				text += char
				src = src[1:]
				char = src[:1]
			
			if char != '"':
				error.error(line, "string must be terminated with \"")
			
			src = src[1:]
			record(String, text)
		
		elif char == '#':
			while len(src) > 0 and char != "\n":
				src = src[1:]
				char = src[:1]
		
		elif src[:2] == "/*":
			src = src[2:]
			
			while len(src) > 0 and src[:2] != "*/":
				src = src[1:]
			
			if src[:2] != '*/':
				error.error(line, "multiline-comment must be terminated with */")
			else:
				src = src[2:]
		
		else:
			error.error(line, "unrecognized token", char)
			src = src[1:]
	
	record(End)
	return tokens

def isAlpha(char):
	return char >= "a" and char <= "z" or char >= "A" and char <= "Z"

def isDec(char):
	return char >= "0" and char <= "9"

def isAlDec(char):
	return isAlpha(char) or isDec(char)

def isWhite(char):
	return char == " " or char == "\t"

def isSpecial(char):
	return char in special_chars

def parse_int(text, line):
	val = 0
	
	for char in text:
		val *= 10
		val += ord(char) - ord("0")
	
	if val > 0xffFFffFF:
		error.error(line, "value", text, "is bigger than 0xffFFffFF")
		return 0
	
	return val

