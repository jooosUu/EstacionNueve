#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ENGameplayTypes.h"
#include "ENCharacter.generated.h"

class UCameraComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UInputMappingContext;
class UInputAction;
class AENInteractableActor;
class AENMount;
struct FInputActionValue;

/** Entrada de la tienda (lista de compra / venta). */
struct FENShopEntry
{
	FString Label;
	int32 Price = 0;
	EENLootType Loot = EENLootType::None;
	int32 Amount = 0;

	FENShopEntry() {}
	FENShopEntry(const FString& InLabel, int32 InPrice, EENLootType InLoot, int32 InAmount)
		: Label(InLabel), Price(InPrice), Loot(InLoot), Amount(InAmount) {}
};

/**
 * Cyrus Hale: camara en primera persona, ruido como recurso, herramientas y mochila.
 */
UCLASS()
class AENCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AENCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;

	// ---------------------------------------------------------------- componentes
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN")
	TObjectPtr<UStaticMeshComponent> RideMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN")
	TObjectPtr<UPointLightComponent> LampLight;

	// ---------------------------------------------------------------- estado
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") float Health = 100.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") float MaxHealth = 100.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") float Stamina = 100.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") float MaxStamina = 100.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") float Noise = 0.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") float LampOil = 60.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") FENInventory Inv;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") int32 RevolverLoaded = 6;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") int32 BucketCharges = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") EENTool CurrentTool = EENTool::Revolver;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") bool bSprinting = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") bool bAiming = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") bool bLampOn = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") bool bRiding = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") bool bDead = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") bool bShopOpen = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") bool bInventoryOpen = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") bool bFading = false;

	// ---------------------------------------------------------------- interaccion
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN")
	TObjectPtr<AENInteractableActor> FocusedActor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN")
	FString FocusedPrompt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN")
	TObjectPtr<AENMount> Mount;

	// ---------------------------------------------------------------- api para actores
	void Notify(const FString& Text, FLinearColor Color = FLinearColor::White, float Seconds = 4.f);
	void AddNoise(float Amount, bool bInstant = true);
	void ApplyDamageEN(float Amount, AActor* Causer);
	void Heal(float Amount);
	void TravelTo(const FVector& Location, float Yaw);
	void SetCheckpoint(const FVector& Location, const FRotator& Rotation);
	void OpenShop();
	void CloseUI();
	void ToggleRide(AENMount* InMount);
	void SetRiding(bool bInRiding);

	FString GetToolName() const;
	FString GetClockText() const;
	float GetNoiseRatio() const { return Noise / 100.f; }
	float GetFadeAlpha() const;
	bool IsMoving() const;

	// ---------------------------------------------------------------- tienda
	TArray<FENShopEntry> BuyList;
	TArray<FENShopEntry> SellList;
	void BuildShopLists();
	void ShopBuy(int32 Index);
	void ShopSell(int32 Index);

	// ---------------------------------------------------------------- atajos de depuracion
	UFUNCTION(Exec) void ENSkipToNight();
	UFUNCTION(Exec) void ENGiveAmmo();
	UFUNCTION(Exec) void ENSelfTest();
	UFUNCTION(Exec) void ENToggleFog();

