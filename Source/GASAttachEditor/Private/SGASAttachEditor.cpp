#include "SGASAttachEditor.h"
#include "Widgets/SBoxPanel.h"
#include "SGASReflectorNodeBase.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "GameplayAbilitySpec.h"
#include "../Public/GASAttachEditorStyle.h"
#include "Widgets/Views/STreeView.h"
#include "SGASCharacterTagsBase.h"
#include "SGASAttributesNodeBase.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Input/SComboButton.h"
#include "Framework/Application/SlateApplication.h"
#include "GameplayTagsManager.h"
#if WITH_EDITOR
#include "SGameplayTagWidget.h"
#endif

#define LOCTEXT_NAMESPACE "SGASAttachEditor"

const FName GAActivationColumnName("GAActivation");

struct FASCDebugTargetInfo
{
	FASCDebugTargetInfo()
	{
		DebugCategoryIndex = 0;
		DebugCategories.Add(TEXT("Attributes"));
		DebugCategories.Add(TEXT("GameplayEffects"));
		DebugCategories.Add(TEXT("Ability"));
	}

	TArray<FName> DebugCategories;
	int32 DebugCategoryIndex;

	TWeakObjectPtr<UWorld>	TargetWorld;
	TWeakObjectPtr<UAbilitySystemComponent>	LastDebugTarget;
};

TArray<FASCDebugTargetInfo>	AbilitySystemAttachDebugInfoList;

FASCDebugTargetInfo* GetASCDebugTargetInfo(UWorld* World)
{
	FASCDebugTargetInfo* TargetInfo = nullptr;
	for (FASCDebugTargetInfo& Info : AbilitySystemAttachDebugInfoList)
	{
		if (Info.TargetWorld.Get() == World)
		{
			TargetInfo = &Info;
			break;
		}
	}
	if (TargetInfo == nullptr)
	{
		TargetInfo = &AbilitySystemAttachDebugInfoList[AbilitySystemAttachDebugInfoList.AddDefaulted()];
		TargetInfo->TargetWorld = World;
	}
	return TargetInfo;
}

TArray<TWeakObjectPtr< UAbilitySystemComponent>> PlayerComp;

void UpDataPlayerComp(UWorld* World)
{
	PlayerComp.Reset();

	if (!World)
	{
		return;
	}

	for (TObjectIterator<UAbilitySystemComponent> It; It; ++It)
	{
		if (UAbilitySystemComponent* ASC = *It)
		{
			if (ASC->GetWorld() != World)
			{
				continue;
			}
			if (!MakeWeakObjectPtr(ASC).Get())
			{
				continue;
			}
			PlayerComp.Add(ASC);
		}
	}
}

UAbilitySystemComponent* GetDebugTarget(FASCDebugTargetInfo* Info,const UAbilitySystemComponent* InSelectComponent)
{
	// Return target if we already have one
	if (UAbilitySystemComponent* ASC = Info->LastDebugTarget.Get())
	{
		if ( ASC == InSelectComponent)
		{
			return ASC;
		}
	}

	// Find one
	bool bIsSelect = false;

	for (TWeakObjectPtr<UAbilitySystemComponent>& ASC :PlayerComp)
	{
		if (!ASC.IsValid())
		{
			continue;
		}

		if (InSelectComponent == ASC.Get() && !bIsSelect)
		{
			Info->LastDebugTarget = ASC;
			bIsSelect = true;
			break;
		}
	}

	if (!bIsSelect)
	{
		if (PlayerComp.IsValidIndex(0))
		{
			Info->LastDebugTarget = PlayerComp[0];
		}
		else
		{
			UpDataPlayerComp(Info->TargetWorld.Get());
			if (PlayerComp.IsValidIndex(0))
			{
				Info->LastDebugTarget = PlayerComp[0];
			}
			else
			{
				Info->LastDebugTarget = nullptr;
			}
		}
	}

	return Info->LastDebugTarget.Get();
}

class SGASAttachEditorImpl : public SGASAttachEditor
{
	typedef STreeView<TSharedRef<FGASAbilitieNodeBase>> SAbilitieTree;
	typedef STreeView<TSharedRef<FGASAttributesNodeBase>> SAttributesTree;

public:
	virtual void Construct(const FArguments& InArgs) override;

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

protected:
	// 当前筛选世界场景
	TSharedRef<SWidget>	OnGetShowWorldTypeMenu();

	// 选中世界场景
	void HandleShowWorldTypeChange(FWorldContext InWorldContext);

private:
	// 当期选择的世界场景句柄
	FName SelectWorldSceneConetextHandle;

	FText SelectWorldSceneText;

protected:

	FText GenerateToolTipForText(TSharedRef<FGASAbilitieNodeBase> InReflectorNode) const;

protected:
	/** 树视图生成树的回调 */
	TSharedRef<ITableRow> OnGenerateWidgetForFilterListView(TSharedRef< FGASAbilitieNodeBase > InItem, const TSharedRef<STableViewBase>& OwnerTable);

	/** 用于获取给定反射树节点的子项的回调 */
	void HandleReflectorTreeGetChildren( TSharedRef<FGASAbilitieNodeBase> InReflectorNode, TArray<TSharedRef<FGASAbilitieNodeBase>>& OutChildren );

