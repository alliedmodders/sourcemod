@types =
	int: 0
	float: 1
	bool: 2
	boolean: 2

@ref = (val,size) ->
	if size?
		value: val
		size: size
	else
		value: val

@toFloat = (val) ->
	forcefloat: true
	value: val
