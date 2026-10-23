#include "ENCreatures.h"

#include "ENCharacter.h"
#include "ENGameState.h"
#include "ENInteractable.h"
#include "ENAnchors.h"

#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

// --------------------------------------------------------------------------- dinosaurio salvaje

AENDinosaur::AENDinosaur()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(45.f, 70.f);
	GetCharacterMovement()->MaxWalkSpeed = 380.f;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 420.f, 0.f);
	bUseControllerRotationYaw = false;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(GetRootComponent());
	Body->SetRelativeScale3D(FVector(1.5f, 0.6f, 0.65f));
	Body->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
	Head->SetupAttachment(GetRootComponent());
	Head->SetRelativeScale3D(FVector(0.6f, 0.35f, 0.35f));
	Head->SetRelativeLocation(FVector(90.f, 0.f, 35.f));
	Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Tail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tail"));
	Tail->SetupAttachment(GetRootComponent());
	Tail->SetRelativeScale3D(FVector(0.8f, 0.18f, 0.18f));
	Tail->SetRelativeLocation(FVector(-100.f, 0.f, 18.f));
	Tail->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeAsset.Succeeded())
	{
		Body->SetStaticMesh(CubeAsset.Object);
		Tail->SetStaticMesh(CubeAsset.Object);
	}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeAsset(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeAsset.Succeeded())
	{
		Head->SetStaticMesh(ConeAsset.Object);
	}
}

void AENDinosaur::BeginPlay()
{
	Super::BeginPlay();
	HomeLocation = GetActorLocation();

	if (UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Dino.M_Dino")))
	{
		Body->SetMaterial(0, Mat);
		Head->SetMaterial(0, Mat);
		Tail->SetMaterial(0, Mat);
	}

	ConfigureAsRaptor();
}

void AENDinosaur::ConfigureAsRaptor()
{
	if (!bIsRaptor)
	{
		return;
	}

	Health = 70.f;
	MoveSpeed = 500.f;
	AttackDamage = 15.f;
	AggroRadius = 1800.f;

	GetCapsuleComponent()->SetCapsuleSize(60.f, 100.f);
	Body->SetRelativeScale3D(FVector(2.4f, 0.8f, 0.9f));
	Head->SetRelativeScale3D(FVector(0.9f, 0.45f, 0.45f));
	Head->SetRelativeLocation(FVector(140.f, 0.f, 50.f));
	Tail->SetRelativeScale3D(FVector(1.4f, 0.25f, 0.25f));
	Tail->SetRelativeLocation(FVector(-160.f, 0.f, 25.f));

	if (UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Raptor.M_Raptor")))
	{
		Body->SetMaterial(0, Mat);
		Head->SetMaterial(0, Mat);
		Tail->SetMaterial(0, Mat);
	}
}

AENCharacter* AENDinosaur::GetPlayer() const
{
	return Cast<AENCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
}

void AENDinosaur::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateAI(DeltaSeconds);
}

void AENDinosaur::UpdateAI(float DeltaSeconds)
{
	if (State == EENDinoState::Dead)
	{
		return;
	}

	AttackCooldown = FMath::Max(0.f, AttackCooldown - DeltaSeconds);
	AENCharacter* Player = GetPlayer();
	if (!Player || Player->bDead)
	{
		GetCharacterMovement()->StopMovementImmediately();
		return;
	}

	const float Dist = FVector::Dist(Player->GetActorLocation(), GetActorLocation());
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;

	const bool bPlayerLoud = Player->Noise >= 55.f;
	if (Dist < AggroRadius && (Dist < AggroRadius * 0.6f || bPlayerLoud))
	{
		State = Dist < 200.f ? EENDinoState::Attack : EENDinoState::Chase;
	}
	else if (State == EENDinoState::Chase || State == EENDinoState::Attack)
	{
		State = EENDinoState::Idle;
	}

	if (State == EENDinoState::Chase || State == EENDinoState::Attack)
	{
		FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
		ToPlayer.Z = 0.f;
		if (ToPlayer.SizeSquared() > 100.f)
		{
			AddMovementInput(ToPlayer.GetSafeNormal(), 1.f);
		}

		if (State == EENDinoState::Attack)
		{
			Bite();
		}
	}
	else
	{
		// Vagar cerca de casa.
		IdleWanderTimer -= DeltaSeconds;
		if (IdleWanderTimer <= 0.f)
		{
			IdleWanderTimer = FMath::FRandRange(2.f, 5.f);
			const FVector Wander = HomeLocation + FVector(FMath::FRandRange(-600.f, 600.f), FMath::FRandRange(-600.f, 600.f), 0.f);
			FVector ToHome = Wander - GetActorLocation();
			ToHome.Z = 0.f;
			if (ToHome.SizeSquared() > 100.f)
			{
				AddMovementInput(ToHome.GetSafeNormal(), 0.4f);
			}
		}
	}
}

void AENDinosaur::Bite()
{
	if (AttackCooldown > 0.f)
	{
		return;
	}
	AttackCooldown = 1.4f;
	if (AENCharacter* Player = GetPlayer())
	{
		Player->ApplyDamageEN(AttackDamage, this);
	}
}

float AENDinosaur::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (State == EENDinoState::Dead)
	{
		return 0.f;
	}

	Health -= DamageAmount;

	if (AENCharacter* Player = GetPlayer())
	{
		State = EENDinoState::Chase;
		if (Player->bDead)
		{
			return DamageAmount;
		}
	}

	if (Health <= 0.f)
	{
		Die();
	}

	return DamageAmount;
}

