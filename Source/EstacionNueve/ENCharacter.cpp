#include "ENCharacter.h"

#include "ENInteractable.h"
#include "ENGameState.h"
#include "ENCreatures.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputTriggers.h"
#include "InputCoreTypes.h"

AENCharacter::AENCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = false;
	Move->MaxWalkSpeed = 350.f;
	Move->JumpZVelocity = 420.f;
	Move->AirControl = 0.35f;
	Move->SetCrouchedHalfHeight(48.f);
	Move->NavAgentProps.bCanCrouch = true;
	Move->MaxStepHeight = 45.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	Camera->bUsePawnControlRotation = true;
	Camera->SetFieldOfView(90.f);

	// Cuello de Brisa visible en primera persona cuando se cabalga.
	RideMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RideMesh"));
	RideMesh->SetupAttachment(Camera);
	RideMesh->SetRelativeLocation(FVector(110.f, 0.f, -70.f));
	RideMesh->SetRelativeScale3D(FVector(0.9f, 0.35f, 0.35f));
	RideMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RideMesh->SetVisibility(false);
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeAsset(TEXT("/Engine/BasicShapes/Cone.Cone"));
		if (ConeAsset.Succeeded())
		{
			RideMesh->SetStaticMesh(ConeAsset.Object);
		}
	}

	LampLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LampLight"));
	LampLight->SetupAttachment(Camera);
	LampLight->SetRelativeLocation(FVector(20.f, 10.f, -10.f));
	LampLight->SetIntensity(0.f);
	LampLight->SetAttenuationRadius(1200.f);
	LampLight->SetLightColor(FLinearColor(1.f, 0.78f, 0.45f));
	LampLight->SetCastShadows(false);
}

// --------------------------------------------------------------------------- ciclo de vida

void AENCharacter::BeginPlay()
{
	Super::BeginPlay();

	BuildShopLists();
	RespawnLocation = GetActorLocation();
	RespawnRotation = GetActorRotation();
	RespawnInventory = Inv;
	RespawnLoaded = RevolverLoaded;
	RespawnAmmo = Inv.Ammo;

	BuildInputAssets();
	AddInputMapping();

	if (AENGameState* GS = Cast<AENGameState>(GetWorld()->GetGameState()))
	{
		GS->Notify(TEXT("Cyrus Hale llega a la Estacion Nueve. Muevete con WASD."), FLinearColor(0.95f, 0.88f, 0.7f), 6.f);
	}

	UE_LOG(LogTemp, Warning, TEXT("EN_CHARACTER|BeginPlay at %s"), *GetActorLocation().ToString());
}

void AENCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	AddInputMapping();
}

void AENCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!IMC)
	{
		BuildInputAssets();
	}
	AddInputMapping();

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC)
	{
		UE_LOG(LogTemp, Error, TEXT("EN_INPUT|El proyecto necesita EnhancedInputComponent (DefaultInput.ini)."));
		return;
	}

	EIC->BindAction(IA_MoveForward, ETriggerEvent::Started, this, &AENCharacter::MoveForwardStart);
	EIC->BindAction(IA_MoveForward, ETriggerEvent::Completed, this, &AENCharacter::MoveForwardStop);
	EIC->BindAction(IA_MoveBack, ETriggerEvent::Started, this, &AENCharacter::MoveBackStart);
	EIC->BindAction(IA_MoveBack, ETriggerEvent::Completed, this, &AENCharacter::MoveBackStop);
	EIC->BindAction(IA_MoveRight, ETriggerEvent::Started, this, &AENCharacter::MoveRightStart);
	EIC->BindAction(IA_MoveRight, ETriggerEvent::Completed, this, &AENCharacter::MoveRightStop);
	EIC->BindAction(IA_MoveLeft, ETriggerEvent::Started, this, &AENCharacter::MoveLeftStart);
	EIC->BindAction(IA_MoveLeft, ETriggerEvent::Completed, this, &AENCharacter::MoveLeftStop);

	EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &AENCharacter::OnLook);
	EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &AENCharacter::OnJumpStart);
	EIC->BindAction(IA_Sprint, ETriggerEvent::Started, this, &AENCharacter::OnSprintStart);
	EIC->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &AENCharacter::OnSprintStop);
	EIC->BindAction(IA_Crouch, ETriggerEvent::Started, this, &AENCharacter::OnCrouchStart);
	EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &AENCharacter::OnInteractStart);
	EIC->BindAction(IA_Fire, ETriggerEvent::Started, this, &AENCharacter::OnFireStart);
	EIC->BindAction(IA_Aim, ETriggerEvent::Started, this, &AENCharacter::OnAimStart);
	EIC->BindAction(IA_Aim, ETriggerEvent::Completed, this, &AENCharacter::OnAimStop);
	EIC->BindAction(IA_Reload, ETriggerEvent::Started, this, &AENCharacter::OnReloadStart);
	EIC->BindAction(IA_Flare, ETriggerEvent::Started, this, &AENCharacter::OnFlareStart);
	EIC->BindAction(IA_Whistle, ETriggerEvent::Started, this, &AENCharacter::OnWhistleStart);
	EIC->BindAction(IA_Lamp, ETriggerEvent::Started, this, &AENCharacter::OnLampStart);
	EIC->BindAction(IA_Bandage, ETriggerEvent::Started, this, &AENCharacter::OnBandageStart);
	EIC->BindAction(IA_Inventory, ETriggerEvent::Started, this, &AENCharacter::OnInventoryStart);
	EIC->BindAction(IA_CloseUI, ETriggerEvent::Started, this, &AENCharacter::OnCloseUIStart);

	EIC->BindAction(IA_Slot1, ETriggerEvent::Started, this, &AENCharacter::OnSlot1);
	EIC->BindAction(IA_Slot2, ETriggerEvent::Started, this, &AENCharacter::OnSlot2);
	EIC->BindAction(IA_Slot3, ETriggerEvent::Started, this, &AENCharacter::OnSlot3);
	EIC->BindAction(IA_Slot4, ETriggerEvent::Started, this, &AENCharacter::OnSlot4);
	EIC->BindAction(IA_Slot5, ETriggerEvent::Started, this, &AENCharacter::OnSlot5);
	EIC->BindAction(IA_Sell1, ETriggerEvent::Started, this, &AENCharacter::OnSell1);
	EIC->BindAction(IA_Sell2, ETriggerEvent::Started, this, &AENCharacter::OnSell2);
	EIC->BindAction(IA_Sell3, ETriggerEvent::Started, this, &AENCharacter::OnSell3);
	EIC->BindAction(IA_Sell4, ETriggerEvent::Started, this, &AENCharacter::OnSell4);
}

