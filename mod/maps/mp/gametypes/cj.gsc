#include maps\mp\gametypes\_hud_util;
#include maps\mp\_utility;

init()
{
	setDvar("scr_tdm_timelimit", 0);
	setDvar("scr_tdm_scorelimit", 0);

	setDvar("player_sprintUnlimited", 1);
	setDvar("jump_slowdownEnable", 0);

	setDvar("bg_fallDamageMaxHeight", 9999);
	setDvar("bg_fallDamageMinHeight", 9998);

	setDvar("player_bayonetLaunchProof", 0); // Allow bayonet launches and bounces

	setDvar("scr_showperksonspawn", 0); // Remove perks icons shown on spawn
	setDvar("scr_game_hardpoints", 0);	// Remove killstreaks

	level thread delete_death_barriers();

	level thread onPlayerConnect();
}

onPlayerConnect()
{
	for (;;)
	{
		level waittill("connecting", player);

		// Don't setup bots
		if (isDefined(player.pers["isBot"]))
			continue;

		player init_client_dvars();
		player init_player_once();

		player thread onPlayerSpawned();
	}
}

onPlayerSpawned()
{
	self endon("disconnect");

	for (;;)
	{
		self waittill("spawned_player");

		self thread watch_player_buttons();
		self thread replenish_ammo();
		self toggle_godmode();

		// Remove all grenades
		self TakeWeapon("frag_grenade_mp");
		self TakeWeapon("m8_white_smoke_mp");
		self TakeWeapon("sticky_grenade_mp");
		self TakeWeapon("tabun_gas_mp");
		self TakeWeapon("molotov_mp");

		self setClientDvar("cg_drawCrosshair", 0);
		self setClientDvar("ui_hud_hardcore", 1);	   // Hardcore HUD
		self setClientDvar("g_compassShowEnemies", 0); // Hide compass from HUD

		self setclientdvar("player_view_pitch_down", 70);

		self toggle_meter_hud();
	}
}

init_player_once()
{
	if (isdefined(self.init_player))
		return;

	self.cj = [];

	self.init_player = true;
}

init_client_dvars()
{
	self setClientDvar("loc_warnings", 0); // Disable unlocalized text warnings

	self setClientDvar("cg_drawSpectatorMessages", 0);
	self setClientDvar("cg_descriptiveText", 0);		  // Remove spectator button icons and text
	self setClientDvar("player_spectateSpeedScale", 1.5); // Faster movement in spectator

	self setclientdvar("player_view_pitch_up", 89.9); // Allow looking straight up
	self setClientDvars("fx_enable", 0);			  // Disable FX (RPG smoke etc)

	self setClientDvar("aim_automelee_range", 0); // Remove melee lunge on enemy players
	// Remove aim assist on enemy players
	self setClientDvar("aim_slowdown_enabled", 0);
	self setClientDvar("aim_lockon_enabled", 0);

	// Remove overhead names and ranks on enemy players
	self setClientDvar("cg_overheadNamesSize", 0);
	self setClientDvar("cg_overheadRankSize", 0);

	self setClientDvar("cg_drawCrosshairNames", 0);
}

/**
 * Check if a button is pressed.
 */
button_pressed(button)
{
	switch (ToLower(button))
	{
	case "ads":
		return self adsbuttonpressed();
	case "attack":
		return self attackbuttonpressed();
	case "frag":
		return self fragbuttonpressed();
	case "melee":
		return self meleebuttonpressed();
	case "smoke":
		return self secondaryoffhandbuttonpressed();
	case "use":
		return self usebuttonpressed();
	default:
		self iprintln("^1Unknown button " + button);
		return false;
	}
}

/**
 * Check if a button is pressed twice within 500ms.
 */
button_pressed_twice(button)
{
	if (self button_pressed(button))
	{
		// Wait for the button to be released after the first press
		while (self button_pressed(button))
		{
			wait 0.05;
		}

		// Now, wait for a second press within 500ms
		for (elapsed_time = 0; elapsed_time < 0.5; elapsed_time += 0.05)
		{
			if (self button_pressed(button))
			{
				// Ensure it was released before this second press
				return true;
			}

			wait 0.05;
		}
	}
	return false;
}

toggle_godmode()
{
	gentity_base_address = 2193281456;
	gentity_size = 816;

	gentity_address = gentity_base_address + (self GetEntityNumber() * gentity_size);
	gentity_flags_address = gentity_address + 436;

	gentity_flags = ReadInt(gentity_flags_address);
	WriteInt(gentity_flags_address, gentity_flags ^ 1); // set the 0x1 bit for godmode

	if ((ReadInt(gentity_flags_address) & 1) != 0)
	{
		self iprintln("godmode ON");
	}
	else
	{
		self iprintln("godmode OFF");
	}
}

toggle_noclip()
{
	gentity_base_address = 2193281456;
	gentity_size = 816;

	gentity_address = gentity_base_address + (self GetEntityNumber() * gentity_size);
	client_address = ReadInt(gentity_address + 388);
	client_noclip_address = client_address + 15132;

	if (ReadInt(client_noclip_address) == 0)
	{
		WriteInt(client_noclip_address, 1);
		self iprintln("noclip ON");
	}
	else
	{
		WriteInt(client_noclip_address, 0);
		self iprintln("noclip OFF");
	}
}

