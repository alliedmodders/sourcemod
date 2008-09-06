echo "sdktools.games.ep2.txt"
./vtablecheck ~/srcds/orangebox/tf/bin/server_i486.so _ZTV9CTFPlayer RemovePlayerItem
./vtablecheck ~/srcds/orangebox/tf/bin/server_i486.so _ZTV9CTFPlayer Weapon_GetSlot
./vtablecheck ~/srcds/orangebox/tf/bin/server_i486.so _ZTV9CTFPlayer Ignite
./vtablecheck ~/srcds/orangebox/tf/bin/server_i486.so _ZTV9CTFPlayer Extinguish
./vtablecheck ~/srcds/orangebox/tf/bin/server_i486.so _ZTV9CTFPlayer Teleport
./vtablecheck ~/srcds/orangebox/tf/bin/server_i486.so _ZTV9CTFPlayer CommitSuicide "(bool, bool)"
./vtablecheck ~/srcds/orangebox/tf/bin/server_i486.so _ZTV9CTFPlayer GetVelocity
./vtablecheck ~/srcds/orangebox/tf/bin/server_i486.so _ZTV9CTFPlayer EyeAngles
./vtablecheck ~/srcds/orangebox/tf/bin/server_i486.so _ZTV9CTFPlayer KeyValue "(char const*, char const*)"
./vtablecheck ~/srcds/orangebox/tf/bin/server_i486.so _ZTV9CTFPlayer KeyValue "(char const*, float)"
./vtablecheck ~/srcds/orangebox/tf/bin/server_i486.so _ZTV9CTFPlayer KeyValue "(char const*, Vector const&)"
./vtablecheck ~/srcds/orangebox/tf/bin/server_i486.so _ZTV9CTFPlayer SetModel
./vtablecheck ~/srcds/orangebox/tf/bin/server_i486.so _ZTV9CTFPlayer AcceptInput
./vtablecheck ~/srcds/orangebox/tf/bin/server_i486.so _ZTV9CTFPlayer Activate
echo "sm-tf2.games.txt"
./vtablecheck ~/srcds/orangebox/tf/bin/server_i486.so _ZTV9CTFPlayer ForceRespawn
