350-Project
===========

**General TODO:**  
_ Fix Response to Scout Rush  
_ Fix Response to flying buildigns  
_ Fix Response to repairing SCVs  
_ Fix Searching for Enemy when Enemy base is destroyed/not found  
_ Fix ghetto "Only attack if mining" stuff that makes my probes not attack an SCV that is attacking us (not mining) and instead attack a new miner and we lose sometimes because of this on small maps vs icebot.

**ScoutRushManager TODO:**  
X Add buildings to attack order
X Return probes to Worker Manager when done  / Scout Map



**Air Rush:**  
X implement hard-coded build order for cannons + 2 stargates  
  _ NICETOHAVE: Try to implement logic to have the stargates not visible.. Right now they are spiral  
  X : Add in air offense upgrade, we can afford it  
X implement defensible cannon layout (currently holds off Ualberta bot zealots, )  
  _ NICETOHAVE: Improve placement further (get rid of spiral even... could probably hardcode initial positions using distance from nexus and borders for relativity)  
X Add dynamic building for scouts
	X Increase stargate count based on expansions
	X Create scouts based on our stargate count
	X Build fleet beacon after first expansion
	X Upgrade thrusters
X Implement AirManager logic + Air Micro
	X Only kite air units 
	X Turn off combat sim because it causes a disconect
_ Implement AirHarassmentCommander
	_ Initial base assault mode (kill probes if we have air superiority)
	_ Maintain air superiority mode (kill gas, cybernetics core, missle turrets, etc..)
		_ Target tech buildings/gas  
		_ OPTIONAL: add in corsairs w/ disruptor webs to kill turrets .. cool idea but a lot of work  
	_ Harassment mode (When air superiority is lost, just do hit and run on resources, like Carpet Bombing)  
		_ Add a fleet beacon and research speed tech  
_ Figure out a way to find proxy bases faster.