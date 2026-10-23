#include "ENInteractable.h"

#include "ENCharacter.h"
#include "ENGameState.h"
#include "ENCreatures.h"

#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "TimerManager.h"

// --------------------------------------------------------------------------- base

AENInteractableActor::AENInteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	Mesh->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BasicMat.Succeeded())
	{
		Mesh->SetMaterial(0, BasicMat.Object);
	}
}

bool AENInteractableActor::CanInteract(AENCharacter* Char) const
{
	return bEnabled;
}

FString AENInteractableActor::GetPrompt(AENCharacter* Char) const
{
	return Prompt;
}

void AENInteractableActor::Interact(AENCharacter* Char)
{
}

void AENInteractableActor::SetMeshPath(const FString& Path)
{
	if (UStaticMesh* Loaded = LoadMesh(Path))
	{
		Mesh->SetStaticMesh(Loaded);
	}
}

void AENInteractableActor::SetMaterialPath(const FString& Path)
{
	if (UMaterialInterface* Mat = LoadMaterial(Path))
	{
		Mesh->SetMaterial(0, Mat);
	}
}

UStaticMesh* AENInteractableActor::LoadMesh(const FString& Path) const
{
	return LoadObject<UStaticMesh>(nullptr, *Path);
}

UMaterialInterface* AENInteractableActor::LoadMaterial(const FString& Path) const
{
	return LoadObject<UMaterialInterface>(nullptr, *Path);
}

// --------------------------------------------------------------------------- parcela

AENFarmPlot::AENFarmPlot()
{
	PlotIndex = 0;
	State = EENFarmState::Fallow;

	// Volumen invisible de interaccion de 16 x 20 m sobre la parcela.
	Mesh->SetRelativeScale3D(FVector(16.f, 20.f, 0.4f));
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Mesh->SetHiddenInGame(true);
	Mesh->SetCastShadow(false);

	CropMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Crop"));
	CropMesh->SetupAttachment(GetRootComponent());
	CropMesh->SetRelativeLocation(FVector(0.f, 0.f, 30.f));
	CropMesh->SetRelativeScale3D(FVector(0.08f, 0.08f, 0.35f));
	CropMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CropMesh->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cyl.Succeeded())
	{
		CropMesh->SetStaticMesh(Cyl.Object);
	}
}

bool AENFarmPlot::IsPlanted() const
{
	return State == EENFarmState::Planted || State == EENFarmState::Watered
		|| State == EENFarmState::Sprout || State == EENFarmState::Grown;
}

bool AENFarmPlot::CanInteract(AENCharacter* Char) const
{
	if (!Char)
	{
		return false;
	}
	switch (State)
	{
	case EENFarmState::Fallow:  return true;
	case EENFarmState::Tilled:  return Char->Inv.Seeds > 0;
	case EENFarmState::Planted: return Char->BucketCharges > 0;
	case EENFarmState::Watered: return Char->BucketCharges > 0 && WaterCount < 2;
	case EENFarmState::Sprout:  return false;
	case EENFarmState::Grown:   return true;
	default: return false;
	}
}

FString AENFarmPlot::GetPrompt(AENCharacter* Char) const
{
	switch (State)
	{
	case EENFarmState::Fallow:
		return TEXT("Arar la parcela [E]");
	case EENFarmState::Tilled:
		return Char && Char->Inv.Seeds > 0
			? FString::Printf(TEXT("Sembrar helecho (%d semillas) [E]"), Char->Inv.Seeds)
			: FString(TEXT("Sin semillas: compralas al Mercader"));
	case EENFarmState::Planted:
		return Char && Char->BucketCharges > 0
			? FString::Printf(TEXT("Regar la parcela (cubo: %d) [E]"), Char->BucketCharges)
			: FString(TEXT("Necesitas agua de la torre"));
	case EENFarmState::Watered:
		return WaterCount < 2 ? FString(TEXT("Regar otra vez [E]")) : FString(TEXT("Regada: vuelve manana"));
	case EENFarmState::Sprout:
		return TEXT("Brote: vuelve manana");
	case EENFarmState::Grown:
		return TEXT("Cosechar forraje [E]");
	default:
		return TEXT("Parcela seca");
	}
}

