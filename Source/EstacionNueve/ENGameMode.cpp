#include "ENGameMode.h"

#include "ENCharacter.h"
#include "ENGameState.h"
#include "ENHUD.h"
#include "ENPlayerController.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "TimerManager.h"
#include "UnrealClient.h"

static FString ShotPath(const TCHAR* FileName)
{
	const FString Directory = FPaths::ProjectDir() / TEXT("Capturas");
	IFileManager::Get().MakeDirectory(*Directory, true);
	return Directory / FileName;
}

AENGameMode::AENGameMode()
{
	DefaultPawnClass = AENCharacter::StaticClass();
	HUDClass = AENHUD::StaticClass();
	PlayerControllerClass = AENPlayerController::StaticClass();
	GameStateClass = AENGameState::StaticClass();
}

void AENGameMode::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("EN_GAMEMODE|BeginPlay (Estacion Nueve prototipo)"));

	if (FParse::Param(FCommandLine::Get(), TEXT("enautotest")))
	{
		GetWorldTimerManager().SetTimer(AutoTestHandle, this, &AENGameMode::RunAutoTest, 4.f, false);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("enshot")))
	{
		GetWorldTimerManager().SetTimer(ShotHandle, this, &AENGameMode::TakePlayerShot, 3.f, false);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("enaerial")))
	{
		GetWorldTimerManager().SetTimer(ShotHandle, this, &AENGameMode::TakeAerialShot, 3.f, false);
	}
}

void AENGameMode::StartPlay()
{
	Super::StartPlay();
	UE_LOG(LogTemp, Warning, TEXT("EN_GAMEMODE|StartPlay"));
}

void AENGameMode::RunAutoTest()
{
	if (AENPlayerController* PC = Cast<AENPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PC->ENAutoTest();
		PC->ConsoleCommand(TEXT("quit"));
	}
}

void AENGameMode::TakePlayerShot()
{
	FScreenshotRequest::RequestScreenshot(ShotPath(TEXT("vista_jugador.png")), false, false);
	UE_LOG(LogTemp, Warning, TEXT("EN_SHOT|vista_jugador"));

	GetWorldTimerManager().SetTimer(QuitHandle, FTimerDelegate::CreateLambda([this]()
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			PC->ConsoleCommand(TEXT("quit"));
		}
	}), 3.f, false);
}

void AENGameMode::TakeAerialShot()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	const FVector CameraLocation(1500.f, -11000.f, 7000.f);
	const FVector Target(8500.f, 300.f, 0.f);
	const FRotator CameraRotation = (Target - CameraLocation).Rotation();

	ACameraActor* CameraActor = GetWorld()->SpawnActor<ACameraActor>(
		ACameraActor::StaticClass(), CameraLocation, CameraRotation);
	if (CameraActor)
	{
		CameraActor->GetCameraComponent()->SetFieldOfView(75.f);
		PC->SetViewTarget(CameraActor);
	}

	PC->ClientSetHUD(nullptr);

	GetWorldTimerManager().SetTimer(QuitHandle, FTimerDelegate::CreateLambda([this]()
	{
		FScreenshotRequest::RequestScreenshot(ShotPath(TEXT("vista_aerea.png")), false, false);
		UE_LOG(LogTemp, Warning, TEXT("EN_SHOT|vista_aerea"));

		FTimerHandle FinalQuit;
		GetWorldTimerManager().SetTimer(FinalQuit, FTimerDelegate::CreateLambda([this]()
		{
			if (APlayerController* InnerPC = UGameplayStatics::GetPlayerController(this, 0))
			{
				InnerPC->ConsoleCommand(TEXT("quit"));
			}
		}), 2.5f, false);
	}), 2.f, false);
}
