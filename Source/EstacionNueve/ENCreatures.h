#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ENCreatures.generated.h"

class AENCharacter;
class UStaticMeshComponent;
class UPointLightComponent;
class AENGameState;

/** Estado de la IA de un dinosaurio salvaje. */
UENUM()
enum class EENDinoState : uint8
{
	Idle,
	Alert,
	Chase,
	Attack,
	Dead
};

/** Chillon o Rastreador: IA simple de persecucion y mordisco. */
UCLASS()
class AENDinosaur : public ACharacter
{
	GENERATED_BODY()

public:
	AENDinosaur();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EN") bool bIsRaptor = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EN") float Health = 15.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EN") float MoveSpeed = 380.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EN") float AttackDamage = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EN") float AggroRadius = 1500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EN") bool bStationGuard = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") TObjectPtr<UStaticMeshComponent> Head;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") TObjectPtr<UStaticMeshComponent> Tail;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") EENDinoState State = EENDinoState::Idle;

	void ConfigureAsRaptor();

protected:
	void UpdateAI(float DeltaSeconds);
	void Bite();
	void Die();

	AENCharacter* GetPlayer() const;
	float AttackCooldown = 0.f;
	float IdleWanderTimer = 0.f;
	FVector HomeLocation = FVector::ZeroVector;
	bool bDeadHandled = false;
};

/** La Carnotaurus: invencible, guiada por el ruido. */
UCLASS()
class AENCarnotaurus : public ACharacter
{
	GENERATED_BODY()

public:
	AENCarnotaurus();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") TObjectPtr<UStaticMeshComponent> Head;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") TObjectPtr<UStaticMeshComponent> EyeL;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") TObjectPtr<UStaticMeshComponent> EyeR;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") TObjectPtr<UPointLightComponent> EyeGlow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") bool bActive = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") FString StateName = TEXT("dormida");

	void ActivateAt(const FVector& Start, const TArray<FVector>& InPatrol);
	void Deactivate();
	void DistractAt(const FVector& Location);

protected:
	enum class EState : uint8 { Inactive, Rising, Stalk, Investigate, Charge, Distracted, Leaving };
	EState State = EState::Inactive;

	void UpdateAI(float DeltaSeconds);
	void MoveTowards(const FVector& Target, float Speed, float DeltaSeconds);
	void Roar(const FString& Text);
	void KillPlayer(AENCharacter* Player);

	AENGameState* GetENGameState() const;
	AENCharacter* GetPlayer() const;

	TArray<FVector> Patrol;
	int32 PatrolIndex = 0;
	float StateTimer = 0.f;
	float DistractTimer = 0.f;
	FVector DistractLocation = FVector::ZeroVector;
	float LoudTimer = 0.f;
	bool bRoared = false;
	FVector StartLocation = FVector::ZeroVector;
};

/** Bengala: fuente de ruido y luz que distrae a la Carnotaurus. */
UCLASS()
class AENFlare : public AActor
{
	GENERATED_BODY()

public:
	AENFlare();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") TObjectPtr<UPointLightComponent> Light;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EN") float Life = 20.f;

protected:
	float Timer = 0.f;
};