void AENDinosaur::Die()
{
	if (bDeadHandled)
	{
		return;
	}
	bDeadHandled = true;
	State = EENDinoState::Dead;
	Health = 0.f;

	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Body->SetRelativeRotation(FRotator(0.f, 0.f, 75.f));
	Body->SetRelativeLocation(FVector(0.f, 0.f, -30.f));
	Tail->SetVisibility(false);
	Head->SetVisibility(false);

	if (AENGameState* GS = Cast<AENGameState>(GetWorld()->GetGameState()))
	{
		GS->OnDinosaurKilled();
		GS->Notify(TEXT("El dinosaurio cae."), FLinearColor(0.9f, 0.85f, 0.6f), 2.5f);
	}

	SetLifeSpan(4.f);
}

// --------------------------------------------------------------------------- Carnotaurus

AENCarnotaurus::AENCarnotaurus()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(180.f, 220.f);
	GetCharacterMovement()->MaxWalkSpeed = 300.f;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	bUseControllerRotationYaw = false;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(GetRootComponent());
	Body->SetRelativeScale3D(FVector(8.f, 3.2f, 3.6f));
	Body->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
	Head->SetupAttachment(GetRootComponent());
	Head->SetRelativeScale3D(FVector(2.6f, 1.4f, 1.4f));
	Head->SetRelativeLocation(FVector(430.f, 0.f, 160.f));
	Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	EyeL = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EyeL"));
	EyeL->SetupAttachment(GetRootComponent());
	EyeL->SetRelativeScale3D(FVector(0.12f, 0.1f, 0.1f));
	EyeL->SetRelativeLocation(FVector(520.f, -55.f, 230.f));
	EyeL->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	EyeR = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EyeR"));
	EyeR->SetupAttachment(GetRootComponent());
	EyeR->SetRelativeScale3D(FVector(0.12f, 0.1f, 0.1f));
	EyeR->SetRelativeLocation(FVector(520.f, 55.f, 230.f));
	EyeR->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeAsset.Succeeded())
	{
		Body->SetStaticMesh(CubeAsset.Object);
	}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeAsset(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeAsset.Succeeded())
	{
		Head->SetStaticMesh(ConeAsset.Object);
	}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereAsset.Succeeded())
	{
		EyeL->SetStaticMesh(SphereAsset.Object);
		EyeR->SetStaticMesh(SphereAsset.Object);
	}

	EyeGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("EyeGlow"));
	EyeGlow->SetupAttachment(GetRootComponent());
	EyeGlow->SetRelativeLocation(FVector(520.f, 0.f, 240.f));
	EyeGlow->SetIntensity(2500.f);
	EyeGlow->SetAttenuationRadius(900.f);
	EyeGlow->SetLightColor(FLinearColor(1.f, 0.12f, 0.06f));
	EyeGlow->SetCastShadows(false);
}

