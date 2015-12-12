"""vtable_dump.py: IDAPython script to dump a linux vtable (and a reconstructed windows one) from a binary."""

"""
This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.
"""

__author__ = "Asher Baker"
__copyright__ = "Copyright 2014, Asher Baker"
__license__ = "zlib/libpng"

import re

"""
Output Format:"
    VTable for <1>: (<2>, <3>)
    Lin    Win Function
    <4><5> <6> <7>
    <4><5> <6> <7>
    ...
    <4><5> <6> <7>

1: Classname
2: VTable Offset
3: Linux This Pointer Offset
4: "T" if the function is a MI thunk
5: Linux VTable Index
6: Windows VTable Index
7: Function Signature
"""

catchclass = False
innerclass = ""

classname = None
offsetdata = {}

# Detect address size
adr_size = 8 if __EA64__ else 4

def ExtractTypeInfo(ea, level = 0):
	global catchclass
	global innerclass
	global classname
	global offsetdata
	
	end = ea + adr_size
	
	while len(Name(end)) == 0:
		end += adr_size
	
	while Dword(end - adr_size) == 0:
		end -= adr_size
	
	# Skip vtable
	ea += adr_size
	
	# Get type name
	name = Demangle("_Z" + GetString(Dword(ea)), GetLongPrm(INF_LONG_DN))
	ea += adr_size
	
	if classname is None and level == 0:
		classname = name
	
	if catchclass:
		innerclass = name
		catchclass = False
	
	print " %*s%s" % (level, "", name)
	
	if not ea < end: # Base Type
		pass
	elif Dword(ea) != 0: #elif isData(GetFlags(Dword(ea))): # Single Inheritance
		ExtractTypeInfo(Dword(ea), level + 1)
		ea += adr_size
	else: # Multiple Inheritance
		ea += 8
		while ea < end:
			catchclass = True
			ExtractTypeInfo(Dword(ea), level + 1)
			ea += adr_size
			offset = Dword(ea)
			ea += adr_size
			#print "%*s Offset: 0x%06X" % (level, "", offset >> 8)
			if (offset >> 8) != 0:
				offsetdata[offset >> 8] = innerclass

# Source: http://stackoverflow.com/a/9147327
def twos_comp(val, bits):
	"""compute the 2's compliment of int value val"""
	if (val & (1 << (bits - 1))) != 0:
		val = val - (1 << bits)
	return val

