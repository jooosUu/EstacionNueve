#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ENPlayerController.generated.h"

/**
 * Controlador del jugador. Ademas de poseer a Cyrus, expone los comandos de
 * consola que usa la verificacion automatica del prototipo.
 */
UCLASS()
class AENPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AENPlayerController();

	virtual void BeginPlay() override;

	UFUNCTION(Exec) void ENSelfTest();
	UFUNCTION(Exec) void ENAutoTest();
	UFUNCTION(Exec) void ENSkipToNight();
	UFUNCTION(Exec) void ENSaveNow();
	UFUNCTION(Exec) void ENLoadNow();
	UFUNCTION(Exec) void ENTeleportTo(const FString& AnchorName);
};
