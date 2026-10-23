#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ENHUD.generated.h"

class AENCharacter;
class AENGameState;
class AENInteractableActor;
class UFont;

/**
 * HUD dibujado en canvas, sin assets de UMG: el medidor de ruido es la pieza
 * central de la interfaz, con la raya roja en 70.
 */
UCLASS()
class AENHUD : public AHUD
{
	GENERATED_BODY()

public:
	AENHUD();
	virtual void DrawHUD() override;

protected:
	void DrawStats(AENCharacter* C, AENGameState* GS);
	void DrawNoise(AENCharacter* C);
	void DrawClock(AENCharacter* C, AENGameState* GS);
	void DrawObjectives(AENGameState* GS);
	void DrawPrompt(AENCharacter* C);
	void DrawCrosshair(AENCharacter* C);
	void DrawNotices(AENGameState* GS);
	void DrawShop(AENCharacter* C, AENGameState* GS);
	void DrawInventory(AENCharacter* C);
	void DrawSummary(AENGameState* GS);
	void DrawDeath(AENCharacter* C);
	void DrawFade(AENCharacter* C);
	void DrawHelp(AENCharacter* C);

	void Bar(float X, float Y, float W, float H, float Ratio, const FLinearColor& Fill, const FLinearColor& Back);
	void Panel(float X, float Y, float W, float H, const FLinearColor& Color);
	void Label(const FString& Text, float X, float Y, const FLinearColor& Color, float TextScale, bool bCenter = false);
	void Outlined(const FString& Text, float X, float Y, const FLinearColor& Color, float TextScale);
	FString ClockString(float Hours) const;

	UPROPERTY() TObjectPtr<UFont> FontSmall;
	UPROPERTY() TObjectPtr<UFont> FontMed;
	UPROPERTY() TObjectPtr<UFont> FontBig;

	float UIScale = 1.f;
	bool bShowHelp = true;
};
