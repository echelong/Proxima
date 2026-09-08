#include "UI/SProximaWorkshopPanel.h"
#include "Core/ProximaPlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
FLinearColor Ink(0.055f, 0.068f, 0.067f, 0.98f);
FLinearColor Paper(0.92f, 0.91f, 0.85f);
FLinearColor Mint(0.19f, 0.63f, 0.46f);
TSharedRef<STextBlock> Caption(const FString& Text)
{
    return SNew(STextBlock).Text(FText::FromString(Text)).ColorAndOpacity(Paper)
        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 11));
}
}

TSharedRef<SWidget> SProximaWorkshopPanel::Action(const FString& Label, TFunction<void(UProximaWorkshopComponent&)> Callback)
{
    const TWeakObjectPtr<UProximaWorkshopComponent> Weak = Workshop;
    return SNew(SButton).IsFocusable(false).ButtonColorAndOpacity(FLinearColor(0.15f, 0.19f, 0.18f)).ContentPadding(FMargin(8, 7))
        .OnClicked_Lambda([Weak, Callback]()
        {
            if (Weak.IsValid()) { Callback(*Weak.Get()); }
            return FReply::Handled();
        })[Caption(Label)];
}
TSharedRef<SWidget> SProximaWorkshopPanel::ToolButton(EProximaBuildTool Tool, const FString& Label)
{
    const TWeakObjectPtr<UProximaWorkshopComponent> Weak = Workshop;
    return SNew(SButton).IsFocusable(false).ContentPadding(FMargin(8, 8))
        .ButtonColorAndOpacity_Lambda([Weak, Tool]() { return Weak.IsValid() && Weak->GetTool() == Tool ? Mint : FLinearColor(0.18f, 0.22f, 0.21f); })
        .OnClicked_Lambda([Weak, Tool]() { if (Weak.IsValid()) { Weak->SelectTool(Tool); } return FReply::Handled(); })
        [Caption(Label)];
}
TSharedRef<SWidget> SProximaWorkshopPanel::Field(const FString& Label, FName Key, TFunction<float(const UProximaWorkshopComponent&)> Getter)
{
    const TWeakObjectPtr<UProximaWorkshopComponent> Weak = Workshop;
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(0, 7, 0, 3)[Caption(Label)]
        + SVerticalBox::Slot().AutoHeight()
        [SNew(SEditableTextBox)
            .Text_Lambda([Weak, Getter]() { return FText::FromString(Weak.IsValid() ? FString::Printf(TEXT("%.2f m"), Getter(*Weak.Get()) / 100.0f) : FString()); })
            .SelectAllTextWhenFocused(true)
            .OnTextCommitted_Lambda([Weak, Key](const FText& Text, ETextCommit::Type Commit)
            {
                if (Weak.IsValid()) { Weak->SetDimension(Key, Text.ToString()); }
                if (Commit == ETextCommit::OnEnter && FSlateApplication::IsInitialized()) { FSlateApplication::Get().SetAllUserFocusToGameViewport(); }
            })];
}
bool SProximaWorkshopPanel::IsPointerOverPanel() const
{
    return SidePanel.IsValid() && SidePanel->IsHovered();
}
void SProximaWorkshopPanel::Construct(const FArguments& Args)
{
    Workshop = Args._Workshop;
    const TWeakObjectPtr<UProximaWorkshopComponent> Weak = Workshop;
    const auto Show = [Weak](std::initializer_list<EProximaBuildTool> Tools)
    {
        if (!Weak.IsValid()) { return EVisibility::Collapsed; }
        for (const auto Tool : Tools) { if (Weak->GetTool() == Tool) { return EVisibility::Visible; } }
        return EVisibility::Collapsed;
    };
    ChildSlot
    [SNew(SOverlay).Visibility(EVisibility::SelfHitTestInvisible)
        + SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Fill).Padding(16, 16, 0, 104)
        [SAssignNew(SidePanel, SBox).WidthOverride(292)
            .Visibility_Lambda([Weak]() { return Weak.IsValid() && Weak->IsBuildModeActive() ? EVisibility::Visible : EVisibility::Collapsed; })
            [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Ink).Padding(16)
                [SNew(SScrollBox)
                    + SScrollBox::Slot()
                    [SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("P R O X I M A")))
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 21)).ColorAndOpacity(Paper)]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 12)[Caption(TEXT("A place to make your own"))]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
                        [SNew(STextBlock)
                            .Text_Lambda([Weak]()
                            {
                                return FText::FromString(
                                    Weak.IsValid()
                                        ? Weak->GetActiveLevelLabel()
                                        : FString());
                            })
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
                            .ColorAndOpacity(Mint)]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
                        [SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 3, 0)
                            [SNew(SButton)
                                .IsFocusable(false)
                                .ContentPadding(FMargin(5, 6))
                                .ButtonColorAndOpacity_Lambda([Weak]()
                                {
                                    return Weak.IsValid() &&
                                           Weak->GetActiveLevelIndex() == 0
                                        ? Mint
                                        : FLinearColor(0.18f, 0.22f, 0.21f);
                                })
                                .OnClicked_Lambda([Weak]()
                                {
                                    if (Weak.IsValid())
                                    {
                                        Weak->SetActiveLevel(0);
                                    }

                                    return FReply::Handled();
                                })
                                [Caption(TEXT("LEVEL 1"))]]
                            + SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 3, 0)
                            [SNew(SButton)
                                .IsFocusable(false)
                                .ContentPadding(FMargin(5, 6))
                                .ButtonColorAndOpacity_Lambda([Weak]()
                                {
                                    return Weak.IsValid() &&
                                           Weak->GetActiveLevelIndex() == 1
                                        ? Mint
                                        : FLinearColor(0.18f, 0.22f, 0.21f);
                                })
                                .OnClicked_Lambda([Weak]()
                                {
                                    if (Weak.IsValid())
                                    {
                                        Weak->SetActiveLevel(1);
                                    }

                                    return FReply::Handled();
                                })
                                [Caption(TEXT("LEVEL 2"))]]
                            + SHorizontalBox::Slot().FillWidth(1)
                            [SNew(SButton)
                                .IsFocusable(false)
                                .ContentPadding(FMargin(5, 6))
                                .ButtonColorAndOpacity_Lambda([Weak]()
                                {
                                    return Weak.IsValid() &&
                                           Weak->GetActiveLevelIndex() == 2
                                        ? Mint
                                        : FLinearColor(0.18f, 0.22f, 0.21f);
                                })
                                .OnClicked_Lambda([Weak]()
                                {
                                    if (Weak.IsValid())
                                    {
                                        Weak->SetActiveLevel(2);
                                    }

                                    return FReply::Handled();
                                })
                                [Caption(TEXT("LEVEL 3"))]]]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
                        [SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 3, 0)[ToolButton(EProximaBuildTool::Select, TEXT("1  Select"))]
                            + SHorizontalBox::Slot().FillWidth(1)[ToolButton(EProximaBuildTool::Wall, TEXT("2  Wall"))]]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
                        [SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 3, 0)[ToolButton(EProximaBuildTool::Room, TEXT("3  Room"))]
                            + SHorizontalBox::Slot().FillWidth(1)[ToolButton(EProximaBuildTool::Door, TEXT("4  Doorway"))]]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
                        [SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 3, 0)[ToolButton(EProximaBuildTool::Window, TEXT("5  Window"))]
                            + SHorizontalBox::Slot().FillWidth(1)[ToolButton(EProximaBuildTool::Floor, TEXT("6  Floor"))]]
                        + SVerticalBox::Slot().AutoHeight()[ToolButton(EProximaBuildTool::Roof, TEXT("7  Flat roof"))]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 4)
                        [SNew(STextBlock).Text_Lambda([Weak]() { return FText::FromString(Weak.IsValid() ? Weak->GetToolName() : FString()); })
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 15)).ColorAndOpacity(Mint)]
                        + SVerticalBox::Slot().AutoHeight()
                        [SNew(SVerticalBox).Visibility_Lambda([Show]() { return Show({EProximaBuildTool::Wall, EProximaBuildTool::Room, EProximaBuildTool::Roof}); })
                            + SVerticalBox::Slot().AutoHeight()[Field(TEXT("Wall height / roof underside"), TEXT("Height"), [](const auto& W) { return W.HeightCm; })]]
                        + SVerticalBox::Slot().AutoHeight()
                        [SNew(SVerticalBox).Visibility_Lambda([Show]() { return Show({EProximaBuildTool::Wall, EProximaBuildTool::Room}); })
                            + SVerticalBox::Slot().AutoHeight()[Field(TEXT("Wall thickness"), TEXT("Thickness"), [](const auto& W) { return W.ThicknessCm; })]]
                        + SVerticalBox::Slot().AutoHeight()
                        [SNew(SVerticalBox).Visibility_Lambda([Show]() { return Show({EProximaBuildTool::Wall}); })
                            + SVerticalBox::Slot().AutoHeight()[Field(TEXT("Exact length (0 = follow cursor)"), TEXT("Length"), [](const auto& W) { return W.ExactLengthCm; })]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 8)
                            [SNew(SCheckBox).IsChecked_Lambda([Weak]() { return Weak.IsValid() && Weak->bAngleLock ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
                                .OnCheckStateChanged_Lambda([Weak](ECheckBoxState State) { if (Weak.IsValid()) { Weak->bAngleLock = State == ECheckBoxState::Checked; } })
                                [Caption(TEXT("45° angle lock"))]]]
                        + SVerticalBox::Slot().AutoHeight()
                        [SNew(SVerticalBox).Visibility_Lambda([Show]() { return Show({EProximaBuildTool::Room, EProximaBuildTool::Floor, EProximaBuildTool::Roof}); })
                            + SVerticalBox::Slot().AutoHeight()[Caption(TEXT("Rooms use clear internal dimensions."))]
                            + SVerticalBox::Slot().AutoHeight()[Field(TEXT("Width (0 = draw freely)"), TEXT("Width"), [](const auto& W) { return W.RoomWidthCm; })]
                            + SVerticalBox::Slot().AutoHeight()[Field(TEXT("Depth (0 = draw freely)"), TEXT("Depth"), [](const auto& W) { return W.RoomDepthCm; })]]
                        + SVerticalBox::Slot().AutoHeight()
                        [SNew(SVerticalBox).Visibility_Lambda([Show]() { return Show({EProximaBuildTool::Door, EProximaBuildTool::Window}); })
                            + SVerticalBox::Slot().AutoHeight()[Field(TEXT("Clear opening width"), TEXT("OpeningWidth"), [](const auto& W) { return W.OpeningWidthCm; })]
                            + SVerticalBox::Slot().AutoHeight()[Field(TEXT("Clear opening height"), TEXT("OpeningHeight"), [](const auto& W) { return W.OpeningHeightCm; })]]
                        + SVerticalBox::Slot().AutoHeight()
                        [SNew(SVerticalBox).Visibility_Lambda([Show]() { return Show({EProximaBuildTool::Window}); })
                            + SVerticalBox::Slot().AutoHeight()[Field(TEXT("Sill height above floor"), TEXT("Sill"), [](const auto& W) { return W.SillCm; })]]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 4)
                        [SNew(SButton).IsFocusable(false).ContentPadding(8)
                            .OnClicked_Lambda([Weak]() { if (Weak.IsValid()) { Weak->CycleGrid(); } return FReply::Handled(); })
                            [SNew(STextBlock).Text_Lambda([Weak]() { return FText::FromString(Weak.IsValid() && Weak->GridCm > 0.0f
                                ? FString::Printf(TEXT("Grid: %.0f cm  •  click to change"), Weak->GridCm) : TEXT("Grid off  •  click to change")); }).ColorAndOpacity(Paper)]]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
                        [SNew(SCheckBox).IsChecked_Lambda([Weak]() { return Weak.IsValid() && Weak->bShowRoofs ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
                            .OnCheckStateChanged_Lambda([Weak](ECheckBoxState) { if (Weak.IsValid()) { Weak->ToggleRoofs(); } })
                            [Caption(TEXT("Show roofs while building"))]]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 14, 0, 4)[Caption(TEXT("WALL COLOUR"))]
                        + SVerticalBox::Slot().AutoHeight()
                        [SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().FillWidth(1)[Action(TEXT("Lime"), [](auto& W) { W.SetFinish(TEXT("Proxima.Wall.Plaster")); })]
                            + SHorizontalBox::Slot().FillWidth(1)[Action(TEXT("Sand"), [](auto& W) { W.SetFinish(TEXT("Proxima.Wall.Sand")); })]
                            + SHorizontalBox::Slot().FillWidth(1)[Action(TEXT("Slate"), [](auto& W) { W.SetFinish(TEXT("Proxima.Wall.Slate")); })]
                            + SHorizontalBox::Slot().FillWidth(1)[Action(TEXT("Clay"), [](auto& W) { W.SetFinish(TEXT("Proxima.Wall.Clay")); })]]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 0)
                        [SNew(SVerticalBox).Visibility_Lambda([Show]() { return Show({EProximaBuildTool::Select}); })
                            + SVerticalBox::Slot().AutoHeight()[Action(TEXT("Delete selected wall / surface"), [](auto& W) { W.DeleteSelection(); })]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)[Action(TEXT("Remove last opening from wall"), [](auto& W) { W.RemoveLastOpening(); })]]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 14, 0, 4)
                        [SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 3, 0)[Action(TEXT("Undo"), [](auto& W) { W.Undo(); })]
                            + SHorizontalBox::Slot().FillWidth(1)[Action(TEXT("Redo"), [](auto& W) { W.Redo(); })]]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
                        [SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 3, 0)[Action(TEXT("F5  Save"), [](auto& W) { W.Save(); })]
                            + SHorizontalBox::Slot().FillWidth(1)[Action(TEXT("F9  Load"), [](auto& W) { W.Load(); })]]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)[Action(TEXT("Add example home to empty lot"), [](auto& W) { W.AddExampleHome(); })]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 5)[Caption(TEXT("WASD pan | MMB rotate | wheel zoom"))]
                        + SVerticalBox::Slot().AutoHeight()[Action(TEXT("B  Walk through your home"), [](auto& W)
                            { if (auto* PC = Cast<AProximaPlayerController>(W.GetOwner())) { PC->ToggleBuildMode(); } })]
                    ]
                ]
            ]
        ]
        + SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Bottom).Padding(16)
        [SNew(SBorder).Visibility(EVisibility::HitTestInvisible).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(Ink).Padding(FMargin(18, 12))
            [SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [SNew(STextBlock).Text_Lambda([Weak]() { return FText::FromString(Weak.IsValid() && Weak->IsBuildModeActive() ? Weak->GetReadout() : TEXT("PROXIMA  |  WASD walk  •  Shift sprint  •  V view  •  B build")); })
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 13)).ColorAndOpacity(Paper).AutoWrapText(true)]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 5, 0, 0)
                [SNew(STextBlock).Text_Lambda([Weak]() { return FText::FromString(Weak.IsValid() ? Weak->Status : FString()); })
                    .ColorAndOpacity(Mint).AutoWrapText(true)]
            ]
        ]
    ];
}
