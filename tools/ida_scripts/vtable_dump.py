"""vtable_dump.py: IDAPython script to dump a linux vtable (and a reconstructed windows one) from a binary."""

"""
This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.
"""

__author__ = "Asher Baker"
__copyright__ = "Copyright 2012, Asher Baker"
__license__ = "zlib/libpng"

import re

def Analyze():
	SetStatus(IDA_STATUS_WORK)
	
	if GetLongPrm(INF_COMPILER).id != COMP_GNU:
		Warning("This script is for binaries compiled with GCC only.")
		SetStatus(IDA_STATUS_READY)
		return
	
	ea = ScreenEA()
	
	if not isHead(GetFlags(ea)):
		ea = PrevHead(ea)
	
	# Param needed to support old IDAPython versions
	end = NextHead(ea, 4294967295)
	
	name = Demangle(Name(ea), GetLongPrm(INF_LONG_DN))
	if ea == BADADDR or name is None or not re.search(r"vf?table(?: |'\{)for", name):
		Warning("No vtable selected!\nSelect vtable block first.")
		SetStatus(IDA_STATUS_READY)
		return
	
	linux_vtable = []
	temp_windows_vtable = []
	
	# Extract vtable
	while ea < end:
		offset = Dword(ea)
		
		# A vtable starts with some metadata, if it's missing...
		if isCode(GetFlags(offset)):
			Warning("Something went wrong!")
			SetStatus(IDA_STATUS_READY)
			return
		
		# Skip thisoffs and typeinfo address
		ea += 8
		
		while ea < end and isCode(GetFlags(Dword(ea))):
			name = Demangle(Name(Dword(ea)), GetLongPrm(INF_LONG_DN))
			
			if offset == 0:
				linux_vtable.append(name)
				temp_windows_vtable.append(name)
			else:
				# MI entry, strip "`non-virtual thunk to'" and remove from list
				#     But not if it's a dtor... what the hell is this.
				if (name.find("`non-virtual thunk to'") != -1) and name.find("::~") == -1:
					name = name[22:]
					#print "Stripping '%s' from windows vtable." % (name)
					temp_windows_vtable.remove(name)
			
			ea += 4
	
	for i, v in enumerate(temp_windows_vtable):
		if v.find("::~") != -1:
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
			if len(overload_stack) > 0:
				#print overload_stack
				while len(overload_stack) > 0:
					windows_vtable.append(overload_stack.pop())
			
			windows_vtable.append(v)
		
		prev_function = function
		prev_symbol = v
	
	print "Lin Win Function"
	for i, v in enumerate(linux_vtable):
		winindex = windows_vtable.index(v) if v in windows_vtable else None
		if winindex is not None:
			print "%3d %3d %s" % (i, winindex, v)
		else:
			print "%3d     %s" % (i, v)
	
	SetStatus(IDA_STATUS_READY)

if __name__ == '__main__':
	Analyze()