void AENFarmPlot::Interact(AENCharacter* Char)
{
	if (!Char)
	{
		return;
	}

	switch (State)
	{
	case EENFarmState::Fallow:
		State = EENFarmState::Tilled;
		Char->Notify(FString::Printf(TEXT("Parcela %d arada. La azada no hace ruido."), PlotIndex + 1),
			FLinearColor(0.7f, 0.9f, 0.6f));
		break;

	case EENFarmState::Tilled:
		if (Char->Inv.Seeds > 0)
		{
			Char->Inv.Seeds--;
			State = EENFarmState::Planted;
			Char->Notify(FString::Printf(TEXT("Helecho forrajero sembrado en la parcela %d."), PlotIndex + 1),
				FLinearColor(0.7f, 0.9f, 0.6f));
		}
		break;

	case EENFarmState::Planted:
		if (Char->BucketCharges > 0)
		{
			Char->BucketCharges--;
			WaterCount++;
			bWateredToday = true;
			State = WaterCount >= 2 ? EENFarmState::Sprout : EENFarmState::Watered;
			if (State == EENFarmState::Sprout)
			{
				Char->Notify(TEXT("La parcela ya tiene brotes. Manana habra forraje."), FLinearColor(0.6f, 1.f, 0.7f));
				if (AENGameState* GS = Cast<AENGameState>(GetWorld()->GetGameState()))
				{
					GS->CompleteObjective(4);
				}
			}
			else
			{
				Char->Notify(TEXT("Primer riego hecho. El helecho pide un segundo riego."), FLinearColor(0.7f, 0.85f, 1.f));
			}
		}
		break;

	case EENFarmState::Watered:
		if (WaterCount < 2 && Char->BucketCharges > 0)
		{
			Char->BucketCharges--;
			WaterCount++;
			bWateredToday = true;
			State = WaterCount >= 2 ? EENFarmState::Sprout : EENFarmState::Watered;
			if (State == EENFarmState::Sprout)
			{
				Char->Notify(TEXT("La parcela ya tiene brotes. Manana habra forraje."), FLinearColor(0.6f, 1.f, 0.7f));
				if (AENGameState* GS = Cast<AENGameState>(GetWorld()->GetGameState()))
				{
					GS->CompleteObjective(4);
				}
			}
		}
		break;

	case EENFarmState::Grown:
		Char->Inv.Forage += 3;
		Char->Notify(TEXT("+3 forraje al corral."), FLinearColor(0.6f, 1.f, 0.7f));
		if (AENGameState* GS = Cast<AENGameState>(GetWorld()->GetGameState()))
		{
			GS->PlotsHarvested++;
		}
		State = EENFarmState::Fallow;
		WaterCount = 0;
		bWateredToday = false;
		break;

	default:
		break;
	}

	UpdateVisual();
}

void AENFarmPlot::AdvanceDay()
{
	bWateredToday = false;

	if (State == EENFarmState::Sprout)
	{
		State = EENFarmState::Grown;
	}
	else if (State == EENFarmState::Watered && WaterCount >= 2)
	{
		State = EENFarmState::Sprout;
	}
	else if (State == EENFarmState::Planted || State == EENFarmState::Watered)
	{
		DryDays++;
		if (DryDays >= 2)
		{
			State = EENFarmState::Dead;
		}
	}

	UpdateVisual();
}

void AENFarmPlot::UpdateVisual()
{
	const bool bVisible = IsPlanted();
	CropMesh->SetVisibility(bVisible);

	if (!bVisible)
	{
		return;
	}

	float Height = 0.18f;
	switch (State)
	{
	case EENFarmState::Planted: Height = 0.10f; break;
	case EENFarmState::Watered: Height = 0.18f; break;
	case EENFarmState::Sprout:  Height = 0.35f; break;
	case EENFarmState::Grown:   Height = 0.75f; break;
	default: break;
	}
	CropMesh->SetRelativeScale3D(FVector(0.35f, 0.35f, Height));

	if (UMaterialInterface* Mat = LoadMaterial(State == EENFarmState::Grown
		? TEXT("/Game/Materials/M_CropRipe.M_CropRipe")
		: TEXT("/Game/Materials/M_Crop.M_Crop")))
	{
		CropMesh->SetMaterial(0, Mat);
	}
}