protected:
	// input
	UPROPERTY() TObjectPtr<UInputMappingContext> IMC;
	UPROPERTY() TObjectPtr<UInputAction> IA_MoveForward;
	UPROPERTY() TObjectPtr<UInputAction> IA_MoveBack;
	UPROPERTY() TObjectPtr<UInputAction> IA_MoveRight;
	UPROPERTY() TObjectPtr<UInputAction> IA_MoveLeft;
	UPROPERTY() TObjectPtr<UInputAction> IA_Look;
	UPROPERTY() TObjectPtr<UInputAction> IA_Jump;
	UPROPERTY() TObjectPtr<UInputAction> IA_Sprint;
	UPROPERTY() TObjectPtr<UInputAction> IA_Crouch;
	UPROPERTY() TObjectPtr<UInputAction> IA_Interact;
	UPROPERTY() TObjectPtr<UInputAction> IA_Fire;
	UPROPERTY() TObjectPtr<UInputAction> IA_Aim;
	UPROPERTY() TObjectPtr<UInputAction> IA_Reload;
	UPROPERTY() TObjectPtr<UInputAction> IA_Flare;
	UPROPERTY() TObjectPtr<UInputAction> IA_Whistle;
	UPROPERTY() TObjectPtr<UInputAction> IA_Lamp;
	UPROPERTY() TObjectPtr<UInputAction> IA_Bandage;
	UPROPERTY() TObjectPtr<UInputAction> IA_Inventory;
	UPROPERTY() TObjectPtr<UInputAction> IA_CloseUI;
	UPROPERTY() TObjectPtr<UInputAction> IA_Slot1;
	UPROPERTY() TObjectPtr<UInputAction> IA_Slot2;
	UPROPERTY() TObjectPtr<UInputAction> IA_Slot3;
	UPROPERTY() TObjectPtr<UInputAction> IA_Slot4;
	UPROPERTY() TObjectPtr<UInputAction> IA_Slot5;
	UPROPERTY() TObjectPtr<UInputAction> IA_Sell1;
	UPROPERTY() TObjectPtr<UInputAction> IA_Sell2;
	UPROPERTY() TObjectPtr<UInputAction> IA_Sell3;
	UPROPERTY() TObjectPtr<UInputAction> IA_Sell4;

	// flags de entrada
	bool bInputForward = false;
	bool bInputBack = false;
	bool bInputRight = false;
	bool bInputLeft = false;
	bool bSprintHeld = false;
	bool bReloading = false;
	float FadeAlpha = 0.f;

	void BuildInputAssets();
	void AddInputMapping();

	// callbacks
	void OnLook(const FInputActionValue& Value);
	void OnJumpStart();
	void OnSprintStart();
	void OnSprintStop();
	void OnCrouchStart();
	void OnCrouchStop();
	void OnInteractStart();
	void OnFireStart();
	void OnAimStart();
	void OnAimStop();
	void OnReloadStart();
	void OnFlareStart();
	void OnWhistleStart();
	void OnLampStart();
	void OnBandageStart();
	void OnInventoryStart();
	void OnCloseUIStart();
	void OnSlot1();
	void OnSlot2();
	void OnSlot3();
	void OnSlot4();
	void OnSlot5();
	void OnSell1();
	void OnSell2();
	void OnSell3();
	void OnSell4();

	// acciones
	void UseTool();
	void CycleTool(int32 Index);
	void FireRevolver();
	void SwingPick();
	void MoveForwardStart() { bInputForward = true; }
	void MoveForwardStop() { bInputForward = false; }
	void MoveBackStart() { bInputBack = true; }
	void MoveBackStop() { bInputBack = false; }
	void MoveRightStart() { bInputRight = true; }
	void MoveRightStop() { bInputRight = false; }
	void MoveLeftStart() { bInputLeft = true; }
	void MoveLeftStop() { bInputLeft = false; }

	void UpdateFocus();
	void UpdateNoise(float DeltaSeconds);
	void UpdateStamina(float DeltaSeconds);
	void UpdateLamp(float DeltaSeconds);
	void UpdateReload(float DeltaSeconds);
	void UpdateDeath(float DeltaSeconds);
	void FinishReloadStep();

	AENInteractableActor* TraceInteractable(float Distance) const;
	void SpawnFlare();
	void Die();
	void Respawn();

	// checkpoint
	FVector RespawnLocation = FVector(0.f, 0.f, 120.f);
	FRotator RespawnRotation = FRotator::ZeroRotator;
	FENInventory RespawnInventory;
	int32 RespawnLoaded = 6;
	int32 RespawnAmmo = 12;
	bool bHasCheckpoint = false;

	float ReloadTimer = 0.f;
	float FireCooldown = 0.f;
	float StaminaRegenDelay = 0.f;
	float LastNoiseImpulse = -100.f;
	float DeathTimer = 0.f;
	float FadeTimer = 0.f;
	FVector FadeTarget = FVector::ZeroVector;
	float FadeYaw = 0.f;
	bool bFadeTeleported = false;
	bool bCrouchHeld = false;

	FTimerHandle TravelTimer;
};
