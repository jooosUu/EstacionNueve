#include "ENGameState.h"

#include "ENCharacter.h"
#include "ENCreatures.h"
#include "ENInteractable.h"
#include "ENAnchors.h"
#include "ENSaveGame.h"

#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<float> CVarDayLength(
	TEXT("EN.DayLength"),
	0.f,
	TEXT("Duracion de un dia del juego en segundos (0 = 1500)."),
	ECVF_Cheat);

AENGameState::AENGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AENGameState::BeginPlay()
{
	Super::BeginPlay();

	Day = 1;
	TimeOfDay = 0.03f;
	Money = 20;
	Debt = 1500;

	const float Override = CVarDayLength.GetValueOnGameThread();
	if (Override > 0.f)
	{
		DayLength = Override;
	}

	Objectives.Reset();
	Objectives.Add(FENObjective(TEXT("Habla con Barnaby Quill, el Mercader"), true));
	Objectives.Add(FENObjective(TEXT("Abre el porton con la valvula P1")));
	Objectives.Add(FENObjective(TEXT("Despeja el establo (3 Chillones)")));
	Objectives.Add(FENObjective(TEXT("Calma a Brisa con forraje")));
	Objectives.Add(FENObjective(TEXT("Ara, siembra y riega una parcela")));
	Objectives.Add(FENObjective(TEXT("Baja a la mina y pica 3 vetas")));
	Objectives.Add(FENObjective(TEXT("Refina resina en el taller")));
	Objectives.Add(FENObjective(TEXT("Enciende el generador")));
	Objectives.Add(FENObjective(TEXT("Sobrevive a la Carnotaurus con ruido bajo 30")));

	for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It) { Sun = *It; break; }
	for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It) { Sky = *It; break; }
	for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It) { Fog = *It; break; }

	bFoundSkyActors = Sun != nullptr;
	if (Sun && Sun->GetLightComponent())
	{
		BaseSunIntensity = Sun->GetLightComponent()->Intensity;
	}
	if (Fog && Fog->GetComponent())
	{
		BaseFogDensity = 0.008f;
	}

	UpdateSky(0.f);
	Notify(TEXT("Dia 1. La deuda con la Compania es de $1500."), FLinearColor(0.95f, 0.85f, 0.6f), 6.f);

	UE_LOG(LogTemp, Warning, TEXT("EN_GAMESTATE|BeginPlay day=%d sun=%s fog=%s"),
		Day, Sun ? TEXT("yes") : TEXT("no"), Fog ? TEXT("yes") : TEXT("no"));
}

// --------------------------------------------------------------------------- notificaciones y objetivos

void AENGameState::Notify(const FString& Text, const FLinearColor& Color, float Seconds)
{
	FENNotice Notice(Text, Color, Seconds);
	Notices.Insert(Notice, 0);
	if (Notices.Num() > 6)
	{
		Notices.SetNum(6);
	}
}