	/** 当反射树中的选择发生更改时的回调 */
	void HandleReflectorTreeSelectionChanged(TSharedPtr<FGASAbilitieNodeBase>, ESelectInfo::Type /*SelectInfo*/);

	// 提示小部件
	TSharedPtr<IToolTip> GenerateToolTipForReflectorNode( TSharedRef<FGASAbilitieNodeBase> InReflectorNode );

	// 当请求反射器树中的上下文菜单时的回调
	TSharedPtr<SWidget> HandleReflectorTreeContextMenuPtr();

	/** 反射树头列表更改时的回调. */
	void HandleReflectorTreeHiddenColumnsListChanged();

protected:
	/** 树视图生成树的回调 */
	TSharedRef<ITableRow> HandleAttributesWidgetForFilterListView(TSharedRef< FGASAttributesNodeBase > InItem, const TSharedRef<STableViewBase>& OwnerTable);

	void HandleAttributesTreeGetChildren( TSharedRef<FGASAttributesNodeBase> InReflectorNode, TArray<TSharedRef<FGASAttributesNodeBase>>& OutChildren );

protected:
	// 当前筛选的角色
	TSharedRef<SWidget>	OnGetShowOverrideTypeMenu();

	// 当前选择的角色
	FText GetOverrideTypeDropDownText() const;

	// 选中的GAS组件
	void HandleOverrideTypeChange(TWeakObjectPtr<UAbilitySystemComponent> InComp);

protected:
	// 是否被选中
	ECheckBoxState HandleGetPickingButtonChecked() const;

	// 选中状态更改
	void HandlePickingModeStateChanged(ECheckBoxState NewValue);

	// 设置状态改变的东东
	virtual void SetPickingMode(bool bTick) override;

	// 设置单选框名称
	FText HandleGetPickingModeText() const;



private:

	void SaveSettings();
	void LoadSettings();

	// 刷新树状表
	void UpdateGameplayCueListItems();
	FReply UpdateGameplayCueListItemsButtom();

	// 创建查看Categories类型的筛选框
	TSharedRef<SWidget> OnGetShowDebugAbilitieCategories();

	// 当前选择的Categories类型
	FText GetShowDebugAbilitieCategoriesDropDownText() const;

	// 选中查看的Categories类型
	void HandleShowDebugAbilitieCategories(EDebugAbilitieCategories InType);

	FORCEINLINE FName GetAbilitieCategoriesName(EDebugAbilitieCategories InType) const;

	FORCEINLINE FText GetAbilitieCategoriesText(EDebugAbilitieCategories InType) const;

	// 创建Categories查看控件
	void CreateDebugAbilitieCategories(EDebugAbilitieCategories InType);

private:
	// Categories查看Tool的Slot控件
	TSharedPtr<SOverlay> CategoriesToolSlot;

private:
	// 当前选择Categories类型
	EDebugAbilitieCategories SelectAbilitieCategories;

protected: 
	
	FORCEINLINE UWorld* GetWorld();

private:

	// 拥有标签组控件
	TSharedPtr<SWrapBox> FilteredOwnedTagsView;

	// 阻止标签组控件
	TSharedPtr<SWrapBox> FilteredBlockedTagsView;

	// 老的Tag数据
	FGameplayTagContainer OldOwnerTags;

	// 老的阻止Tag数据
	FGameplayTagContainer OldBlockedTags;

protected:

	// 创建Tags控件
	TSharedPtr<SWidget> CreateAbilityTagWidget();

protected:

	// 按键监听
	void OnApplicationPreInputKeyDownListener(const FKeyEvent& InKeyEvent);

#if WITH_EDITOR
	// <tag控件的监听
	FReply OnMouseButtonUpTags(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, FName TagsName);

	// <Tag控件选择
	void RefreshTagList(FName TagsName);
#endif

public:
	// 筛选激活状态图表Tree
	void HandleScreenModeStateChanged(ECheckBoxState NewValue, EScreenGAModeState InState);

protected:
	// 是否被选中
	ECheckBoxState HandleGetScreenButtonChecked(EScreenGAModeState InState) const;

	// 创建选中框
	TSharedPtr<SCheckBox> CreateScreenCheckBox(EScreenGAModeState InState);

	// 设置筛选名称
	FText HandleGeScreenModeText(EScreenGAModeState InState) const;

	// 创建Ability查看控件
	TSharedPtr<SWidget> CreateAbilityToolWidget();

protected:

	// 创建Attributes查看控件
	TSharedPtr<SWidget> CreateAttributesToolWidget();


private:

	// 当前选中状态
	uint8 ScreenModeState;

private:
	TSharedPtr<SAbilitieTree> AbilitieReflectorTree;

	TArray<FString> HiddenReflectorTreeColumns;

	// 选择的控件
	TArray<TSharedRef<FGASAbilitieNodeBase>> SelectedNodes;

	// 根部的部件组
	TArray<TSharedRef<FGASAbilitieNodeBase>> AbilitieFilteredTreeRoot;

	// 选中的角色的GA
	TWeakObjectPtr<UAbilitySystemComponent> SelectAbilitySystemComponent;


	bool bPickingTick;

private:

	TSharedPtr<SAttributesTree> AttributesReflectorTree;