def Analyze():
	SetStatus(IDA_STATUS_WORK)
	
	if GetLongPrm(INF_COMPILER).id != COMP_GNU:
		Warning("This script is for binaries compiled with GCC only.")
		SetStatus(IDA_STATUS_READY)
		return
	
	ea = ScreenEA()
	
	end = ea + adr_size
	while Demangle(Name(end), GetLongPrm(INF_LONG_DN)) is None:
		end += adr_size
	
	while Dword(end - adr_size) == 0:
		end -= adr_size
	
	while Demangle(Name(ea), GetLongPrm(INF_LONG_DN)) is None:
		ea -= adr_size
	
	name = Demangle(Name(ea), GetLongPrm(INF_LONG_DN))
	if ea == BADADDR or name is None or not re.search(r"vf?table(?: |'\{)for", name):
		Warning("No vtable selected!\nSelect vtable block first.")
		SetStatus(IDA_STATUS_READY)
		return
	
	linux_vtable = []
	temp_windows_vtable = []
	
	other_linux_vtables = {}
	other_thunk_linux_vtables = {}
	temp_other_windows_vtables = {}
	
	# Extract vtable
	while ea < end:
		# Read thisoffs
		offset = -twos_comp(Dword(ea), 32)
		#print "Offset: 0x%08X (%08X)" % (offset, ea)
		ea += adr_size
		
		# Read typeinfo address
		typeinfo = Dword(ea)
		ea += adr_size
		
		if offset == 0: # We only need to read this once
			print "Inheritance Tree:"
			ExtractTypeInfo(typeinfo)
		
		while ea < end and (isCode(GetFlags(Dword(ea))) or Name(Dword(ea)) == "___cxa_pure_virtual"):
			name = Name(Dword(ea))
			demangled = Demangle(name, GetLongPrm(INF_LONG_DN))
			#print "Name: %s, Demangled: %s" % (name, demangled)
			
			name = demangled if demangled else name
			
			if offset == 0:
				linux_vtable.append(name)
				temp_windows_vtable.append(name)
			else:
				if offset not in other_linux_vtables:
					other_linux_vtables[offset] = []
					temp_other_windows_vtables[offset] = []
					other_thunk_linux_vtables[offset] = []
				
				if "`non-virtual thunk to'" in name:
					other_linux_vtables[offset].append(name[22:])
					other_thunk_linux_vtables[offset].append(name[22:])
					temp_other_windows_vtables[offset].append(name[22:])
				else:
					other_linux_vtables[offset].append(name)
					temp_other_windows_vtables[offset].append(name)
				
				# MI entry, strip "`non-virtual thunk to'" and remove from list
				#     But not if it's a dtor... what the hell is this.
				if "`non-virtual thunk to'" in name and "::~" not in name:
					name = name[22:]
					#print "Stripping '%s' from windows vtable." % (name)
					temp_windows_vtable.remove(name)
			
			ea += adr_size
	
	for i, v in enumerate(temp_windows_vtable):
		if "::~" in v:
			#print "Found destructor at index %d: %s" % (i, v)
			del temp_windows_vtable[i]
			break
	
	windows_vtable = []
	overload_stack = []
	prev_function = ""
	prev_symbol = ""
	for v in temp_windows_vtable:
		function = v.split("(", 1)[0]
		
		if function == prev_function:
			# If we don't have a stack, we need to push the last function on first
			if len(overload_stack) == 0:
				# We will have added this in the previous run, remove it again...
				windows_vtable.pop()
				#print "Storing '%s' (!)" % (prev_symbol)
				overload_stack.append(prev_symbol)
			#print "Storing '%s'" % (v)
			overload_stack.append(v)
		else:
			# If we've moved onto something new, dump the stack first
			while len(overload_stack) > 0:
				windows_vtable.append(overload_stack.pop())
			
			windows_vtable.append(v)
		
		prev_function = function
		prev_symbol = v
	
	# If there is anything left in the stack, dump it
	while len(overload_stack) > 0:
		windows_vtable.append(overload_stack.pop())
	
	print "\nVTable for %s: (0, 0)" % (classname)
	print " Lin  Win Function"
	for i, v in enumerate(linux_vtable):
		if "__cxa_pure_virtual" in v:
			print "P%3d" % (i)
			continue
		
		winindex = windows_vtable.index(v) if v in windows_vtable else None
		if winindex is not None:
			print "%4d %4d %s" % (i, winindex, v)
		else:
			print "%4d      %s" % (i, v)
	
	for k in temp_other_windows_vtables:
		for i, v in enumerate(temp_other_windows_vtables[k]):
			if v.find("::~") != -1:
				#print "Found destructor at index %d: %s" % (i, v)
				del temp_other_windows_vtables[k][i]
				break
	
	other_windows_vtables = {}
	for k in temp_other_windows_vtables:
		other_windows_vtables[k] = []
		overload_stack = []
		prev_function = ""
		prev_symbol = ""
		for v in temp_other_windows_vtables[k]:
			function = v.split("(", 1)[0]
			if function == prev_function:
				if len(overload_stack) == 0:
					other_windows_vtables[k].pop()
					#print "Storing '%s' (!)" % (prev_symbol)
					overload_stack.append(prev_symbol)
				#print "Storing '%s'" % (v)
				overload_stack.append(v)
			else:
				if len(overload_stack) > 0:
					while len(overload_stack) > 0:
						other_windows_vtables[k].append(overload_stack.pop())
				other_windows_vtables[k].append(v)
			prev_function = function
			prev_symbol = v
	
	for k in other_linux_vtables:
		print "\nVTable for %s: (%d, %d)" % (offsetdata[k], offsetdata.keys().index(k) + 1, k)
		print " Lin  Win Function"
		for i, v in enumerate(other_linux_vtables[k]):
			if "__cxa_pure_virtual" in v:
				print "P%3d" % (i)
				continue
			
			winindex = other_windows_vtables[k].index(v)
			if v not in other_thunk_linux_vtables[k]:
				print "%4d %4d %s" % (i, winindex, v)
			else:
				print "T%3d %4d %s" % (i, winindex, v)
	
	SetStatus(IDA_STATUS_READY)

if __name__ == '__main__':
	Analyze()
