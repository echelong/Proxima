#include "Save/ProximaSerializationUtility.h"

bool UProximaSerializationUtility::ValidateSaveVersion(int32 Version)
{
    return Version == CurrentSaveFormatVersion;
}