	// 根部的部件组
	TArray<TSharedRef<FGASAttributesNodeBase>> AttributesFilteredTreeRoot;

#if WITH_EDITOR
	// 当前拥有的Tag
	TArray<SGameplayTagWidget::FEditableGameplayTagContainerDatum> EditableOwnerContainers;
	FGameplayTagContainer OwnweTagContainer;
	FGameplayTagContainer OldOwnweTagContainer;

	// 当前阻止的Tag
	TArray<SGameplayTagWidget::FEditableGameplayTagContainerDatum> EditableBlockedContainers;
	FGameplayTagContainer BlockedTagContainer;
	FGameplayTagContainer OldBlockedTagContainer;
#endif

};

TSharedRef<SGASAttachEditor> SGASAttachEditor::New()
{
	return MakeShareable(new SGASAttachEditorImpl());
}

void SGASAttachEditorImpl::Construct(const FArguments& InArgs)
{
	bPickingTick = false;

	SelectAbilitieCategories = Ability;

	ScreenModeState = EScreenGAModeState::Active | EScreenGAModeState::Blocked | EScreenGAModeState::NoActive;

	LoadSettings();


	ChildSlot
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			//.HAlign(HAlign_Left)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.HAlign(HAlign_Left)
					.Text(LOCTEXT("Refresh", "刷新查看"))
					.OnClicked(this, &SGASAttachEditorImpl::UpdateGameplayCueListItemsButtom)
				]
				
				+ SHorizontalBox::Slot() 
				.FillWidth(1.f)
				.HAlign(HAlign_Right)
				[
					SNew(SComboButton)
					.OnGetMenuContent(this, &SGASAttachEditorImpl::OnGetShowWorldTypeMenu)
					.VAlign(VAlign_Center)
					.ContentPadding(2)
					.ButtonContent()
					[
						SNew(STextBlock)
						.ToolTipText(LOCTEXT("ShowWorldTypeType", "选择需要查看场景"))
						.Text_Lambda([this]() -> FText { return SelectWorldSceneText; })
					]
				]

			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(2.0f)
				.AutoWidth()
				[
					SNew(SCheckBox)
					.Padding(FMargin(4, 0))
					.IsChecked(this, &SGASAttachEditorImpl::HandleGetPickingButtonChecked)
					.OnCheckStateChanged(this, &SGASAttachEditorImpl::HandlePickingModeStateChanged)
					[
						SNew(SBox)
						.MinDesiredWidth(125)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(this, &SGASAttachEditorImpl::HandleGetPickingModeText)
						]

					]
				]
				+ SHorizontalBox::Slot()
				.Padding(2.f, 2.f)
				.AutoWidth()
				[
					SNew(SComboButton)
					.OnGetMenuContent(this, &SGASAttachEditorImpl::OnGetShowOverrideTypeMenu)
					.VAlign( VAlign_Center )
					.ContentPadding(2)
					.ButtonContent()
					[
						SNew( STextBlock )
						.ToolTipText(LOCTEXT("ShowOverrideType", "选择需要查看的角色的GA"))
						.Text(this, &SGASAttachEditorImpl::GetOverrideTypeDropDownText )
					]
				]
			]

			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.Padding(2.f, 2.f)
				.AutoHeight()
				.HAlign(HAlign_Left)
				[
					SNew(SComboButton)
					.OnGetMenuContent(this, &SGASAttachEditorImpl::OnGetShowDebugAbilitieCategories)
					.VAlign(VAlign_Center)
					.ContentPadding(2)
					.ButtonContent()
					[
						SNew(STextBlock)
						.ToolTipText(LOCTEXT("ShowCharactAbilitieType", "选择需要查看角色身上的效果类型"))
						.Text(this, &SGASAttachEditorImpl::GetShowDebugAbilitieCategoriesDropDownText)
					]
				]

				+ SVerticalBox::Slot()
				.FillHeight(1.f)
				[
					//CreateAbilityToolWidget()
					SAssignNew(CategoriesToolSlot, SOverlay)
				]
				
			]

		];

	HandleShowDebugAbilitieCategories(Ability);

	UpdateGameplayCueListItemsButtom();

#if WITH_EDITOR
	FSlateApplication::Get().OnApplicationPreInputKeyDownListener().AddRaw(this, &SGASAttachEditorImpl::OnApplicationPreInputKeyDownListener);

	EditableOwnerContainers.Empty();
	EditableBlockedContainers.Empty();

	EditableOwnerContainers.Add(SGameplayTagWidget::FEditableGameplayTagContainerDatum(nullptr,&OwnweTagContainer));
	EditableBlockedContainers.Add(SGameplayTagWidget::FEditableGameplayTagContainerDatum(nullptr,&BlockedTagContainer));
#else
	FSlateApplication::Get().RegisterInputPreProcessor(MakeShareable(new FAttachInputProcessor(this)));
#endif
}


void SGASAttachEditorImpl::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if (bPickingTick)
	{
		UpdateGameplayCueListItems();
	}
}

