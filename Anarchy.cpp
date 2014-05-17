/*
=========================================================
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE 
PROPERTY OF OUTRAGE ENTERTAINMENT, INC. 
('OUTRAGE').  OUTRAGE, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND 
CONDITIONS HEREIN, GRANTS A ROYALTY-FREE, 
PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY 
SUCH END-USERS IN USING, DISPLAYING,  AND 
CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-
COMMERCIAL, ROYALTY OR REVENUE FREE PURPOSES. 
IN NO EVENT SHALL THE END-USER USE THE 
COMPUTER CODE CONTAINED HEREIN FOR REVENUE-
BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE 
SAME BY USE OF THIS FILE.
COPYRIGHT 1999 OUTRAGE ENTERTAINMENT, INC.  ALL 
RIGHTS RESERVED.
=========================================================
*/

/*
* $Logfile: /DescentIII/Main/anarchy/anarchy.cpp $
* $Revision: 80 $
* $Date: 5/23/99 3:04a $
* $Author: Jason $
*
* <insert description of file here>
*
* $Log: /DescentIII/Main/anarchy/anarchy.cpp $
 * 
 * 80    5/23/99 3:04a Jason
 * fixed bug with player rankings not being updated correctly
 * 
 * 79    5/12/99 11:28a Jeff
 * added sourcesafe comment block
*
* $NoKeywords: $
*/



#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "idmfc.h"		// Needed for DMFC
#include "Anarchy.h"	// Some prototypes
#include "anarchystr.h"	// String table header

#ifndef WIN32
	#include <sys/time.h>
#endif

#include <limits.h>
#include <time.h>

#ifdef WIN32
	#include <windows.h>
#endif

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
	#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
	#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

#ifndef max
#define max(a,b)	    (((a) > (b)) ? (a) : (b))
#endif

// Anarchy HUD Modes
#define AHD_NONE		0	//No HUD
#define AHD_SCORE		1	//Show player score
#define AHD_EFFICIENCY	2	//Show player efficiency

// Anarchy HUD Color Modes
#define ACM_PLAYERCOLOR	0	//Use the color of their ship
#define ACM_NORMAL		1	//Use green

//Grim's stuff
object *dObjects;
player *dPlayers;
player_record *dPr;
//texture *dTextures;


#define ADD_SCORE		0
#define CHECK_VERSION	1
#define SPID_CHANGEW	2
#define SPID_CHANGEWREQUEST 3
#define SPID_GAMESTATE 4
#define SPID_RECEIVESHOT 6
#define SPID_RECEIVECHEAT 7
int version_num = 22;

unsigned long shipShootIntervals[32];
unsigned long playerLastShots[32];

int IRNumOfTeams;
int TeamScores[DLLMAX_TEAMS];		//teams scores
int OverallTeamScores[DLLMAX_TEAMS];	//overall scores per team
int SortedTeams[DLLMAX_TEAMS];		//sorted team scores
short SndSav = (-1);
int MassIndex = (-1);
int RedIndex = (-1); 
int BlueIndex = (-1);
int GreenIndex = (-1); 
int YellowIndex = (-1); 

int sndCheaterHandle = -1;

bool first_shot = true;
float save_reload = 0;
/*
Our DMFC Interface.  All Interaction with DMFC is via this
*/
IDMFC *DMFCBase = NULL;

/*
Our DMFC Stats Interface (F7 Stats screen manager).  All interaction
with the stats is via this
*/
IDmfcStats *dstat = NULL;

/*
SortedPlayers - An array of indicies into the Player records.  This
gets sorted every frame and is used throughout the mod (for instance
when displaying the scores on the HUD.  SortedPlayers[0] = the player record
value of the player in first place.
*/
int SortedPlayers[MAX_PLAYER_RECORDS];	//sorted player nums

/*
DisplayScoreScreen - true if the F7 Stats screen is up
*/
bool DisplayScoreScreen;

/*
Highlight_bmp - Bitmap handle for the highlight bar used on the HUD when
displaying the player names, to highlight your name
*/
int Highlight_bmp = -1;

/*
Anarchy_hud_display - What should we display on the HUD? Nothing? Player
scores? Player efficiencies?  This is changed in the F6 On-Screen Menu
*/
ubyte Anarchy_hud_display = AHD_SCORE;

/*
HUD_color_model - What color mode is the HUD in? Should the players be displayed
using the color of their ship?  Or just green.  This is changed in the F6 
On-Screen menu
*/
ubyte HUD_color_model = ACM_PLAYERCOLOR; 

/*
display_my_welcom - Takes the value of false until we display the "Welcome" message
when you join/start an Anarchy game.  HUD messages can't be displayed until the
first frame of the game has displayed, so we wait until then to display the welcome
message.
*/
bool display_my_welcome = false;

// HUD callback function when it is time to render our HUD
void DisplayHUDScores(struct tHUDItem *hitem);

// Displays a welcome message for a joining client
void DisplayWelcomeMessage(int player_num);

// Called with a filename to save the current state of the game
// stats to file.
void SaveStatsToFile(char *filename);

// Switches the HUD Color mode to a new value
void SwitchHUDColor(int i);
// SwitchAnarchyScores
//
//	Call this function to switch the HUD display mode for what
//  we display on the HUD (Player Scores/Player Efficiency/Nothing)
void SwitchAnarchyScores(int i);

///////////////////////////////////////////////
//localization info (string table functions and variables)
//
// String tables are only needed if you plan on making a multi-lingual mod.
char **StringTable;
int StringTableSize = 0;
char **D3StringTable;
int D3StringTableSize = 0;
char *_ErrorString = "Missing String";
char *GetString(int d){if( (d<0) || (d>=StringTableSize) ) return _ErrorString; else return StringTable[d];}
char *GetD3String(int d){if( (d<0) || (d>=D3StringTableSize) ) return _ErrorString; else return D3StringTable[d];}
///////////////////////////////////////////////

#ifdef MACINTOSH
#pragma export on
#endif

#ifdef WIN32

struct timezone
{
  int tz_minuteswest;
  int tz_dsttime;
};

int gettimeofday (struct timeval *tv, struct timezone *tz)
{
	FILETIME ft;
	unsigned __int64 tmpres = 0;
	static int tzflag = 0;

	if (NULL != tv)
	{
		GetSystemTimeAsFileTime (&ft);
		tmpres |= ft.dwHighDateTime;
		tmpres <<= 32;
		tmpres |= ft.dwLowDateTime;
		tmpres /= 10;
		tmpres -= DELTA_EPOCH_IN_MICROSECS;
		tv->tv_sec = (long)(tmpres / 1000000UL);
		tv->tv_usec = (long)(tmpres % 1000000UL);
	}

	if (NULL != tz)
	{
		if (!tzflag)
		{
			_tzset ();
			tzflag++;
		}

		tz->tz_minuteswest = _timezone / 60;
		tz->tz_dsttime = _daylight;
	}

	return 0;
}
#endif

void SendPlayerShots(int pnum)
{
	ubyte uData[MAX_GAME_DATA_SIZE];
	int iCount = 0;
	int iShots;
	
	effcnt *stat;
	stat = (effcnt *)DMFCBase->GetPlayerRecordData(pnum);
	if(stat)
		iShots = stat->shots;

	DMFCBase->StartPacket(uData, SPID_RECEIVESHOT, &iCount); 
	int iSaved = iCount;
	
	MultiAddInt(pnum, uData, &iCount); 
	MultiAddInt(iShots, uData, &iCount);

	DMFCBase->SendPacket(uData, iCount, SP_ALL); 	

}

void ReceivePlayerShots(ubyte *data)
{
	int iCount = 0;
	int iPnum = 0;
	int iShots = 0;

	iPnum = MultiGetInt(data, &iCount); 
	iShots = MultiGetInt(data, &iCount);
	
	effcnt *stat;
	stat = (effcnt *)DMFCBase->GetPlayerRecordData(iPnum);
	if(stat)
		stat->shots = iShots;

}

void SendCheatMessage(int pnum)
{
	ubyte uData[MAX_GAME_DATA_SIZE];
	int iCount = 0;
	
	DMFCBase->StartPacket(uData, SPID_RECEIVECHEAT, &iCount); 
	int iSaved = iCount;
	
	MultiAddInt(pnum, uData, &iCount); 

	DMFCBase->SendPacket(uData, iCount, SP_ALL); 
}

void ReceiveCheatMessage(ubyte *data)
{
	int iCount = 0;
	int iPnum = 0;

	iPnum = MultiGetInt(data, &iCount);
	
	player *plrs = &(DMFCBase->GetPlayers())[iPnum];
	DLLAddHUDMessage("%s was caught cheating or experiences severe lags - kicking...", plrs->callsign);
	DLLPlay2dSound(sndCheaterHandle);
}

void OnPlayerConnect(int player_num)
{
	DMFCBase->OnPlayerConnect(player_num);

	effcnt *stat;
	stat = (effcnt *)DMFCBase->GetPlayerRecordData(player_num);
	if(stat){
		stat->shots = 0;
	}
}

int pack_pstat(effcnt *user_info,ubyte *data)
{
	int count = 0;
	MultiAddInt(user_info->shots,data,&count);
	return count;
}

int unpack_pstat(effcnt *user_info,ubyte *data)
{
	int count = 0;
	user_info->shots = MultiGetInt(data,&count);
	return count;
}

void ChangeNumberOfTeams(int newsize)
{
	IRNumOfTeams = newsize;
	DMFCBase->SetNumberOfTeams(IRNumOfTeams);
}

void OnWeaponFired(object *weapon_obj,object *shooter)
{
	if (DMFCBase->GetLocalRole() == LR_SERVER && weapon_obj->id != FLARE_INDEX && weapon_obj->id != 108) // 108 --> black pyro flare
	{
		struct timeval now;
		gettimeofday(&now, NULL);

		unsigned long recentShot = (now.tv_sec * (unsigned int)1e6 + now.tv_usec);
		unsigned long difference;

		// workaround for time running over the buffer of ULONG_MAX and starting at 0 again
		if (recentShot < playerLastShots[shooter->id])
			difference = ULONG_MAX - playerLastShots[shooter->id] + recentShot;
		else
			difference = recentShot - playerLastShots[shooter->id];

		if (difference < shipShootIntervals[shooter->id] || (weapon_obj->id != MASSDRIVER_INDEX && weapon_obj->id != LASER_INDEX))
		{
			player *plrs = &(DMFCBase->GetPlayers())[shooter->id];
			SendCheatMessage(shooter->id);
			DLLAddHUDMessage("%s was caught cheating or experiences severe lagging - kicking...", plrs->callsign);
			DLLPlay2dSound(sndCheaterHandle);
			DLLMultiDisconnectPlayer(shooter->id);
		}
		else
			playerLastShots[shooter->id] = recentShot;
	}

	if(DMFCBase->GetPlayerNum() == shooter->id && weapon_obj->id == MASSDRIVER_INDEX)
	{
		if(first_shot)
		{
			player *player = &(DMFCBase->GetPlayers())[shooter->id];
			ship *ship = &(DMFCBase->GetShips())[player->ship_index];

			ship->static_wb[6].gp_fire_wait[0] = save_reload;
			
			first_shot = false;
		}
	}

	if(weapon_obj->id == MassIndex)
	{
		effcnt *stat;
		stat = (effcnt *)DMFCBase->GetPlayerRecordData(shooter->id);
		if(stat)
			stat->shots++;
		switch (DMFCBase->GetPlayerTeam(shooter->id))
		{
		case 0:
			DLLCreateAndFireWeapon(&weapon_obj->pos, &shooter->orient.fvec, shooter, RedIndex);
			DLLCreateAndFireWeapon(&weapon_obj->pos, &shooter->orient.fvec, shooter, RedIndex);
			break;
		case 1:
			DLLCreateAndFireWeapon(&weapon_obj->pos, &shooter->orient.fvec, shooter, BlueIndex);
			DLLCreateAndFireWeapon(&weapon_obj->pos, &shooter->orient.fvec, shooter, BlueIndex);
			break;
		case 2:
			DLLCreateAndFireWeapon(&weapon_obj->pos, &shooter->orient.fvec, shooter, GreenIndex);
			DLLCreateAndFireWeapon(&weapon_obj->pos, &shooter->orient.fvec, shooter, GreenIndex);
			break;
		case 3:
			DLLCreateAndFireWeapon(&weapon_obj->pos, &shooter->orient.fvec, shooter, YellowIndex);
			DLLCreateAndFireWeapon(&weapon_obj->pos, &shooter->orient.fvec, shooter, YellowIndex);
			break;
		}
	}
	if(DMFCBase->GetLocalRole() == LR_SERVER)
		SendPlayerShots(shooter->id);
	

	DMFCBase->OnWeaponFired(weapon_obj, shooter);
}

