#include "ENPlayerController.h"

#include "ENCharacter.h"
#include "ENGameState.h"
#include "ENInteractable.h"
#include "ENCreatures.h"
#include "ENAnchors.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

AENPlayerController::AENPlayerController()
{
	bShowMouseCursor = false;
}

void AENPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
	UE_LOG(LogTemp, Warning, TEXT("EN_PC|BeginPlay"));
}

void AENPlayerController::ENSelfTest()
{
	int32 Plots = 0, Veins = 0, Valves = 0, Gates = 0, Pickups = 0, Checkpoints = 0, Dinos = 0, Travels = 0;
	for (TActorIterator<AENFarmPlot> It(GetWorld()); It; ++It) { Plots++; }
	for (TActorIterator<AENAmberVein> It(GetWorld()); It; ++It) { Veins++; }
	for (TActorIterator<AENValve> It(GetWorld()); It; ++It) { Valves++; }
	for (TActorIterator<AENGate> It(GetWorld()); It; ++It) { Gates++; }
	for (TActorIterator<AENPickup> It(GetWorld()); It; ++It) { Pickups++; }
	for (TActorIterator<AENCheckpoint> It(GetWorld()); It; ++It) { Checkpoints++; }
	for (TActorIterator<AENDinosaur> It(GetWorld()); It; ++It) { Dinos++; }
	for (TActorIterator<AENTravelPoint> It(GetWorld()); It; ++It) { Travels++; }

	AENGameState* GS = Cast<AENGameState>(GetWorld()->GetGameState());
	AENCharacter* Char = Cast<AENCharacter>(GetPawn());

	const bool bStateOk = GS != nullptr && Char != nullptr;
	const bool bLevelOk = Plots >= 4 && Veins >= 10 && Valves >= 2 && Gates >= 1 && Pickups >= 5 && Checkpoints >= 5 && Travels >= 2;
	const bool bOk = bStateOk && bLevelOk;

	UE_LOG(LogTemp, Warning, TEXT("EN_SELFTEST|plots=%d|veins=%d|valves=%d|gates=%d|pickups=%d|checkpoints=%d|dinos=%d|travels=%d|state=%s|ok=%s"),
		Plots, Veins, Valves, Gates, Pickups, Checkpoints, Dinos, Travels,
		bStateOk ? TEXT("true") : TEXT("false"), bOk ? TEXT("true") : TEXT("false"));

	if (GS)
	{
		UE_LOG(LogTemp, Warning, TEXT("EN_SELFTEST_STATE|day=%d|clock=%s|money=%d|objectives=%d|phase=%s"),
			GS->Day, *GS->GetClockText(), GS->Money, GS->Objectives.Num(), *GS->GetPhaseName());
	}
	if (Char)
	{
		UE_LOG(LogTemp, Warning, TEXT("EN_SELFTEST_CHAR|health=%.0f|noise=%.0f|tool=%s|loc=%s"),
			Char->Health, Char->Noise, *Char->GetToolName(), *Char->GetActorLocation().ToString());
		Char->Notify(bOk ? TEXT("[test] Nivel y sistemas OK") : TEXT("[test] Faltan actores en el nivel"),
			bOk ? FLinearColor(0.6f, 1.f, 0.6f) : FLinearColor(1.f, 0.4f, 0.35f), 8.f);
	}
}

// --------------------------------------------------------------------------- prueba funcional