void AENGameState::CompleteObjective(int32 Index)
{
	if (!Objectives.IsValidIndex(Index) || Objectives[Index].bDone)
	{
		return;
	}

	Objectives[Index].bDone = true;
	Objectives[Index].bActive = false;
	Notify(FString::Printf(TEXT("OBJETIVO: %s"), *Objectives[Index].Title),
		FLinearColor(0.55f, 1.f, 0.6f), 6.f);

	for (int32 i = 0; i < Objectives.Num(); ++i)
	{
		if (!Objectives[i].bDone)
		{
			Objectives[i].bActive = true;
			break;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("EN_OBJECTIVE|%d|%s"), Index, *Objectives[Index].Title);
}

bool AENGameState::IsObjectiveDone(int32 Index) const
{
	return Objectives.IsValidIndex(Index) && Objectives[Index].bDone;
}

FString AENGameState::GetActiveObjectiveText() const
{
	for (const FENObjective& Objective : Objectives)
	{
		if (!Objective.bDone && Objective.bActive)
		{
			return Objective.Title;
		}
	}
	for (const FENObjective& Objective : Objectives)
	{
		if (!Objective.bDone)
		{
			return Objective.Title;
		}
	}
	return TEXT("Dia completado");
}

// --------------------------------------------------------------------------- reloj y cielo

void AENGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	for (int32 i = Notices.Num() - 1; i >= 0; --i)
	{
		Notices[i].TimeLeft -= DeltaSeconds;
		if (Notices[i].TimeLeft <= 0.f)
		{
			Notices.RemoveAt(i);
		}
	}

	if (bSleeping)
	{
		SleepTimer -= DeltaSeconds;
		if (SleepTimer <= 0.f)
		{
			bSleeping = false;
			bDayComplete = false;
			Day++;
			TimeOfDay = 0.03f;
			Furia++;
			AdvancePlots();
			Notify(FString::Printf(TEXT("Dia %d. El huerto ha crecido durante la noche."), Day),
				FLinearColor(0.95f, 0.9f, 0.65f), 7.f);
		}
	}

	UpdateClock(DeltaSeconds);
	UpdateSky(DeltaSeconds);
	UpdateAmbience(DeltaSeconds);

	if (!bNight && TimeOfDay >= 0.9f && !bDayComplete)
	{
		BeginNight();
	}

	if (bNight && !bDayComplete)
	{
		NightTimer += DeltaSeconds;
		if (NightTimer > 120.f)
		{
			EndNight();
		}
	}

	// Aviso de ruido alto, con enfriamiento.
	NoiseWarnTimer -= DeltaSeconds;
	AENCharacter* Char = GetENCharacter();
	if (Char && !Char->bDead && NoiseWarnTimer <= 0.f && Char->Noise >= 70.f)
	{
		NoiseWarnTimer = 6.f;
		Notify(TEXT("Ruido alto: algo grande te esta escuchando."), FLinearColor(1.f, 0.45f, 0.35f), 4.f);
	}
}

void AENGameState::UpdateClock(float DeltaSeconds)
{
	if (bDayComplete || bSleeping)
	{
		return;
	}

	TimeOfDay += DeltaSeconds / FMath::Max(60.f, DayLength);
	if (TimeOfDay >= 1.f)
	{
		TimeOfDay = 0.f;
		Day++;
		Furia++;
		AdvancePlots();
		if (bNight)
		{
			EndNight();
		}
		Notify(FString::Printf(TEXT("Amanece el dia %d."), Day), FLinearColor(0.95f, 0.9f, 0.7f), 5.f);
	}

	EENDayPhase NewPhase = EENDayPhase::Morning;
	if (TimeOfDay >= 0.9f)      { NewPhase = EENDayPhase::Night; }
	else if (TimeOfDay >= 0.75f) { NewPhase = EENDayPhase::Sunset; }
	else if (TimeOfDay >= 0.55f) { NewPhase = EENDayPhase::Afternoon; }
	else if (TimeOfDay >= 0.28f) { NewPhase = EENDayPhase::Midday; }

	if (NewPhase != Phase)
	{
		Phase = NewPhase;
		if (Phase == EENDayPhase::Sunset)
		{
			Notify(TEXT("Atardece: la niebla empieza a subir del barranco."), FLinearColor(1.f, 0.7f, 0.5f), 6.f);
		}
		else if (Phase == EENDayPhase::Midday)
		{
			Notify(TEXT("Mediodia. Buen momento para bajar a la mina."), FLinearColor(0.9f, 0.9f, 0.75f), 4.f);
		}
	}
}

void AENGameState::UpdateSky(float DeltaSeconds)
{
	if (!Sun || !Sun->GetLightComponent())
	{
		return;
	}

	ULightComponent* SunLight = Sun->GetLightComponent();
	const float Elevation = FMath::Clamp(FMath::Sin(TimeOfDay * PI), 0.f, 1.f);
	const float NightFactor = bNight || TimeOfDay >= 0.88f ? 1.f : 0.f;

	const float Pitch = -18.f - 62.f * Elevation * (1.f - NightFactor);
	Sun->SetActorRotation(FRotator(Pitch, 35.f, 0.f));

	const float Intensity = NightFactor > 0.f
		? 0.05f
		: BaseSunIntensity * FMath::Max(0.15f, Elevation);
	SunLight->SetIntensity(Intensity);

	FLinearColor SunColor = FLinearColor(1.f, 0.97f, 0.92f);
	if (Elevation < 0.35f)
	{
		SunColor = FLinearColor(1.f, 0.72f, 0.50f);
	}
	if (NightFactor > 0.f)
	{
		SunColor = FLinearColor(0.50f, 0.60f, 0.85f);
	}
	SunLight->SetLightColor(SunColor);

	if (Sky && Sky->GetLightComponent())
	{
		Sky->GetLightComponent()->SetIntensity(NightFactor > 0.f ? 0.12f : FMath::Lerp(0.6f, 1.2f, Elevation));
	}

	if (Fog && Fog->GetComponent())
	{
		const float NightDensity = NightFactor > 0.f ? 0.060f : BaseFogDensity;
		const float TargetDensity = Phase == EENDayPhase::Sunset ? FMath::Max(NightDensity, 0.045f) : NightDensity;
		CurrentFogDensity = FMath::FInterpTo(CurrentFogDensity, TargetDensity, DeltaSeconds, 0.8f);
		Fog->GetComponent()->SetFogDensity(CurrentFogDensity);
		Fog->GetComponent()->SetFogInscatteringColor(
			NightFactor > 0.f ? FLinearColor(0.07f, 0.09f, 0.13f) : FLinearColor(0.50f, 0.57f, 0.65f));
	}
}