// --------------------------------------------------------------------------- torre de agua

AENWaterTower::AENWaterTower()
{
	Prompt = TEXT("Llenar el cubo");
	Mesh->SetRelativeScale3D(FVector(4.4f, 4.4f, 1.6f));
}

FString AENWaterTower::GetPrompt(AENCharacter* Char) const
{
	if (!Char)
	{
		return Prompt;
	}
	return Char->BucketCharges >= 3
		? FString(TEXT("El cubo ya esta lleno (3 riegos)"))
		: FString::Printf(TEXT("Llenar el cubo en la torre (tienes %d) [E]"), Char->BucketCharges);
}

void AENWaterTower::Interact(AENCharacter* Char)
{
	if (!Char)
	{
		return;
	}
	Char->BucketCharges = 3;
	Char->Notify(TEXT("Cubo lleno: 3 riegos."), FLinearColor(0.55f, 0.8f, 1.f));
}

// --------------------------------------------------------------------------- valvula

AENValve::AENValve()
{
	Prompt = TEXT("Girar la valvula");
	Mesh->SetRelativeScale3D(FVector(0.5f, 1.2f, 1.2f));
}

bool AENValve::CanInteract(AENCharacter* Char) const
{
	return !bUsed;
}

FString AENValve::GetPrompt(AENCharacter* Char) const
{
	return bUsed ? FString(TEXT("Valvula abierta")) : Prompt;
}

void AENValve::Interact(AENCharacter* Char)
{
	if (bUsed || !Char)
	{
		return;
	}
	bUsed = true;

	AddActorLocalRotation(FRotator(0.f, 0.f, 90.f));
	Char->AddNoise(60.f);
	Char->Notify(TEXT("La valvula escupe vapor."), FLinearColor(0.9f, 0.8f, 0.6f));

	// Abre los portones que comparten identificador.
	for (TActorIterator<AENGate> It(GetWorld()); It; ++It)
	{
		if (It->GateId.Equals(ValveId, ESearchCase::IgnoreCase))
		{
			It->OpenGate();
		}
	}

	// El chorro de vapor aturde y quema a los dinosaurios cercanos.
	if (bSteamTrap)
	{
		for (TActorIterator<AENDinosaur> It(GetWorld()); It; ++It)
		{
			if (FVector::Dist(It->GetActorLocation(), GetActorLocation()) < 600.f)
			{
				It->TakeDamage(60.f, FDamageEvent(), nullptr, this);
			}
		}
	}
}

// --------------------------------------------------------------------------- porton

AENGate::AENGate()
{
	Prompt = TEXT("Porton");
	Mesh->SetRelativeScale3D(FVector(0.6f, 12.f, 5.f));
}

FString AENGate::GetPrompt(AENCharacter* Char) const
{
	return bOpen ? FString(TEXT("El porton esta abierto"))
		: FString(TEXT("Porton atascado: gira la valvula de vapor"));
}

void AENGate::Interact(AENCharacter* Char)
{
	if (!Char)
	{
		return;
	}
	Char->Notify(bOpen ? TEXT("El porton ya esta abierto.")
		: TEXT("El porton no cede: falta presion en la valvula."),
		FLinearColor(1.f, 0.75f, 0.4f));
}

void AENGate::OpenGate()
{
	if (bOpen)
	{
		return;
	}
	if (ClosedLocation.IsNearlyZero())
	{
		ClosedLocation = GetActorLocation();
	}
	bOpen = true;
	SetActorLocation(ClosedLocation + FVector(0.f, 0.f, 520.f));

	if (AENGameState* GS = Cast<AENGameState>(GetWorld()->GetGameState()))
	{
		GS->Notify(TEXT("El porton se abre con un golpe de presion."), FLinearColor(0.8f, 0.95f, 0.7f));
		if (GateId.Equals(TEXT("P1"), ESearchCase::IgnoreCase))
		{
			GS->CompleteObjective(1);
		}
	}
}

// --------------------------------------------------------------------------- veta

AENAmberVein::AENAmberVein()
{
	Prompt = TEXT("Picar la veta");
	Mesh->SetRelativeScale3D(FVector(1.4f, 1.4f, 2.2f));
	Mesh->SetMobility(EComponentMobility::Movable);
}

