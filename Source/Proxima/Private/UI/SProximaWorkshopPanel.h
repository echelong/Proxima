#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "BuildMode/ProximaWorkshopComponent.h"
class SBox;

class SProximaWorkshopPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SProximaWorkshopPanel) {}
        SLATE_ARGUMENT(UProximaWorkshopComponent*, Workshop)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    bool IsPointerOverPanel() const;
private:
    TWeakObjectPtr<UProximaWorkshopComponent> Workshop;
    TSharedPtr<SBox> SidePanel;
    TSharedRef<SWidget> ToolButton(EProximaBuildTool Tool, const FString& Label);
    TSharedRef<SWidget> Action(const FString& Label, TFunction<void(UProximaWorkshopComponent&)> Callback);
    TSharedRef<SWidget> Field(const FString& Label, FName Key, TFunction<float(const UProximaWorkshopComponent&)> Getter);
};
