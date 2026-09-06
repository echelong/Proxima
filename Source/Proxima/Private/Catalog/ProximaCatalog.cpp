#include "Catalog/ProximaCatalog.h"

TArray<FProximaCatalogMaterialEntry> UProximaCatalog::GetMaterialsByCategory(FName Category) const
{
    TArray<FProximaCatalogMaterialEntry> Results;
    for (const FProximaCatalogMaterialEntry& Entry : MaterialEntries)
    {
        if (Entry.Category == Category)
        {
            Results.Add(Entry);
        }
    }
    return Results;
}

bool UProximaCatalog::FindMaterial(const FProximaCatalogID& Id, FProximaCatalogMaterialEntry& OutEntry) const
{
    if (!Id.IsValid())
    {
        return false;
    }

    for (const FProximaCatalogMaterialEntry& Entry : MaterialEntries)
    {
        if (Entry.CatalogId.Value == Id.Value)
        {
            OutEntry = Entry;
            return true;
        }
    }
    return false;
}
