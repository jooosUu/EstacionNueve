#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ENGameplayTypes.h"
#include "ENGameState.generated.h"

class AENCharacter;
class AENCarnotaurus;
class ADirectionalLight;
class ASkyLight;
class AExponentialHeightFog;
class UENSaveGame;

/** Aviso efimero que el HUD muestra en la esquina. */
struct FENNotice
{
	FString Text;
	float TimeLeft = 4.f;
	FLinearColor Color = FLinearColor::White;

	FENNotice() {}
	FENNotice(const FString& InText, const FLinearColor& InColor, float InTime)
		: Text(InText), TimeLeft(InTime), Color(InColor) {}
};

/**
 * Estado de la partida: reloj del dia, economia, objetivos, noche y guardado.
 */
UCLASS()
class AENGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AENGameState();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// ---------------------------------------------------------------- reloj
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") int32 Day = 1;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") float TimeOfDay = 0.03f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EN") float DayLength = 1500.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") EENDayPhase Phase = EENDayPhase::Morning;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") bool bNight = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") int32 Furia = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") float NightTimer = 0.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") bool bNightSurvived = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") bool bDayComplete = false;

	// ---------------------------------------------------------------- economia
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") int32 Money = 20;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") int32 Debt = 1500;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") int32 StationLevel = 1;

	// ---------------------------------------------------------------- progreso del dia
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") int32 VeinsMined = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") int32 DinosaursKilled = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") int32 PlotsHarvested = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") TArray<FENObjective> Objectives;
	TArray<FENNotice> Notices;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EN") TObjectPtr<AENCarnotaurus> Carnotaurus;

	// ---------------------------------------------------------------- api
	void Notify(const FString& Text, const FLinearColor& Color = FLinearColor::White, float Seconds = 4.f);
	void CompleteObjective(int32 Index);
	bool IsObjectiveDone(int32 Index) const;
	FString GetActiveObjectiveText() const;

	void OnGeneratorStarted();
	void BeginNight();
	void EndNight();
	void SleepAndAdvanceDay(AENCharacter* Char);
	void OnPlayerDied(AENCharacter* Char);
	void OnVeinMined();
	void OnDinosaurKilled();

	float GetClockHours() const;
	FString GetClockText() const;
	FString GetPhaseName() const;
	FString GetDayNightText() const;

	void SaveGame();
	bool LoadGame();
	void ApplyLoadedState(const UENSaveGame* Save);

	AENCharacter* GetENCharacter() const;

	// depuracion
	void ForceNight();

private:
	void UpdateClock(float DeltaSeconds);
	void UpdateSky(float DeltaSeconds);
	void UpdateAmbience(float DeltaSeconds);
	void AdvancePlots();

	UPROPERTY() TObjectPtr<ADirectionalLight> Sun;
	UPROPERTY() TObjectPtr<ASkyLight> Sky;
	UPROPERTY() TObjectPtr<AExponentialHeightFog> Fog;

	float BaseSunIntensity = 3.f;
	float BaseFogDensity = 0.008f;
	float CurrentFogDensity = 0.008f;
	bool bFoundSkyActors = false;
	float AnnounceTimer = 0.f;
	float NoiseWarnTimer = 0.f;
	int32 LastPhaseIndex = -1;
	float SleepTimer = 0.f;
	bool bSleeping = false;
};
