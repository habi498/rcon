#include "main.h"
#ifdef WIN32
#define EXPORT __declspec(dllexport)
#else 
#define EXPORT
#endif
PluginFuncs* VCMP;
HSQUIRRELVM v = NULL; HSQAPI sq = NULL;
char* rcon_password = NULL;
bool rcon_init = false;
bool fsplugin = false;
SQInteger SendRconCommand(HSQUIRRELVM v)
{
	const SQChar* command;
	sq->getstring(v, 2, &command); char reply[1024];
	char* buffer = new char[strlen(command) + 1];
	if (buffer)
	{
		memcpy(buffer, command, strlen(command) + 1);
		OnRCONCommand(buffer, "script", reply, sizeof(reply));
		sq->pushstring(v, reply, strlen(reply));
		delete[] buffer;
		return 1;
	}
}
uint8_t RCN_OnPluginCommand (uint32_t commandIdentifier, const char* message)
{
	if (commandIdentifier == 0x7D6E22D8)
	{
		int32_t id=VCMP->FindPlugin("SQHost2");
		if (id >= 0)
		{
			size_t size;
			const void** exports=VCMP->GetPluginExports(id, &size);
			if (VCMP->GetLastError() != vcmpErrorNoSuchEntity && size > 0)
			{
				SquirrelImports** _imports =(SquirrelImports**) exports;
				SquirrelImports* imports = (SquirrelImports*)(*_imports);
				if (imports)
				{
					v = *imports->GetSquirrelVM();
					sq = *imports->GetSquirrelAPI();
					if (sq && v)
					{
						int top = sq->gettop(v);
						sq->pushroottable(v);
						sq->pushstring(v, "SendRconCommand", -1);
						sq->newclosure(v, SendRconCommand, 0);
						sq->setparamscheck(v, 2, "ts");
						sq->newslot(v, -3, 0);
						sq->settop(v, top);
					}
				}
			}
		}
	}
	else if (commandIdentifier == 0x2A1A3D09)
	{
		char* buffer = new char[strlen(message) + 1];
		char reply[1024];
		if (buffer)
		{
			memcpy(buffer, message, strlen(message) + 1);
			OnRCONCommand(buffer, "script", reply, sizeof(reply));
			delete[] buffer;
			return 1;
		}
	}
	return 1;
}

uint8_t RCN_OnPlayerCommand(int32_t playerId, const char* message)
{
	char* msg = new char[strlen(message) + 1];
	memcpy(msg, message, strlen(message) + 1);
	char* cmd = strtok(msg, " "); char* param = NULL;
	if (cmd != NULL)
		param = msg + strlen(cmd) + 1; 
	if (cmd && strcmp(cmd, "rcon") == 0 && param[0]=='l'
		&&param[1]=='o'&&param[2]=='g'&&param[3]=='i'&&param[4]=='n')
	{
		if (VCMP->IsPlayerAdmin(playerId))
		{
			VCMP->SendClientMessage(playerId, 0x148031FF, "[RCON]: You are already admin");
			return 1;
		}
		if (strlen(message) == 10)
		{
			VCMP->SendClientMessage(playerId, 0x148031FF, "[RCON]: Password not provided");
			return 1;
		}
		const char* password = message + 11;
		char ip[30]; VCMP->GetPlayerIP(playerId, ip, sizeof(ip));
		if (strcmp(rcon_password, password) == 0)
		{
			VCMP->SetPlayerAdmin(playerId, 1);
			VCMP->SendClientMessage(playerId, 0x148031FF, "[RCON]: You are now logged in");
		}
		else {
			VCMP->SendClientMessage(playerId, 0x148031FF, "[RCON]: Invalid password");
			OnRCONLoginFailed(ip);
		}
	}
	else if (cmd && strcmp(cmd, "rcon") == 0&&strlen(message)>5)
	{
		if (VCMP->IsPlayerAdmin(playerId))
		{
			char* buffer = new char[strlen(message + 5) + 1];
			if (buffer)
			{
				memcpy(buffer, message + 5, strlen(message + 5) + 1);
				char reply[1024]; char plrIP[25];
				VCMP->GetPlayerIP(playerId, plrIP, sizeof(plrIP));

				OnRCONCommand(buffer, plrIP, reply, sizeof(reply));
				VCMP->SendClientMessage(playerId, 0x148031FF, reply);
				delete[] buffer;
			}
		}else VCMP->SendClientMessage(playerId, 0x148031FF, "[RCON]: Not logged in");
	}
	else if ((cmd && strcmp(cmd, "rcon") == 0) || strcmp(message, "rcon") == 0)
	{
		if(!VCMP->IsPlayerAdmin(playerId))
			VCMP->SendClientMessage(playerId, 0x148031FF, "[RCON]: Use /rcon login pass");
	}
	return 1;
}
uint8_t RCN_OnServerInitialise (void)
{
	int32_t id=VCMP->FindPlugin("FilterScripts");
	if (id != -1)
	{
		fsplugin = true;
	}
	return 1;
}
extern "C" EXPORT unsigned int VcmpPluginInit(PluginFuncs * Funcs, PluginCallbacks * Calls, PluginInfo * Info)
{
	VCMP = Funcs;
	Info->pluginVersion = 0x0;
	Info->apiMinorVersion = PLUGIN_API_MINOR;
	Info->apiMajorVersion = PLUGIN_API_MAJOR;
	memcpy(Info->name, "rcon", 5);
	Calls->OnPluginCommand = RCN_OnPluginCommand;
	Calls->OnPlayerCommand = RCN_OnPlayerCommand;
	Calls->OnServerInitialise = RCN_OnServerInitialise;
	//Read RCON Password from server.cfg
	cfg rconpassword_cfg; 
	rconpassword_cfg.read("server.cfg", "rcon_password");
	if (rconpassword_cfg.argc > 0)
	{
		//rcon_password provided in server.cfg
		short len = strlen(rconpassword_cfg.ptr[0]);
		rcon_password = (char*)malloc(len + 1);
		if (rcon_password)
		{
			memcpy(rcon_password, rconpassword_cfg.ptr[0], len + 1);
			rcon_init = true;
			if (rcon_install_hooks())
			{
				return 1;
			}
			
			
		}
		else printf("Failed when allocating memory");
	}
	else {
		printf("RCON: No password provided in server.cfg. Provide one by rcon_password passwd\n");
	}
	return 1;
}