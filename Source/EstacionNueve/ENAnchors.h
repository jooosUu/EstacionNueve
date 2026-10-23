#pragma once

#include "CoreMinimal.h"

/**
 * Anclas del Nivel 1 en coordenadas de Unreal (cm).
 *
 * El blockout se diseno en "metros de diseno": X = este, Y = norte, con el
 * inicio del nivel en (4, -80). El script de editor usa la misma conversion,
 * asi que estas constantes son la unica fuente de verdad compartida.
 */
namespace ENA
{
	constexpr float OX = 4.f;
	constexpr float OY = -80.f;

	FORCEINLINE FVector W(float East, float North, float ZMeters = 0.f)
	{
		return FVector((North - OY) * 100.f, (East - OX) * 100.f, ZMeters * 100.f);
	}

	FORCEINLINE FVector Start()          { return W(4.f, -80.f, 0.1f); }
	FORCEINLINE FVector Merchant()       { return W(20.f, -64.f, 0.f); }
	FORCEINLINE FVector Gate()           { return W(4.f, -50.f, 0.f); }
	FORCEINLINE FVector ValveP1()        { return W(12.5f, -51.5f, 0.f); }
	FORCEINLINE FVector Stable()         { return W(26.f, -32.f, 0.f); }
	FORCEINLINE FVector Brisa()          { return W(25.f, -27.f, 0.f); }
	FORCEINLINE FVector WaterTower()     { return W(-48.f, 1.f, 0.f); }
	FORCEINLINE FVector Barracks()       { return W(-50.f, -38.f, 0.f); }
	FORCEINLINE FVector HuntersCamp()    { return W(35.f, 29.f, 0.f); }
	FORCEINLINE FVector MineMouth()      { return W(62.f, 42.f, 0.f); }
	FORCEINLINE FVector Workshop()       { return W(50.f, 2.f, 0.f); }
	FORCEINLINE FVector Refinery()       { return W(50.f, -8.f, 0.f); }
	FORCEINLINE FVector MachineRoom()    { return W(0.f, 42.f, 0.f); }
	FORCEINLINE FVector BoilerYard()     { return W(0.f, 0.f, 0.f); }
	FORCEINLINE FVector Generator()      { return W(20.f, 52.f, 0.f); }
	FORCEINLINE FVector Canyon()         { return W(34.f, 64.f, 0.f); }
	FORCEINLINE FVector CommandHouse()   { return W(-50.f, 42.f, 0.f); }
	FORCEINLINE FVector Plot(int32 Index)
	{
		switch (Index)
		{
		case 0: return W(-58.f, 14.f, 0.f);
		case 1: return W(-38.f, 14.f, 0.f);
		case 2: return W(-58.f, -12.f, 0.f);
		default: return W(-38.f, -12.f, 0.f);
		}
	}

	FORCEINLINE TArray<FVector> NightPatrol()
	{
		TArray<FVector> P;
		P.Add(W(34.f, 64.f, 0.f));
		P.Add(W(27.f, 58.f, 0.f));
		P.Add(W(22.f, 54.f, 0.f));
		P.Add(W(18.f, 48.f, 0.f));
		P.Add(W(24.f, 46.f, 0.f));
		P.Add(W(20.f, 43.f, 0.f));
		P.Add(W(14.f, 44.f, 0.f));
		return P;
	}
}