TSharedRef<SWidget> SGASAttachEditorImpl::OnGetShowWorldTypeMenu()
{
	FMenuBuilder MenuBuilder( true, NULL );

	const TIndirectArray<FWorldContext>& WorldList = GEngine->GetWorldContexts();

	for (auto& Item : WorldList)
	{
		if (Item.WorldType == EWorldType::Type::PIE)
		{
			FUIAction NoAction( FExecuteAction::CreateSP( this, &SGASAttachEditorImpl::HandleShowWorldTypeChange, Item ) );

			FText ShowName;
			if (Item.RunAsDedicated)
			{
				ShowName = LOCTEXT("Dedicated","专用服务器");
			}
			else
			{
				ShowName = FText::Format(FText::FromString("{0}  [{1}]"), LOCTEXT("Client","客户端") ,FText::AsNumber(Item.PIEInstance));
			}

			MenuBuilder.AddMenuEntry(ShowName, FText(), FSlateIcon(), NoAction);
		}
	}

	return MenuBuilder.MakeWidget();
}


void SGASAttachEditorImpl::HandleShowWorldTypeChange(FWorldContext InWorldContext)
{
	SelectWorldSceneConetextHandle = InWorldContext.ContextHandle;


	if (InWorldContext.RunAsDedicated)
	{
		SelectWorldSceneText = LOCTEXT("Dedicated", "专用服务器");
	}
	else
	{
		SelectWorldSceneText = FText::Format(FText::FromString("{0}  [{1}]"), LOCTEXT("Client", "客户端"), FText::AsNumber(InWorldContext.PIEInstance));
	}

	UpdateGameplayCueListItemsButtom();
}

FText SGASAttachEditorImpl::GenerateToolTipForText(TSharedRef<FGASAbilitieNodeBase> InReflectorNode) const
{
	return InReflectorNode->GetAbilitieHasTag();
}

TSharedRef<ITableRow> SGASAttachEditorImpl::OnGenerateWidgetForFilterListView(TSharedRef< FGASAbilitieNodeBase > InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SGASAbilitieTreeItem, OwnerTable)
		.WidgetInfoToVisualize(InItem)
		.ToolTip(GenerateToolTipForReflectorNode(InItem));
}


void SGASAttachEditorImpl::HandleReflectorTreeGetChildren(TSharedRef<FGASAbilitieNodeBase> InReflectorNode, TArray<TSharedRef<FGASAbilitieNodeBase>>& OutChildren)
{
	OutChildren = InReflectorNode->GetChildNodes();
}

void SGASAttachEditorImpl::HandleReflectorTreeSelectionChanged(TSharedPtr<FGASAbilitieNodeBase>, ESelectInfo::Type /*SelectInfo*/)
{
	SelectedNodes = AbilitieReflectorTree->GetSelectedItems();

}

TSharedPtr<IToolTip> SGASAttachEditorImpl::GenerateToolTipForReflectorNode(TSharedRef<FGASAbilitieNodeBase> InReflectorNode)
{
	return SNew(SToolTip)
			.Text(this, &SGASAttachEditorImpl::GenerateToolTipForText, InReflectorNode)
		;
}

TSharedPtr<SWidget> SGASAttachEditorImpl::HandleReflectorTreeContextMenuPtr()
{
	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, nullptr);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("NotFunction", "没有功能"),
		LOCTEXT("NotFunctionTooltip", "弟弟，还没开放出来"),
		FSlateIcon(),
		FUIAction()
	);

	return MenuBuilder.MakeWidget();
}

void SGASAttachEditorImpl::HandleReflectorTreeHiddenColumnsListChanged()
{
#if WITH_EDITOR
	if (AbilitieReflectorTree && AbilitieReflectorTree->GetHeaderRow())
	{
		const TArray<FName> HiddenColumnIds = AbilitieReflectorTree->GetHeaderRow()->GetHiddenColumnIds();
		HiddenReflectorTreeColumns.Reset(HiddenColumnIds.Num());
		for (const FName Id : HiddenColumnIds)
		{
			HiddenReflectorTreeColumns.Add(Id.ToString());
		}
		SaveSettings();
	}
#endif
}

TSharedRef<ITableRow> SGASAttachEditorImpl::HandleAttributesWidgetForFilterListView(TSharedRef< FGASAttributesNodeBase > InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SGASAttributesTreeItem, OwnerTable)
		.WidgetInfoToVisualize(InItem);
}

void SGASAttachEditorImpl::HandleAttributesTreeGetChildren(TSharedRef<FGASAttributesNodeBase> InReflectorNode, TArray<TSharedRef<FGASAttributesNodeBase>>& OutChildren)
{
}

FText GetOverrideTypeDropDownText_Explicit(const TWeakObjectPtr<UAbilitySystemComponent>& InComp)
{
	if (!InComp.IsValid())
	{
		return FText();
	}

	AActor* LocalAvatarActor = InComp->GetAvatarActor_Direct();
	AActor* LocalOwnerActor = InComp->GetOwnerActor();

	FText OutName = FText::FromString(LocalAvatarActor->GetName());

	if (LocalOwnerActor)
	{
		OutName = FText::Format(FText::FromString(TEXT("{0}[{1}]")), OutName, FText::FromString(LocalOwnerActor->GetName()));
	}

	return OutName;
}

TSharedRef<SWidget> SGASAttachEditorImpl::OnGetShowOverrideTypeMenu()
{
	FMenuBuilder MenuBuilder( true, NULL );

	for (TWeakObjectPtr<UAbilitySystemComponent>& Comp :PlayerComp)
	{
		FUIAction NoAction( FExecuteAction::CreateSP( this, &SGASAttachEditorImpl::HandleOverrideTypeChange, Comp ) );
		MenuBuilder.AddMenuEntry(GetOverrideTypeDropDownText_Explicit(Comp),FText(),FSlateIcon(),NoAction);
	}
	return MenuBuilder.MakeWidget();
}