void OnPlayerChangeTeam(int player_num,int newteam,bool announce,bool spew_onrespawn)
{
	DMFCBase->OnPlayerChangeTeam(player_num,newteam,announce,spew_onrespawn);
	AddWeaponToPlayer(player_num,MASSDRIVER_INDEX,15);
	RemWeaponFromPlayer(player_num,LASER_INDEX);
	ChangeWeapon(player_num,MASSDRIVER_INDEX);
	RemWeaponFromPlayer(player_num,CONCUSSION_INDEX);
	DLLInvAddTypeID(player_num,OBJ_POWERUP,DLLFindObjectIDName("Afterburner"),-1,-1,0,GetD3String(809));

	player *player = &(DMFCBase->GetPlayers())[player_num];
	ship *ship = &(DMFCBase->GetShips())[player->ship_index];

	ship->static_wb[6].gp_fire_wait[0] = 0.01f;
	first_shot = true;

	//player_record *pr = DMFCBase->GetPlayerRecordByPnum(player_num);
}

void ReceiveGameState(ubyte *data)
{
	int count = 0;
	int i;
	
	IRNumOfTeams = MultiGetInt(data, &count);
	for(i = 0; i<IRNumOfTeams; i++)
	{
		TeamScores[i] = MultiGetInt(data, &count);
	}
	ChangeNumberOfTeams(IRNumOfTeams);
}

void SendGameState(int pnum)
{
	ubyte data[MAX_GAME_DATA_SIZE];
	int count = 0;
	int i;

	DMFCBase->StartPacket(data,SPID_GAMESTATE,&count);
	MultiAddInt(IRNumOfTeams, data, &count);
	//pack score in this game
	for(i = 0; i<IRNumOfTeams; i++)
	{
		MultiAddInt(TeamScores[i],data,&count);
	}
	
	DMFCBase->SendPacket(data,count,pnum);
}


void TeamScoreCallback(int team,char *buffer,int buffer_size)
{
	ASSERT(team>=0 && team<DLLMAX_TEAMS);

	sprintf(buffer," %d",TeamScores[team]);
}


void SendChangeWeaponsRequest(int iPnum)
{
	ubyte uData[MAX_GAME_DATA_SIZE];
	int iCount = 0;
	
	DMFCBase->StartPacket(uData, SPID_CHANGEWREQUEST, &iCount);
	int iSaved = iCount;

	MultiAddInt(iPnum, uData, &iCount);

	if (DMFCBase->GetLocalRole() == LR_SERVER)
		ReceiveSendChangeWeaponsRequest(&uData[iSaved]);
	else
		DMFCBase->SendPacket(uData, iCount, SP_SERVER);
	

}

void ReceiveSendChangeWeaponsRequest(ubyte *uData)
{
	int iCount = 0;
	int iPnum = 0;

	iPnum = MultiGetInt(uData, &iCount);

	int bCount = 0;
	ubyte bData[MAX_GAME_DATA_SIZE];

	DMFCBase->StartPacket(bData, SPID_CHANGEW, &bCount); 
	int bSaved = bCount;
	
	MultiAddInt(iPnum, bData, &bCount);

	DMFCBase->SendPacket(bData, bCount, SP_ALL); 

	ReceiveChangeWeapons(&bData[bSaved]);
}


void ReceiveChangeWeapons(ubyte *uData)
{
	int iCount = 0;
	int iPnum = 0;

	iPnum = MultiGetInt(uData, &iCount);

	AddWeaponToPlayer(iPnum,MASSDRIVER_INDEX,15);
	RemWeaponFromPlayer(iPnum,LASER_INDEX);
	ChangeWeapon(iPnum,MASSDRIVER_INDEX);
	RemWeaponFromPlayer(iPnum,CONCUSSION_INDEX);
	DLLInvAddTypeID(iPnum,OBJ_POWERUP,DLLFindObjectIDName("Afterburner"),-1,-1,0,GetD3String(809));
}


void CheckVersionNumber(ubyte *data)
{
	int count = 0;
	int ver = MultiGetInt(data,&count);
	if(ver != version_num)
		DMFCBase->DisconnectMe();
	
}

void GetVersionNumber(int pnum)
{
	int count = 0;
	ubyte data[MAX_GAME_DATA_SIZE];
	DMFCBase->StartPacket(data,CHECK_VERSION,&count);
	MultiAddInt(version_num,data,&count);
	DMFCBase->SendPacket(data,count,pnum);
}

void OnServerPlayerKilled(object *killer_obj,int victim_pnum)
{
	DMFCBase->OnServerPlayerKilled(killer_obj, victim_pnum);
}

void AddScoreToPlayer(ubyte *data)
{
	int count = 0;
	int pnum = MultiGetInt(data,&count);
	player_record *pr = DMFCBase->GetPlayerRecordByPnum(pnum);
	pr->dstats.kills[DSTAT_LEVEL]++;
	
}

void OnServerPlayerEntersGame(int player_num)
{
	GetVersionNumber(player_num);
/*	players[currentslot].player_num = player_num;
	currentslot++;*/
	
	DMFCBase->OnServerPlayerEntersGame(player_num);
	
	SendGameState(player_num);
}

void OnServerPlayerDisconnect(int player_num)
{

	DMFCBase->OnServerPlayerDisconnect(player_num);
}


void OnServerCollide(object *me_obj,object *it_obj)
{
	DMFCBase->OnServerCollide(me_obj,it_obj);
}

int ChangeWeapon(int slot, int weap_index)
{
	player *player = &(DMFCBase->GetPlayers())[slot];
	ship *ship = &(DMFCBase->GetShips())[player->ship_index];
	//player->weapon[PRIMARY_INDEX].index = weap_index;
	return 1;
}


int RemWeaponFromPlayer(int slot,int weap_index)
{
	// Get the player and ship pointers for this player
	player *player = &(DMFCBase->GetPlayers())[slot];
	ship *ship = &(DMFCBase->GetShips())[player->ship_index];

	// "Take" the weapon from the player
	player->weapon_flags &= ~(HAS_FLAG(weap_index));

	otype_wb_info *wb = &ship->static_wb[weap_index];

	ASSERT(wb != NULL);

	//if secondary or primary that uses ammo, then remove the ammo
	if((weap_index >= SECONDARY_INDEX) || wb->ammo_usage)
	{
		//figure out much ammo to remove
		int removed = player->weapon_ammo[weap_index];

		//now add it
		player->weapon_ammo[weap_index] = player->weapon_ammo[weap_index] - (ushort)removed;
	}		
	return 1;
}
int AddWeaponToPlayer(int slot,int weap_index, int ammo)
{

	// Get the player and ship pointers for this player
	player *player = &(DMFCBase->GetPlayers())[slot];
	ship *ship = &(DMFCBase->GetShips())[player->ship_index];

	// "Give" the weapon to the player
	player->weapon_flags |= HAS_FLAG(weap_index);	

	// get the weapon battery information for the given weapon
	// it is ship dependent, since each ship can hold different
	// amounts of ammo
	otype_wb_info *wb = &ship->static_wb[weap_index];

	ASSERT(wb != NULL);

	//if secondary or primary that uses ammo, then use the ammo
	if((weap_index >= SECONDARY_INDEX) || wb->ammo_usage)
	{
		//figure out much ammo to add
		int added = ammo;
		//min(ship->max_ammo[weap_index] - player->weapon_ammo[weap_index],ammo);

		//now add it
		player->weapon_ammo[weap_index] += (ushort)added;
	}	

	return 1;
}
void SetupTables()
{
	//Weapons***********
	int index = DLLFindWeaponName("Mass Driver");
	if(index != (-1))
	{
		
		MassIndex = index;
		weapon *weapons = &(DMFCBase->GetWeapons())[index];
		weapons->player_damage = 250;
	}
	index = DLLFindWeaponName("Laser Level 1 - Red");
	if(index != (-1))
	{
		RedIndex = index;
		weapon *weapons = &(DMFCBase->GetWeapons())[index];
		SndSav = weapons->sounds[1];
		weapons->sounds[1] = (-1);
		weapons->phys_info.velocity.z = 2000.0f;
		weapons->flags |= WF_SMOKE;
		weapons->flags |= WF_PLANAR_SMOKE;
		weapons->player_damage = 0;	
		weapons->generic_damage = 0;			
		weapons->explode_image_handle = (-1);
		weapons->smoke_handle = DLLFindTextureName("rsa");
		weapons->impact_player_damage = 0;
		weapons->impact_generic_damage = 0;
		weapons->impact_force = 0;
		weapons->life_time = 1.0f;
	}
	index = DLLFindWeaponName("Laser Level 2 - Blue");
	if(index != (-1))
	{
		BlueIndex = index;
		weapon *weapons = &(DMFCBase->GetWeapons())[index];
		weapons->sounds[1] = (-1);
		weapons->phys_info.velocity.z = 2000.0f;
		weapons->flags |= WF_SMOKE;
		weapons->flags |= WF_PLANAR_SMOKE;
		weapons->player_damage = 0;	
		weapons->generic_damage = 0;			
		weapons->explode_image_handle = (-1);
		weapons->smoke_handle = DLLFindTextureName("rsa");
		weapons->impact_player_damage = 0;
		weapons->impact_generic_damage = 0;
		weapons->impact_force = 0;
		weapons->life_time = 1.0f;
	}
	index = DLLFindWeaponName("Laser Level 4 - Green");
	if(index != (-1))
	{
		GreenIndex = index;
		weapon *weapons = &(DMFCBase->GetWeapons())[index];
		weapons->sounds[1] = (-1);
		weapons->phys_info.velocity.z = 2000.0f;
		weapons->flags |= WF_SMOKE;
		weapons->flags |= WF_PLANAR_SMOKE;
		weapons->player_damage = 0;	
		weapons->generic_damage = 0;			
		weapons->explode_image_handle = (-1);
		weapons->smoke_handle = DLLFindTextureName("rsa");
		weapons->impact_player_damage = 0;
		weapons->impact_generic_damage = 0;
		weapons->impact_force = 0;
		weapons->life_time = 1.0f;
	}
	index = DLLFindWeaponName("Laser Level 5 -Yellow");
	if(index != (-1))
	{
		YellowIndex = index;
		weapon *weapons = &(DMFCBase->GetWeapons())[index];
		weapons->sounds[1] = (-1);
		weapons->phys_info.velocity.z = 2000.0f;
		weapons->flags |= WF_SMOKE;
		weapons->flags |= WF_PLANAR_SMOKE;
		weapons->player_damage = 0;	
		weapons->generic_damage = 0;			
		weapons->explode_image_handle = (-1);
		weapons->smoke_handle = DLLFindTextureName("rsa");
		weapons->impact_player_damage = 0;
		weapons->impact_generic_damage = 0;
		weapons->impact_force = 0;
		weapons->life_time = 1.0f;
	}
	//Ships***********
	index = DLLFindShipName("Pyro-GL");
	if(index != (-1))
	{
		ship *ships = &(DMFCBase->GetShips())[index];
		ships->static_wb[6].ammo_usage = 0;
		ships->static_wb[0].ammo_usage = 1;
	}
	index = DLLFindShipName("Phoenix");
	if(index != (-1))
	{
		ship *ships = &(DMFCBase->GetShips())[index];
		ships->static_wb[6].ammo_usage = 0;
		ships->static_wb[0].ammo_usage = 1;
	}
	index = DLLFindShipName("Magnum-AHT");
	if(index != (-1))
	{
		ship *ships = &(DMFCBase->GetShips())[index];
		ships->static_wb[6].ammo_usage = 0;
		ships->static_wb[0].ammo_usage = 1;
	}
	index = DLLFindShipName("Black Pyro");
	if(index != (-1))
	{
		ship *ships = &(DMFCBase->GetShips())[index];
		ships->static_wb[6].ammo_usage = 0;
		ships->static_wb[0].ammo_usage = 1;
	}
	
	//Objects ***********
	object_info *obinfo;
	for(int inc = 0; inc <= MAX_OBJECT_IDS; inc++)
	{
		if(obinfo = DMFCBase->GetObjectInfo(inc))
		{
			if(obinfo->type == OBJ_POWERUP)
			{
				if(strcmp(obinfo->name, "energy"))
					obinfo->multi_allowed = false;
			}
		}
	}; //boo

}