toggle_ufo()
{
	gentity_base_address = 2193281456;
	gentity_size = 816;

	gentity_address = gentity_base_address + (self GetEntityNumber() * gentity_size);
	client_address = ReadInt(gentity_address + 388);
	client_ufo_address = client_address + 15136;

	if (ReadInt(client_ufo_address) == 0)
	{
		WriteInt(client_ufo_address, 1);
		self.ufo = true;
		self setclientdvar("player_view_pitch_down", 89.9); // Allow looking straight down
		self iprintln("ufomode ON");
	}
	else
	{
		WriteInt(client_ufo_address, 0);
		self.ufo = false;
		self setclientdvar("player_view_pitch_down", 70); // Reset pitch
		self iprintln("ufomode OFF");
	}
}

watch_player_buttons()
{
	self endon("disconnect");
	self endon("death");

	for (;;)
	{
		if (self button_pressed("frag"))
		{
			self toggle_ufo();
			wait 0.2;
		}
		if (self button_pressed_twice("melee"))
		{
			self thread save_position(0);
			wait 0.2;
		}
		if (self button_pressed("smoke"))
		{
			self thread load_position(0);
			wait 0.2;
		}
		// Host only
		if (self button_pressed("use") && self GetEntityNumber() == 0 && self.ufo == true)
		{
			DisablePlayerClipForBrushesContainingPoint(self.origin);
			wait 0.2;
		}
		wait 0.05;
	}
}

/**
 * Constantly replace the players ammo.
 */
replenish_ammo()
{
	self endon("disconnect");
	self endon("death");

	for (;;)
	{
		currentWeapon = self getCurrentWeapon(); // undefined if the player is mantling or on a ladder
		if (isdefined(currentWeapon))
			self giveMaxAmmo(currentWeapon);
		wait 1;
	}
}

toggle_meter_hud()
{
	if (!isdefined(self.cj["meter_hud"]))
		self.cj["meter_hud"] = [];

	// not defined means OFF
	if (!isdefined(self.cj["meter_hud"]["speed"]))
	{
		self.cj["meter_hud"] = [];
		self thread start_hud_speed();
		self thread start_hud_z_origin();
	}
	else
	{
		self notify("end_hud_speed");
		self notify("end_hud_z_origin");

		self.cj["meter_hud"]["speed"] destroy();
		self.cj["meter_hud"]["z_origin"] destroy();
	}
}

start_hud_speed()
{
	self endon("disconnect");
	self endon("end_hud_speed");

	fontScale = 1.4;
	x = 62;
	y = 22;
	alpha = 0.4;

	self.cj["meter_hud"]["speed"] = createFontString("small", fontScale);
	self.cj["meter_hud"]["speed"] setPoint("BOTTOMRIGHT", "BOTTOMRIGHT", x, y);
	self.cj["meter_hud"]["speed"].alpha = alpha;
	self.cj["meter_hud"]["speed"].label = &"speed:&&1";

	for (;;)
	{
		velocity3D = self getVelocity();
		horizontalSpeed2D = int(sqrt(velocity3D[0] * velocity3D[0] + velocity3D[1] * velocity3D[1]));
		self.cj["meter_hud"]["speed"] setValue(horizontalSpeed2D);

		wait 0.05;
	}
}

start_hud_z_origin()
{
	self endon("disconnect");
	self endon("end_hud_z_origin");

	fontScale = 1.4;
	x = 62;
	y = 36;
	alpha = 0.4;

	self.cj["meter_hud"]["z_origin"] = createFontString("small", fontScale);
	self.cj["meter_hud"]["z_origin"] setPoint("BOTTOMRIGHT", "BOTTOMRIGHT", x, y);
	self.cj["meter_hud"]["z_origin"].alpha = alpha;
	self.cj["meter_hud"]["z_origin"].label = &"z:&&1";

	for (;;)
	{
		self.cj["meter_hud"]["z_origin"] setValue(self.origin[2]);

		wait 0.05;
	}
}

save_position(i)
{
	if (!self isonground())
		return;

	save = spawnstruct();
	save.origin = self.origin;
	save.angles = self getplayerangles();

	self.cj["saves"][i] = save;
}

load_position(i)
{
	self freezecontrols(true);
	wait 0.05;

	save = self.cj["saves"][i];

	self setplayerangles(save.angles);
	self setorigin(save.origin);

	wait 0.05;
	self freezecontrols(false);
}

delete_death_barriers()
{
	// Wait for the game to start otherwise the entities won't be spawned
	level waittill("connected");

	ents = getEntArray("trigger_multiple", "classname");
	for (i = 0; i < ents.size; i++)
	{
		ents[i] delete ();
	}

	ents = getEntArray("trigger_radius", "classname");
	for (i = 0; i < ents.size; i++)
	{
		ents[i] delete ();
	}
}