FText SGASAttachEditorImpl::GetOverrideTypeDropDownText() const
{
	if (SelectAbilitySystemComponent.IsValid())
	{
		return FText::FromString(SelectAbilitySystemComponent->GetAvatarActor_Direct()->GetName());
	}

	return FText();
}

void SGASAttachEditorImpl::HandleOverrideTypeChange(TWeakObjectPtr<UAbilitySystemComponent> InComp)
{
	if (!InComp.IsValid())
	{
		return;
	}

	SelectAbilitySystemComponent = InComp;

	UpdateGameplayCueListItems();
}

ECheckBoxState SGASAttachEditorImpl::HandleGetPickingButtonChecked() const
{
	return bPickingTick ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SGASAttachEditorImpl::HandlePickingModeStateChanged(ECheckBoxState NewValue)
{
	SetPickingMode(!bPickingTick);
}

void SGASAttachEditorImpl::SetPickingMode(bool bTick)
{
/*
#if WITH_SLATE_DEBUGGING
	static auto CVarSlateGlobalInvalidation = IConsoleManager::Get().FindConsoleVariable(TEXT("Slate.EnableGlobalInvalidation"));
#endif*/

	bPickingTick = bTick;
}

FText SGASAttachEditorImpl::HandleGetPickingModeText() const
{
	return bPickingTick ? LOCTEXT("bPickingTickYes", "按 END 键停止刷新") : LOCTEXT("bPickingTickNo", "持续更新") ;
}

void SGASAttachEditorImpl::SaveSettings()
{
	GConfig->SetArray(TEXT("GASAttachEditor"), TEXT("HiddenReflectorTreeColumns"), HiddenReflectorTreeColumns, *GEditorPerProjectIni);
}

void SGASAttachEditorImpl::LoadSettings()
{
	GConfig->GetArray(TEXT("WidgetReflector"), TEXT("HiddenReflectorTreeColumns"), HiddenReflectorTreeColumns, *GEditorPerProjectIni);
}

void SGASAttachEditorImpl::UpdateGameplayCueListItems()
{

	AbilitieFilteredTreeRoot.Reset();

	AttributesFilteredTreeRoot.Reset();


	FASCDebugTargetInfo* TargetInfo = GetASCDebugTargetInfo(GetWorld());

	if (UAbilitySystemComponent* ASC = GetDebugTarget(TargetInfo, SelectAbilitySystemComponent.Get()))
	{
		SelectAbilitySystemComponent = ASC;

		// 标签组
		if (FilteredOwnedTagsView.IsValid())
		{
			FGameplayTagContainer OwnerTags;
			SelectAbilitySystemComponent->GetOwnedGameplayTags(OwnerTags);
			if (OldOwnerTags != OwnerTags)
			{
				OldOwnerTags = OwnerTags;
				FilteredOwnedTagsView->ClearChildren();
				OwnweTagContainer.Reset();
				for (FGameplayTag InTag : OwnerTags)
				{
					FilteredOwnedTagsView->AddSlot()
						[
							SNew(SCharacterTagsViewItem)
							.TagsItem(FGASCharacterTags::Create(ASC, InTag, "ActivationOwnedTags"))
						];
					OwnweTagContainer.AddTag(InTag);
				}

			}
			

			FGameplayTagContainer BlockTags;
			ASC->GetBlockedAbilityTags(BlockTags);

			if (BlockTags != OldBlockedTags)
			{
				FilteredBlockedTagsView->ClearChildren();
				BlockedTagContainer.Reset();
				for (FGameplayTag InTag : BlockTags)
				{
					FilteredBlockedTagsView->AddSlot()
						[
							SNew(SCharacterTagsViewItem)
							.TagsItem(FGASCharacterTags::Create(ASC, InTag, "ActivationBlockedTags"))
						];
					BlockedTagContainer.AddTag(InTag);
				}

			}
		}

		TArray<FName> LocalDisplayNames;
		LocalDisplayNames.Add(TargetInfo->DebugCategories[TargetInfo->DebugCategoryIndex]);

		// 技能组
		if (AbilitieReflectorTree.IsValid())
		{
			for (FGameplayAbilitySpec& AbilitySpec : ASC->GetActivatableAbilities())
			{
				if (!AbilitySpec.Ability) continue;
				TSharedRef<FGASAbilitieNode> NewItem = FGASAbilitieNode::Create(ASC, AbilitySpec);

				NewItem->SetItemVisility(NewItem->ScreenGAMode & ScreenModeState);

				AbilitieFilteredTreeRoot.Add(NewItem);

			}
			AbilitieReflectorTree->RequestTreeRefresh();

		}

		// 属性组
		if (AttributesReflectorTree.IsValid())
		{
			for (UAttributeSet* Set : ASC->GetSpawnedAttributes())
			{
				if (!Set)
				{
					continue;
				}

				for (TFieldIterator<FProperty> It(Set->GetClass()); It; ++It)
				{
					FGameplayAttribute	Attribute(*It);

					TSharedRef<FGASAttributesNode> NewItem = FGASAttributesNode::Create(ASC, Attribute);

					AttributesFilteredTreeRoot.Add(NewItem);
				}
			}
			AttributesReflectorTree->RequestTreeRefresh();

		}

	}
}

FReply SGASAttachEditorImpl::UpdateGameplayCueListItemsButtom()
{
	UpDataPlayerComp(GetWorld());

	UpdateGameplayCueListItems();

	return FReply::Handled();
}

TSharedRef<SWidget> SGASAttachEditorImpl::OnGetShowDebugAbilitieCategories()
{
	FMenuBuilder MenuBuilder(true, NULL);

	TArray<EDebugAbilitieCategories> Categories({ EDebugAbilitieCategories::Ability,EDebugAbilitieCategories::Attributes,EDebugAbilitieCategories::GameplayEffects, EDebugAbilitieCategories::Tags });

	for (EDebugAbilitieCategories& Type : Categories)
	{

		FUIAction NoAction(FExecuteAction::CreateSP(this, &SGASAttachEditorImpl::HandleShowDebugAbilitieCategories, Type));
		MenuBuilder.AddMenuEntry(FText::FromName(GetAbilitieCategoriesName(Type)), GetAbilitieCategoriesText(Type), FSlateIcon(), NoAction);
	}


	return MenuBuilder.MakeWidget();
}

FText SGASAttachEditorImpl::GetShowDebugAbilitieCategoriesDropDownText() const
{
	return FText::FromName(GetAbilitieCategoriesName(SelectAbilitieCategories));
}

void SGASAttachEditorImpl::HandleShowDebugAbilitieCategories(EDebugAbilitieCategories InType)
{
	SelectAbilitieCategories = InType;

	// 刷新查看东东
	CreateDebugAbilitieCategories(InType);
}

FORCEINLINE FName SGASAttachEditorImpl::GetAbilitieCategoriesName(EDebugAbilitieCategories InType) const
{
	FName TypeName;
	switch (InType)
	{
	case EDebugAbilitieCategories::Tags:
		TypeName = "Tags";
		break;
	case EDebugAbilitieCategories::Ability:
		TypeName = "Ability";
		break;
	case EDebugAbilitieCategories::Attributes:
		TypeName = "Attributes";
		break;
	case EDebugAbilitieCategories::GameplayEffects:
		TypeName = "GameplayEffects";
		break;
	}

	return TypeName;
}

FORCEINLINE FText SGASAttachEditorImpl::GetAbilitieCategoriesText(EDebugAbilitieCategories InType) const
{
	FText TypeText;
	switch (InType)
	{
	case EDebugAbilitieCategories::Tags:
		TypeText = LOCTEXT("Categories_Tigs", "标签");
		break;
	case EDebugAbilitieCategories::Ability:
		TypeText = LOCTEXT("Categories_Ability", "技能");
		break;
	case EDebugAbilitieCategories::Attributes:
		TypeText = LOCTEXT("Categories_Attributes", "属性");
		break;
	case EDebugAbilitieCategories::GameplayEffects:
		TypeText = LOCTEXT("Categories_GameplayEffects", "效果");
		break;
	}

	return TypeText;
}

void SGASAttachEditorImpl::CreateDebugAbilitieCategories(EDebugAbilitieCategories InType)
{
	if (!CategoriesToolSlot.IsValid())
	{
		return;
	}

	CategoriesToolSlot->ClearChildren();

	TSharedPtr<SWidget> CategoriesWidget;

	switch (InType)
	{
	case EDebugAbilitieCategories::Attributes:
		CategoriesWidget = CreateAttributesToolWidget();
		break;
	case EDebugAbilitieCategories::GameplayEffects:
		CategoriesWidget = SNew(SBox)
			.HAlign(EHorizontalAlignment::HAlign_Center)
			.VAlign(EVerticalAlignment::VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("还没有做，不急")))
			];
		break;
	case EDebugAbilitieCategories::Ability:
		CategoriesWidget = CreateAbilityToolWidget();
		break;
	case EDebugAbilitieCategories::Tags:
		CategoriesWidget = CreateAbilityTagWidget();
		break;
	}

	if (!CategoriesWidget.IsValid())
	{
		return;
	}
	
	CategoriesToolSlot->AddSlot()
		[
			CategoriesWidget.ToSharedRef()
		];
}