void AENCarnotaurus::BeginPlay()
{
	Super::BeginPlay();

	if (UMaterialInterface* BodyMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Carnotaurus.M_Carnotaurus")))
	{
		Body->SetMaterial(0, BodyMat);
		Head->SetMaterial(0, BodyMat);
	}
	if (UMaterialInterface* EyeMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_EyeGlow.M_EyeGlow")))
	{
		EyeL->SetMaterial(0, EyeMat);
		EyeR->SetMaterial(0, EyeMat);
	}

	// Sin gravedad: la persecucion se resuelve en el plano del suelo.
	GetCharacterMovement()->SetMovementMode(MOVE_None);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

AENGameState* AENCarnotaurus::GetENGameState() const
{
	return Cast<AENGameState>(GetWorld()->GetGameState());
}

AENCharacter* AENCarnotaurus::GetPlayer() const
{
	return Cast<AENCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
}

void AENCarnotaurus::ActivateAt(const FVector& Start, const TArray<FVector>& InPatrol)
{
	SetActorLocation(FVector(Start.X, Start.Y, 240.f), false, nullptr, ETeleportType::TeleportPhysics);
	Patrol = InPatrol;
	State = EState::Rising;
	StateTimer = 0.f;
	bActive = true;
	PatrolIndex = 0;
	StateName = TEXT("emergiendo");
	Roar(TEXT("Un rugido grave hace temblar la niebla."));
}

void AENCarnotaurus::Deactivate()
{
	bActive = false;
	State = EState::Inactive;
}

void AENCarnotaurus::DistractAt(const FVector& Location)
{
	DistractLocation = Location;
	DistractTimer = 20.f;
	State = EState::Distracted;
	StateName = TEXT("distraida");
}

void AENCarnotaurus::Roar(const FString& Text)
{
	if (AENGameState* GS = GetENGameState())
	{
		GS->Notify(Text, FLinearColor(1.f, 0.35f, 0.3f), 6.f);
	}
}

void AENCarnotaurus::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bActive)
	{
		UpdateAI(DeltaSeconds);
	}
}

void AENCarnotaurus::UpdateAI(float DeltaSeconds)
{
	AENCharacter* Player = GetPlayer();
	AENGameState* GS = GetENGameState();
	StateTimer += DeltaSeconds;

	// Una bengala cercana la aparta de su presa.
	for (TActorIterator<AENFlare> It(GetWorld()); It; ++It)
	{
		if (FVector::Dist(It->GetActorLocation(), GetActorLocation()) < 2500.f && State != EState::Distracted)
		{
			DistractAt(It->GetActorLocation());
			Roar(TEXT("La luz de la bengala la desvia."));
			break;
		}
	}

	const FVector MyLocation = GetActorLocation();
	const float PlayerDist = Player ? FVector::Dist(Player->GetActorLocation(), MyLocation) : 1.e9f;
	const float PlayerNoise = Player ? Player->Noise : 0.f;

	switch (State)
	{
	case EState::Rising:
		StateName = TEXT("emergiendo");
		if (StateTimer > 2.5f)
		{
			State = EState::Stalk;
			StateTimer = 0.f;
			StateName = TEXT("acechando");
		}
		break;

	case EState::Stalk:
		StateName = TEXT("acechando");
		// Patrulla alrededor del generador mientras el ruido sea bajo.
		if (Patrol.Num() > 0)
		{
			const FVector Target = Patrol[PatrolIndex % Patrol.Num()];
			MoveTowards(Target, 260.f, DeltaSeconds);
			if (FVector::Dist2D(MyLocation, Target) < 200.f)
			{
				PatrolIndex++;
			}
		}
		LoudTimer = PlayerNoise > 35.f ? LoudTimer + DeltaSeconds : 0.f;
		if (LoudTimer > 1.5f && PlayerDist < 3500.f)
		{
			State = EState::Investigate;
			StateTimer = 0.f;
			StateName = TEXT("investigando");
		}
		else if (StateTimer > 45.f)
		{
			State = EState::Leaving;
			StateTimer = 0.f;
			StateName = TEXT("retirandose");
		}
		break;

	case EState::Investigate:
		StateName = TEXT("investigando");
		if (Player)
		{
			MoveTowards(Player->GetActorLocation(), 340.f, DeltaSeconds);
		}
		if (PlayerDist < 900.f)
		{
			if (PlayerNoise > 30.f || !Player->bIsCrouched)
			{
				State = EState::Charge;
				StateTimer = 0.f;
				StateName = TEXT("cargando");
				Roar(TEXT("La Carnotaurus te ha visto. Calla y agachate."));
			}
			else
			{
				// Sigilo conseguido: olfatea y se aleja.
				State = EState::Stalk;
				StateTimer = 0.f;
				if (GS)
				{
					GS->Notify(TEXT("Te huele... pero el silencio la confunde."), FLinearColor(0.7f, 0.95f, 0.75f), 4.f);
				}
			}
		}
		if (StateTimer > 20.f)
		{
			State = EState::Stalk;
			StateTimer = 0.f;
		}
		break;

	case EState::Charge:
		StateName = TEXT("cargando");
		if (Player)
		{
			MoveTowards(Player->GetActorLocation(), 1100.f, DeltaSeconds);
		}
		if (PlayerDist < 500.f)
		{
			KillPlayer(Player);
		}
		if (StateTimer > 4.f)
		{
			State = EState::Stalk;
			StateTimer = 0.f;
			PatrolIndex++;
		}
		break;

	case EState::Distracted:
		StateName = TEXT("distraida");
		MoveTowards(DistractLocation, 420.f, DeltaSeconds);
		DistractTimer -= DeltaSeconds;
		if (DistractTimer <= 0.f)
		{
			State = EState::Stalk;
			StateTimer = 0.f;
		}
		break;

	case EState::Leaving:
		StateName = TEXT("retirandose");
		MoveTowards(ENA::Canyon(), 500.f, DeltaSeconds);
		if (StateTimer > 10.f)
		{
			Deactivate();
		}
		break;

	default:
		break;
	}

	// Los ojos siempre miran al jugador mas cercano cuando esta cerca.
	if (Player && PlayerDist < 2000.f)
	{
		const FRotator Look = (Player->GetActorLocation() - MyLocation).Rotation();
		SetActorRotation(FRotator(0.f, Look.Yaw, 0.f));
	}
}

void AENCarnotaurus::MoveTowards(const FVector& Target, float Speed, float DeltaSeconds)
{
	FVector Direction = Target - GetActorLocation();
	Direction.Z = 0.f;
	const float Distance = Direction.Size();
	if (Distance < 1.f)
	{
		return;
	}

	Direction /= Distance;
	const FVector NewLocation = GetActorLocation() + Direction * FMath::Min(Speed * DeltaSeconds, Distance);
	SetActorLocation(FVector(NewLocation.X, NewLocation.Y, 240.f), false, nullptr, ETeleportType::TeleportPhysics);
	SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));
}