void CleanupTables()
{
	//Weapons***************
	int index = DLLFindWeaponName("Mass Driver");
	if(index != (-1))
	{
		weapon *weapons = &(DMFCBase->GetWeapons())[index];
		weapons->player_damage = 40;
	}
	index = DLLFindWeaponName("Laser Level 1 - Red");
	if(index != (-1))
	{
		RedIndex = index;
		weapon *weapons = &(DMFCBase->GetWeapons())[index];
		weapons->sounds[1] = SndSav;
		weapons->phys_info.velocity.z = 200.0f;
		weapons->flags &= ~WF_SMOKE;
		weapons->flags &= ~WF_PLANAR_SMOKE;
		weapons->player_damage = 6.750f;	
		weapons->generic_damage = 6.750f;			
		weapons->explode_image_handle = DLLFindTextureName("RedLaserHit");
		weapons->smoke_handle = (-1);
		weapons->impact_player_damage = 6.750f;
		weapons->impact_generic_damage = 6.750f;
		weapons->impact_force = 675.0f;
		weapons->life_time = 5.0f;
	}
	index = DLLFindWeaponName("Laser Level 2 - Blue");
	if(index != (-1))
	{
		BlueIndex = index;
		weapon *weapons = &(DMFCBase->GetWeapons())[index];
		weapons->sounds[1] = SndSav;
		weapons->phys_info.velocity.z = 225.0f;
		weapons->flags &= ~WF_SMOKE;
		weapons->flags &= ~WF_PLANAR_SMOKE;
		weapons->player_damage = 7.5f;	
		weapons->generic_damage = 7.5f;			
		weapons->explode_image_handle = DLLFindTextureName("BlueLaserHit");
		weapons->smoke_handle = (-1);
		weapons->impact_player_damage = 2.0f;
		weapons->impact_generic_damage = 2.0f;
		weapons->impact_force = 750.0f;
		weapons->life_time = 5.0f;
	}
	index = DLLFindWeaponName("Laser Level 4 - Green");
	if(index != (-1))
	{
		GreenIndex = index;
		weapon *weapons = &(DMFCBase->GetWeapons())[index];
		weapons->sounds[1] = SndSav;
		weapons->phys_info.velocity.z = 170.0f;
		weapons->flags &= ~WF_SMOKE;
		weapons->flags &= ~WF_PLANAR_SMOKE;
		weapons->player_damage = 8.25f;	
		weapons->generic_damage = 8.25f;			
		weapons->explode_image_handle = DLLFindTextureName("GreenLaserHit");
		weapons->smoke_handle = (-1);
		weapons->impact_player_damage = 8.25f;
		weapons->impact_generic_damage = 8.25f;
		weapons->impact_force = 825.0f;
		weapons->life_time = 5.0f;
	}
	index = DLLFindWeaponName("Laser Level 5 -Yellow");
	if(index != (-1))
	{
		YellowIndex = index;
		weapon *weapons = &(DMFCBase->GetWeapons())[index];
		weapons->sounds[1] = SndSav;
		weapons->phys_info.velocity.z = 250.0f;
		weapons->flags &= ~WF_SMOKE;
		weapons->flags &= ~WF_PLANAR_SMOKE;
		weapons->player_damage = 9.0f;	
		weapons->generic_damage = 9.0f;			
		weapons->explode_image_handle = DLLFindTextureName("YellowLaserHit");
		weapons->smoke_handle = (-1);
		weapons->impact_player_damage = 2.0f;
		weapons->impact_generic_damage = 2.0f;
		weapons->impact_force = 900.0f;
		weapons->life_time = 5.0f;
	}
	//Ships*************
	index = DLLFindShipName("Pyro-GL");
	if(index != (-1))
	{
		ship *ships = &(DMFCBase->GetShips())[index];
		ships->static_wb[6].ammo_usage = 1;
		ships->static_wb[0].ammo_usage = 0;
	}
	index = DLLFindShipName("Phoenix");
	if(index != (-1))
	{
		ship *ships = &(DMFCBase->GetShips())[index];
		ships->static_wb[6].ammo_usage = 1;
		ships->static_wb[0].ammo_usage = 0;
	}
	index = DLLFindShipName("Magnum-AHT");
	if(index != (-1))
	{
		ship *ships = &(DMFCBase->GetShips())[index];
		ships->static_wb[6].ammo_usage = 1;
		ships->static_wb[0].ammo_usage = 0;
	}
	index = DLLFindShipName("Black Pyro");
	if(index != (-1))
	{
		ship *ships = &(DMFCBase->GetShips())[index];
		ships->static_wb[6].ammo_usage = 1;
		ships->static_wb[0].ammo_usage = 0;
	}	
	//Objects*********
	object_info *obinfo;
	for(int inc = 0; inc <= MAX_OBJECT_IDS; inc++)
	{
		if(obinfo = DMFCBase->GetObjectInfo(inc))
		{
			if(obinfo->type == OBJ_POWERUP)
			{
				obinfo->multi_allowed = true;
			}
		}
	};
	


}
//	DLLGetGameInfo
//
//	This function gets called by the game when it wants to learn some info about 
//	the multiplayer mod.
void DLLFUNCCALL DLLGetGameInfo (tDLLOptions *options)
{
	// Specify what values we will be filling in
	options->flags		= DOF_MAXTEAMS|DOF_MINTEAMS;	//only max teams
	options->max_teams	= 4;			//not a team game, so it's a 1
	options->min_teams	= 2;

	// Mandatory values
	strcpy(options->game_name,"Team - InstaReap");	// The name of our game
	strcpy(options->requirements,"");		// the MSN file KEYWORD requirements
											// there are no requirements for
											// Anarchy, all levels are ok
}

//	DLLGameInit
//
//	Call by the game when the D3M is loaded.  Initialize everything
//	required for the mod here.
//
//	Parms:
//		api_func : List of D3 exported functions and variable pointers
//					You don't have to do anything with this but pass it
//					to DMFC::LoadFunctions();
//		all_ok	: When this function returns, Descent 3 checks the bool this
//					pointer points to.  If it's true (1) then the game continues
//					to load.  If it's false (0) then Descent 3 refuses to load
//					the mod.
//		num_teams_to_use : In the case of team games (2, 3 or 4 teams), this is 
//					the value the user selected for the number of teams.
void DLLFUNCCALL DLLGameInit (int *api_func,ubyte *all_ok,int num_teams_to_use)
{
	// Initialize all_ok to true
	*all_ok = 1;

	// This MUST BE the absolute first thing done.  Create the DMFC interface.
	// CreateDMFC() returns a pointer to an instance of a DMFC class.  If it
	// returns NULL there was some problem creating the DMFC instance and you
	// must bail right away!
	DMFCBase = CreateDMFC();
	if(!DMFCBase)
	{
		//no all is not ok!
		*all_ok = 0;
		return;
	}

	// Since we want an F7 stats screen, we will now try to create an instance
	// of DMFCStats class.  Calling CreateDmfcStats() is just like CreateDMFC()
	// but it creates an instance of DMFCStats.
	dstat = CreateDmfcStats();
	if(!dstat)
	{
		//no all is not ok!
		*all_ok = 0;
		return;
	}

	// We can't call any functions exported from Descent 3 until we initialize
	// them.  Calling DMFC::LoadFunctions() will initialize them.
	DMFCBase->LoadFunctions(api_func);

	// In order to capture events from the game, we must tell DMFC what
	// events we want to get notified about.  The following list of functions
	// call the appropriate DMFC member function to register a callback for
	// an event.  When a callback is registered for an event, as soon as the event
	// happens, the callback you give will be called.  It is your responsibility
	// to call the default event handler of the specific event, DMFC performs
	// many behind-the-scenes work in these default handlers.

	// register for the keypress event (the user pressed a key)
	DMFCBase->Set_OnKeypress(OnKeypress);

	// register for the HUD interval event (gets called once per frame when it
	// is time to render the HUD)
	DMFCBase->Set_OnHUDInterval(OnHUDInterval);

	// register for the interval event (gets called once per frame)
	DMFCBase->Set_OnInterval(OnInterval);

	// register for the client version of the player killed event (gets called
	// on all clients when a player dies)
	DMFCBase->Set_OnClientPlayerKilled(OnClientPlayerKilled);

	// register for the client version of the player joined game event (gets 
	// called on all clients when a player enters the game)
	DMFCBase->Set_OnClientPlayerEntersGame(OnClientPlayerEntersGame);

	// register for the client version of the level start event (gets called
	// on all clients when they are about to start/enter a level)
	DMFCBase->Set_OnClientLevelStart(OnClientLevelStart);

	// register for the client version of the level end event (gets called on
	// all clients when the level ends)
	DMFCBase->Set_OnClientLevelEnd(OnClientLevelEnd);

	// register for the server event, that a game has been created (gets called
	// only once, when the mod is loaded)
	DMFCBase->Set_OnServerGameCreated(OnServerGameCreated);

	// register for the Post Level Results interval event (gets called every frame
	// when it's time to render a Post Level Results screen)
	DMFCBase->Set_OnPLRInterval(OnPLRInterval);

	// register for the Post Level Results init event (gets called before the first
	// frame of the Post Level Results, per level, so we can setup some values).
	DMFCBase->Set_OnPLRInit(OnPLRInit);

	// register for the Save Stats event (gets called when the client wants to save
	// the game stats now!)
	DMFCBase->Set_OnSaveStatsToFile(OnSaveStatsToFile);

	// register for the Save End of level stats event (gets called when the level 
	// ends if the client wants the stats saved at the end of each level.)
	DMFCBase->Set_OnLevelEndSaveStatsToFile(OnLevelEndSaveStatsToFile);

	// register for the Save on Disconnect event (gets called when you get disconnected
	// from the server for some reason and the client wants stats saved)
	DMFCBase->Set_OnDisconnectSaveStatsToFile(OnDisconnectSaveStatsToFile);

	// register for the print scores event (gets called when a dedicated server
	// does a $scores request)
	DMFCBase->Set_OnPrintScores(OnPrintScores);

	// Since our version of anarchy is localized (same code works for multiple
	// languages) we need to load our string table now.  The function knows
	// what language to load in automatically.  Anarchy.str is located in
	// Anarchy.d3m hog file of Descent 3.  It needs to be in a place where
	// Descent 3 will be able to find it.

	DMFCBase->Set_OnWeaponFired(OnWeaponFired);

	DMFCBase->Set_OnPlayerChangeTeam(OnPlayerChangeTeam);

	DMFCBase->Set_OnServerCollide(OnServerCollide);

	DMFCBase->Set_OnServerPlayerEntersGame(OnServerPlayerEntersGame);
	
	DMFCBase->Set_OnServerPlayerDisconnect(OnServerPlayerDisconnect);
	
	DMFCBase->Set_OnServerPlayerKilled(OnServerPlayerKilled);

	DMFCBase->Set_OnPlayerConnect(OnPlayerConnect);

	DMFCBase->RegisterPacketReceiver(SPID_CHANGEWREQUEST, ReceiveSendChangeWeaponsRequest);
	DMFCBase->RegisterPacketReceiver(SPID_CHANGEW, ReceiveChangeWeapons);
	DMFCBase->RegisterPacketReceiver(SPID_RECEIVESHOT, ReceivePlayerShots);
	DMFCBase->RegisterPacketReceiver(SPID_RECEIVECHEAT, ReceiveCheatMessage);
	

	SetupTables();

	// initialize player shoot times
	if (DMFCBase->GetLocalRole() == LR_SERVER)
	{
		struct timeval now;
		gettimeofday(&now, NULL);

		for (int i = 0; i < 32; ++i)
			playerLastShots[i] = (now.tv_sec * (unsigned int)1e6 + now.tv_usec);

		// support for up to 16 players (+ dedicated server)
		if (DMFCBase->GetNetgameInfo()->max_players > 17)
			DMFCBase->GetNetgameInfo()->max_players = 17;
	}
	
	//Grim's stuff
	IRNumOfTeams = num_teams_to_use;

	int i;

	for(i=0;i<IRNumOfTeams;i++){
		TeamScores[i] = 0;
		OverallTeamScores[i] = 0;
	}

	DLLCreateStringTable("XTeam-InstaReap.str",&StringTable,&StringTableSize);
	DLLCreateStringTable("d3.str",&D3StringTable,&D3StringTableSize);
	mprintf((0,"%d strings loaded from string table\n",StringTableSize));
	if(!StringTableSize){
		// we were unable to load the string table, bail out
		*all_ok = 0;
		return;
	}
	// Initialize Anarchy!
	AnarchyGameInit(IRNumOfTeams);
	
	// Allocate a bitmap for the highlight bar used when displaying the
	// scores on the HUD.  We'll allocate a 32x32 (smallest square bitmap
	// allowed by D3).  You can only draw square bitmaps.
	Highlight_bmp = DLLbm_AllocBitmap(32,32,0);
	if(Highlight_bmp>BAD_BITMAP_HANDLE){
		// get a pointer to the bitmap data so we can
		// initialize it to grey/
		ushort *data = DLLbm_data(Highlight_bmp,0);
		if(!data){
			//bail on out of here
			*all_ok = 0;
			return;
		}

		// go through the bitmap (each pixel is 16 bits) and
		// set the RGB values to 50,50,50.  We also want to set the
		// opaque flag for each pixel, else the pixel will be transparent!
		for(int x=0;x<32*32;x++){
			data[x] = GR_RGB16(50,50,50)|OPAQUE_FLAG;
		}
	}
	DMFCBase->RegisterPacketReceiver(ADD_SCORE,AddScoreToPlayer);
	DMFCBase->RegisterPacketReceiver(CHECK_VERSION,CheckVersionNumber);
	DMFCBase->RegisterPacketReceiver(SPID_GAMESTATE,ReceiveGameState);
	// Register our HUD display callback function with Descent 3...it will
	//be called at the appropriate time to display HUD items.
	DMFCBase->AddHUDItemCallback(HI_TEXT,DisplayHUDScores);

	// Initialize this to false, we're not displaying the stats screen
	DisplayScoreScreen = false;

	DMFCBase->SetupPlayerRecord(sizeof(effcnt),(int (*)(void *,ubyte *))pack_pstat,(int (*)(void *,ubyte *))unpack_pstat);

	dPlayers = DMFCBase->GetPlayers();
	//dTextures = DMFCBase->GetGameTextures();

	// Initialize OSIRIS  old osiris stuff
	/*tOSIRISModuleInit *mi = DMFCBase->GetOsirisModuleData();
	osicommon_Initialize(mi);*/

	sndCheaterHandle = DLLFindSoundName("Cheater!");

}

