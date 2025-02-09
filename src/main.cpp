#include <xtl.h>
#include <cstdint>
#include <string>
#include <fstream>

#include "detour.h"
#include "structs.h"

extern "C"
{
	long DbgPrint(const char *format, ...);

	uint32_t XamGetCurrentTitleId();

	uint32_t ExCreateThread(
		HANDLE *pHandle,
		uint32_t stackSize,
		uint32_t *pThreadId,
		void *pApiThreadStartup,
		PTHREAD_START_ROUTINE pStartAddress,
		void *pParameter,
		uint32_t creationFlags);
}

void *ResolveFunction(const std::string &moduleName, uint32_t ordinal)
{
	HMODULE moduleHandle = GetModuleHandle(moduleName.c_str());
	if (moduleHandle == nullptr)
		return nullptr;

	return GetProcAddress(moduleHandle, reinterpret_cast<const char *>(ordinal));
}

typedef void (*XNOTIFYQUEUEUI)(uint32_t type, uint32_t userIndex, uint64_t areas, const wchar_t *displayText, void *pContextData);
XNOTIFYQUEUEUI XNotifyQueueUI = static_cast<XNOTIFYQUEUEUI>(ResolveFunction("xam.xex", 656));

namespace game
{
	unsigned int TITLE_ID = 0x4156081C;

	const int CONTENTS_PLAYERCLIP = 0x10000;

	// Game functions
	va_t va = reinterpret_cast<va_t>(0x822C38D8);

	CG_GameMessage_t CG_GameMessage = reinterpret_cast<CG_GameMessage_t>(0x8216EC68);
	CG_BoldGameMessage_t CG_BoldGameMessage = reinterpret_cast<CG_BoldGameMessage_t>(0x8216EC88);

	Scr_ReadFile_FastFile_t Scr_ReadFile_FastFile = reinterpret_cast<Scr_ReadFile_FastFile_t>(0x82339EF8);
	Scr_GetFunction_t Scr_GetFunction = reinterpret_cast<Scr_GetFunction_t>(0x822416B0);
	Scr_GetVector_t Scr_GetVector = reinterpret_cast<Scr_GetVector_t>(0x8234B790);
	Scr_GetInt_t Scr_GetInt = reinterpret_cast<Scr_GetInt_t>(0x8234AFD0);
	Scr_AddInt_t Scr_AddInt = reinterpret_cast<Scr_AddInt_t>(0x82345668);

	// knockback handler
	sub_8220D2D0_t sub_8220D2D0 = reinterpret_cast<sub_8220D2D0_t>(0x8220D2D0);

	// Variables
	auto cm = reinterpret_cast<clipMap_t *>(0x82DD4F80);
}

Detour sub_8220D2D0_Detour;

game::gentity_s *sub_8220D2D0_Hook(game::gentity_s *ent, int a2, float *a3, int a4, int a5)
{

	auto velocity = ent->client->velocity;

	const float g_knockback_value = 1000.0f;
	const float SCALING_FACTOR = 0.0040000002f;
	const float knockback_intensity = 60.0f;

	float velocity_scale = g_knockback_value * knockback_intensity * SCALING_FACTOR;

	// Apply knockback to velocity
	velocity[0] += a3[0] * velocity_scale;
	velocity[1] += a3[1] * velocity_scale;
	velocity[2] += a3[2] * velocity_scale;

	return ent;
}

bool is_point_inside_bounds(const float point[3], const float mins[3], const float maxs[3])
{
	return (
		point[0] >= mins[0] && point[0] <= maxs[0] &&		   // X within bounds
		point[1] >= mins[1] && point[1] <= maxs[1] &&		   // Y within bounds
		((point[2] >= mins[2] && point[2] <= maxs[2]) ||	   // Z within bounds
		 (point[2] - 1 >= mins[2] && point[2] - 1 <= maxs[2])) // OR (Z - 1) within bounds
	);
}

// TODO: find appropriate name for this
void GScr_DisablePlayerClipForBrushesContainingPoint()
{
	float point[3] = {0.0f, 0.0f, 0.0f};
	game::Scr_GetVector(0, point, 0, -1);

	bool found = false;

	for (int i = 0; i < game::cm->numBrushes; i++)
	{
		auto &brush = game::cm->brushes[i];
		if (
			(brush.contents & game::CONTENTS_PLAYERCLIP) && // Check if brush has player clip flag first, cheaper than is_point_inside_brush
			is_point_inside_bounds(point, brush.mins, brush.maxs))
		{
			brush.contents &= ~game::CONTENTS_PLAYERCLIP;
			found = true;
			game::CG_GameMessage(0, game::va("brush %d collision ^1disabled", i));
		}
	}

	if (!found)
	{
		game::CG_GameMessage(0, "No brush with player clip found at this point.");
	}
}

