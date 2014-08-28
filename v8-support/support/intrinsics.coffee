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



util = require 'sourcemod/util'

class ForwardListener
	constructor: (@manager, @eventdef) ->
		forwards[@eventdef] = @onEvent

	onEvent: =>
		@manager._raise @eventdef, arguments

util.code.mixin @plugin, util.eventmanager.PermanentListenerBasedEventManager
@plugin.ListenerType = ForwardListener
@plugin.info =
	name: "?????"
	description: "God knows what this does..."
	author: "Shitty programmer who can't write plugin info"
	version: "v0.0.0.0.0.0.0.1aa"
	url: "http://fillinyourplugininfo.dude"