bool AENAmberVein::CanInteract(AENCharacter* Char) const
{
	return HitsLeft > 0;
}

FString AENAmberVein::GetPrompt(AENCharacter* Char) const
{
	if (HitsLeft <= 0)
	{
		return TEXT("Veta agotada");
	}
	if (Char && Char->CurrentTool == EENTool::Pickaxe)
	{
		return FString::Printf(TEXT("Picar %s [E]"), *ENLootName(Loot));
	}
	return FString(TEXT("Necesitas el pico (tecla 2)"));
}

void AENAmberVein::Interact(AENCharacter* Char)
{
	if (Char && Char->CurrentTool == EENTool::Pickaxe)
	{
		HitWithPick(Char);
	}
	else if (Char)
	{
		Char->Notify(TEXT("Equipa el pico con la tecla 2."), FLinearColor(1.f, 0.8f, 0.4f));
	}
}

bool AENAmberVein::HitWithPick(AENCharacter* Char)
{
	if (!Char || HitsLeft <= 0)
	{
		return false;
	}

	HitsLeft--;
	Char->Inv.AddLoot(Loot, AmountPerHit);
	Char->AddNoise(15.f);
	Char->Notify(FString::Printf(TEXT("+%d %s"), AmountPerHit, *ENLootName(Loot)), FLinearColor(1.f, 0.8f, 0.35f));

	if (AENGameState* GS = Cast<AENGameState>(GetWorld()->GetGameState()))
	{
		GS->OnVeinMined();
	}

	if (HitsLeft <= 0)
	{
		Mesh->SetVisibility(false);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	else
	{
		SetActorScale3D(GetActorScale3D() * 0.92f);
	}

	return HitsLeft <= 0;
}

// --------------------------------------------------------------------------- recogible

AENPickup::AENPickup()
{
	Prompt = TEXT("Recoger");
	Mesh->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.2f));
}

FString AENPickup::GetPrompt(AENCharacter* Char) const
{
	return FString::Printf(TEXT("Recoger %d %s [E]"), Amount, *ENLootName(Loot));
}

void AENPickup::Interact(AENCharacter* Char)
{
	if (!Char)
	{
		return;
	}
	Char->Inv.AddLoot(Loot, Amount);
	Char->Notify(FString::Printf(TEXT("+%d %s"), Amount, *ENLootName(Loot)), FLinearColor(0.95f, 0.85f, 0.5f));
	Destroy();
}

// --------------------------------------------------------------------------- mercader

AENMerchant::AENMerchant()
{
	Prompt = TEXT("Comerciar con Barnaby Quill");
	Mesh->SetRelativeScale3D(FVector(1.5f, 1.5f, 2.2f));
}

FString AENMerchant::GetPrompt(AENCharacter* Char) const
{
	return Prompt;
}

void AENMerchant::Interact(AENCharacter* Char)
{
	if (!Char)
	{
		return;
	}
	Char->OpenShop();
	if (AENGameState* GS = Cast<AENGameState>(GetWorld()->GetGameState()))
	{
		GS->CompleteObjective(0);
		GS->Notify(TEXT("Barnaby: \"Donde ellos disparan, la bestia llega.\""), FLinearColor(1.f, 0.9f, 0.7f), 6.f);
	}
}

// --------------------------------------------------------------------------- refineria

AENRefinery::AENRefinery()
{
	Prompt = TEXT("Refinar resina (1 en bruto)");
	Mesh->SetRelativeScale3D(FVector(1.6f, 1.6f, 2.4f));
}

FString AENRefinery::GetPrompt(AENCharacter* Char) const
{
	if (Char && Char->Inv.Resin <= 0)
	{
		return TEXT("Sin resina en bruto: baja a la mina");
	}
	return TEXT("Refinar 1 resina [E]");
}

void AENRefinery::Interact(AENCharacter* Char)
{
	if (!Char || Char->Inv.Resin <= 0)
	{
		return;
	}
	Char->Inv.Resin--;
	Char->Inv.Refined++;
	Char->AddNoise(55.f);
	Char->Notify(TEXT("Resina refinada. El horno hace ruido."), FLinearColor(1.f, 0.7f, 0.35f));
	if (AENGameState* GS = Cast<AENGameState>(GetWorld()->GetGameState()))
	{
		GS->CompleteObjective(6);
	}
}

