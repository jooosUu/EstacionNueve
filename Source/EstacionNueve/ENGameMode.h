#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ENGameMode.generated.h"

/**
 * Modo de juego base del prototipo: Cyrus, HUD de canvas y estado del dia.
 * Tambien orquesta la verificacion automatica (-enautotest) y las capturas (-enshot).
 */
UCLASS()
class AENGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AENGameMode();

	virtual void BeginPlay() override;
	virtual void StartPlay() override;

protected:
	void RunAutoTest();
	void TakePlayerShot();
	void TakeAerialShot();

	FTimerHandle AutoTestHandle;
	FTimerHandle ShotHandle;
	FTimerHandle QuitHandle;
};