void AENCharacter::BuildInputAssets()
{
	if (IMC)
	{
		return;
	}

	IMC = NewObject<UInputMappingContext>(this, TEXT("IMC_EstacionNueve"));

	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type) -> UInputAction*
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = Type;
		return Action;
	};

	IA_MoveForward = MakeAction(TEXT("IA_MoveForward"), EInputActionValueType::Boolean);
	IA_MoveBack = MakeAction(TEXT("IA_MoveBack"), EInputActionValueType::Boolean);
	IA_MoveRight = MakeAction(TEXT("IA_MoveRight"), EInputActionValueType::Boolean);
	IA_MoveLeft = MakeAction(TEXT("IA_MoveLeft"), EInputActionValueType::Boolean);
	IA_Look = MakeAction(TEXT("IA_Look"), EInputActionValueType::Axis2D);
	IA_Jump = MakeAction(TEXT("IA_Jump"), EInputActionValueType::Boolean);
	IA_Sprint = MakeAction(TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	IA_Crouch = MakeAction(TEXT("IA_Crouch"), EInputActionValueType::Boolean);
	IA_Interact = MakeAction(TEXT("IA_Interact"), EInputActionValueType::Boolean);
	IA_Fire = MakeAction(TEXT("IA_Fire"), EInputActionValueType::Boolean);
	IA_Aim = MakeAction(TEXT("IA_Aim"), EInputActionValueType::Boolean);
	IA_Reload = MakeAction(TEXT("IA_Reload"), EInputActionValueType::Boolean);
	IA_Flare = MakeAction(TEXT("IA_Flare"), EInputActionValueType::Boolean);
	IA_Whistle = MakeAction(TEXT("IA_Whistle"), EInputActionValueType::Boolean);
	IA_Lamp = MakeAction(TEXT("IA_Lamp"), EInputActionValueType::Boolean);
	IA_Bandage = MakeAction(TEXT("IA_Bandage"), EInputActionValueType::Boolean);
	IA_Inventory = MakeAction(TEXT("IA_Inventory"), EInputActionValueType::Boolean);
	IA_CloseUI = MakeAction(TEXT("IA_CloseUI"), EInputActionValueType::Boolean);
	IA_Slot1 = MakeAction(TEXT("IA_Slot1"), EInputActionValueType::Boolean);
	IA_Slot2 = MakeAction(TEXT("IA_Slot2"), EInputActionValueType::Boolean);
	IA_Slot3 = MakeAction(TEXT("IA_Slot3"), EInputActionValueType::Boolean);
	IA_Slot4 = MakeAction(TEXT("IA_Slot4"), EInputActionValueType::Boolean);
	IA_Slot5 = MakeAction(TEXT("IA_Slot5"), EInputActionValueType::Boolean);
	IA_Sell1 = MakeAction(TEXT("IA_Sell1"), EInputActionValueType::Boolean);
	IA_Sell2 = MakeAction(TEXT("IA_Sell2"), EInputActionValueType::Boolean);
	IA_Sell3 = MakeAction(TEXT("IA_Sell3"), EInputActionValueType::Boolean);
	IA_Sell4 = MakeAction(TEXT("IA_Sell4"), EInputActionValueType::Boolean);

	IMC->MapKey(IA_MoveForward, EKeys::W);
	IMC->MapKey(IA_MoveBack, EKeys::S);
	IMC->MapKey(IA_MoveRight, EKeys::D);
	IMC->MapKey(IA_MoveLeft, EKeys::A);
	IMC->MapKey(IA_Look, EKeys::Mouse2D);
	IMC->MapKey(IA_Jump, EKeys::SpaceBar);
	IMC->MapKey(IA_Sprint, EKeys::LeftShift);
	IMC->MapKey(IA_Crouch, EKeys::LeftControl);
	IMC->MapKey(IA_Interact, EKeys::E);
	IMC->MapKey(IA_Fire, EKeys::LeftMouseButton);
	IMC->MapKey(IA_Aim, EKeys::RightMouseButton);
	IMC->MapKey(IA_Reload, EKeys::R);
	IMC->MapKey(IA_Flare, EKeys::Q);
	IMC->MapKey(IA_Whistle, EKeys::V);
	IMC->MapKey(IA_Lamp, EKeys::F);
	IMC->MapKey(IA_Bandage, EKeys::H);
	IMC->MapKey(IA_Inventory, EKeys::Tab);
	IMC->MapKey(IA_CloseUI, EKeys::Escape);
	IMC->MapKey(IA_Slot1, EKeys::One);
	IMC->MapKey(IA_Slot2, EKeys::Two);
	IMC->MapKey(IA_Slot3, EKeys::Three);
	IMC->MapKey(IA_Slot4, EKeys::Four);
	IMC->MapKey(IA_Slot5, EKeys::Five);
	IMC->MapKey(IA_Sell1, EKeys::Six);
	IMC->MapKey(IA_Sell2, EKeys::Seven);
	IMC->MapKey(IA_Sell3, EKeys::Eight);
	IMC->MapKey(IA_Sell4, EKeys::Nine);
}

void AENCharacter::AddInputMapping()
{
	if (!IMC)
	{
		return;
	}
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->ClearAllMappings();
			Subsystem->AddMappingContext(IMC, 0);
		}
	}
}

// --------------------------------------------------------------------------- tick

void AENCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FireCooldown = FMath::Max(0.f, FireCooldown - DeltaSeconds);
	StaminaRegenDelay = FMath::Max(0.f, StaminaRegenDelay - DeltaSeconds);

	UpdateFocus();
	UpdateNoise(DeltaSeconds);
	UpdateStamina(DeltaSeconds);
	UpdateLamp(DeltaSeconds);
	UpdateReload(DeltaSeconds);
	UpdateDeath(DeltaSeconds);

	// Movimiento relativo a la vista.
	if (!bDead && !bShopOpen && !bInventoryOpen)
	{
		FVector Direction = FVector::ZeroVector;
		if (bInputForward) { Direction += GetActorForwardVector(); }
		if (bInputBack)    { Direction -= GetActorForwardVector(); }
		if (bInputRight)   { Direction += GetActorRightVector(); }
		if (bInputLeft)    { Direction -= GetActorRightVector(); }
		if (!Direction.IsNearlyZero())
		{
			AddMovementInput(Direction.GetSafeNormal(), 1.f);
		}
	}

	// Velocidad segun postura, montura y aguante.
	float Speed = 350.f;
	if (bRiding)
	{
		Speed = 900.f;
	}
	else if (bIsCrouched)
	{
		Speed = 180.f;
	}
	else if (bSprinting && Stamina > 1.f)
	{
		Speed = 600.f;
	}
	if (bShopOpen || bInventoryOpen || bDead)
	{
		Speed = 0.f;
	}
	GetCharacterMovement()->MaxWalkSpeed = Speed;

	// Zoom al apuntar.
	const float TargetFOV = bAiming ? 62.f : 90.f;
	Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView, TargetFOV, DeltaSeconds, 10.f));

	// Fundido de viaje.
	if (bFading)
	{
		FadeTimer += DeltaSeconds;
		if (!bFadeTeleported && FadeTimer >= 0.35f)
		{
			bFadeTeleported = true;
			SetActorLocation(FadeTarget, false, nullptr, ETeleportType::TeleportPhysics);
			if (APlayerController* PC = Cast<APlayerController>(GetController()))
			{
				PC->SetControlRotation(FRotator(0.f, FadeYaw, 0.f));
			}
		}
		if (FadeTimer >= 0.9f)
		{
			bFading = false;
			FadeAlpha = 0.f;
		}
		else if (FadeTimer < 0.35f)
		{
			FadeAlpha = FadeTimer / 0.35f;
		}
		else
		{
			FadeAlpha = 1.f - FMath::Clamp((FadeTimer - 0.5f) / 0.4f, 0.f, 1.f);
		}
	}
}