//	DLLGameClose
//
//	Called when the DLL is about to final shutdown
void DLLFUNCCALL DLLGameClose(void)
{
	//Grim's Stuff
	CleanupTables();

	// if our HUD highlight bar's bitmap was a valid bitmap
	// we must free it, we don't want a memory leak.
	if(Highlight_bmp>BAD_BITMAP_HANDLE)
		DLLbm_FreeBitmap(Highlight_bmp);

	// Since we had a string table (anarchy.str) we must
	// remember to destroy it to free up all memory it used.
	DLLDestroyStringTable(StringTable,StringTableSize);
	DLLDestroyStringTable(D3StringTable,D3StringTableSize);

	// destroy the instance of our DmfcStats, we want our memory back
	// it's not going to be used anymore.
	if(dstat)
	{
		dstat->DestroyPointer();
		dstat = NULL;
	}	

	// And finally, the absolute last thing before we leave, destroy
	// DMFC.
	if(DMFCBase)
	{
		// Call our Anarchy close function for any last minute final
		// shutdown
		AnarchyGameClose();

		// We need to close the game in DMFC before destroying it
		DMFCBase->GameClose();
		DMFCBase->DestroyPointer();
		DMFCBase = NULL;
	}
}

// DetermineScore
//
//	Callback function for the DmfcStats manager.  It gets called for
//	custom text items in the stats.  
//
//	Parms:
//		precord_num : Player record index of the player we are referring to.
//		column_num	: Which column of the stats manager is this call about
//						This is all we have to distinguish multiple custom columns
//		buffer		: The buffer we should fill in
//		buffer_size	: Size of the buffer passed in, DON'T OVERWRITE 
void DetermineScore(int precord_num,int column_num,char *buffer,int buffer_size)
{
	// Get a pointer to the player record data for the given player record
	player_record *pr = DMFCBase->GetPlayerRecord(precord_num);

	if(column_num == 1)
	{
		// If it was an invalid player record, or the player was never in the game
		// then short-circuit
		if(!pr || pr->state==STATE_EMPTY){
			buffer[0] = '\0';
			return;
		}

		// Fill in the score (kills - suicides) for this level
		sprintf(buffer,"%d",pr->dstats.kills[DSTAT_LEVEL]-pr->dstats.suicides[DSTAT_LEVEL]);
	}
	else if(column_num == 6)
	{
		if(!pr || pr->state==STATE_EMPTY){
			buffer[0] = '\0';
			return;
		}
		float eff_num = 0;
		float fkill, fshot;

		effcnt *stat;
		stat = (effcnt *)pr->user_info;
		if(stat)
		{
			if(stat->shots > 0)
			{
				//fkill = eff_counter[pr->pnum].kills; fshot = eff_counter[pr->pnum].shots;
				fkill = pr->dstats.kills[DSTAT_LEVEL]; fshot = stat->shots;
				eff_num = (fkill / fshot);
				eff_num *= 100;
			}

		}

		sprintf(buffer,"%f",eff_num);
	}

}

//	AnarchyGameClose
//	
//	Calls for any final shutdown
void AnarchyGameClose(void)
{
}

// AnarchyGameInit
//
//	Initializes Anarchy so it is ready to be played
void AnarchyGameInit(int teams)
{

	// First we are going to create the On-Screen menu submenus for Anarchy.
	// This must be done before the call to DMFC::GameInit() if we want the
	// Anarchy sub-menu to be at the top.

	IMenuItem *lev1,*lev2;

	// Using CreateMenuItemWArgs we can create a sub menu instance
	// First we'll create the base sub-directory with the "Anarchy" sub title
	lev1 = CreateMenuItemWArgs("XTeam-InstaReap",MIT_NORMAL,0,NULL);

	// Now we'll create the HUD style sub menu, since it is a state item, we
	// need to pass a callback, which will get called if the state changes
	lev2 = CreateMenuItemWArgs(TXT_MNU_HUDSTYLE,MIT_STATE,0,SwitchAnarchyScores);
	// Set the text of all the states
	lev2->SetStateItemList(3,TXT_NONE,TXT_SCOREHD,TXT_EFFICIENCY);
	// Set initial value
	lev2->SetState(1);
	// Now add this submenu to the base-subdirectory
	lev1->AddSubMenu(lev2);

	// Next create the sub directory for HUD color state.  Again, this is a
	// state item, so we have to pass a callback also.
	lev2 = CreateMenuItemWArgs(TXT_MNU_HUDCOLOR,MIT_STATE,0,SwitchHUDColor);
	// Set the text of all the states
	lev2->SetStateItemList(2,TXT_PLAYERCOLORS,TXT_NORMAL);
	// Set the initial state
	lev2->SetState(HUD_color_model);
	// Now add this submenu to the base-subdirectory
	lev1->AddSubMenu(lev2);

	// Grab a pointer to the On-Screen menu root
	lev2 = DMFCBase->GetOnScreenMenu();
	// Add the sub-menu tree created above (which was all branched from the
	// base sub-directory "Anarchy" to the root.
	lev2->AddSubMenu(lev1);

	// Now that we setup our On-Screen menu, we can initialize DMFC,
	// all we have to do is pass the initial number of teams.
	DMFCBase->GameInit(teams);

	// Initialize the Dmfc stats Manager
	tDmfcStatsInit tsi;
	tDmfcStatsColumnInfo pl_col[7];
	char gname[30];
	strcpy(gname,"Team InstaReap");

	// we want to show the Pilot Picture/Ship Logo of the pilot if available
	// we also want to show the observer mode icon, if the player is observing
	tsi.flags = DSIF_SHOW_PIC|DSIF_SHOW_OBSERVERICON|DSIF_SEPERATE_BY_TEAM;
	tsi.cColumnCountDetailed = 0;	// 0 columns in the detailed list (specific info about the highlighted player)
	tsi.cColumnCountPlayerList = 7;	// 6 columns in the player list
	tsi.clbDetailedColumn = NULL;	// No custom items in the detailed list, no 
									//callback needed
	tsi.clbDetailedColumnBMP = NULL;// No custom bitmap items in the detailed list, no callback needed
	tsi.clbPlayerColumn = DetermineScore;	// we do have a custom text column, so
											// set it's callback
	tsi.clbPlayerColumnBMP = NULL;		// no custom bitamp items in the detailed 
										//list
	tsi.DetailedColumns = NULL;		// pointer to an array of tDmfcStatsColumnInfo
									// to describe the detail columns (there are none,
									// so this is NULL)
	tsi.GameName = gname;			// The title for the Stats screen
	tsi.MaxNumberDisplayed = NULL;	// This can be set to a pointer to an int, that is the maximum number of players to display in the stats screen
									// by default it is all of them.  Make sure you use a global variable or a static, as this pointer must always
									// be valid.  Since we want all of them, just set this to NULL.
	tsi.PlayerListColumns = pl_col;	// pointer to an array of tDmfcStatsColumnInfo
									// to describe the player columns (this array 
									// is filled out below)
	tsi.SortedPlayerRecords = SortedPlayers;	// This is a pointer to an array of ints which is of
												// size MAX_PLAYER_RECORDS.  This array is a sorted list
												// of the player records.  The stats manager uses this
												// so it knows the order that it should display the players.
	tsi.clTeamLine = TeamScoreCallback;

	// Now setup the Player List column items

	// The first column is the name of the player, we can use the predefined
	// column type.
	pl_col[0].color_type = DSCOLOR_TEAM;
	strcpy(pl_col[0].title,TXT_PILOT);
	pl_col[0].type = DSCOL_PILOT_NAME;
	pl_col[0].width = 120;

	// The second column is the score, this is a custom column, so in order to get
	// it's data it will call the custom callback.
	pl_col[1].color_type = DSCOLOR_TEAM;
	strcpy(pl_col[1].title,TXT_POINTS);
	pl_col[1].type = DSCOL_CUSTOM;
	pl_col[1].width = 52;

	// The third column is the number of kills so far this level, we can use
	// the prefined column type.
	pl_col[2].color_type = DSCOLOR_TEAM;
	strcpy(pl_col[2].title,TXT_KILLS_SHORT);
	pl_col[2].type = DSCOL_KILLS_LEVEL;
	pl_col[2].width = 47;

	// The fourth column is the number of deaths so far this level, we can use
	// the predefined column type.
	pl_col[3].color_type = DSCOLOR_TEAM;
	strcpy(pl_col[3].title,TXT_DEATHS_SHORT);
	pl_col[3].type = DSCOL_DEATHS_LEVEL;
	pl_col[3].width = 47;

	// The fifth column is the number of suicides so far this level, we can use
	// the predefined column type.
	pl_col[4].color_type = DSCOLOR_TEAM;
	strcpy(pl_col[4].title,TXT_SUICIDES_SHORT);
	pl_col[4].type = DSCOL_SUICIDES_LEVEL;
	pl_col[4].width = 47;

	// The sixth and final column is ping, there is also a predefined column
	// type for this also.
	pl_col[5].color_type = DSCOLOR_TEAM;
	strcpy(pl_col[5].title,TXT_PING);
	pl_col[5].type = DSCOL_PING;
	pl_col[5].width = 40;
	// column type.
	
	pl_col[6].color_type = DSCOLOR_TEAM;
	strcpy(pl_col[6].title, "Hit Eff");
	pl_col[6].type = DSCOL_CUSTOM;
	pl_col[6].width = 40;

	// We are done setting up all the structs, do the final step,
	// initialize the stats!
	dstat->Initialize(&tsi);	

	//add the death and suicide messages
	// these are the HUD messages that pop-up when a player dies
	// DMFC has one built in death message and one built in suicide message
	DMFCBase->AddDeathMessage(TXT_DEATH1,true);
	DMFCBase->AddDeathMessage(TXT_DEATH2,true);
	DMFCBase->AddDeathMessage(TXT_DEATH3,false);
	DMFCBase->AddDeathMessage(TXT_DEATH4,false);
	DMFCBase->AddDeathMessage(TXT_DEATH5,true);
	DMFCBase->AddDeathMessage(TXT_DEATH6,true);
	DMFCBase->AddDeathMessage(TXT_DEATH7,false);
	DMFCBase->AddDeathMessage(TXT_DEATH8,true);
	DMFCBase->AddDeathMessage(TXT_DEATH9,true);
	DMFCBase->AddDeathMessage(TXT_DEATH10,true);

	DMFCBase->AddSuicideMessage(TXT_SUICIDE1);
	DMFCBase->AddSuicideMessage(TXT_SUICIDE2);
	DMFCBase->AddSuicideMessage(TXT_SUICIDE3);
	DMFCBase->AddSuicideMessage(TXT_SUICIDE4);
	DMFCBase->AddSuicideMessage(TXT_SUICIDE5);
	DMFCBase->AddSuicideMessage(TXT_SUICIDE6);

	// We must tell DMFC how many teams our game is, this sets up a number
	// of states of DMFC.
	DMFCBase->SetNumberOfTeams(teams);

	// By default, you are not allowed to hurt your teammates.  And since there
	// is only 1 team in Anarchy, everyone is considered to be on the same
	// team.  What we want is to be able to hurt others, so we need to set
	// the netgame flag for damage friendly.
	netgame_info *Netgame = DMFCBase->GetNetgameInfo();
	Netgame->flags |= (NF_TRACK_RANK);

}


