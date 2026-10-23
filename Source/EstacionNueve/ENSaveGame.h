#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ENGameplayTypes.h"
#include "ENSaveGame.generated.h"

/** Partida guardada al dormir en la casa de mando. */
UCLASS()
class UENSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY() int32 Day = 1;
	UPROPERTY() float TimeOfDay = 0.03f;
	UPROPERTY() int32 Money = 20;
	UPROPERTY() int32 Debt = 1500;
	UPROPERTY() int32 StationLevel = 1;
	UPROPERTY() int32 Furia = 0;
	UPROPERTY() FENInventory Inventory;
	UPROPERTY() int32 RevolverLoaded = 6;
	UPROPERTY() FVector PlayerLocation = FVector::ZeroVector;
	UPROPERTY() FString StationName = TEXT("Estacion Nueve");
	UPROPERTY() FString SavedAt = TEXT("");
};