// --------------------------------------------------------------------------- stats

void AENCharacter::UpdateNoise(float DeltaSeconds)
{
	float Activity = 0.f;
	if (bRiding)
	{
		Activity = 25.f;
	}
	else if (bIsCrouched)
	{
		Activity = 5.f;
	}
	else if (bSprinting && IsMoving() && Stamina > 1.f)
	{
		Activity = 50.f;
	}
	else if (IsMoving())
	{
		Activity = 20.f;
	}

	const float Rate = (Activity > Noise) ? 45.f : 10.f;
	Noise = FMath::FInterpConstantTo(Noise, Activity, DeltaSeconds, Rate);
}

void AENCharacter::UpdateStamina(float DeltaSeconds)
{
	const bool bDraining = bSprinting && IsMoving() && !bIsCrouched && Stamina > 1.f;
	if (bDraining)
	{
		Stamina = FMath::Max(0.f, Stamina - 15.f * DeltaSeconds);
		StaminaRegenDelay = 1.f;
	}
	else if (StaminaRegenDelay <= 0.f)
	{
		Stamina = FMath::Min(MaxStamina, Stamina + 20.f * DeltaSeconds);
	}
}

void AENCharacter::UpdateLamp(float DeltaSeconds)
{
	if (!bLampOn)
	{
		LampLight->SetIntensity(0.f);
		return;
	}

	LampOil = FMath::Max(0.f, LampOil - (100.f / 360.f) * DeltaSeconds);
	LampLight->SetIntensity(2800.f);

	if (LampOil <= 0.f)
	{
		bLampOn = false;
		Notify(TEXT("La lampara se apago: se acabo el aceite."), FLinearColor(1.f, 0.6f, 0.4f));
	}
}

void AENCharacter::UpdateReload(float DeltaSeconds)
{
	if (!bReloading)
	{
		return;
	}
	ReloadTimer -= DeltaSeconds;
	if (ReloadTimer > 0.f)
	{
		return;
	}
	if (RevolverLoaded < 6 && Inv.Ammo > 0)
	{
		RevolverLoaded++;
		Inv.Ammo--;
		ReloadTimer = 0.4f;
	}
	if (RevolverLoaded >= 6 || Inv.Ammo <= 0)
	{
		bReloading = false;
	}
}

void AENCharacter::UpdateDeath(float DeltaSeconds)
{
	if (!bDead)
	{
		return;
	}
	DeathTimer -= DeltaSeconds;
	if (DeathTimer <= 0.f)
	{
		Respawn();
	}
}

void AENCharacter::UpdateFocus()
{
	FocusedActor = nullptr;
	FocusedPrompt.Empty();

	if (bDead || bShopOpen || bInventoryOpen)
	{
		return;
	}

	if (AENInteractableActor* Found = TraceInteractable(320.f))
	{
		FocusedActor = Found;
		FocusedPrompt = Found->GetPrompt(this);
	}
}

AENInteractableActor* AENCharacter::TraceInteractable(float Distance) const
{
	FVector Start = Camera->GetComponentLocation();
	FVector End = Start + Camera->GetForwardVector() * Distance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ENInteract), false, this);
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		return Cast<AENInteractableActor>(Hit.GetActor());
	}
	return nullptr;
}

void AENCharacter::AddNoise(float Amount, bool bInstant)
{
	Noise = FMath::Clamp(Noise + Amount, 0.f, 100.f);
	LastNoiseImpulse = GetWorld()->GetTimeSeconds();

	if (AENGameState* GS = Cast<AENGameState>(GetWorld()->GetGameState()))
	{
		GS->Notify(FString::Printf(TEXT("Ruido %+d (%.0f)"), FMath::RoundToInt(Amount), Noise),
			FLinearColor(1.f, 0.75f, 0.45f), 2.f);
	}
}