void AENPlayerController::ENAutoTest()
{
	int32 Pass = 0;
	int32 Fail = 0;

	auto Check = [&Pass, &Fail](bool bCondition, const FString& Name)
	{
		if (bCondition)
		{
			++Pass;
			UE_LOG(LogTemp, Warning, TEXT("EN_AUTOTEST|PASS|%s"), *Name);
		}
		else
		{
			++Fail;
			UE_LOG(LogTemp, Error, TEXT("EN_AUTOTEST|FAIL|%s"), *Name);
		}
	};

	AENGameState* GS = Cast<AENGameState>(GetWorld()->GetGameState());
	AENCharacter* Char = Cast<AENCharacter>(GetPawn());

	Check(GS != nullptr, TEXT("GameStateExiste"));
	Check(Char != nullptr, TEXT("CyrusExiste"));
	Check(GetHUD() != nullptr, TEXT("HUDCreado"));
	if (!GS || !Char)
	{
		UE_LOG(LogTemp, Error, TEXT("EN_AUTOTEST|RESULT=FAIL|pass=%d|fail=%d"), Pass, Fail);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("EN_AUTOTEST|START|game=%s"), *GetWorld()->GetName());

	// --- contenido del nivel
	int32 Plots = 0, Veins = 0, Valves = 0, Gates = 0, Checkpoints = 0, Dinos = 0, Travels = 0;
	for (TActorIterator<AENFarmPlot> It(GetWorld()); It; ++It) { Plots++; }
	for (TActorIterator<AENAmberVein> It(GetWorld()); It; ++It) { Veins++; }
	for (TActorIterator<AENValve> It(GetWorld()); It; ++It) { Valves++; }
	for (TActorIterator<AENGate> It(GetWorld()); It; ++It) { Gates++; }
	for (TActorIterator<AENCheckpoint> It(GetWorld()); It; ++It) { Checkpoints++; }
	for (TActorIterator<AENDinosaur> It(GetWorld()); It; ++It) { Dinos++; }
	for (TActorIterator<AENTravelPoint> It(GetWorld()); It; ++It) { Travels++; }

	Check(Plots >= 4, FString::Printf(TEXT("Parcelas>=4 (%d)"), Plots));
	Check(Veins >= 10, FString::Printf(TEXT("Vetas>=10 (%d)"), Veins));
	Check(Valves >= 2, FString::Printf(TEXT("Valvulas>=2 (%d)"), Valves));
	Check(Gates >= 1, FString::Printf(TEXT("Porton>=1 (%d)"), Gates));
	Check(Checkpoints >= 5, FString::Printf(TEXT("Checkpoints>=5 (%d)"), Checkpoints));
	Check(Dinos >= 6, FString::Printf(TEXT("Dinosaurios>=6 (%d)"), Dinos));
	Check(Travels >= 2, FString::Printf(TEXT("Viajes>=2 (%d)"), Travels));
	Check(GS->Objectives.Num() == 9, FString::Printf(TEXT("Objetivos=9 (%d)"), GS->Objectives.Num()));

	// --- el jugador aparece en el inicio
	const float StartDist = FVector::Dist2D(Char->GetActorLocation(), ENA::Start());
	Check(StartDist < 500.f, FString::Printf(TEXT("CyrusEnInicio (%.0f cm)"), StartDist));

	// --- ruido
	Char->Noise = 0.f;
	Char->AddNoise(50.f);
	Check(FMath::IsNearlyEqual(Char->Noise, 50.f, 1.f), FString::Printf(TEXT("RuidoSuma (%.0f)"), Char->Noise));
	Char->Noise = 0.f;

	// --- huerto: arar, sembrar, regar dos veces -> brote
	AENFarmPlot* Plot = nullptr;
	for (TActorIterator<AENFarmPlot> It(GetWorld()); It; ++It)
	{
		if (It->PlotIndex == 0)
		{
			Plot = *It;
			break;
		}
	}
	Check(Plot != nullptr, TEXT("Parcela0Encontrada"));
	if (Plot)
	{
		Plot->Interact(Char);
		Check(Plot->State == EENFarmState::Tilled, TEXT("Huerto_Arar"));
		Char->Inv.Seeds = 3;
		Plot->Interact(Char);
		Check(Plot->State == EENFarmState::Planted, TEXT("Huerto_Sembrar"));
		Char->BucketCharges = 3;
		Plot->Interact(Char);
		Plot->Interact(Char);
		Check(Plot->State == EENFarmState::Sprout, TEXT("Huerto_RegarDosVeces"));
		Check(GS->IsObjectiveDone(4), TEXT("Objetivo_Huerto"));
	}

	// --- valvula P1 abre el porton
	AENValve* Valve = nullptr;
	AENGate* Gate = nullptr;
	for (TActorIterator<AENValve> It(GetWorld()); It; ++It)
	{
		if (It->ValveId == TEXT("P1")) { Valve = *It; break; }
	}
	for (TActorIterator<AENGate> It(GetWorld()); It; ++It)
	{
		if (It->GateId == TEXT("P1")) { Gate = *It; break; }
	}
	Check(Valve != nullptr && Gate != nullptr, TEXT("ValvulaYPortonP1"));
	if (Valve && Gate)
	{
		Valve->Interact(Char);
		Check(Gate->bOpen, TEXT("PortonAbierto"));
		Check(GS->IsObjectiveDone(1), TEXT("Objetivo_Porton"));
	}

	// --- mineria
	AENAmberVein* Vein = nullptr;
	for (TActorIterator<AENAmberVein> It(GetWorld()); It; ++It) { Vein = *It; break; }
	Check(Vein != nullptr, TEXT("VetaEncontrada"));
	if (Vein)
	{
		const int32 Before = GS->VeinsMined;
		Vein->HitWithPick(Char);
		Check(GS->VeinsMined == Before + 1, TEXT("MinarVeta"));
	}

	// --- refineria
	AENRefinery* Refinery = nullptr;
	for (TActorIterator<AENRefinery> It(GetWorld()); It; ++It) { Refinery = *It; break; }
	Check(Refinery != nullptr, TEXT("RefineriaEncontrada"));
	if (Refinery)
	{
		Char->Inv.Resin = 2;
		const int32 RefinedBefore = Char->Inv.Refined;
		Refinery->Interact(Char);
		Check(Char->Inv.Refined == RefinedBefore + 1, TEXT("RefinarResina"));
	}

	// --- tienda
	GS->Money = 20;
	Char->OpenShop();
	Char->ShopBuy(0);
	Check(GS->Money == 17, FString::Printf(TEXT("CompraSemillas ($%d)"), GS->Money));
	Check(Char->Inv.Seeds >= 3, TEXT("SemillasRecibidas"));
	Char->CloseUI();

	// --- guardado
	GS->SaveGame();
	Check(UGameplayStatics::DoesSaveGameExist(TEXT("EstacionNueve"), 0), TEXT("GuardadoAlDormir"));

	// --- noche y Carnotaurus
	GS->ForceNight();
	Check(GS->bNight, TEXT("NocheEmpieza"));
	Check(GS->Carnotaurus != nullptr, TEXT("CarnotaurusAparece"));
	if (GS->Carnotaurus)
	{
		GS->Carnotaurus->Deactivate();
		GS->Carnotaurus->Destroy();
		GS->Carnotaurus = nullptr;
	}
	GS->bNight = false;

	// --- balance del dia
	const float Clock = GS->GetClockHours();
	Check(Clock >= 0.f && Clock <= 24.f, FString::Printf(TEXT("Reloj (%s)"), *GS->GetClockText()));

	UE_LOG(LogTemp, Warning, TEXT("EN_AUTOTEST|RESULT=%s|pass=%d|fail=%d"),
		Fail == 0 ? TEXT("PASS") : TEXT("FAIL"), Pass, Fail);

	Char->Notify(FString::Printf(TEXT("[autotest] %d ok, %d fallos"), Pass, Fail),
		Fail == 0 ? FLinearColor(0.5f, 1.f, 0.55f) : FLinearColor(1.f, 0.4f, 0.35f), 10.f);
}