// --------------------------------------------------------------------------- generador

AENGenerator::AENGenerator()
{
	Prompt = TEXT("Encender el generador (2 resina refinada)");
	Mesh->SetRelativeScale3D(FVector(3.2f, 2.2f, 3.5f));

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(GetRootComponent());
	Glow->SetRelativeLocation(FVector(0.f, 0.f, 250.f));
	Glow->SetIntensity(0.f);
	Glow->SetAttenuationRadius(1400.f);
	Glow->SetLightColor(FLinearColor(1.f, 0.65f, 0.25f));
}

bool AENGenerator::CanInteract(AENCharacter* Char) const
{
	return !bOn;
}

FString AENGenerator::GetPrompt(AENCharacter* Char) const
{
	if (bOn)
	{
		return TEXT("El generador ruge: la sirena llama a la Carnotaurus");
	}
	if (Char && Char->Inv.Refined < 2)
	{
		return FString::Printf(TEXT("Falta combustible: %d/2 resina refinada"), Char->Inv.Refined);
	}
	return Prompt;
}

void AENGenerator::Interact(AENCharacter* Char)
{
	if (!Char || bOn)
	{
		return;
	}
	if (Char->Inv.Refined < 2)
	{
		Char->Notify(TEXT("El generador necesita 2 de resina refinada."), FLinearColor(1.f, 0.7f, 0.4f));
		return;
	}

	Char->Inv.Refined -= 2;
	bOn = true;
	Glow->SetIntensity(6000.f);
	Char->AddNoise(100.f);
	Char->Notify(TEXT("Sirena encendida. Algo enorme se mueve en el barranco."), FLinearColor(1.f, 0.5f, 0.35f), 7.f);

	if (AENGameState* GS = Cast<AENGameState>(GetWorld()->GetGameState()))
	{
		GS->CompleteObjective(7);
		GS->OnGeneratorStarted();
	}
}

// --------------------------------------------------------------------------- cama

AENSleepSpot::AENSleepSpot()
{
	Prompt = TEXT("Dormir y guardar la partida");
	Mesh->SetRelativeScale3D(FVector(2.f, 1.f, 0.6f));
}

FString AENSleepSpot::GetPrompt(AENCharacter* Char) const
{
	if (AENGameState* GS = Cast<AENGameState>(GetWorld()->GetGameState()))
	{
		if (GS->bDayComplete)
		{
			return TEXT("El dia ha terminado");
		}
		if (GS->TimeOfDay < 0.72f && !GS->bNight)
		{
			return TEXT("Dormir (mejor esperar al atardecer)");
		}
	}
	return Prompt;
}

void AENSleepSpot::Interact(AENCharacter* Char)
{
	if (AENGameState* GS = Cast<AENGameState>(GetWorld()->GetGameState()))
	{
		GS->SleepAndAdvanceDay(Char);
	}
}

// --------------------------------------------------------------------------- viaje

AENTravelPoint::AENTravelPoint()
{
	Prompt = TEXT("Bajar a la caverna");
	Mesh->SetRelativeScale3D(FVector(2.f, 2.f, 1.f));
}

FString AENTravelPoint::GetPrompt(AENCharacter* Char) const
{
	return Prompt;
}

void AENTravelPoint::Interact(AENCharacter* Char)
{
	if (!Char)
	{
		return;
	}
	Char->TravelTo(TargetLocation, TargetYaw);
}

// --------------------------------------------------------------------------- checkpoint

AENCheckpoint::AENCheckpoint()
{
	Prompt = TEXT("Checkpoint");
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeScale3D(FVector(0.3f, 0.3f, 3.5f));
	Mesh->SetVisibility(false);

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(GetRootComponent());
	Trigger->SetBoxExtent(FVector(500.f, 500.f, 300.f));
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetGenerateOverlapEvents(true);
}

void AENCheckpoint::BeginPlay()
{
	Super::BeginPlay();
	if (Trigger)
	{
		Trigger->OnComponentBeginOverlap.AddDynamic(this, &AENCheckpoint::OnTriggerBegin);
	}
}