bool AENCharacter::IsMoving() const
{
	return GetVelocity().SizeSquared2D() > 100.f;
}

// --------------------------------------------------------------------------- input

void AENCharacter::OnLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	const float Sens = bAiming ? 0.35f : 0.6f;
	AddControllerYawInput(Axis.X * Sens);
	AddControllerPitchInput(-Axis.Y * Sens); // raton arriba = mirar arriba
}

void AENCharacter::OnJumpStart()
{
	if (!bDead && !bIsCrouched)
	{
		Jump();
	}
}

void AENCharacter::OnSprintStart() { bSprintHeld = true; bSprinting = true; }
void AENCharacter::OnSprintStop()  { bSprintHeld = false; bSprinting = false; }

void AENCharacter::OnCrouchStart()
{
	if (bDead)
	{
		return;
	}
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void AENCharacter::OnInteractStart()
{
	if (bShopOpen || bInventoryOpen)
	{
		CloseUI();
		return;
	}
	if (bDead)
	{
		return;
	}
	if (FocusedActor && FocusedActor->CanInteract(this))
	{
		FocusedActor->Interact(this);
	}
}

void AENCharacter::OnFireStart()
{
	if (!bDead && !bShopOpen && !bInventoryOpen)
	{
		UseTool();
	}
}

void AENCharacter::OnAimStart() { bAiming = true; }
void AENCharacter::OnAimStop()  { bAiming = false; }

void AENCharacter::OnReloadStart()
{
	if (bDead || bShopOpen)
	{
		return;
	}
	if (RevolverLoaded >= 6)
	{
		Notify(TEXT("El tambor ya esta lleno."), FLinearColor(0.85f, 0.85f, 0.8f), 2.f);
		return;
	}
	if (Inv.Ammo <= 0)
	{
		Notify(TEXT("No quedan balas de reserva."), FLinearColor(1.f, 0.55f, 0.4f), 2.f);
		return;
	}
	bReloading = true;
	ReloadTimer = 0.f;
}

void AENCharacter::OnFlareStart()
{
	if (bDead || bShopOpen)
	{
		return;
	}
	SpawnFlare();
}

void AENCharacter::OnWhistleStart()
{
	if (bDead || bShopOpen)
	{
		return;
	}
	for (TActorIterator<AENMount> It(GetWorld()); It; ++It)
	{
		It->WhistleTo(GetActorLocation());
		Notify(TEXT("Silbas: Brisa acude."), FLinearColor(0.8f, 0.95f, 0.8f), 3.f);
		AddNoise(35.f);
		break;
	}
}

void AENCharacter::OnLampStart()
{
	if (LampOil <= 0.f)
	{
		Notify(TEXT("La lampara no tiene aceite."), FLinearColor(1.f, 0.6f, 0.4f), 3.f);
		return;
	}
	bLampOn = !bLampOn;
	Notify(bLampOn ? TEXT("Lampara encendida.") : TEXT("Lampara apagada."), FLinearColor(1.f, 0.85f, 0.55f), 2.f);
}

void AENCharacter::OnBandageStart()
{
	if (bDead)
	{
		return;
	}
	if (Inv.Bandages <= 0)
	{
		Notify(TEXT("No tienes vendas."), FLinearColor(1.f, 0.6f, 0.4f), 2.f);
		return;
	}
	if (Health >= MaxHealth)
	{
		Notify(TEXT("No estas herido."), FLinearColor(0.85f, 0.85f, 0.8f), 2.f);
		return;
	}
	Inv.Bandages--;
	Heal(35.f);
	Notify(TEXT("Te vendas la herida (+35)."), FLinearColor(0.7f, 1.f, 0.7f), 2.f);
}

void AENCharacter::OnInventoryStart()
{
	if (bShopOpen)
	{
		return;
	}
	bInventoryOpen = !bInventoryOpen;
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetShowMouseCursor(bInventoryOpen);
	}
}

void AENCharacter::OnCloseUIStart()
{
	CloseUI();
}

void AENCharacter::OnSlot1() { if (bShopOpen) { ShopBuy(0); return; } CycleTool(0); }
void AENCharacter::OnSlot2() { if (bShopOpen) { ShopBuy(1); return; } CycleTool(1); }
void AENCharacter::OnSlot3() { if (bShopOpen) { ShopBuy(2); return; } CycleTool(2); }
void AENCharacter::OnSlot4() { if (bShopOpen) { ShopBuy(3); return; } CycleTool(3); }
void AENCharacter::OnSlot5() { if (bShopOpen) { ShopBuy(4); return; } CycleTool(4); }
void AENCharacter::OnSell1() { if (bShopOpen) { ShopSell(0); } }
void AENCharacter::OnSell2() { if (bShopOpen) { ShopSell(1); } }
void AENCharacter::OnSell3() { if (bShopOpen) { ShopSell(2); } }
void AENCharacter::OnSell4() { if (bShopOpen) { ShopSell(3); } }