// OnHUDInterval
//
//	Event handler for when it's time to render the HUD
void OnHUDInterval(void)
{
	// We must process the stats manager each frame, so
	// we'll do this now. (Which will render it if it's enabled)
	dstat->DoFrame();

	// Finally, call the default event handler
	DMFCBase->OnHUDInterval();
}

//	OnInterval
//
//	Event handler, called every frame
void OnInterval(void)
{
	// Since we are only simply sorting by (kills-suicides), typical 
	// Anarchy scoring style, we can call the built in sort function
	// of DMFC, which sorts this way.
	DMFCBase->GetSortedPlayerSlots(SortedPlayers,MAX_PLAYER_RECORDS);

	// Finally, call the default event handler
	DMFCBase->OnInterval();

	SortTeamScores(SortedTeams,TeamScores);

	static bool im_dead = false; //set to tell player is not dead

	int pnum = DMFCBase->GetPlayerNum(); //get the player's number

	if (dPlayers[pnum].flags & PLAYER_FLAGS_DEAD)
		im_dead = true;

	if (im_dead)
	{
		if (!(dPlayers[pnum].flags & PLAYER_FLAGS_DEAD))
			if(pnum==DMFCBase->GetPlayerNum() || DMFCBase->GetLocalRole()==LR_SERVER)
			{
			// I'm respawning, so give weapons.
			SendChangeWeaponsRequest(pnum);
			im_dead = false;
			}
	}
	
	static bool im_observer = false;
	
	if(DMFCBase->IsPlayerObserver(pnum))
		im_observer = true;
	
	if (im_observer)
	{
		if(!(DMFCBase->IsPlayerObserver(pnum)))
		{
			SendChangeWeaponsRequest(pnum);
			im_observer = false;
		}
	}

	
}

// OnKeypress
//
//	Event handler, called whenever the client presses a key
void OnKeypress(int key)
{
	// first get the pointer to the data structure that was passed
	// to the mod, we may need it to tell Descent 3 to ignore
	// this keypress.
	dllinfo *Data = DMFCBase->GetDLLInfoCallData();

	// if Data->iRet is 1 on return from this function, then
	// Descent 3 will not process this keypress.

	// see what key was pressed
	switch(key){
	case K_F7:
		// The user wants to display the stats screen, turn off the
		// on-screen menu if it's up
		DisplayScoreScreen = !DisplayScoreScreen;
		DMFCBase->EnableOnScreenMenu(false);
		dstat->Enable(DisplayScoreScreen);
		break;
	case K_PAGEDOWN:
		// The user wants to scroll down a row in the stats manager
		if(DisplayScoreScreen){
			dstat->ScrollDown();
			Data->iRet = 1;
		}
		break;
	case K_PAGEUP:
		// The user wants to scroll up a row in the stats manager
		if(DisplayScoreScreen){
			dstat->ScrollUp();
			Data->iRet = 1;
		}
		break;
	case K_F6:
		// The user wants to go into the On-Screen menu (DMFC automatically
		// turns this on, so we just need to intercept it to turn off
		// the stats screen if it's on)
		DisplayScoreScreen = false;
		dstat->Enable(false);		
		break;
	case K_ESC:
		// If the stats screen is up, close it
		if(DisplayScoreScreen){
			dstat->Enable(false);
			DisplayScoreScreen = false;
			Data->iRet = 1;
		}
		break;
	}

	// Call the default event handler
	DMFCBase->OnKeypress(key);
}

// Out-dated, nothing done in here now
// -- event gets called when the mod is loaded and about to start
void OnServerGameCreated(void)
{
	DMFCBase->OnServerGameCreated();
	
	effcnt *stat;
	player_record *pr;
	int i;
	
	for(i=0;i<MAX_PLAYER_RECORDS;i++)
	{
		pr = DMFCBase->GetPlayerRecord(i);
		if(pr){
			stat = (effcnt *)pr->user_info;
			if(stat){
				stat->shots = 0;
			}
		}
	}	

}
// -- event gets called for a client when the level ends
void OnClientLevelEnd(void)
{


	DMFCBase->OnClientLevelEnd();
}


//	OnClientLevelStart
//
//	The server has started a new level, so initialize the sort list to all -1
//	since there has been no sorting yet.
void OnClientLevelStart(void)
{
	effcnt *stat;
	
	int i;
	for(i=0;i<MAX_PLAYER_RECORDS;i++){
		SortedPlayers[i] = -1;	
	}
	for(i=0;i<DLLMAX_TEAMS;i++){
		
		SortedTeams[i] = -1;
	}
	
	for(i=0;i<IRNumOfTeams;i++){
		TeamScores[i] = 0;
		OverallTeamScores[i] = 0;
	}
	
	player_record *pr;
	for(i=0;i<MAX_PLAYER_RECORDS;i++)
	{
		pr = DMFCBase->GetPlayerRecord(i);
		if(pr){
			stat = (effcnt *)pr->user_info;
			if(stat){
				stat->shots = 0;
			}
		}
	}
	// Call the default event handler
	DMFCBase->OnClientLevelStart();
}

//	OnClientPlayerEntersGame
//
//	A new player has entered the game, zero their stats out
void OnClientPlayerEntersGame(int player_num)
{
	player *player = &(DMFCBase->GetPlayers())[player_num];
	ship *ship = &(DMFCBase->GetShips())[player->ship_index];

	if (DMFCBase->GetLocalRole()==LR_SERVER)
		shipShootIntervals[player_num] = (unsigned long)((ship->static_wb[6].gp_fire_wait[0]-0.1)*1000000);

	if(player_num == DMFCBase->GetPlayerNum())
	{
		save_reload = ship->static_wb[6].gp_fire_wait[0];
		ship->static_wb[6].gp_fire_wait[0] = 0.01f;

		DLLAddHUDMessage("This is XTeam-InstaReap revision %d", version_num);
		DLLAddHUDMessage("Cheat prevention is active - I'm watching you! :)");
	}

	// Call the default handler first
	DMFCBase->OnClientPlayerEntersGame(player_num);

	// If the player isn't us (this event gets called for everyone, including
	// the player entering the game) then inform us that the player has
	// joined the game
	if(player_num!=DMFCBase->GetPlayerNum())
		DisplayWelcomeMessage(player_num);
	else // set the flag that we need to display our welcome message the first frame
		display_my_welcome = true;
	
	// only add weapons/ammo for this player num if it is us, or we are the 
	// server.  These are the only two that need to add the ammo...but BOTH 
	// must do it.
		
	// We'll add:
	// 2: A Mass Driver (primary w/ ammo)
	//15 rounds of ammo for mass driver

	if(player_num==DMFCBase->GetPlayerNum() || DMFCBase->GetLocalRole()==LR_SERVER)
	{
		AddWeaponToPlayer(player_num,MASSDRIVER_INDEX,15);
		RemWeaponFromPlayer(player_num,LASER_INDEX);
		ChangeWeapon(player_num,MASSDRIVER_INDEX);
		RemWeaponFromPlayer(player_num,CONCUSSION_INDEX);
	}
}

//	OnClientPlayerKilled
//
//	Usually this is done automatically, but we need to handle if the
//	goal score is reached  (to end the level).
void OnClientPlayerKilled(object *killer_obj,int victim_pnum)
{
	// First call the default handler to do the real processesing
	
	DMFCBase->OnClientPlayerKilled(killer_obj,victim_pnum);

	if(victim_pnum == DMFCBase->GetPlayerNum())
	{
		player *player = &(DMFCBase->GetPlayers())[victim_pnum];
		ship *ship = &(DMFCBase->GetShips())[player->ship_index];

		ship->static_wb[6].gp_fire_wait[0] = 0.01f;
		first_shot = true;

	}
	// Now we need to do the same thing as the default handler, but
	// we just need to see if the player's score is >= the goal.
	// Another way (and perhaps easier) to do this check is just every
	// frame go through all the players and check their score.
	int tnum;
	int kpnum;
	
	if(killer_obj){
		if((killer_obj->type==OBJ_PLAYER)||(killer_obj->type==OBJ_GHOST))
		{
			kpnum = killer_obj->id;
			if(kpnum == victim_pnum)
			{
				TeamScores[DMFCBase->GetPlayerTeam(kpnum)]--;
				
			}
			else
			{
				TeamScores[DMFCBase->GetPlayerTeam(kpnum)]++;
				
			}
		}
		else if(killer_obj->type==OBJ_ROBOT || killer_obj->type == OBJ_BUILDING){
			//countermeasure kill
			kpnum = DMFCBase->GetCounterMeasureOwner(killer_obj);
		}else{
			kpnum = -1;
		}
	}else{
		kpnum = -1;
	}

	tnum = DMFCBase->GetPlayerTeam(kpnum);
	if(tnum != (-1)){
		int goal;
		if(DMFCBase->GetScoreLimit(&goal)){
			// A Score limit was set for this game, check it to see if it has been
			// met.
			int score = TeamScores[tnum];
			if(score>=goal){
				DMFCBase->EndLevel();
			}
		}
	}
	//*killer_obj->shields = 100;
}

// compares 2 player records's scores to see which one is ahead of the other
bool compare_slots(int a,int b)
{
	int ascore,bscore;
	player_record *apr,*bpr;
	apr = DMFCBase->GetPlayerRecord(a);
	bpr = DMFCBase->GetPlayerRecord(b);
	if( !apr )
		return true;
	if( !bpr )
		return false;
	if( apr->state==STATE_EMPTY )
		return true;
	if( bpr->state==STATE_EMPTY )
		return false;
	if( (apr->state==STATE_INGAME) && (bpr->state==STATE_INGAME) ){
		//both players were in the game
		ascore = apr->dstats.kills[DSTAT_LEVEL] - apr->dstats.suicides[DSTAT_LEVEL];
		bscore = bpr->dstats.kills[DSTAT_LEVEL] - bpr->dstats.suicides[DSTAT_LEVEL];
		return (ascore<bscore);
	}
	if( (apr->state==STATE_INGAME) && (bpr->state==STATE_DISCONNECTED) ){
		//apr gets priority since he was in the game on exit
		return false;
	}
	if( (apr->state==STATE_DISCONNECTED) && (bpr->state==STATE_INGAME) ){
		//bpr gets priority since he was in the game on exit
		return true;
	}
	//if we got here then both players were disconnected
	ascore = apr->dstats.kills[DSTAT_LEVEL] - apr->dstats.suicides[DSTAT_LEVEL];
	bscore = bpr->dstats.kills[DSTAT_LEVEL] - bpr->dstats.suicides[DSTAT_LEVEL];
	return (ascore<bscore);
}

