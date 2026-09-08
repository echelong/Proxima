#include "Save/ProximaSerializationUtility.h"

bool UProximaSerializationUtility::ValidateSaveVersion(int32 Version)
{
    // V1 has no Slabs property; Unreal initializes the missing array empty.
    return Version == 1 || Version == CurrentSaveFormatVersion;
}
