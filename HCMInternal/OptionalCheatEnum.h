#pragma once

// MUST HAVE SAME NAME AS ASSOCIATED CLASS (the macro will be used in a std::variant and etc to convert from enum to type)
#define ALLOPTIONALCHEATS 	\
TogglePause,\
AdvanceTicks,\
ToggleBlockInput,\
ToggleFreeCursor,\
ForceCheckpoint,\
ForceRevert,\
ForceDoubleRevert,\
ForceCoreSave,\
ForceCoreLoad,\
IgnoreCheckpointChecksum,\
InjectCheckpoint,\
DumpCheckpoint,\
InjectCore,\
DumpCore,\
Speedhack,\
Invulnerability,\
GetCurrentDifficulty,\
GetPlayerDatum,\
GetObjectAddress,\
GetCurrentLevelCode,\
AIFreeze,\
ConsoleCommand\


enum class OptionalCheatEnum {
	ALLOPTIONALCHEATS
};

