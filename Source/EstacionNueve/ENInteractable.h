#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ENGameplayTypes.h"
#include "ENInteractable.generated.h"

class AENCharacter;
class UStaticMeshComponent;
class UBoxComponent;
class UPointLightComponent;
class AENGate;

/**
 * Base de todo lo que Cyrus puede usar con E.
 * Sin interfaz UObject a proposito: una jerarquia simple mantiene el prototipo legible.
 */
UCLASS(Abstract)
class AENInteractableActor : public AActor
{
	GENERATED_BODY()

public:
	AENInteractableActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EN")
	FString Prompt = TEXT("Usar");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EN")
	bool bEnabled = true;

	virtual bool CanInteract(AENCharacter* Char) const;
	virtual FString GetPrompt(AENCharacter* Char) const;
	virtual void Interact(AENCharacter* Char);

	/** Utilidades para el script de editor. */
	UFUNCTION(BlueprintCallable, Category = "EN")
	void SetMeshPath(const FString& Path);

	UFUNCTION(BlueprintCallable, Category = "EN")
	void SetMaterialPath(const FString& Path);

protected:
	UStaticMesh* LoadMesh(const FString& Path) const;
	UMaterialInterface* LoadMaterial(const FString& Path) const;
};

/** Parcela del huerto: arar, sembrar, regar, cosechar. */
UCLASS()
class AENFarmPlot : public AENInteractableActor
{
	GENERATED_BODY()

public:
	AENFarmPlot();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EN") int32 PlotIndex = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") EENFarmState State = EENFarmState::Fallow;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") int32 WaterCount = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") int32 DryDays = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") bool bWateredToday = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") TObjectPtr<UStaticMeshComponent> CropMesh;

	virtual bool CanInteract(AENCharacter* Char) const override;
	virtual FString GetPrompt(AENCharacter* Char) const override;
	virtual void Interact(AENCharacter* Char) override;

	/** Avanza un dia: crece o se seca. */
	void AdvanceDay();
	void UpdateVisual();

	bool IsPlanted() const;
	bool IsGrown() const { return State == EENFarmState::Grown; }
};

/** Torre de agua: rellena el cubo. */
UCLASS()
class AENWaterTower : public AENInteractableActor
{
	GENERATED_BODY()

public:
	AENWaterTower();
	virtual FString GetPrompt(AENCharacter* Char) const override;
	virtual void Interact(AENCharacter* Char) override;
};

/** Valvula de vapor: abre puertas y aturde dinosaurios. */
UCLASS()
class AENValve : public AENInteractableActor
{
	GENERATED_BODY()

public:
	AENValve();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EN") FString ValveId = TEXT("P1");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EN") bool bSteamTrap = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") bool bUsed = false;

	virtual bool CanInteract(AENCharacter* Char) const override;
	virtual FString GetPrompt(AENCharacter* Char) const override;
	virtual void Interact(AENCharacter* Char) override;
};

/** Porton de la estacion. */
UCLASS()
class AENGate : public AENInteractableActor
{
	GENERATED_BODY()

public:
	AENGate();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EN") FString GateId = TEXT("P1");
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") bool bOpen = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") FVector ClosedLocation = FVector::ZeroVector;

	virtual FString GetPrompt(AENCharacter* Char) const override;
	virtual void Interact(AENCharacter* Char) override;
	void OpenGate();
};

/** Veta minable. */
UCLASS()
class AENAmberVein : public AENInteractableActor
{
	GENERATED_BODY()

public:
	AENAmberVein();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EN") EENLootType Loot = EENLootType::Resin;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EN") int32 HitsLeft = 2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EN") int32 AmountPerHit = 1;

	virtual bool CanInteract(AENCharacter* Char) const override;
	virtual FString GetPrompt(AENCharacter* Char) const override;
	virtual void Interact(AENCharacter* Char) override;

	/** Devuelve true si el golpe ha gastado la veta. */
	bool HitWithPick(AENCharacter* Char);
};

/** Objeto recogible del suelo. */
UCLASS()
class AENPickup : public AENInteractableActor
{
	GENERATED_BODY()

public:
	AENPickup();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EN") EENLootType Loot = EENLootType::Ammo;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EN") int32 Amount = 1;

	virtual FString GetPrompt(AENCharacter* Char) const override;
	virtual void Interact(AENCharacter* Char) override;
};

/** Barnaby Quill: abre la tienda. */
UCLASS()
class AENMerchant : public AENInteractableActor
{
	GENERATED_BODY()

public:
	AENMerchant();
	virtual FString GetPrompt(AENCharacter* Char) const override;
	virtual void Interact(AENCharacter* Char) override;
};

/** Refineria del taller. */
UCLASS()
class AENRefinery : public AENInteractableActor
{
	GENERATED_BODY()

public:
	AENRefinery();
	virtual FString GetPrompt(AENCharacter* Char) const override;
	virtual void Interact(AENCharacter* Char) override;
};

/** Generador y sirena: enciende la noche. */
UCLASS()
class AENGenerator : public AENInteractableActor
{
	GENERATED_BODY()

public:
	AENGenerator();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") bool bOn = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") TObjectPtr<UPointLightComponent> Glow;

	virtual bool CanInteract(AENCharacter* Char) const override;
	virtual FString GetPrompt(AENCharacter* Char) const override;
	virtual void Interact(AENCharacter* Char) override;
};

/** Cama / litera de la casa de mando. */
UCLASS()
class AENSleepSpot : public AENInteractableActor
{
	GENERATED_BODY()

public:
	AENSleepSpot();
	virtual FString GetPrompt(AENCharacter* Char) const override;
	virtual void Interact(AENCharacter* Char) override;
};

/** Punto de viaje (boca de la mina <-> caverna). */
UCLASS()
class AENTravelPoint : public AENInteractableActor
{
	GENERATED_BODY()

public:
	AENTravelPoint();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EN") FVector TargetLocation = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EN") float TargetYaw = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EN") bool bReturnToSurface = false;

	virtual FString GetPrompt(AENCharacter* Char) const override;
	virtual void Interact(AENCharacter* Char) override;
};

/** Bandera de checkpoint. */
UCLASS()
class AENCheckpoint : public AENInteractableActor
{
	GENERATED_BODY()

public:
	AENCheckpoint();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") TObjectPtr<UBoxComponent> Trigger;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") bool bActivated = false;

	virtual void BeginPlay() override;

	virtual bool CanInteract(AENCharacter* Char) const override { return false; }
	virtual void Interact(AENCharacter* Char) override {}

	UFUNCTION()
	void OnTriggerBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);
};

/** Brisa, la Parasaurolofo. */
UCLASS()
class AENMount : public AENInteractableActor
{
	GENERATED_BODY()

public:
	AENMount();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") bool bCalmed = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") bool bMounted = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") bool bComingToWhistle = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") TObjectPtr<UStaticMeshComponent> Head;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool CanInteract(AENCharacter* Char) const override;
	virtual FString GetPrompt(AENCharacter* Char) const override;
	virtual void Interact(AENCharacter* Char) override;

	void WhistleTo(const FVector& Location);
	void SetMounted(bool bInMounted);
};
