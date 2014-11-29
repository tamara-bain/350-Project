350-Project
===========

General TODO:  
_ Fix Response to Scout Rush  
_ Fix Response to flying buildigns  
_ Fix Response to repairing SCVs  
_ Fix Searching for Enemy when Enemy base is destroyed/not found  
_ Fix ghetto "Only attack if mining" stuff that makes my probes not attack an SCV that is attacking us (not mining) and instead attack a new miner and we lose sometimes because of this on small maps vs icebot.

ScoutRushManager TODO:  
_ Take leader code out of loop  
_ Add condition for if leader dies  
_ Add buildings to attack order
_ Return probes to Worker Manager when done  / Scout Map



Air Rush:
X implement hard-coded build order for cannons + 2 stargates
  _ Try to implement logic to have the stargates not visible.. Right now they are spiral
  _ Add in air offense upgrade, we can afford it
X implement defensible cannon layout (currently holds off Ualberta bot zealots, )
  _ Improve placement further (get rid of spiral even... could probably hardcode initial positions using distance from nexus and borders for relativity)
_ Add dynamic building for scouts so it builds those instead of dragoons..
_ Implement Air Commander logic + Air Micro
	_ Initial base assault mode (kill probes if we have air superiority)
	_ Maintain air superiority mode (kill gas, cybernetics core, missle turrets, etc..)
		_ OPTIONAL: add in corsairs w/ disruptor webs to kill turrets .. cool idea but a lot of work
	_ Harassment mode (When air superiority is lost, just do hit and run on resources, like Carpet Bombing)
		_ Add a fleet beacon and research speed tech