UWorld* SGASAttachEditorImpl::GetWorld()
{

	UWorld* World = nullptr;

	const TIndirectArray<FWorldContext>& WorldList = GEngine->GetWorldContexts();

	TArray<FWorldContext> NewWorldList;

	for (auto& Item : WorldList)
	{
		if (Item.WorldType == EWorldType::Type::PIE || Item.WorldType == EWorldType::Type::Game)
		{
			NewWorldList.Add(Item);
			if (Item.ContextHandle == SelectWorldSceneConetextHandle)
			{
				return Item.World();
			}
		}
	}

	if (NewWorldList.Num())
	{
		SelectWorldSceneConetextHandle = NewWorldList[0].ContextHandle;

		if (NewWorldList[0].RunAsDedicated)
		{
			SelectWorldSceneText = LOCTEXT("Dedicated", "专用服务器");
		}
		else
		{
			SelectWorldSceneText = FText::Format(FText::FromString("{0}  [{1}]"), LOCTEXT("Client", "客户端"), FText::AsNumber(NewWorldList[0].PIEInstance));
		}

		return NewWorldList[0].World();
	}

	return nullptr;

}

TSharedPtr<SWidget> SGASAttachEditorImpl::CreateAbilityTagWidget()
{
	return SNew(SSplitter)
		.Orientation(Orient_Vertical)
		+ SSplitter::Slot()
		.Value(0.7f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			.Padding(2.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("CharacterHasOwnTags", "当前角色拥有的Tags"))
			]

			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SNew(SBorder)
				.Padding(2.f)
#if WITH_EDITOR
				.OnMouseButtonUp(this, &SGASAttachEditorImpl::OnMouseButtonUpTags, FName("OwnTags"))
#endif
				[
					SAssignNew(FilteredOwnedTagsView, SWrapBox)
					.Orientation(EOrientation::Orient_Horizontal)
					.UseAllottedSize(true)
					.InnerSlotPadding(FVector2D(5.f))
				]
			]
		]

		+ SSplitter::Slot()
		.Value(0.3f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			.Padding(2.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("CharacterHasBlaTags", "当前角色阻止的Tags"))
			]

			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SNew(SBorder)
