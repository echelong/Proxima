#include "Building/ProximaIdentifiers.h"

FProximaID FProximaID::NewId()
{
    FProximaID NewId;
    NewId.Value = FGuid::NewGuid();
    return NewId;
}
