#include <cstddef>

namespace game
{

	struct cplane_s
	{
		float normal[3];
		float dist;
		unsigned __int8 type;
		unsigned __int8 signbits;
		unsigned __int8 pad[2];
	};

	struct __declspec(align(2)) cbrushside_t
	{
		cplane_s *plane;
		unsigned int materialNum;
		__int16 firstAdjacentSideOffset;
		unsigned __int8 edgeCount;
	};

#pragma warning(push)
#pragma warning(disable : 4324)

	struct __declspec(align(16)) cbrush_t
	{
		float mins[3];
		int contents;
		float maxs[3];
		unsigned int numsides;
		cbrushside_t *sides;
		__int16 axialMaterialNum[2][3];
		unsigned __int8 *baseAdjacentSide;
		__int16 firstAdjacentSideOffsets[2][3];
		unsigned __int8 edgeCount[2][3];
	};

#pragma warning(pop)

	struct gclient_s
	{
		char pad_0000[44];	  // Padding to reach offset 44
		float velocity[3];	  // Offset 44
		char pad_0056[15076]; // Padding to reach offset 15132
		int noclip;			  // Offset 15132
		int ufo;			  // Offset 15136
	};

	struct gentity_s
	{
		char pad_0000[388];
		gclient_s *client; // Offset 388
		char pad_0188[44]; //
		int flags;		   // Offset 436
		char pad_01BC[816 - 440];
	};
	static_assert(sizeof(gentity_s) == 816, "");

	struct clipMap_t
	{
		const char *name;
		int isInUse;
		char _pad[0x94];
		unsigned __int16 numBrushes;
		cbrush_t *brushes;
	};
	static_assert(offsetof(clipMap_t, numBrushes) == 156, "");

	typedef void(__fastcall *BuiltinFunctionPtr)();

	typedef void (*CG_GameMessage_t)(int localClientNum, const char *msg);
	typedef void (*CG_BoldGameMessage_t)(int localClientNum, const char *msg);

	typedef char *(*va_t)(char *format, ...);

	typedef char *(*Scr_ReadFile_FastFile_t)(const char *filename, const char *extFilename, const char *codePos, bool archive);
	typedef BuiltinFunctionPtr (*Scr_GetFunction_t)(const char **pName, int *type);
	typedef void (*Scr_GetVector_t)(unsigned int result, float *a2, __int64 a3, __int64 a4);
	typedef int (*Scr_GetInt_t)(unsigned int a1, int a2, __int64 a3, __int64 a4);
	typedef void (*Scr_AddInt_t)(int value, int a2);

	typedef game::gentity_s *(*sub_8220D2D0_t)(game::gentity_s *result, int a2, float *a3, char a4, char a5);
}