#if WITH_EDITOR
				.OnMouseButtonUp(this, &SGASAttachEditorImpl::OnMouseButtonUpTags, FName("BlaTags"))
#endif
				.Padding(2.f)
				[
					SAssignNew(FilteredBlockedTagsView, SWrapBox)
					.Orientation(EOrientation::Orient_Horizontal)
					.UseAllottedSize(true)
					.InnerSlotPadding(FVector2D(5.f))
				]
			]
	];
}

void SGASAttachEditorImpl::OnApplicationPreInputKeyDownListener(const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::End)
	{
		SetPickingMode(false);
	}
}

FReply SGASAttachEditorImpl::OnMouseButtonUpTags(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, FName TagsName)
{
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		TSharedPtr<SGameplayTagWidget> TagWidget = 
			SNew(SGameplayTagWidget, TagsName == "OwnTags" ? EditableOwnerContainers : EditableBlockedContainers)
			.GameplayTagUIMode(EGameplayTagUIMode::SelectionMode)
			.ReadOnly(false)
			.OnTagChanged(this, &SGASAttachEditorImpl::RefreshTagList, TagsName);

		if (TagWidget.IsValid())
		{
			if (TagsName == "OwnTags")
			{
				OldOwnweTagContainer = OwnweTagContainer;
			}
			else
			{
				OldBlockedTagContainer = BlockedTagContainer;
			}

			FWidgetPath WidgetPath = MouseEvent.GetEventPath() != nullptr ? *MouseEvent.GetEventPath() : FWidgetPath();
			FSlateApplication::Get().PushMenu(AsShared(), WidgetPath, TagWidget.ToSharedRef(), MouseEvent.GetScreenSpacePosition(), FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
		}

	}

	return FReply::Handled();
}

void SGASAttachEditorImpl::RefreshTagList(FName TagsName)
{
	bool bIsOwnTag = TagsName == "OwnTags";

	FGameplayTagContainer* NewTagContainer =  bIsOwnTag ? &OwnweTagContainer : &BlockedTagContainer;
	FGameplayTagContainer* OldTagContainer = bIsOwnTag ? &OldOwnweTagContainer : &OldBlockedTagContainer;

	if (!SelectAbilitySystemComponent.IsValid() || !NewTagContainer || !OldTagContainer) return;

	TArray<FGameplayTag> NewGameplayTagArr;
	NewTagContainer->GetGameplayTagArray(NewGameplayTagArr);

	// 是否增加
	for (const FGameplayTag& Item : NewGameplayTagArr)
	{
		if (OldTagContainer->HasTag(Item)) continue;

		if (bIsOwnTag)
		{
			SelectAbilitySystemComponent->UpdateTagMap(Item,1);
		}
		else
		{

		}
	}

	TArray<FGameplayTag> OldGameplayTagArr;
	OldTagContainer->GetGameplayTagArray(OldGameplayTagArr);
	// 是否是减少
	for (const FGameplayTag& Item  : OldGameplayTagArr)
	{
		if(NewTagContainer->HasTag(Item)) continue;

		if (bIsOwnTag)
		{
			SelectAbilitySystemComponent->UpdateTagMap(Item,-1);
		}
		else
		{

		}
	}
	*OldTagContainer = *NewTagContainer;
}

void SGASAttachEditorImpl::HandleScreenModeStateChanged(ECheckBoxState NewValue, EScreenGAModeState InState)
{
	switch (NewValue)
	{
	case ECheckBoxState::Unchecked:
		ScreenModeState ^= InState;
		break;
	case ECheckBoxState::Checked:
		ScreenModeState |= InState;
		break;
	case ECheckBoxState::Undetermined:
		break;
	default:
		break;
	}

	for (TSharedRef<FGASAbilitieNodeBase>& Item : AbilitieFilteredTreeRoot)
	{
		Item->SetItemVisility(Item->ScreenGAMode & ScreenModeState);
	}
}