void AENCharacter::CycleTool(int32 Index)
{
	if (Index < 0 || Index > 4)
	{
		return;
	}
	CurrentTool = static_cast<EENTool>(Index);
	Notify(FString::Printf(TEXT("Herramienta: %s"), *GetToolName()), FLinearColor(0.9f, 0.9f, 0.8f), 2.f);
}

// --------------------------------------------------------------------------- herramientas

void AENCharacter::UseTool()
{
	switch (CurrentTool)
	{
	case EENTool::Revolver:
		FireRevolver();
		break;
	case EENTool::Pickaxe:
		SwingPick();
		break;
	case EENTool::Hoe:
	case EENTool::Bucket:
		if (FocusedActor)
		{
			FocusedActor->Interact(this);
		}
		else
		{
			Notify(CurrentTool == EENTool::Hoe ? TEXT("Apunta a una parcela.") : TEXT("Llena el cubo en la torre de agua."),
				FLinearColor(0.9f, 0.9f, 0.8f), 2.f);
		}
		break;
	case EENTool::Flare:
		SpawnFlare();
		break;
	default:
		break;
	}
}

void AENCharacter::FireRevolver()
{
	if (FireCooldown > 0.f)
	{
		return;
	}
	if (RevolverLoaded <= 0)
	{
		Notify(TEXT("Tambor vacio: pulsa R para recargar."), FLinearColor(1.f, 0.6f, 0.4f), 2.f);
		return;
	}

	FireCooldown = 0.5f;
	RevolverLoaded--;
	AddNoise(40.f);

	FVector Start = Camera->GetComponentLocation();
	FVector End = Start + Camera->GetForwardVector() * 2500.f;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ENShot), false, this);

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		if (AENDinosaur* Dino = Cast<AENDinosaur>(Hit.GetActor()))
		{
			Dino->TakeDamage(25.f, FDamageEvent(), GetController(), this);
			Notify(TEXT("Impacto en el dinosaurio."), FLinearColor(1.f, 0.85f, 0.5f), 1.5f);
		}
		else
		{
			Notify(TEXT("Impacto en el entorno."), FLinearColor(0.8f, 0.8f, 0.75f), 1.f);
		}
	}
}

void AENCharacter::SwingPick()
{
	if (FireCooldown > 0.f)
	{
		return;
	}
	FireCooldown = 0.8f;

	FVector Start = Camera->GetComponentLocation();
	FVector End = Start + Camera->GetForwardVector() * 220.f;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ENPick), false, this);

	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Visibility,
		FCollisionShape::MakeSphere(45.f), Params);

	for (const FHitResult& Hit : Hits)
	{
		if (AENAmberVein* Vein = Cast<AENAmberVein>(Hit.GetActor()))
		{
			Vein->HitWithPick(this);
			return;
		}
		if (AENDinosaur* Dino = Cast<AENDinosaur>(Hit.GetActor()))
		{
			Dino->TakeDamage(20.f, FDamageEvent(), GetController(), this);
			AddNoise(15.f);
			return;
		}
	}

	AddNoise(15.f);
	Notify(TEXT("Golpe al aire. Busca una veta."), FLinearColor(0.85f, 0.85f, 0.8f), 1.5f);
}

void AENCharacter::SpawnFlare()
{
	if (Inv.Flares <= 0)
	{
		Notify(TEXT("No quedan bengalas."), FLinearColor(1.f, 0.6f, 0.4f), 2.f);
		return;
	}
	Inv.Flares--;
	AddNoise(30.f);

	FVector Location = Camera->GetComponentLocation() + Camera->GetForwardVector() * 300.f + FVector(0.f, 0.f, -40.f);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	GetWorld()->SpawnActor<AENFlare>(AENFlare::StaticClass(), Location, FRotator::ZeroRotator, Params);

	Notify(TEXT("Bengala lanzada: hara ruido y luz."), FLinearColor(1.f, 0.6f, 0.4f), 3.f);
}

// --------------------------------------------------------------------------- viaje, checkpoint, vida

void AENCharacter::TravelTo(const FVector& Location, float Yaw)
{
	if (bFading)
	{
		return;
	}
	FadeTarget = Location;
	FadeYaw = Yaw;
	FadeTimer = 0.f;
	FadeAlpha = 0.f;
	bFadeTeleported = false;
	bFading = true;
}

