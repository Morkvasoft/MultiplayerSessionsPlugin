// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"

#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"

void UMenu::MenuSetup(int32 NumberOfPublicConnections, FString TypeOfMatch, FString LobbyPath)
{
    PathToLobby = FString::Printf(TEXT("%s?listen"), *LobbyPath);
    NumPublicConnections = NumberOfPublicConnections;
    MatchType = TypeOfMatch;

    this->AddToViewport();
    this->SetVisibility(ESlateVisibility::Visible);
    this->SetIsFocusable(true);

    UWorld* World = this->GetWorld();
    if(World)
    {
        APlayerController* PlayerController = World->GetFirstPlayerController();
        if(PlayerController)
        {
            FInputModeUIOnly InputModeData;
            InputModeData.SetWidgetToFocus(this->TakeWidget());
            InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            PlayerController->SetInputMode(InputModeData);
            PlayerController->SetShowMouseCursor(true);
        }
    }

    UGameInstance* GameInstance = this->GetGameInstance();
    if(GameInstance)
    {
        MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
    }

    if(MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
        MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessions);
        MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSession);
        MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySession);
        MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSession);
    }
}

bool UMenu::Initialize()
{
    if(!Super::Initialize())
    {
        return false;
    }

    if(HostButton)
    {
        HostButton->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
    }

    if(JoinButton)
    {
        JoinButton->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
    }

    return true;
}

void UMenu::NativeDestruct()
{
    MenuTearDown();
    Super::NativeDestruct();
}

void UMenu::OnCreateSession(bool bWasSuccessful)
{
    if(bWasSuccessful)
    {
        if(GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, FString(TEXT("Session created successfully")));
        }

        UWorld* World = this->GetWorld();
        if(World)
        {
            World->ServerTravel(PathToLobby);
        }
    }
    else
    {
        if(GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString(TEXT("Failed to create session")));
        }
        HostButton->SetIsEnabled(true);
    }
}

void UMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult> &SessionResult, bool bWasSuccessful)
{
    if(!MultiplayerSessionsSubsystem)
    {
        return;
    }

    for (auto Result : SessionResult)
    {
        FString SettingValue;
        Result.Session.SessionSettings.Get(FName("MatchType"), SettingValue);
        if(SettingValue == MatchType)
        {
            MultiplayerSessionsSubsystem->JoinSession(Result);
            return;
        }
    }

    if(!bWasSuccessful || SessionResult.Num() == 0)
    {
        JoinButton->SetIsEnabled(true);
    }
}

void UMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
    IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
    if(Subsystem)
    {
        IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
        if(SessionInterface.IsValid())
        {
            FString Address;
            if(SessionInterface->GetResolvedConnectString(NAME_GameSession, Address))
            {
                APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
                if(PlayerController)
                {
                    PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
                }
            }

        }
    }

    if(Result != EOnJoinSessionCompleteResult::Success)
    {
        JoinButton->SetIsEnabled(true);
    }
}

void UMenu::OnDestroySession(bool bWasSuccessful) {}

void UMenu::OnStartSession(bool bWasSuccessful) {}

void UMenu::HostButtonClicked()
{
    HostButton->SetIsEnabled(false);

    if(GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, FString(TEXT("Host button clicked")));
    }

    if(MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
    }
}

void UMenu::JoinButtonClicked()
{
    JoinButton->SetIsEnabled(false);

    if(GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, FString(TEXT("Join button clicked")));
    }

    if(MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->FindSessions(10000);
    }
}

void UMenu::MenuTearDown()
{
    this->RemoveFromParent();
    UWorld* World = this->GetWorld();
    if(World)
    {
        APlayerController* PlayerController = World->GetFirstPlayerController();
        if(PlayerController)
        {
            FInputModeGameOnly InputModeData;
            PlayerController->SetInputMode(InputModeData);
            PlayerController->SetShowMouseCursor(false);
        }
    }
}