ECheckBoxState SGASAttachEditorImpl::HandleGetScreenButtonChecked(EScreenGAModeState InState) const
{
	return ScreenModeState & InState ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

TSharedPtr<SCheckBox> SGASAttachEditorImpl::CreateScreenCheckBox(EScreenGAModeState InModeState)
{
	return SNew(SCheckBox)
			.Padding(FMargin(4, 0))
			.IsChecked(this, &SGASAttachEditorImpl::HandleGetScreenButtonChecked, InModeState)
			.OnCheckStateChanged_Lambda([this,InModeState](ECheckBoxState NewValue){ HandleScreenModeStateChanged(NewValue,InModeState);})
			[
				SNew(SBox)
				.MinDesiredWidth(80.f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(this, &SGASAttachEditorImpl::HandleGeScreenModeText, InModeState)
				]
			];
}

FText SGASAttachEditorImpl::HandleGeScreenModeText(EScreenGAModeState InState) const
{
	FText StateText;
	switch (InState)
	{
	case Active:
		StateText = LOCTEXT("ActiveScreen","激活");
		break;
	case Blocked:
		StateText = LOCTEXT("BlockedScreen","阻止");
		break;
	case NoActive:
		StateText = LOCTEXT("NoActive","未激活");
		break;
	default:
		break;
	}

	return StateText;
}

TSharedPtr<SWidget> SGASAttachEditorImpl::CreateAbilityToolWidget()
{

	TArray<FName> HiddenColumnsList;
	HiddenColumnsList.Reserve(HiddenReflectorTreeColumns.Num());
	for (const FString& Item : HiddenReflectorTreeColumns)
	{
		HiddenColumnsList.Add(*Item);
	}

	TSharedPtr<SVerticalBox> Widget = 
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding(2.f, 2.f)
			.AutoHeight()
			.HAlign(HAlign_Left)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				[
					CreateScreenCheckBox(Active).ToSharedRef()
				]

				+ SHorizontalBox::Slot()
				[
					CreateScreenCheckBox(Blocked).ToSharedRef()
				]

				+ SHorizontalBox::Slot()
				[
					CreateScreenCheckBox(NoActive).ToSharedRef()
				]
			]

		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SNew(SBorder)
			.Padding(0)
			[
				SAssignNew(AbilitieReflectorTree, SAbilitieTree)
				.ItemHeight(24.f)
				.TreeItemsSource(&AbilitieFilteredTreeRoot)
				.OnGenerateRow(this, &SGASAttachEditorImpl::OnGenerateWidgetForFilterListView)
				.OnGetChildren(this, &SGASAttachEditorImpl::HandleReflectorTreeGetChildren)
				.OnSelectionChanged(this, &SGASAttachEditorImpl::HandleReflectorTreeSelectionChanged)
				.OnContextMenuOpening(this, &SGASAttachEditorImpl::HandleReflectorTreeContextMenuPtr)
				.HighlightParentNodesForSelection(true)
				.HeaderRow
				(
					SNew(SHeaderRow)
					.CanSelectGeneratedColumn(true)
					.HiddenColumnsList(HiddenColumnsList)
					.OnHiddenColumnsListChanged(this, &SGASAttachEditorImpl::HandleReflectorTreeHiddenColumnsListChanged)

					+ SHeaderRow::Column(NAME_AbilitietName)
					.DefaultLabel(LOCTEXT("AbilitietName", "名称"))
					.DefaultTooltip(LOCTEXT("AbilitietNameToolTip", "技能名称/任务名称/调试名称"))
					.FillWidth(0.7f)
					.ShouldGenerateWidget(true)

					+ SHeaderRow::Column(NAME_GAStateType)
					.DefaultLabel(LOCTEXT("GAStateType", "当前状态"))
					.DefaultTooltip(LOCTEXT("GAStateTypeToolTip", "当前状态是否激活，或者是可以被激活但是因为某些原因被拦截"))
					.FillWidth(0.3f)

					+ SHeaderRow::Column(NAME_GAIsActive)
					.DefaultLabel(LOCTEXT("GAIsActive", "是否激活"))
					.FixedWidth(80.0f)
				)
			]
		];


	return Widget;
}

TSharedPtr<SWidget> SGASAttachEditorImpl::CreateAttributesToolWidget()
{
	return SNew(SBorder)
			.Padding(0)
			[
				SAssignNew(AttributesReflectorTree,SAttributesTree)
				.ItemHeight(32.f)
				.TreeItemsSource(&AttributesFilteredTreeRoot) 
				.OnGenerateRow(this, &SGASAttachEditorImpl::HandleAttributesWidgetForFilterListView)
				.OnGetChildren(this, &SGASAttachEditorImpl::HandleAttributesTreeGetChildren)
				.HighlightParentNodesForSelection(true)
				.HeaderRow
				(
					SNew(SHeaderRow)
					.CanSelectGeneratedColumn(true)

					+ SHeaderRow::Column(NAME_AttributesName)
					.DefaultLabel(LOCTEXT("AttributesName", "属性名称"))
					.FillWidth(0.4f)
					.ShouldGenerateWidget(true)

					+ SHeaderRow::Column(NAME_GANumericAttribute)
					.DefaultLabel(LOCTEXT("GAStateType", "当前属性值"))
					.FillWidth(0.6f)

				)
			];
}

FAttachInputProcessor::FAttachInputProcessor(SGASAttachEditor* InWidgetPtr)
	:GASAttachEditorWidgetPtr(InWidgetPtr)
{
}

bool FAttachInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::End && GASAttachEditorWidgetPtr)
	{
		GASAttachEditorWidgetPtr->SetPickingMode(false);
	}

	return false;
}


#undef LOCTEXT_NAMESPACE
