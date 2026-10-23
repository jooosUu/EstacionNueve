#pragma once

#include "CoreMinimal.h"
#include "ENGameplayTypes.generated.h"

/** Herramienta equipada en las teclas 1-5. */
UENUM(BlueprintType)
enum class EENTool : uint8
{
	Revolver	UMETA(DisplayName = "Revolver"),
	Pickaxe		UMETA(DisplayName = "Pico"),
	Hoe			UMETA(DisplayName = "Azada"),
	Bucket		UMETA(DisplayName = "Cubo"),
	Flare		UMETA(DisplayName = "Bengala")
};

/** Estado de una parcela del huerto. */
UENUM(BlueprintType)
enum class EENFarmState : uint8
{
	Fallow		UMETA(DisplayName = "Sin arar"),
	Tilled		UMETA(DisplayName = "Arada"),
	Planted		UMETA(DisplayName = "Sembrada"),
	Watered		UMETA(DisplayName = "Regada"),
	Sprout		UMETA(DisplayName = "Brote"),
	Grown		UMETA(DisplayName = "Cosechable"),
	Dead		UMETA(DisplayName = "Seca")
};

/** Fase del dia. */
UENUM(BlueprintType)
enum class EENDayPhase : uint8
{
	Morning		UMETA(DisplayName = "Manana"),
	Midday		UMETA(DisplayName = "Mediodia"),
	Afternoon	UMETA(DisplayName = "Tarde"),
	Sunset		UMETA(DisplayName = "Atardecer"),
	Night		UMETA(DisplayName = "Noche")
};

/** Tipo de botin que suelta una veta o un cadaver. */
UENUM(BlueprintType)
enum class EENLootType : uint8
{
	None,
	Resin,
	Iron,
	Coal,
	Amber,
	Forage,
	Seeds,
	Ammo,
	Bandage,
	Flare,
	Oil,
	Refined
};

/** Objetivo del dia 1. */
USTRUCT(BlueprintType)
struct FENObjective
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString Title;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bDone = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bActive = false;

	FENObjective() {}
	FENObjective(const FString& InTitle, bool bInActive = false)
		: Title(InTitle), bDone(false), bActive(bInActive) {}
};

/** Mochila y recursos de Cyrus. */
USTRUCT(BlueprintType)
struct FENInventory
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Resin = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Refined = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Iron = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Coal = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Amber = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Forage = 2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Seeds = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Ammo = 12;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Bandages = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Flares = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Oil = 60;

	int32 GetLootAmount(EENLootType Type) const
	{
		switch (Type)
		{
		case EENLootType::Resin:   return Resin;
		case EENLootType::Refined: return Refined;
		case EENLootType::Iron:    return Iron;
		case EENLootType::Coal:    return Coal;
		case EENLootType::Amber:   return Amber;
		case EENLootType::Forage:  return Forage;
		case EENLootType::Seeds:   return Seeds;
		case EENLootType::Ammo:    return Ammo;
		case EENLootType::Bandage: return Bandages;
		case EENLootType::Flare:   return Flares;
		case EENLootType::Oil:     return Oil;
		default: return 0;
		}
	}

	void AddLoot(EENLootType Type, int32 Amount)
	{
		switch (Type)
		{
		case EENLootType::Resin:   Resin += Amount; break;
		case EENLootType::Refined: Refined += Amount; break;
		case EENLootType::Iron:    Iron += Amount; break;
		case EENLootType::Coal:    Coal += Amount; break;
		case EENLootType::Amber:   Amber += Amount; break;
		case EENLootType::Forage:  Forage += Amount; break;
		case EENLootType::Seeds:   Seeds += Amount; break;
		case EENLootType::Ammo:    Ammo += Amount; break;
		case EENLootType::Bandage: Bandages += Amount; break;
		case EENLootType::Flare:   Flares += Amount; break;
		case EENLootType::Oil:     Oil = FMath::Clamp(Oil + Amount, 0, 100); break;
		default: break;
		}
	}
};

/** Etiqueta legible de un botin. */
FORCEINLINE FString ENLootName(EENLootType Type)
{
	switch (Type)
	{
	case EENLootType::Resin:   return TEXT("resina");
	case EENLootType::Refined: return TEXT("resina refinada");
	case EENLootType::Iron:    return TEXT("hierro");
	case EENLootType::Coal:    return TEXT("carbon");
	case EENLootType::Amber:   return TEXT("ambar");
	case EENLootType::Forage:  return TEXT("forraje");
	case EENLootType::Seeds:   return TEXT("semillas");
	case EENLootType::Ammo:    return TEXT("balas");
	case EENLootType::Bandage: return TEXT("vendas");
	case EENLootType::Flare:   return TEXT("bengalas");
	case EENLootType::Oil:     return TEXT("aceite");
	default: return TEXT("nada");
	}
}