//	OnPLRInit
//
//	Event handler for to initialize anything we need for Post Level Results
void OnPLRInit(void)
{
	int tempsort[MAX_PLAYER_RECORDS];
	int i,t,j;

	for(i=0;i<MAX_PLAYER_RECORDS;i++){
		tempsort[i] = i;
	}

	for(i=1;i<=MAX_PLAYER_RECORDS-1;i++){
		t=tempsort[i];
		// Shift elements down until
		// insertion point found.
		for(j=i-1;j>=0 && compare_slots(tempsort[j],t); j--){
			tempsort[j+1] = tempsort[j];
		}
		// insert
		tempsort[j+1] = t;
	}

	//copy the array over (we only have to copy over DLLMAX_PLAYERS, because
	//that how much of it we are going to use)
	memcpy(SortedPlayers,tempsort,DLLMAX_PLAYERS*sizeof(int));

	DMFCBase->OnPLRInit();
}

// OnPLRInterval
//
//	Frame interval call for Post Level Results
void OnPLRInterval(void)
{
#define PLAYERS_COL		100
#define SCORE_COL		250
#define DEATHS_COL		300
#define SUICIDES_COL	360
#define TOTAL_COL		430
#define EFF_COL			490

	// The FIRST thing that must be done is call the default handler...this
	// must be first
	DMFCBase->OnPLRInterval();

	
	//our Y position on the screen
	int y = 40;
	
	// get the height of our font, so we can space correctly
	int height = DLLgrfont_GetHeight((DMFCBase->GetGameFontTranslateArray())[SMALL_UI_FONT_INDEX]) + 1;
	DLLgrtext_SetFont((DMFCBase->GetGameFontTranslateArray())[SMALL_UI_FONT_INDEX]);

	//print out header
	DLLgrtext_SetColor(GR_RGB(255,255,150));
	DLLgrtext_Printf(PLAYERS_COL,y,TXT_PILOT);
	DLLgrtext_Printf(SCORE_COL,y,TXT_KILLS_SHORT);
	DLLgrtext_Printf(DEATHS_COL,y,TXT_DEATHS_SHORT);
	DLLgrtext_Printf(SUICIDES_COL,y,TXT_SUICIDES_SHORT);
	DLLgrtext_Printf(TOTAL_COL,y,TXT_SCORE);
	DLLgrtext_Printf(EFF_COL, y, "Hit Eff");
	y+=height;	//move down the Y value

	int j;
	int rank[4];
	int team_y[4];
	int tempy = y;
	for(j = 0; j<IRNumOfTeams; j++)
	{
		rank[j] = 1;
		team_y[SortedTeams[j]] = tempy;
		tempy += (height * 8);
	}
	

	for(j = 0; j<IRNumOfTeams; j++)
	{
		DLLgrtext_SetColor(DMFCBase->GetTeamColor(j));
		DLLgrtext_Printf(PLAYERS_COL, team_y[j], DMFCBase->GetTeamString(j));
		DLLgrtext_Printf(TOTAL_COL, team_y[j], "%d", TeamScores[j]);
		team_y[j] += height;
	}
	//print out player stats
	//int rank = 1;
	int slot,pnum;//,team;
	player_record *pr;

	// We're only going to print out, at most 32 (DLLMAX_PLAYERS)
	for(int i=0;i<DLLMAX_PLAYERS;i++){
		// use the sort array to translate to a real player record index
		slot = SortedPlayers[i];
		pr = DMFCBase->GetPlayerRecord(slot);

		// we only want player records that are for players that have been in the
		// game
		if((pr) && (pr->state!=STATE_EMPTY) ){

			if(DMFCBase->IsPlayerDedicatedServer(pr))
				continue;//skip a dedicated server
			
			//team = DMFCBase->GetPlayerTeam(slot);
			if(rank[pr->team] > 7)
				continue;

			// since at the end of a level, player's disconnect
			// DMFC stores the players that were in the game at
			// level end.  Calling this function with a given player record
			// will return the player number of the player if they were
			// in the game at level end, else it will return -1
			pnum=DMFCBase->WasPlayerInGameAtLevelEnd(slot);
			if(pnum!=-1)
			{
				// set the current color for text to their player ship color
				DLLgrtext_SetColor(DMFCBase->GetTeamColor(pr->team));
			}else
			{
				// set the color to grey (they weren't in the game at the end)
				DLLgrtext_SetColor(GR_RGB(128,128,128));
			}

			// Print out the actual information about the player
			char temp[100];
			sprintf(temp,"%d)%s",rank[pr->team],pr->callsign); //callsign and rank

			// Calling clip string will ensure that a string doesn't run
			// over into other columns.
			DMFCBase->ClipString(SCORE_COL - PLAYERS_COL - 10,temp,true);
			DLLgrtext_Printf(PLAYERS_COL,team_y[pr->team],"%s",temp);

			DLLgrtext_Printf(SCORE_COL,team_y[pr->team],"%d",pr->dstats.kills[DSTAT_LEVEL]);
			DLLgrtext_Printf(DEATHS_COL,team_y[pr->team],"%d",pr->dstats.deaths[DSTAT_LEVEL]);
			DLLgrtext_Printf(SUICIDES_COL,team_y[pr->team],"%d",pr->dstats.suicides[DSTAT_LEVEL]);
			DLLgrtext_Printf(TOTAL_COL,team_y[pr->team],"%d",pr->dstats.kills[DSTAT_LEVEL]-pr->dstats.suicides[DSTAT_LEVEL]);
			
			float eff_num = 0;
			float fkill, fshot;
			
			effcnt *stat;
			stat = (effcnt *)pr->user_info;
			if(stat)
			{
				if(stat->shots > 0)
				{
					fkill = pr->dstats.kills[DSTAT_LEVEL]; fshot = stat->shots;
					eff_num = (fkill / fshot);
					eff_num *= 100;
				}
			}
			DLLgrtext_Printf(EFF_COL,team_y[pr->team], "%f", eff_num);
			team_y[pr->team]+=height;
			rank[pr->team]++;
			if(team_y[pr->team]>=440)
				goto quick_exit;

		}
	}

quick_exit:;
}

// SaveStatsToFile
//
//	Save our current stats to the given filename
void SaveStatsToFile(char *filename)
{
	CFILE *file;
	DLLOpenCFILE(&file,filename,"wt");
	if(!file){
		mprintf((0,"Unable to open output file\n"));
		return;
	}

	//write out game stats
	#define BUFSIZE	150
	char buffer[BUFSIZE];
	char tempbuffer[25];
	int sortedslots[MAX_PLAYER_RECORDS];
	player_record *pr,*dpr;
	effcnt *pdstat;
	tPInfoStat stat;
	int count,length,p;

	//sort the stats
	DMFCBase->GetSortedPlayerSlots(sortedslots,MAX_PLAYER_RECORDS);
	count = 1;

	sprintf(buffer,"Team-InstaReap\r\nGame: %s\r\nLevel: %d\r\n",(DMFCBase->GetNetgameInfo())->name,(DMFCBase->GetCurrentMission())->cur_level);
	DLLcf_WriteString(file,buffer);

	sprintf(buffer,"%s %s", TXT_SAVE_HEADERB, "[Hit Eff]  [Team]");
	DLLcf_WriteString(file,buffer);
	sprintf(buffer,"-----------------------------------------------------------------------------------------------");
	DLLcf_WriteString(file,buffer);

	for(int j=0;j<IRNumOfTeams; j++){
		memset(buffer,' ', BUFSIZE);

		sprintf(tempbuffer, "%d)", count);
		memcpy(&buffer[0], tempbuffer, strlen(tempbuffer));

		sprintf(tempbuffer,"%s", DMFCBase->GetTeamString(j));
		memcpy(&buffer[7],tempbuffer,strlen(tempbuffer));
			
		sprintf(tempbuffer, "%d", TeamScores[j]);
		memcpy(&buffer[36], tempbuffer, strlen(tempbuffer));
	
		int pos;
		pos = 36 + strlen(tempbuffer) + 1;
		if(pos<BUFSIZE)
			buffer[pos] = '\0';
						
		buffer[BUFSIZE-1] = '\0';
		DLLcf_WriteString(file,buffer);		
		
		count++;
	}
	
	sprintf(buffer,"-----------------------------------------------------------------------------------------------");
	DLLcf_WriteString(file,buffer);
	
	count = 1;
	for(int s=0;s<MAX_PLAYER_RECORDS;s++){
		p = sortedslots[s];
		pr = DMFCBase->GetPlayerRecord(p);
		if( pr && pr->state!=STATE_EMPTY) {

			if(DMFCBase->IsPlayerDedicatedServer(pr))
				continue;	//skip dedicated server

			memset(buffer,' ',BUFSIZE);

			sprintf(tempbuffer,"%d)",count);
			memcpy(&buffer[0],tempbuffer,strlen(tempbuffer));

			sprintf(tempbuffer,"%s%s",(pr->state==STATE_INGAME)?"":"*",pr->callsign);
			memcpy(&buffer[7],tempbuffer,strlen(tempbuffer));

			sprintf(tempbuffer,"%d[%d]",pr->dstats.kills[DSTAT_LEVEL]-pr->dstats.suicides[DSTAT_LEVEL],pr->dstats.kills[DSTAT_OVERALL]-pr->dstats.suicides[DSTAT_OVERALL]);
			memcpy(&buffer[36],tempbuffer,strlen(tempbuffer));

			sprintf(tempbuffer,"%d[%d]",pr->dstats.kills[DSTAT_LEVEL],pr->dstats.kills[DSTAT_OVERALL]);
			memcpy(&buffer[48],tempbuffer,strlen(tempbuffer));

			sprintf(tempbuffer,"%d[%d]",pr->dstats.deaths[DSTAT_LEVEL],pr->dstats.deaths[DSTAT_OVERALL]);
			memcpy(&buffer[60],tempbuffer,strlen(tempbuffer));

			sprintf(tempbuffer,"%d[%d]",pr->dstats.suicides[DSTAT_LEVEL],pr->dstats.suicides[DSTAT_OVERALL]);
			memcpy(&buffer[71],tempbuffer,strlen(tempbuffer));

			sprintf(tempbuffer,"%s",DMFCBase->GetTimeString(DMFCBase->GetTimeInGame(p)));
			memcpy(&buffer[82],tempbuffer,strlen(tempbuffer));
	
			float eff_num = 0;
			float fkill, fshot;

			pdstat = (effcnt *)pr->user_info;
			if(pdstat)
			{
				if(pdstat->shots > 0)
				{
					fkill = pr->dstats.kills[DSTAT_LEVEL]; fshot = pdstat->shots;
					eff_num = (fkill / fshot);
					eff_num *= 100;
				}
			}
			sprintf(tempbuffer,"%f", eff_num);
			memcpy(&buffer[100],tempbuffer,strlen(tempbuffer));

			sprintf(tempbuffer,"%s", DMFCBase->GetTeamString(pr->team));
			memcpy(&buffer[112],tempbuffer,strlen(tempbuffer));

			int pos;
			pos = 112 + strlen(tempbuffer) + 1;
			if(pos<BUFSIZE)
				buffer[pos] = '\0';
							
			buffer[BUFSIZE-1] = '\0';
			DLLcf_WriteString(file,buffer);
			count++;
		}
	}

	DLLcf_WriteString(file,TXT_SAVE_HEADERC);

	// go through all the players individually, and write out detailed stats
	count =1;
	for(p=0;p<MAX_PLAYER_RECORDS;p++){
		pr = DMFCBase->GetPlayerRecord(p);
		if( pr && pr->state!=STATE_EMPTY) {

			if(DMFCBase->IsPlayerDedicatedServer(pr))
				continue;	//skip dedicated server

			//Write out header
			sprintf(buffer,"%d) %s%s",count,(pr->state==STATE_INGAME)?"":"*",pr->callsign);
			DLLcf_WriteString(file,buffer);
			length = strlen(buffer);
			memset(buffer,'=',length);
			buffer[length] = '\0';
			DLLcf_WriteString(file,buffer);
			
			//time in game
			sprintf(buffer,TXT_SAVE_TIMEINGAME,DMFCBase->GetTimeString(DMFCBase->GetTimeInGame(p)));
			DLLcf_WriteString(file,buffer);

			// To go through the list of who killed who stats for each player
			// we use FindPInfoStatFirst/FindPInfoStatNext/FindPInfoStatClose to
			// go through the list.
			if(DMFCBase->FindPInfoStatFirst(p,&stat)){
				sprintf(buffer,TXT_SAVE_KILLERLIST);
				DLLcf_WriteString(file,buffer);
				
				if(stat.slot!=p){
					memset(buffer,' ',BUFSIZE);
					dpr = DMFCBase->GetPlayerRecord(stat.slot);
					int pos;

					ASSERT(dpr!=NULL);
					if(dpr){
						sprintf(tempbuffer,"%s",dpr->callsign);
						memcpy(buffer,tempbuffer,strlen(tempbuffer));

						sprintf(tempbuffer,"%d",stat.kills);
						memcpy(&buffer[30],tempbuffer,strlen(tempbuffer));

						sprintf(tempbuffer,"%d",stat.deaths);
						memcpy(&buffer[40],tempbuffer,strlen(tempbuffer));

						pos = 40 + strlen(tempbuffer) + 1;
						if(pos<BUFSIZE)
							buffer[pos] = '\0';

						buffer[BUFSIZE-1] = '\0';
						DLLcf_WriteString(file,buffer);
					}
				}
						
				// keep going through the list until there are no more
				while(DMFCBase->FindPInfoStatNext(&stat)){															
					if(stat.slot!=p){
						int pos;
						memset(buffer,' ',BUFSIZE);
						dpr = DMFCBase->GetPlayerRecord(stat.slot);

						if(dpr)
						{
							sprintf(tempbuffer,"%s",dpr->callsign);
							memcpy(buffer,tempbuffer,strlen(tempbuffer));

							sprintf(tempbuffer,"%d",stat.kills);
							memcpy(&buffer[30],tempbuffer,strlen(tempbuffer));

							sprintf(tempbuffer,"%d",stat.deaths);
							memcpy(&buffer[40],tempbuffer,strlen(tempbuffer));

							pos = 40 + strlen(tempbuffer) + 1;
							if(pos<BUFSIZE)
								buffer[pos] = '\0';

							buffer[BUFSIZE-1] = '\0';
							DLLcf_WriteString(file,buffer);
						}
					}		
				}
			}
			DMFCBase->FindPInfoStatClose();
			DLLcf_WriteString(file,"");	//skip a line
			count++;
		}
	}

	//done writing stats
	DLLcfclose(file);
	DLLAddHUDMessage(TXT_MSG_SAVESTATS);
}