void AENCheckpoint::OnTriggerBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep)
{
	AENCharacter* Char = Cast<AENCharacter>(OtherActor);
	if (!Char)
	{
		return;
	}

	Char->SetCheckpoint(GetActorLocation() + FVector(0.f, 0.f, 120.f), GetActorRotation());

	if (!bActivated)
	{
		bActivated = true;
		Char->Notify(TEXT("Checkpoint: tu progreso queda registrado."), FLinearColor(0.6f, 0.9f, 1.f), 3.f);
	}
}

// --------------------------------------------------------------------------- Brisa

AENMount::AENMount()
{
	PrimaryActorTick.bCanEverTick = true;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(GetRootComponent());
	Body->SetRelativeScale3D(FVector(3.2f, 1.3f, 1.5f));
	Body->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeAsset.Succeeded())
	{
		Body->SetStaticMesh(CubeAsset.Object);
	}

	Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
	Head->SetupAttachment(GetRootComponent());
	Head->SetRelativeScale3D(FVector(1.1f, 0.55f, 0.6f));
	Head->SetRelativeLocation(FVector(190.f, 0.f, 150.f));
	Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeAsset(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeAsset.Succeeded())
	{
		Head->SetStaticMesh(ConeAsset.Object);
	}

	Mesh->SetRelativeScale3D(FVector(3.2f, 1.3f, 1.5f));
	Mesh->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetVisibility(false);
}

void AENMount::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInterface* Mat = LoadMaterial(TEXT("/Game/Materials/M_Mount.M_Mount")))
	{
		Body->SetMaterial(0, Mat);
		Head->SetMaterial(0, Mat);
	}
}

bool AENMount::CanInteract(AENCharacter* Char) const
{
	return !bMounted;
}

FString AENMount::GetPrompt(AENCharacter* Char) const
{
	if (bMounted)
	{
		return TEXT("Brisa te lleva");
	}
	if (!bCalmed)
	{
		return Char && Char->Inv.Forage > 0
			? FString::Printf(TEXT("Dar forraje a Brisa (%d en la mochila) [E]"), Char->Inv.Forage)
			: FString(TEXT("Brisa esta asustada: necesita forraje"));
	}
	return TEXT("Montar a Brisa [E]");
}

void AENMount::Interact(AENCharacter* Char)
{
	if (!Char || bMounted)
	{
		return;
	}

	if (!bCalmed)
	{
		if (Char->Inv.Forage <= 0)
		{
			Char->Notify(TEXT("No tienes forraje: cosecha helecho o compra semillas."), FLinearColor(1.f, 0.7f, 0.4f));
			return;
		}
		Char->Inv.Forage--;
		bCalmed = true;
		Char->Notify(TEXT("Brisa come de tu mano y se deja ensillar."), FLinearColor(0.7f, 1.f, 0.6f));
		if (AENGameState* GS = Cast<AENGameState>(GetWorld()->GetGameState()))
		{
			GS->CompleteObjective(3);
		}
		return;
	}

	Char->ToggleRide(this);
}

void AENMount::WhistleTo(const FVector& Location)
{
	bComingToWhistle = true;
}

void AENMount::SetMounted(bool bInMounted)
{
	bMounted = bInMounted;
	SetActorHiddenInGame(bInMounted);
	SetActorEnableCollision(!bInMounted);
}

void AENMount::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AENCharacter* Char = Cast<AENCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!Char)
	{
		return;
	}

	if (bComingToWhistle && !bMounted)
	{
		const FVector Target = Char->GetActorLocation() - Char->GetActorForwardVector() * 350.f;
		FVector ToTarget = Target - GetActorLocation();
		ToTarget.Z = 0.f;
		const float Dist = ToTarget.Size();
		if (Dist < 250.f)
		{
			bComingToWhistle = false;
		}
		else
		{
			SetActorLocation(GetActorLocation() + ToTarget.GetSafeNormal() * FMath::Min(900.f * DeltaSeconds, Dist));
			SetActorRotation(ToTarget.Rotation());
		}
	}

	// Un leve cabeceo para que Brisa se sienta viva.
	const float Bob = FMath::Sin(GetWorld()->GetTimeSeconds() * 2.f) * 2.f;
	Head->SetRelativeLocation(FVector(190.f, 0.f, 150.f + Bob));
}