void GScr_ReadInt()
{
	// TODO: add error handling
	auto address = game::Scr_GetInt(0, 0, 0, -1);
	auto value = *reinterpret_cast<int *>(address);
	game::Scr_AddInt(value, 0);
}

void GScr_WriteInt()
{
	// TODO: add error handling
	auto address = game::Scr_GetInt(0, 0, 0, -1);
	auto value = game::Scr_GetInt(1, 0, 0, -1);

	*reinterpret_cast<int *>(address) = value;
}

Detour Scr_GetFunction_Detour;

game::BuiltinFunctionPtr Scr_GetFunction_Hook(const char **pName, int *type)
{
	if (pName != nullptr)
	{
		if (std::strcmp(*pName, "disableplayerclipforbrushescontainingpoint") == 0)
		{
			return reinterpret_cast<game::BuiltinFunctionPtr>(&GScr_DisablePlayerClipForBrushesContainingPoint);
		}
		else if (std::strcmp(*pName, "readint") == 0)
		{
			return reinterpret_cast<game::BuiltinFunctionPtr>(&GScr_ReadInt);
		}
		else if (std::strcmp(*pName, "writeint") == 0)
		{
			return reinterpret_cast<game::BuiltinFunctionPtr>(&GScr_WriteInt);
		}
	}

	return Scr_GetFunction_Detour.GetOriginal<decltype(&Scr_GetFunction_Hook)>()(pName, type);
}

std::string replaceSlashes(const char *path)
{
	std::string str(path);
	for (size_t i = 0; i < str.length(); ++i)
	{
		if (str[i] == '/')
		{
			str.replace(i, 1, "\\");
			++i; // Skip the next index to avoid replacing already inserted backslashes
		}
	}
	return str;
}

Detour Scr_ReadFile_FastFile_Detour;

char *Scr_ReadFile_FastFile_Hook(int a1, int a2, char *extFilename, int a4, char a5)
{
	// Load shadowed files from mod folder
	char *path = game::va("game:\\mod\\%s", replaceSlashes(extFilename).c_str());
	std::ifstream file(path, std::ios::ate);
	if (file.is_open())
	{
		DbgPrint("[PLUGIN] Scr_ReadFile_FastFile: Loading script from mod folder: %s\n", path);

		size_t fileSize = static_cast<size_t>(file.tellg());
		char *fileContents = new char[fileSize + 1];

		file.seekg(0, std::ios::beg);
		file.read(fileContents, fileSize);
		fileContents[fileSize] = '\0';

		file.close();

		return fileContents;
	}

	return Scr_ReadFile_FastFile_Detour.GetOriginal<decltype(&Scr_ReadFile_FastFile_Hook)>()(a1, a2, extFilename, a4, a5);
}

void patch_out_rocket_jump_fix()
{
	// Weapon_RocketLauncher_Fire
	*(volatile uint32_t *)0x8225F98C = 0x60000000;
	*(volatile uint32_t *)0x8225F990 = 0x60000000;
}

void patch_out_idle_weapon_sway()
{
	// NOP out the call to BG_CalculateView_IdleAngles inside BG_CalculateViewAngles
	*(volatile uint32_t *)0x8214804C = 0x60000000;

	// NOP out the call to BG_CalculateWeaponPosition_IdleAngles inside BG_CalculateWeaponAngles
	*(volatile uint32_t *)0x8214789C = 0x60000000;
}

void PluginMain()
{
	Sleep(500);
	XNotifyQueueUI(0, 0, XNOTIFY_SYSTEM, L"T4 CodJumper Loaded", nullptr);

	patch_out_rocket_jump_fix();

	patch_out_idle_weapon_sway();

	Scr_ReadFile_FastFile_Detour = Detour(game::Scr_ReadFile_FastFile, Scr_ReadFile_FastFile_Hook);
	Scr_ReadFile_FastFile_Detour.Install();

	Scr_GetFunction_Detour = Detour(game::Scr_GetFunction, Scr_GetFunction_Hook);
	Scr_GetFunction_Detour.Install();

	sub_8220D2D0_Detour = Detour(game::sub_8220D2D0, sub_8220D2D0_Hook);
	sub_8220D2D0_Detour.Install();
}

void MonitorTitleId(void *pThreadParameter)
{
	for (;;)
	{
		if (XamGetCurrentTitleId() == game::TITLE_ID)
		{
			PluginMain();
			break;
		}
		else
			Sleep(100);
	}
}

int DllMain(HANDLE hModule, DWORD reason, void *pReserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		ExCreateThread(nullptr, 0, nullptr, nullptr, reinterpret_cast<PTHREAD_START_ROUTINE>(MonitorTitleId), nullptr, 2);
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