#define ROOTFILENAME	"Team-InstaReap"
// The user wants to save the stats to file
void OnSaveStatsToFile(void)
{
	char filename[256];
	DMFCBase->GenerateStatFilename(filename,ROOTFILENAME,false);
	SaveStatsToFile(filename);
}
// The level ended and the user has it set to auto save stats
void OnLevelEndSaveStatsToFile(void)
{
	char filename[256];
	DMFCBase->GenerateStatFilename(filename,ROOTFILENAME,true);
	SaveStatsToFile(filename);
}
// The player has disconnected and has it set to auto save stats
void OnDisconnectSaveStatsToFile(void)
{
	char filename[256];
	DMFCBase->GenerateStatFilename(filename,ROOTFILENAME,false);
	SaveStatsToFile(filename);
}

//	OnPrintScores
//
//	The dedicated server (us) requested to see the scores (via $scores)
void OnPrintScores(int level)
{
	char buffer[256];
	char name[50];
	memset(buffer,' ',256);

	int t;
	int pos[6];
	int len[6];
	pos[0] = 0;					t = len[0] = 20;	//give ample room for pilot name
	pos[1] = pos[0] + t + 1;	t = len[1] = strlen(TXT_POINTS);
	pos[2] = pos[1] + t + 1;	t = len[2] = strlen(TXT_KILLS_SHORT);
	pos[3] = pos[2] + t + 1;	t = len[3] = strlen(TXT_DEATHS_SHORT);
	pos[4] = pos[3] + t + 1;	t = len[4] = strlen(TXT_SUICIDES_SHORT);
	pos[5] = pos[4] + t + 1;	t = len[5] = strlen(TXT_PING);

	memcpy(&buffer[pos[0]],TXT_PILOT,strlen(TXT_PILOT));
	memcpy(&buffer[pos[1]],TXT_POINTS,len[1]);
	memcpy(&buffer[pos[2]],TXT_KILLS_SHORT,len[2]);
	memcpy(&buffer[pos[3]],TXT_DEATHS_SHORT,len[3]);
	memcpy(&buffer[pos[4]],TXT_SUICIDES_SHORT,len[4]);
	memcpy(&buffer[pos[5]],TXT_PING,len[5]);
	buffer[pos[5]+len[5]+1] = '\n';
	buffer[pos[5]+len[5]+2] = '\0';
	DPrintf(buffer);

	int slot;
	player_record *pr;
	int pcount;

	if(level<0 || level>=MAX_PLAYER_RECORDS)
		pcount = MAX_PLAYER_RECORDS;
	else
		pcount = level;

	int sortedplayers[MAX_PLAYER_RECORDS];
	DMFCBase->GetSortedPlayerSlots(sortedplayers,MAX_PLAYER_RECORDS);
	
	for(int i=0;i<pcount;i++){
		slot = sortedplayers[i];
		pr = DMFCBase->GetPlayerRecord(slot);
		if((pr)&&(pr->state!=STATE_EMPTY)){

			if(DMFCBase->IsPlayerDedicatedServer(pr))
				continue;	//skip dedicated server

			sprintf(name,"%s%s:",(pr->state==STATE_DISCONNECTED)?"*":"",pr->callsign);
			name[19] = '\0';

			memset(buffer,' ',256);
			t = strlen(name); memcpy(&buffer[pos[0]],name,(t<len[0])?t:len[0]);
			sprintf(name,"%d",pr->dstats.kills[DSTAT_LEVEL]-pr->dstats.suicides[DSTAT_LEVEL]);
			t = strlen(name); memcpy(&buffer[pos[1]],name,(t<len[1])?t:len[1]);
			sprintf(name,"%d",pr->dstats.kills[DSTAT_LEVEL]);
			t = strlen(name); memcpy(&buffer[pos[2]],name,(t<len[2])?t:len[2]);
			sprintf(name,"%d",pr->dstats.deaths[DSTAT_LEVEL]);
			t = strlen(name); memcpy(&buffer[pos[3]],name,(t<len[3])?t:len[3]);
			sprintf(name,"%d",pr->dstats.suicides[DSTAT_LEVEL]);
			t = strlen(name); memcpy(&buffer[pos[4]],name,(t<len[4])?t:len[4]);

			if(pr->state==STATE_INGAME)
				sprintf(name,"%.0f",(DMFCBase->GetNetPlayers())[pr->pnum].ping_time*1000.0f);
			else
				strcpy(name,"---");
			t = strlen(name); memcpy(&buffer[pos[5]],name,(t<len[5])?t:len[5]);
			buffer[pos[5]+len[5]+1] = '\n';
			buffer[pos[5]+len[5]+2] = '\0';
			DPrintf(buffer);
		}
	}
}