void AENGameState::UpdateAmbience(float DeltaSeconds)
{
	if (!bNight)
	{
		AnnounceTimer = 0.f;
		return;
	}

	// Latido de tension: avisos mientras la Carnotaurus esta activa.
	AnnounceTimer += DeltaSeconds;
	if (AnnounceTimer > 25.f)
	{
		AnnounceTimer = 0.f;
		if (Carnotaurus && Carnotaurus->bActive)
		{
			Notify(TEXT("El suelo vibra. Quedate quieto y agachado."), FLinearColor(1.f, 0.5f, 0.4f), 4.f);
		}
	}
}

void AENGameState::AdvancePlots()
{
	for (TActorIterator<AENFarmPlot> It(GetWorld()); It; ++It)
	{
		It->AdvanceDay();
	}
}

// --------------------------------------------------------------------------- noche

void AENGameState::OnGeneratorStarted()
{
	if (bNight)
	{
		return;
	}
	TimeOfDay = FMath::Max(TimeOfDay, 0.9f);
	BeginNight();
}

void AENGameState::ForceNight()
{
	TimeOfDay = 0.9f;
	Phase = EENDayPhase::Sunset;
	if (!bNight)
	{
		BeginNight();
	}
}

void AENGameState::BeginNight()
{
	if (bNight || bDayComplete)
	{
		return;
	}

	bNight = true;
	NightTimer = 0.f;
	bNightSurvived = false;
	Furia++;

	Notify(TEXT("La sirena despierta al barranco. Algo enorme se acerca."), FLinearColor(1.f, 0.4f, 0.35f), 8.f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Carnotaurus = GetWorld()->SpawnActor<AENCarnotaurus>(
		AENCarnotaurus::StaticClass(), ENA::Canyon() + FVector(0.f, 0.f, 220.f), FRotator::ZeroRotator, Params);

	if (Carnotaurus)
	{
		Carnotaurus->ActivateAt(ENA::Canyon(), ENA::NightPatrol());
	}

	UE_LOG(LogTemp, Warning, TEXT("EN_NIGHT|begin day=%d furia=%d"), Day, Furia);
}

void AENGameState::EndNight()
{
	if (!bNight)
	{
		return;
	}

	bNight = false;
	bNightSurvived = true;
	NightTimer = 0.f;

	if (Carnotaurus)
	{
		Carnotaurus->Deactivate();
		Carnotaurus->Destroy();
		Carnotaurus = nullptr;
	}

	Notify(TEXT("La Carnotaurus se retira al barranco. Has sobrevivido a la noche."),
		FLinearColor(0.65f, 1.f, 0.7f), 8.f);
	CompleteObjective(8);
}

// --------------------------------------------------------------------------- dormir y guardar

void AENGameState::SleepAndAdvanceDay(AENCharacter* Char)
{
	if (bDayComplete || bSleeping)
	{
		return;
	}
	if (TimeOfDay < 0.72f && !bNight)
	{
		Notify(TEXT("Todavia es de dia: termina tus tareas antes de dormir."), FLinearColor(1.f, 0.85f, 0.55f), 5.f);
		return;
	}

	if (bNight)
	{
		EndNight();
	}

	SaveGame();

	bDayComplete = true;
	bSleeping = true;
	SleepTimer = 6.f;

	Notify(TEXT("Dia 1 completado. La Compania recibe tu telegrama."), FLinearColor(0.7f, 0.95f, 0.75f), 8.f);
	UE_LOG(LogTemp, Warning, TEXT("EN_DAY|complete day=%d money=%d"), Day, Money);
}

void AENGameState::SaveGame()
{
	UENSaveGame* Save = Cast<UENSaveGame>(UGameplayStatics::CreateSaveGameObject(UENSaveGame::StaticClass()));
	if (!Save)
	{
		return;
	}

	Save->Day = Day;
	Save->TimeOfDay = TimeOfDay;
	Save->Money = Money;
	Save->Debt = Debt;
	Save->StationLevel = StationLevel;
	Save->Furia = Furia;
	Save->RevolverLoaded = 6;
	Save->SavedAt = FDateTime::Now().ToString();

	if (AENCharacter* Char = GetENCharacter())
	{
		Save->Inventory = Char->Inv;
		Save->RevolverLoaded = Char->RevolverLoaded;
		Save->PlayerLocation = Char->GetActorLocation();
	}

	UGameplayStatics::SaveGameToSlot(Save, TEXT("EstacionNueve"), 0);
	Notify(TEXT("Partida guardada."), FLinearColor(0.7f, 0.9f, 1.f), 3.f);
}

bool AENGameState::LoadGame()
{
	if (!UGameplayStatics::DoesSaveGameExist(TEXT("EstacionNueve"), 0))
	{
		Notify(TEXT("No hay partida guardada."), FLinearColor(1.f, 0.7f, 0.4f), 3.f);
		return false;
	}

	UENSaveGame* Save = Cast<UENSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("EstacionNueve"), 0));
	if (!Save)
	{
		return false;
	}

	ApplyLoadedState(Save);
	Notify(TEXT("Partida cargada."), FLinearColor(0.7f, 0.9f, 1.f), 3.f);
	return true;
}