void AENCharacter::SetCheckpoint(const FVector& Location, const FRotator& Rotation)
{
	RespawnLocation = Location;
	RespawnRotation = Rotation;
	RespawnInventory = Inv;
	RespawnLoaded = RevolverLoaded;
	RespawnAmmo = Inv.Ammo;
	bHasCheckpoint = true;
}

void AENCharacter::ApplyDamageEN(float Amount, AActor* Causer)
{
	if (bDead || Amount <= 0.f)
	{
		return;
	}
	Health = FMath::Max(0.f, Health - Amount);
	Notify(FString::Printf(TEXT("Recibes %d de dano (vida %.0f)"), FMath::RoundToInt(Amount), Health),
		FLinearColor(1.f, 0.4f, 0.35f), 2.5f);

	if (Health <= 0.f)
	{
		Die();
	}
}

void AENCharacter::Heal(float Amount)
{
	Health = FMath::Min(MaxHealth, Health + Amount);
}

void AENCharacter::Die()
{
	if (bDead)
	{
		return;
	}
	bDead = true;
	DeathTimer = 2.6f;
	Health = 0.f;
	GetCharacterMovement()->DisableMovement();

	if (AENGameState* GS = Cast<AENGameState>(GetWorld()->GetGameState()))
	{
		GS->OnPlayerDied(this);
	}
}

void AENCharacter::Respawn()
{
	Health = MaxHealth;
	Stamina = MaxStamina;
	Noise = 0.f;
	bDead = false;
	bLampOn = false;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	SetActorLocation(RespawnLocation, false, nullptr, ETeleportType::TeleportPhysics);
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetControlRotation(RespawnRotation);
	}

	Inv = RespawnInventory;
	RevolverLoaded = RespawnLoaded;
	Inv.Ammo = RespawnAmmo;

	if (bRiding)
	{
		SetRiding(false);
	}
}

// --------------------------------------------------------------------------- Brisa

void AENCharacter::ToggleRide(AENMount* InMount)
{
	if (bRiding)
	{
		SetRiding(false);
		return;
	}
	if (!InMount)
	{
		return;
	}
	Mount = InMount;
	SetRiding(true);
}

void AENCharacter::SetRiding(bool bInRiding)
{
	bRiding = bInRiding;
	RideMesh->SetVisibility(bRiding);
	Camera->SetRelativeLocation(bRiding ? FVector(0.f, 0.f, 150.f) : FVector(0.f, 0.f, 64.f));

	if (Mount)
	{
		Mount->SetMounted(bRiding);
		if (!bInRiding)
		{
			// Brisa queda al lado del jinete.
			Mount->SetActorLocation(GetActorLocation() + GetActorRightVector() * 250.f);
		}
	}

	Notify(bInRiding ? TEXT("Cabalga a Brisa (E para desmontar).") : TEXT("Desmontas."),
		FLinearColor(0.75f, 0.95f, 0.7f), 3.f);
}

// --------------------------------------------------------------------------- tienda

void AENCharacter::BuildShopLists()
{
	BuyList.Reset();
	BuyList.Add(FENShopEntry(TEXT("3 semillas de helecho"), 3, EENLootType::Seeds, 3));
	BuyList.Add(FENShopEntry(TEXT("6 balas de revolver"), 3, EENLootType::Ammo, 6));
	BuyList.Add(FENShopEntry(TEXT("1 venda"), 4, EENLootType::Bandage, 1));
	BuyList.Add(FENShopEntry(TEXT("1 bengala"), 3, EENLootType::Flare, 1));
	BuyList.Add(FENShopEntry(TEXT("aceite de lampara (+50)"), 3, EENLootType::Oil, 50));

	SellList.Reset();
	SellList.Add(FENShopEntry(TEXT("resina en bruto"), 6, EENLootType::Resin, 1));
	SellList.Add(FENShopEntry(TEXT("resina refinada"), 10, EENLootType::Refined, 1));
	SellList.Add(FENShopEntry(TEXT("ambar"), 12, EENLootType::Amber, 1));
	SellList.Add(FENShopEntry(TEXT("forraje"), 2, EENLootType::Forage, 1));
}

void AENCharacter::OpenShop()
{
	bShopOpen = true;
	bInventoryOpen = false;
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetShowMouseCursor(true);
	}
	Notify(TEXT("Mercader: [1-5] comprar, [6-9] vender, [E] cerrar."), FLinearColor(1.f, 0.9f, 0.65f), 6.f);
}

void AENCharacter::CloseUI()
{
	bShopOpen = false;
	bInventoryOpen = false;
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetShowMouseCursor(false);
	}
}

