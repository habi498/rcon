#include "main.h"
#include <string>
extern PluginFuncs* VCMP;
extern HSQUIRRELVM v ; 
extern HSQAPI sq;
extern char* rcon_password;
extern bool fsplugin;
void OnRCONCommand(char* buffer, char* ip, char* reply, int replylen)
{
	 char name[30]; reply[0] = 0;
	char* cmd = strtok(buffer, " "); char*  param=NULL;
	if (cmd != NULL)
		param = buffer + strlen(cmd) + 1;
	if (strcmp(buffer, "players") == 0)
	{
		int m = VCMP->GetMaxPlayers(), j = 0;
		for (int i = 0; i < m; i++)
		{
			if (VCMP->IsPlayerConnected(i))
			{
				VCMP->GetPlayerName(i, name, 30);
				sprintf(reply + j, "%d %s\n", i, name);
				j = strlen(reply);
			}
		}
	}
	else if (cmd && param && strcmp(cmd, "say") == 0)
	{
		VCMP->SendClientMessage(-1, 0x148031FF, "Admin: %s", param);
		//printf("Admin: %s", param);
	}
	else if (cmd && param && strcmp(cmd, "kick") == 0)
	{
		try
		{
			int converted = std::stoi(param);
			if (VCMP->IsPlayerConnected((unsigned int)converted))
			{
				VCMP->KickPlayer((unsigned int)converted);
			}
		}
		catch (...)
		{
			int id = VCMP->GetPlayerIdFromName(param);
			if (id != -1)
				VCMP->KickPlayer(id);
		}
	}
	else if (cmd && param && strcmp(cmd, "ban") == 0)
	{
		try
		{
			int converted = std::stoi(param);
			if (VCMP->IsPlayerConnected((unsigned int)converted))
			{
				VCMP->BanPlayer((unsigned int)converted);
			}
		}
		catch (...)
		{
			int id = VCMP->GetPlayerIdFromName(param);
			if (id != -1)
				VCMP->BanPlayer(id);
		}
	}
	else if (cmd && param && strcmp(cmd, "banip") == 0)
	{
		VCMP->BanIP(param);
	}
	else if (cmd && param && strcmp(cmd, "unbanip") == 0)
	{
		VCMP->UnbanIP(param);
	}
	else if (cmd && param && strcmp(cmd, "echo") == 0)
	{
		printf("%s\n", param);
		memcpy(reply, param, strlen(param)+1);
	}
	else if (cmd && param && strcmp(cmd, "hostname") == 0)
	{
		VCMP->SetServerName(param);
	}
	else if (cmd && param && strcmp(cmd, "gamemodetext") == 0)
	{
		VCMP->SetGameModeText(param);
	}
	else if (cmd && param && strcmp(cmd, "gravity") == 0)
	{
		try
		{
			float f = std::stof(param);
			VCMP->SetGravity(f);
		}
		catch (...)
		{

		}
	}
	else if (cmd && param && strcmp(cmd, "weather") == 0)
	{
		try
		{
			int id = std::stoi(param);
			VCMP->SetWeather(id);
		}
		catch (...)
		{

		}
	}
	else if (cmd && param && strcmp(cmd, "password") == 0)
	{
		if (strcmp(param, "0") == 0)VCMP->SetServerPassword("");
		else VCMP->SetServerPassword(param);
	}
	else if (cmd && param && strcmp(cmd, "rcon_password") == 0)
	{

		/*if (strcmp(param, "0") == 0)
			rcon_password = "";
		else */
		{
			if (strlen(param) <= strlen(rcon_password))
				memcpy(rcon_password, param, strlen(param) + 1);
			else
			{
				char* temp = (char*)realloc(rcon_password, strlen(param) + 1);
				if (temp)
				{
					rcon_password = temp;
					memcpy(rcon_password, param, strlen(param) + 1);
				}
			}
			sprintf(reply, "rcon_password %s", rcon_password);
		}
	}
	else if (cmd && param && strcmp(cmd, "exec") == 0)
	{
		if (sq && v)
		{
			int top = sq->gettop(v);
			sq->compilebuffer(v, param, strlen(param), "rcon", 1);
			sq->pushroottable(v);
			if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
			{
				if (sq->gettype(v, -1) != OT_NULL)
				{
					if (SQ_SUCCEEDED(sq->tostring(v, -1)))
					{
						const SQChar* result;
						sq->getstring(v, -1, &result);
						memcpy(reply, result, strlen(result) + 1);
					}
				}
			}
			else
			{
				sq->getlasterror(v);
				if (SQ_SUCCEEDED(sq->tostring(v, -1)))
				{
					const SQChar* result;
					sq->getstring(v, -1, &result);
					memcpy(reply, result, strlen(result) + 1);
				}
			}
			sq->settop(v, top);
		}
	}
	else if (strcmp(buffer,"exit") == 0)
	{
		VCMP->ShutdownServer();
	}
	else if (cmd && param &&strcmp(cmd, "plgncmd") == 0)
	{
		char* identifier = strtok(NULL, " ");
		if (strtok(NULL,""))
		{
			char* msg = identifier + strlen(identifier) + 1;
			try
			{
				unsigned int id = std::stoul(identifier);
				VCMP->SendPluginCommand(id, msg);
			}
			catch (...)
			{

			}
	
		}
	}
	else if (strcmp(buffer, "cmdlist") == 0)
	{
	char* fscmds = ", loadfs, unloadfs, reloadfs";
	sprintf(reply, "echo, hostname, gamemodetext, kick, ban, say,\
players, banip, unbanip, gravity, weather, rcon_password, password, \
exit, plgncmd, exec");
	if (fsplugin)
		memcpy(reply + strlen(reply), fscmds, strlen(fscmds) + 1);
	}
	else if (fsplugin && cmd && param && strcmp(cmd, "loadfs") == 0)
	{
		VCMP->SendPluginCommand(0x2A1A3C4D, param);
	}
	else if (fsplugin && cmd && param && strcmp(cmd, "unloadfs") == 0)
	{
	VCMP->SendPluginCommand(0x2A1A3C4E, param);
	}
	else if (fsplugin && cmd && param && strcmp(cmd, "reloadfs") == 0)
	{
	VCMP->SendPluginCommand(0x2A1A3C4F, param);
	}
	else 
	{
		if (sq && v)
		{
			int top = sq->gettop(v);
			sq->pushroottable(v);
			sq->pushstring(v, "onRconCommand", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushroottable(v);
				sq->pushstring(v, buffer, -1);
				sq->pushstring(v, ip, -1);
				sq->call(v, 3, 1, 1);
				if (sq->gettype(v, -1)==OT_STRING)
				{
					const SQChar* result;
					sq->getstring(v, -1, &result);
					memcpy(reply, result, strlen(result) + 1);
				}
			}
			sq->settop(v, top);
		}else
		{
			char* str = new char[strlen(buffer) + strlen(ip) + 2];
			if (str)
			{
				sprintf(str, "%s %s", buffer, ip);
				VCMP->SendPluginCommand(0x2A1A3D02, str);
				delete[] str;
			}
		}
	}
	
}
void OnRCONLoginFailed(char* ip)
{
	if (sq && v)
	{
		int top = sq->gettop(v);
		sq->pushroottable(v);
		sq->pushstring(v, "onRconLoginFailed", -1);
		if (SQ_SUCCEEDED(sq->get(v, -2)))
		{
			sq->pushroottable(v);
			sq->pushstring(v, ip, -1);
			sq->call(v, 2, 0, 1);
		}
		sq->settop(v, top);
	}
	else
	{
		VCMP->SendPluginCommand(0x2A1A3D01, ip);
	}
}