void AENGameState::ApplyLoadedState(const UENSaveGame* Save)
{
	if (!Save)
	{
		return;
	}
	Day = Save->Day;
	TimeOfDay = Save->TimeOfDay;
	Money = Save->Money;
	Debt = Save->Debt;
	StationLevel = Save->StationLevel;
	Furia = Save->Furia;

	if (AENCharacter* Char = GetENCharacter())
	{
		Char->Inv = Save->Inventory;
		Char->RevolverLoaded = Save->RevolverLoaded;
		Char->SetActorLocation(Save->PlayerLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

// --------------------------------------------------------------------------- progreso

void AENGameState::OnPlayerDied(AENCharacter* Char)
{
	Notify(TEXT("Has muerto. Vuelves al ultimo checkpoint."), FLinearColor(1.f, 0.35f, 0.3f), 5.f);
	UE_LOG(LogTemp, Warning, TEXT("EN_DEATH|day=%d"), Day);
}

void AENGameState::OnVeinMined()
{
	VeinsMined++;
	if (VeinsMined >= 3)
	{
		CompleteObjective(5);
	}
}

void AENGameState::OnDinosaurKilled()
{
	DinosaursKilled++;
	if (DinosaursKilled >= 3)
	{
		CompleteObjective(2);
	}
}

// --------------------------------------------------------------------------- consultas

AENCharacter* AENGameState::GetENCharacter() const
{
	return Cast<AENCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
}

float AENGameState::GetClockHours() const
{
	return 6.f + TimeOfDay * 18.f;
}

FString AENGameState::GetClockText() const
{
	const float Hours = GetClockHours();
	const int32 H = FMath::Clamp(FMath::FloorToInt(Hours), 0, 24);
	const int32 M = FMath::Clamp(FMath::FloorToInt((Hours - H) * 60.f), 0, 59);
	return FString::Printf(TEXT("%02d:%02d"), H, M);
}

FString AENGameState::GetPhaseName() const
{
	switch (Phase)
	{
	case EENDayPhase::Morning:   return TEXT("Manana");
	case EENDayPhase::Midday:    return TEXT("Mediodia");
	case EENDayPhase::Afternoon: return TEXT("Tarde");
	case EENDayPhase::Sunset:    return TEXT("Atardecer");
	case EENDayPhase::Night:     return TEXT("Noche");
	default: return TEXT("");
	}
}

FString AENGameState::GetDayNightText() const
{
	return bNight ? FString(TEXT("NOCHE")) : FString(TEXT("DIA"));
}