#define compGT(a,b) (a < b)
void SortTeamScores(int *sortedindex,int *scores) 
{
    int t;
    int i, j;

	//copy scores into scoreinfo array
	for(i=0;i<DLLMAX_TEAMS;i++){
		sortedindex[i] = i;
	}

    for(i=1;i<=DLLMAX_TEAMS-1;i++){
		t=sortedindex[i];

	// Shift elements down until
	// insertion point found. 
	for(j=i-1;j>=0 && compGT(scores[sortedindex[j]],scores[t]); j--){
			sortedindex[j+1] = sortedindex[j];
		}
		  // insert
	sortedindex[j+1] = t;
    }
}
// DisplayHUDScores
//
//	Our callback function when it's time to render what we need on the HUD
void DisplayHUDScores(struct tHUDItem *hitem)
{
	// It's definitly ok to display the welcome message now
	// before (when we joined) it wasn't a good idea because the first
	// frame of the game hasn't been displayed yet, so internal variables (for
	// screen size) haven't been set yet, and so if we tried to display a HUD
	// message before the first frame, it would get clipped by a pretty much
	// random value (for screen size)....now it is ok, if we are scheduled to
	// display our welcome message
	if(display_my_welcome)
	{
		DisplayWelcomeMessage(DMFCBase->GetPlayerNum());
		display_my_welcome = false;
	}

	// Don't draw anything on the HUD if our F7 stats screen is up
	if(DisplayScoreScreen)
		return;
	
	int height = DLLgrfont_GetHeight((DMFCBase->GetGameFontTranslateArray())[HUD_FONT_INDEX]) + 3;

	// ConvertHUDAlpha takes the given alpha value, and if the F6 On-Screen menu is
	// up, it will lower it a little bit, to dim HUD items
	ubyte alpha = DMFCBase->ConvertHUDAlpha((ubyte)(255));
	int y = (DMFCBase->GetGameWindowH()/2) - ((height*5)/2);
	int x = 520;
	ddgr_color color;

	int i = 0;

	int rank[4];
	for(i=0; i<IRNumOfTeams; i++)
	{
		rank[i] = 0;
	}

	player_record *pr;

	//Display your Kills & Deaths on the top corners of the screen
	pr = DMFCBase->GetPlayerRecordByPnum(DMFCBase->GetPlayerNum());
	if(pr){
		int y = 25,x;
		int lwidth;
		char buffer[20];

		int w_kill,w_death,max_w;
		w_kill = DLLgrtext_GetTextLineWidth(TXT_KILLS);
		w_death = DLLgrtext_GetTextLineWidth(TXT_DEATHS);
		max_w = max(w_kill,w_death);

		x = DMFCBase->GetGameWindowW() - DMFCBase->GetGameWindowW()*0.0078125f;
		DLLgrtext_SetColor(GR_GREEN);
		DLLgrtext_SetAlpha(alpha);
		DLLgrtext_Printf(x-(max_w/2)-(w_kill/2),y,TXT_KILLS);
		y+=height;

		sprintf(buffer,"%d",pr->dstats.kills[DSTAT_LEVEL]);
		lwidth = DLLgrtext_GetTextLineWidth(buffer);
		DLLgrtext_SetColor(GR_GREEN);
		DLLgrtext_SetAlpha(alpha);
		DLLgrtext_Printf(x-(max_w/2)-(lwidth/2),y,buffer);
		y+=height+3;

		DLLgrtext_SetColor(GR_GREEN);
		DLLgrtext_SetAlpha(alpha);
		DLLgrtext_Printf(x - (max_w/2) - (w_death/2),y,TXT_DEATHS);
		y+=height;

		sprintf(buffer,"%d",pr->dstats.deaths[DSTAT_LEVEL]);
		lwidth = DLLgrtext_GetTextLineWidth(buffer);
		DLLgrtext_SetColor(GR_GREEN);
		DLLgrtext_SetAlpha(alpha);
		DLLgrtext_Printf(x - (max_w/2) - (lwidth/2),y,buffer);
	}

	int ESortedPlayers[DLLMAX_PLAYERS];

	switch(Anarchy_hud_display){
	case AHD_NONE:
		return;
		break;
	case AHD_EFFICIENCY:
		DMFCBase->GetSortedPlayerSlots(ESortedPlayers,DLLMAX_PLAYERS);
		break;
	}

	char name[30];

	//determine coordinates to use here
	//we'll use a virtual width of 85 pixels on a 640x480 screen
	//so first determine the new width
	int name_width = 85.0f * DMFCBase->GetHudAspectX();
	int score_width = DLLgrtext_GetTextLineWidth("888");
	int name_x = DMFCBase->GetGameWindowW() - name_width - score_width - 10;
	int score_x = DMFCBase->GetGameWindowW() - score_width - 5;


	int team_y[4];
	int tempy = y;
	for(i=0; i<IRNumOfTeams; i++){
		team_y[SortedTeams[i]] = tempy;
		tempy+=(height* 4);
	}
	//DMFCBase->GetPlayerTeam(pr->pnum)
	for(i=0; i<IRNumOfTeams; i++){
		pr = DMFCBase->GetPlayerRecord(DMFCBase->GetPlayerNum());
		
		if((pr)&&(pr->state!=STATE_EMPTY)){
			if(i == DMFCBase->GetPlayerTeam(pr->pnum))
			{
				color = (DMFCBase->GetTeamColor(i));
				if(Highlight_bmp>BAD_BITMAP_HANDLE){
					//draw the highlite bar in the background
					DLLrend_SetAlphaValue(alpha*0.50f);
					DLLrend_SetZBufferState (0);
					DLLrend_SetTextureType (TT_LINEAR);
					DLLrend_SetLighting (LS_NONE);
					DLLrend_SetAlphaType (AT_CONSTANT_TEXTURE);
					DLLrend_DrawScaledBitmap(name_x-2,team_y[i]-2,score_x+score_width+2,team_y[i]+height-1,Highlight_bmp,0,0,1,1,1.0);
					DLLrend_SetZBufferState (1);
				}
				
				strcpy(name,DMFCBase->GetTeamString(i));
				DMFCBase->ClipString(name_width,name,true);
			
				DLLgrtext_SetAlpha(alpha);
				DLLgrtext_SetColor(color);
				DLLgrtext_Printf(name_x,team_y[i],"%s",name);
				DLLgrtext_Printf(score_x,team_y[i],"%d",TeamScores[i]);
				
				team_y[i]+=height;
			}
			else
			{
				color = (DMFCBase->GetTeamColor(i));
		
				strcpy(name,DMFCBase->GetTeamString(i));
				DMFCBase->ClipString(name_width,name,true);
				
				DLLgrtext_SetAlpha(alpha);
				DLLgrtext_SetColor(color);
				DLLgrtext_Printf(name_x,team_y[i],"%s",name);
				DLLgrtext_Printf(score_x,team_y[i],"%d",TeamScores[i]);
				
				team_y[i]+=height;
			}
		}
	}

	for(i=0;i<DLLMAX_PLAYERS;i++){
		int slot;

		if(Anarchy_hud_display==AHD_EFFICIENCY)
			slot = ESortedPlayers[i];
		else
			slot = SortedPlayers[i];

		pr = DMFCBase->GetPlayerRecord(slot);
		
		if((pr)&&(pr->state!=STATE_EMPTY)){

			if(DMFCBase->IsPlayerDedicatedServer(pr))
				continue;	//skip dedicated server

			if( (pr->state==STATE_DISCONNECTED) || (pr->state==STATE_INGAME && !DMFCBase->IsPlayerObserver(pr->pnum)) ){
	
				if(pr->pnum==DMFCBase->GetPlayerNum() && rank[pr->team]<3){

					switch(HUD_color_model){
					case ACM_PLAYERCOLOR:
						color = (DMFCBase->GetTeamColor(pr->team));
						break;
					case ACM_NORMAL:
						color = GR_RGB(40,255,40);
						break;
					};					

					if(Highlight_bmp>BAD_BITMAP_HANDLE){
						//draw the highlite bar in the background
						DLLrend_SetAlphaValue(alpha*0.50f);
						DLLrend_SetZBufferState (0);
						DLLrend_SetTextureType (TT_LINEAR);
						DLLrend_SetLighting (LS_NONE);
						DLLrend_SetAlphaType (AT_CONSTANT_TEXTURE);
						DLLrend_DrawScaledBitmap(name_x-2,team_y[pr->team]-2,score_x+score_width+2,team_y[pr->team]+height-1,Highlight_bmp,0,0,1,1,1.0);
						DLLrend_SetZBufferState (1);
					}

					strcpy(name,pr->callsign);
					DMFCBase->ClipString(name_width,name,true);
						
					DLLgrtext_SetAlpha(alpha);
					DLLgrtext_SetColor(color);
					DLLgrtext_Printf(name_x,team_y[pr->team],"%s",name);

					if(Anarchy_hud_display==AHD_EFFICIENCY){
						float t = pr->dstats.kills[DSTAT_LEVEL]+pr->dstats.suicides[DSTAT_LEVEL]+pr->dstats.deaths[DSTAT_LEVEL];
						float value = (float)(pr->dstats.kills[DSTAT_LEVEL])/((t)?t:0.0000001f);
						DLLgrtext_Printf(score_x,team_y[pr->team],"%.1f",value);
					}else{
						DLLgrtext_Printf(score_x,team_y[pr->team],"%d",pr->dstats.kills[DSTAT_LEVEL]-pr->dstats.suicides[DSTAT_LEVEL]);
					}
					
					team_y[pr->team]+=height;
				}else
				if(rank[pr->team]<3){

					if(pr->state==STATE_DISCONNECTED){
						color = GR_GREY;
					}else{
						switch(HUD_color_model){
						case ACM_PLAYERCOLOR:
							color = (DMFCBase->GetTeamColor(pr->team));
							break;
						case ACM_NORMAL:
							color = GR_RGB(40,255,40);
							break;
						};
					}
					strcpy(name,pr->callsign);
					DMFCBase->ClipString(name_width,name,true);

					DLLgrtext_SetAlpha(alpha);
					DLLgrtext_SetColor(color);
					DLLgrtext_Printf(name_x,team_y[pr->team],"%s",name);

					if(Anarchy_hud_display==AHD_EFFICIENCY){
						float t = pr->dstats.kills[DSTAT_LEVEL]+pr->dstats.suicides[DSTAT_LEVEL]+pr->dstats.deaths[DSTAT_LEVEL];
						float value = (float)(pr->dstats.kills[DSTAT_LEVEL])/((t)?t:0.0000001f);
						DLLgrtext_Printf(score_x,team_y[pr->team],"%.1f",value);
					}else{
						DLLgrtext_Printf(score_x,team_y[pr->team],"%d",pr->dstats.kills[DSTAT_LEVEL]-pr->dstats.suicides[DSTAT_LEVEL]);
					}

					team_y[pr->team]+=height;
				}
				rank[pr->team]++;
			}
		}
	}
}

// DisplayWelcomeMessage
//
//	Given a player number, if it's us, then say welcome to the game, if it's
//	someone else, say that they have joined the game
void DisplayWelcomeMessage(int player_num)
{
	char name_buffer[64];
	strcpy(name_buffer,(DMFCBase->GetPlayers())[player_num].callsign);

	if(player_num==DMFCBase->GetPlayerNum())
	{
		DLLAddHUDMessage(TXT_WELCOME,name_buffer);
	}
	else
	{
		DLLAddHUDMessage(TXT_JOINED,name_buffer);
	}
}

// SwitchHUDColor
//	
//	Used by the On-Screen menu when the user has changed the HUD color style
void SwitchHUDColor(int i)
{
	if(i<0 || i>1)
		return;
	HUD_color_model = i;

	switch(HUD_color_model){
	case ACM_PLAYERCOLOR:
		DLLAddHUDMessage(TXT_MSG_COLORPLR);
		break;
	case ACM_NORMAL:
		DLLAddHUDMessage(TXT_MSG_COLORNORM);
		break;
	};
}

// SwitchAnarchyScores
//
//	Call this function to switch the HUD display mode for what
//  we display on the HUD (Player Scores/Player Efficiency/Nothing)
void SwitchAnarchyScores(int i)
{
	// clamp the values to make sure they are valid
	if(i<0)
		i = 0;
	if(i>2)
		i = 2;

	// Set the new HUD display mode
	Anarchy_hud_display = i;

	// Give a HUD message to the player telling them the HUD mode has changed
	switch(i){
	case AHD_NONE:
		DLLAddHUDMessage(TXT_HUDD_NONE);
		break;
	case AHD_SCORE:
		DLLAddHUDMessage(TXT_HUDD_SCORES);
		break;
	case AHD_EFFICIENCY:
		DLLAddHUDMessage(TXT_HUDD_EFFIC);
		break;
	};
}

// The main entry point where the game calls the dll
// Just translate the event, this function should always be the
// same, just pass control to DMFC
void DLLFUNCCALL DLLGameCall (int eventnum,dllinfo *data)
{
	// Filter out all server events from clients
	// this is important, or weird things will happen (a client
	// trying to preform server calls)
	if((eventnum<EVT_CLIENT_INTERVAL) && (DMFCBase->GetLocalRole()!=LR_SERVER)){
		return;
	}

	DMFCBase->TranslateEvent(eventnum,data);
}

/*
*****************************************
OSIRIS Section
	- Since no OSIRIS scripts needed for
Anarchy, they are all just stub functions
*****************************************
*/

//	GetGOScriptID
//	Purpose:
//		Given the name of the object (from it's pagename), this function will search through it's
//	list of General Object Scripts for a script with a matching name (to see if there is a script
//	for that type/id of object within this DLL).  If a matching scriptname is found, a UNIQUE ID
//	is to be returned back to Descent 3.  This ID will be used from here on out for all future
//	interaction with the DLL.  Since doors are not part of the generic object's, it's possible
//	for a door to have the same name as a generic object (OBJ_POWERUP, OBJ_BUILDING, OBJ_CLUTTER
//	or OBJ_ROBOT), therefore, a 1 is passed in for isdoor if the given object name refers to a
//	door, else it is a 0.  The return value is the unique identifier, else -1 if the script
//	does not exist in the DLL.
int DLLFUNCCALL GetGOScriptID(char *name,ubyte isdoor)
{
	return -1;
}

//	CreateInstance
//	Purpose:
//		Given an ID from a call to GetGOScriptID(), this function will create a new instance for that
//	particular script (by allocating and initializing memory, etc.).  A pointer to this instance
//	is to be returned back to Descent 3.  This pointer will be passed around, along with the ID
//	for CallInstanceEvent() and DestroyInstance().  Return NULL if there was an error.
void DLLFUNCCALLPTR CreateInstance(int id)
{
	return NULL;
}

//	DestroyInstance
//	Purpose:
//		Given an ID, and a pointer to a particular instance of a script, this function will delete and
//	destruct all information associated with that script, so it will no longer exist.
void DLLFUNCCALL DestroyInstance(int id,void *ptr)
{
}

//	CallInstanceEvent
//	Purpose:
//		Given an ID, a pointer to a script instance, an event and a pointer to the struct of
//	information about the event, this function will translate who this event belongs to and
//	passes the event to that instance of the script to be handled.  Return a combination of
//	CONTINUE_CHAIN and CONTINUE_DEFAULT, to give instructions on what to do based on the
//	event. CONTINUE_CHAIN means to continue through the chain of scripts (custom script, level
//	script, mission script, and finally default script).  If CONTINUE_CHAIN is not specified,
//	than the chain is broken and those scripts of lower priority will never get the event.  Return
//	CONTINUE_DEFAULT in order to tell D3 if you want process the normal action that is built into
//	the game for that event.  This only pertains to certain events.  If the chain continues
//	after this script, than the CONTINUE_DEFAULT setting will be overridden by lower priority
//	scripts return value.
short DLLFUNCCALL CallInstanceEvent(int id,void *ptr,int event,tOSIRISEventInfo *data)
{
	return CONTINUE_CHAIN|CONTINUE_DEFAULT;
}

//	SaveRestoreState
//	Purpose:
//		This function is called when Descent 3 is saving or restoring the game state.  In this function
//	you should save/restore any global data that you want preserved through load/save (which includes
//	demos).  You must be very careful with this function, corrupting the file (reading or writing too
//	much or too little) may be hazardous to the game (possibly making it impossible to restore the
//	state).  It would be best to use version information to keep older versions of saved states still
//	able to be used.  IT IS VERY IMPORTANT WHEN SAVING THE STATE TO RETURN THE NUMBER OF _BYTES_ WROTE
//	TO THE FILE.  When restoring the data, the return value is ignored.  saving_state is 1 when you should
//	write data to the file_ptr, 0 when you should read in the data.
int DLLFUNCCALL SaveRestoreState( void *file_ptr, ubyte saving_state )
{
	return 0;
}

#ifdef MACINTOSH
#pragma export off
#endif