void AENCarnotaurus::KillPlayer(AENCharacter* Player)
{
	if (!Player || Player->bDead)
	{
		return;
	}
	Roar(TEXT("La Carnotaurus te alcanza. Todo se apaga."));
	Player->ApplyDamageEN(1000.f, this);
}

float AENCarnotaurus::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (AENGameState* GS = GetENGameState())
	{
		GS->Notify(TEXT("La Carnotaurus no se puede matar. Corre o calla."), FLinearColor(1.f, 0.4f, 0.35f), 3.f);
	}
	return 0.f;
}

// --------------------------------------------------------------------------- bengala

AENFlare::AENFlare()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetRelativeScale3D(FVector(0.12f, 0.12f, 0.12f));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereAsset.Succeeded())
	{
		Mesh->SetStaticMesh(SphereAsset.Object);
	}

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(GetRootComponent());
	Light->SetIntensity(9000.f);
	Light->SetAttenuationRadius(1800.f);
	Light->SetLightColor(FLinearColor(1.f, 0.45f, 0.12f));
	Light->SetCastShadows(false);
}

void AENFlare::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_FlareGlow.M_FlareGlow")))
	{
		Mesh->SetMaterial(0, Mat);
	}
	Timer = Life;
}

void AENFlare::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Timer -= DeltaSeconds;
	const float Flicker = 0.75f + 0.25f * FMath::Sin(GetWorld()->GetTimeSeconds() * 18.f);
	Light->SetIntensity(9000.f * Flicker);

	if (Timer <= 0.f)
	{
		Destroy();
	}
}