// --------------------------------------------------------------------------- comandos

void AENPlayerController::ENSkipToNight()
{
	if (AENGameState* GS = Cast<AENGameState>(GetWorld()->GetGameState()))
	{
		GS->ForceNight();
	}
}

void AENPlayerController::ENSaveNow()
{
	if (AENGameState* GS = Cast<AENGameState>(GetWorld()->GetGameState()))
	{
		GS->SaveGame();
	}
}

void AENPlayerController::ENLoadNow()
{
	if (AENGameState* GS = Cast<AENGameState>(GetWorld()->GetGameState()))
	{
		GS->LoadGame();
	}
}

void AENPlayerController::ENTeleportTo(const FString& AnchorName)
{
	FVector Target = ENA::Start();
	const FString Name = AnchorName.ToLower();

	if (Name == TEXT("mine") || Name == TEXT("mina"))        { Target = ENA::MineMouth(); }
	else if (Name == TEXT("merchant") || Name == TEXT("mercader")) { Target = ENA::Merchant(); }
	else if (Name == TEXT("generator") || Name == TEXT("generador")) { Target = ENA::Generator(); }
	else if (Name == TEXT("field") || Name == TEXT("huerto")) { Target = ENA::Plot(0); }
	else if (Name == TEXT("stable") || Name == TEXT("establo")) { Target = ENA::Stable(); }
	else if (Name == TEXT("workshop") || Name == TEXT("taller")) { Target = ENA::Workshop(); }

	Target.Z += 150.f;
	if (AENCharacter* Char = Cast<AENCharacter>(GetPawn()))
	{
		Char->TravelTo(Target, 0.f);
	}
}