void AENCharacter::ShopBuy(int32 Index)
{
	if (!BuyList.IsValidIndex(Index))
	{
		return;
	}
	AENGameState* GS = Cast<AENGameState>(GetWorld()->GetGameState());
	if (!GS)
	{
		return;
	}
	const FENShopEntry& Item = BuyList[Index];
	if (GS->Money < Item.Price)
	{
		Notify(TEXT("No te alcanza la plata."), FLinearColor(1.f, 0.55f, 0.4f), 2.f);
		return;
	}
	GS->Money -= Item.Price;
	Inv.AddLoot(Item.Loot, Item.Amount);
	Notify(FString::Printf(TEXT("Compras %s por $%d"), *Item.Label, Item.Price), FLinearColor(0.75f, 1.f, 0.7f), 3.f);
}

void AENCharacter::ShopSell(int32 Index)
{
	if (!SellList.IsValidIndex(Index))
	{
		return;
	}
	AENGameState* GS = Cast<AENGameState>(GetWorld()->GetGameState());
	if (!GS)
	{
		return;
	}
	const FENShopEntry& Item = SellList[Index];
	const int32 Have = Inv.GetLootAmount(Item.Loot);
	if (Have <= 0)
	{
		Notify(FString::Printf(TEXT("No tienes %s."), *Item.Label), FLinearColor(1.f, 0.55f, 0.4f), 2.f);
		return;
	}
	Inv.AddLoot(Item.Loot, -Item.Amount);
	GS->Money += Item.Price;
	Notify(FString::Printf(TEXT("Vendes %s por $%d"), *Item.Label, Item.Price), FLinearColor(0.75f, 1.f, 0.7f), 3.f);
}

// --------------------------------------------------------------------------- info

FString AENCharacter::GetToolName() const
{
	switch (CurrentTool)
	{
	case EENTool::Revolver: return TEXT("Revolver");
	case EENTool::Pickaxe:  return TEXT("Pico");
	case EENTool::Hoe:      return TEXT("Azada");
	case EENTool::Bucket:   return TEXT("Cubo");
	case EENTool::Flare:    return TEXT("Bengala");
	default: return TEXT("Mano");
	}
}

FString AENCharacter::GetClockText() const
{
	if (AENGameState* GS = Cast<AENGameState>(GetWorld()->GetGameState()))
	{
		return GS->GetClockText();
	}
	return TEXT("--:--");
}

float AENCharacter::GetFadeAlpha() const
{
	return bFading ? FadeAlpha : 0.f;
}

void AENCharacter::Notify(const FString& Text, FLinearColor Color, float Seconds)
{
	if (AENGameState* GS = Cast<AENGameState>(GetWorld()->GetGameState()))
	{
		GS->Notify(Text, Color, Seconds);
	}
}

// --------------------------------------------------------------------------- depuracion

void AENCharacter::ENSkipToNight()
{
	if (AENGameState* GS = Cast<AENGameState>(GetWorld()->GetGameState()))
	{
		GS->ForceNight();
	}
}

void AENCharacter::ENGiveAmmo()
{
	Inv.Ammo += 30;
	Inv.Bandages += 2;
	Inv.Flares += 2;
	Notify(TEXT("[debug] +30 balas, +2 vendas, +2 bengalas."), FLinearColor(1.f, 1.f, 0.5f), 3.f);
}

void AENCharacter::ENToggleFog()
{
	Notify(TEXT("[debug] niebla alternada."), FLinearColor(1.f, 1.f, 0.5f), 2.f);
}

void AENCharacter::ENSelfTest()
{
	int32 Plots = 0;
	int32 Veins = 0;
	int32 Valves = 0;
	int32 Gates = 0;
	int32 Pickups = 0;
	for (TActorIterator<AENFarmPlot> It(GetWorld()); It; ++It) { Plots++; }
	for (TActorIterator<AENAmberVein> It(GetWorld()); It; ++It) { Veins++; }
	for (TActorIterator<AENValve> It(GetWorld()); It; ++It) { Valves++; }
	for (TActorIterator<AENGate> It(GetWorld()); It; ++It) { Gates++; }
	for (TActorIterator<AENPickup> It(GetWorld()); It; ++It) { Pickups++; }

	const bool bOk = Plots >= 4 && Veins >= 6 && Valves >= 2 && Gates >= 1 && Pickups >= 1;
	UE_LOG(LogTemp, Warning, TEXT("EN_SELFTEST|plots=%d|veins=%d|valves=%d|gates=%d|pickups=%d|ok=%s"),
		Plots, Veins, Valves, Gates, Pickups, bOk ? TEXT("true") : TEXT("false"));
}
