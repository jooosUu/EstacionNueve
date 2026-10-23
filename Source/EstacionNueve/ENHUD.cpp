#include "ENHUD.h"

#include "ENCharacter.h"
#include "ENGameState.h"
#include "ENInteractable.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerController.h"

AENHUD::AENHUD()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AENHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	FontSmall = GEngine->GetSmallFont();
	FontMed = GEngine->GetMediumFont();
	FontBig = GEngine->GetLargeFont();

	UIScale = FMath::Clamp(Canvas->SizeY / 1080.f, 0.55f, 2.f);

	AENCharacter* C = Cast<AENCharacter>(GetOwningPawn());
	AENGameState* GS = GetWorld() ? Cast<AENGameState>(GetWorld()->GetGameState()) : nullptr;
	if (!C)
	{
		return;
	}

	DrawFade(C);
	if (C->bDead)
	{
		DrawDeath(C);
		return;
	}

	DrawCrosshair(C);
	DrawStats(C, GS);
	DrawNoise(C);
	DrawClock(C, GS);
	DrawObjectives(GS);
	DrawPrompt(C);
	DrawNotices(GS);
	DrawHelp(C);

	if (C->bShopOpen)
	{
		DrawShop(C, GS);
	}
	if (C->bInventoryOpen)
	{
		DrawInventory(C);
	}
	if (GS && GS->bDayComplete)
	{
		DrawSummary(GS);
	}
}

// --------------------------------------------------------------------------- utilidades

void AENHUD::Bar(float X, float Y, float W, float H, float Ratio, const FLinearColor& Fill, const FLinearColor& Back)
{
	DrawRect(Back, X, Y, W, H);
	const float Clamped = FMath::Clamp(Ratio, 0.f, 1.f);
	DrawRect(Fill, X, Y, W * Clamped, H);
}

void AENHUD::Panel(float X, float Y, float W, float H, const FLinearColor& Color)
{
	DrawRect(Color, X, Y, W, H);
}

void AENHUD::Label(const FString& Text, float X, float Y, const FLinearColor& Color, float TextScale, bool bCenter)
{
	UFont* Font = FontMed;
	float TextW = 0.f;
	float TextH = 0.f;
	GetTextSize(Text, TextW, TextH, Font, TextScale * UIScale);
	const float DrawX = bCenter ? X - TextW * 0.5f : X;
	DrawText(Text, Color, DrawX, Y, Font, TextScale * UIScale);
}

void AENHUD::Outlined(const FString& Text, float X, float Y, const FLinearColor& Color, float TextScale)
{
	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, 0.85f), X + 2.f * UIScale, Y + 2.f * UIScale, FontMed, TextScale * UIScale);
	DrawText(Text, Color, X, Y, FontMed, TextScale * UIScale);
}

FString AENHUD::ClockString(float Hours) const
{
	const int32 H = FMath::Clamp(FMath::FloorToInt(Hours), 0, 24);
	const int32 M = FMath::Clamp(FMath::FloorToInt((Hours - H) * 60.f), 0, 59);
	return FString::Printf(TEXT("%02d:%02d"), H, M);
}

// --------------------------------------------------------------------------- paneles

void AENHUD::DrawStats(AENCharacter* C, AENGameState* GS)
{
	const float S = UIScale;
	const float X = 42.f * S;
	const float BaseY = Canvas->SizeY - 120.f * S;

	Panel(X - 12.f * S, BaseY - 34.f * S, 372.f * S, 108.f * S, FLinearColor(0.03f, 0.03f, 0.03f, 0.55f));

	Outlined(FString::Printf(TEXT("VIDA %d"), FMath::RoundToInt(C->Health)), X, BaseY - 30.f * S,
		FLinearColor(1.f, 0.85f, 0.8f), 1.15f);
	Bar(X, BaseY - 6.f * S, 330.f * S, 16.f * S, C->Health / C->MaxHealth,
		FLinearColor(0.85f, 0.18f, 0.15f), FLinearColor(0.12f, 0.05f, 0.05f, 0.8f));

	Outlined(TEXT("AGUANTE"), X, BaseY + 22.f * S, FLinearColor(0.95f, 0.9f, 0.6f), 0.8f);
	Bar(X, BaseY + 42.f * S, 330.f * S, 9.f * S, C->Stamina / C->MaxStamina,
		FLinearColor(0.95f, 0.8f, 0.25f), FLinearColor(0.1f, 0.09f, 0.04f, 0.8f));

	if (C->Health < 30.f)
	{
		DrawRect(FLinearColor(0.6f, 0.f, 0.f, 0.10f), 0.f, 0.f, Canvas->SizeX, Canvas->SizeY);
	}
}

void AENHUD::DrawNoise(AENCharacter* C)
{
	const float S = UIScale;
	const float W = 520.f * S;
	const float H = 24.f * S;
	const float X = (Canvas->SizeX - W) * 0.5f;
	const float Y = 42.f * S;

	Panel(X - 8.f * S, Y - 26.f * S, W + 16.f * S, H + 52.f * S, FLinearColor(0.02f, 0.02f, 0.03f, 0.6f));

	const float Ratio = C->Noise / 100.f;
	const FLinearColor Fill = C->Noise >= 70.f
		? FLinearColor(1.f, 0.2f, 0.12f)
		: FLinearColor(0.95f, 0.62f, 0.18f);

	Bar(X, Y, W, H, Ratio, Fill, FLinearColor(0.08f, 0.08f, 0.09f, 0.9f));

	// Raya roja del umbral 70.
	const float MarkX = X + W * 0.7f;
	DrawRect(FLinearColor(1.f, 0.15f, 0.1f, 0.95f), MarkX, Y - 6.f * S, 3.f * S, H + 12.f * S);

	Outlined(FString::Printf(TEXT("RUIDO %d"), FMath::RoundToInt(C->Noise)),
		X, Y - 22.f * S, FLinearColor(1.f, 0.9f, 0.75f), 1.05f);
	Outlined(TEXT("70"), MarkX + 8.f * S, Y - 22.f * S, FLinearColor(1.f, 0.35f, 0.25f), 0.8f);

	if (C->bRiding)
	{
		Outlined(TEXT("A LOMOS DE BRISA"), X, Y + H + 8.f * S, FLinearColor(0.7f, 0.95f, 0.65f), 0.9f);
	}
}

void AENHUD::DrawClock(AENCharacter* C, AENGameState* GS)
{
	if (!GS)
	{
		return;
	}
	const float S = UIScale;
	const float Right = Canvas->SizeX - 42.f * S;

	const FString Clock = GS->GetClockText();
	float W = 0.f;
	float H = 0.f;
	GetTextSize(Clock, W, H, FontBig, 1.5f * S);
	DrawText(Clock, FLinearColor(0.95f, 0.9f, 0.75f), Right - W, 34.f * S, FontBig, 1.5f * S);

	const FString DayLine = FString::Printf(TEXT("Dia %d  ·  %s  ·  %s"), GS->Day, *GS->GetPhaseName(), *GS->GetDayNightText());
	GetTextSize(DayLine, W, H, FontMed, 1.f * S);
	DrawText(DayLine, FLinearColor(0.85f, 0.85f, 0.8f), Right - W, 34.f * S + 52.f * S, FontMed, 1.f * S);

	// Furia 0..10.
	const float BarW = 220.f * S;
	const float BarX = Right - BarW;
	const float BarY = 34.f * S + 84.f * S;
	Outlined(TEXT("FURIA"), BarX, BarY - 20.f * S, FLinearColor(1.f, 0.55f, 0.4f), 0.8f);
	Bar(BarX, BarY, BarW, 8.f * S, GS->Furia / 10.f, FLinearColor(0.85f, 0.25f, 0.15f),
		FLinearColor(0.12f, 0.06f, 0.05f, 0.85f));

	const FString Money = FString::Printf(TEXT("$%d  ·  deuda $%d"), GS->Money, GS->Debt);
	GetTextSize(Money, W, H, FontMed, 0.95f * S);
	DrawText(Money, FLinearColor(0.78f, 0.95f, 0.72f), Right - W, BarY + 18.f * S, FontMed, 0.95f * S);
}

void AENHUD::DrawObjectives(AENGameState* GS)
{
	if (!GS)
	{
		return;
	}
	const float S = UIScale;
	const float X = 42.f * S;
	float Y = 34.f * S;

	const int32 Total = GS->Objectives.Num();
	int32 Done = 0;
	for (const FENObjective& Obj : GS->Objectives)
	{
		if (Obj.bDone)
		{
			Done++;
		}
	}

	Panel(X - 10.f * S, Y - 8.f * S, 470.f * S, 86.f * S, FLinearColor(0.02f, 0.03f, 0.03f, 0.55f));
	Outlined(FString::Printf(TEXT("OBJETIVO  %d/%d"), Done, Total), X, Y, FLinearColor(0.7f, 0.9f, 1.f), 0.85f);

	// Ultimo completado + objetivo activo.
	FString Active = GS->GetActiveObjectiveText();
	for (const FENObjective& Obj : GS->Objectives)
	{
		if (Obj.bDone && Done > 0 && Obj.Title == GS->Objectives[Done - 1].Title)
		{
			Outlined(FString::Printf(TEXT("OK  %s"), *Obj.Title), X, Y + 24.f * S, FLinearColor(0.55f, 0.95f, 0.6f), 0.78f);
			break;
		}
	}
	Outlined(FString::Printf(TEXT(">  %s"), *Active), X, Y + 48.f * S, FLinearColor(0.98f, 0.92f, 0.75f), 0.85f);
}

void AENHUD::DrawPrompt(AENCharacter* C)
{
	if (C->FocusedPrompt.IsEmpty() || C->bShopOpen || C->bInventoryOpen)
	{
		return;
	}
	const float S = UIScale;
	float W = 0.f;
	float H = 0.f;
	GetTextSize(C->FocusedPrompt, W, H, FontMed, 1.f * S);

	const float X = (Canvas->SizeX - W) * 0.5f;
	const float Y = Canvas->SizeY - 230.f * S;
	Panel(X - 18.f * S, Y - 10.f * S, W + 36.f * S, H + 20.f * S, FLinearColor(0.02f, 0.02f, 0.03f, 0.72f));
	DrawText(FString::Printf(TEXT("[E]  %s"), *C->FocusedPrompt), FLinearColor(0.95f, 0.9f, 0.7f), X, Y, FontMed, 1.f * S);
}

void AENHUD::DrawNotices(AENGameState* GS)
{
	if (!GS)
	{
		return;
	}
	const float S = UIScale;
	float Y = 200.f * S;
	const int32 Max = FMath::Min(5, GS->Notices.Num());
	for (int32 i = 0; i < Max; ++i)
	{
		const FENNotice& Notice = GS->Notices[i];
		const float Alpha = FMath::Clamp(Notice.TimeLeft / 1.5f, 0.f, 1.f) * 0.9f;
		FLinearColor Color = Notice.Color;
		Color.A = Alpha;

		float W = 0.f;
		float H = 0.f;
		GetTextSize(Notice.Text, W, H, FontSmall, 0.85f * S);
		Panel(40.f * S, Y - 4.f * S, W + 24.f * S, H + 10.f * S, FLinearColor(0.f, 0.f, 0.f, 0.45f * Alpha));
		DrawText(Notice.Text, Color, 52.f * S, Y, FontSmall, 0.85f * S);
		Y += H + 14.f * S;
	}
}

void AENHUD::DrawCrosshair(AENCharacter* C)
{
	const float S = UIScale;
	const float CX = Canvas->SizeX * 0.5f;
	const float CY = Canvas->SizeY * 0.5f;
	const float Radius = (C->bAiming ? 2.f : 4.f) * S;

	DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.85f), CX - 1.f * S, CY - 6.f * S, 2.f * S, 12.f * S);
	DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.85f), CX - 6.f * S, CY - 1.f * S, 12.f * S, 2.f * S);
	DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.25f), CX - Radius, CY - Radius, Radius * 2.f, Radius * 2.f);
}

void AENHUD::DrawShop(AENCharacter* C, AENGameState* GS)
{
	const float S = UIScale;
	const float W = 900.f * S;
	const float H = 560.f * S;
	const float X = (Canvas->SizeX - W) * 0.5f;
	const float Y = (Canvas->SizeY - H) * 0.5f;

	Panel(0.f, 0.f, Canvas->SizeX, Canvas->SizeY, FLinearColor(0.f, 0.f, 0.f, 0.55f));
	Panel(X, Y, W, H, FLinearColor(0.07f, 0.06f, 0.05f, 0.96f));
	DrawRect(FLinearColor(0.85f, 0.6f, 0.2f, 0.9f), X, Y, W, 4.f * S);

	DrawText(TEXT("BENJAMIN QUILL  ·  MERCADER ITINERANTE"), FLinearColor(0.95f, 0.8f, 0.5f), X + 32.f * S, Y + 24.f * S, FontBig, 1.05f * S);
	DrawText(FString::Printf(TEXT("Plata: $%d"), GS ? GS->Money : 0), FLinearColor(0.75f, 0.95f, 0.7f), X + 32.f * S, Y + 62.f * S, FontMed, 1.f * S);

	const float ColY = Y + 110.f * S;
	DrawText(TEXT("COMPRAR  [1-5]"), FLinearColor(0.95f, 0.85f, 0.6f), X + 32.f * S, ColY, FontMed, 1.f * S);
	for (int32 i = 0; i < C->BuyList.Num(); ++i)
	{
		const FENShopEntry& Item = C->BuyList[i];
		const float LineY = ColY + 34.f * S + i * 40.f * S;
		const bool bAfford = GS && GS->Money >= Item.Price;
		DrawText(FString::Printf(TEXT("[%d]  %s"), i + 1, *Item.Label), bAfford ? FLinearColor(0.92f, 0.9f, 0.82f) : FLinearColor(0.55f, 0.5f, 0.45f), X + 40.f * S, LineY, FontSmall, 1.f * S);
		DrawText(FString::Printf(TEXT("$%d"), Item.Price), bAfford ? FLinearColor(0.7f, 0.95f, 0.65f) : FLinearColor(0.5f, 0.45f, 0.4f), X + 390.f * S, LineY, FontSmall, 1.f * S);
	}

	DrawText(TEXT("VENDER  [6-9]"), FLinearColor(0.95f, 0.85f, 0.6f), X + 490.f * S, ColY, FontMed, 1.f * S);
	for (int32 i = 0; i < C->SellList.Num(); ++i)
	{
		const FENShopEntry& Item = C->SellList[i];
		const float LineY = ColY + 34.f * S + i * 40.f * S;
		const int32 Have = C->Inv.GetLootAmount(Item.Loot);
		DrawText(FString::Printf(TEXT("[%d]  %s (%d)"), i + 6, *Item.Label, Have), Have > 0 ? FLinearColor(0.92f, 0.9f, 0.82f) : FLinearColor(0.55f, 0.5f, 0.45f), X + 500.f * S, LineY, FontSmall, 1.f * S);
		DrawText(FString::Printf(TEXT("$%d"), Item.Price), FLinearColor(0.7f, 0.95f, 0.65f), X + 800.f * S, LineY, FontSmall, 1.f * S);
	}

	DrawText(TEXT("[E] cerrar"), FLinearColor(0.8f, 0.8f, 0.75f), X + 32.f * S, Y + H - 40.f * S, FontSmall, 0.9f * S);
}

void AENHUD::DrawInventory(AENCharacter* C)
{
	const float S = UIScale;
	const float W = 560.f * S;
	const float H = 470.f * S;
	const float X = Canvas->SizeX - W - 40.f * S;
	const float Y = 160.f * S;

	Panel(X, Y, W, H, FLinearColor(0.04f, 0.05f, 0.05f, 0.94f));
	DrawRect(FLinearColor(0.8f, 0.65f, 0.25f, 0.9f), X, Y, W, 3.f * S);
	DrawText(TEXT("MOCHILA DE CYRUS"), FLinearColor(0.95f, 0.85f, 0.6f), X + 24.f * S, Y + 18.f * S, FontMed, 1.f * S);

	struct FRow { FString Name; int32 Value; };
	TArray<FRow> Rows;
	Rows.Add({ TEXT("Resina en bruto"), C->Inv.Resin });
	Rows.Add({ TEXT("Resina refinada"), C->Inv.Refined });
	Rows.Add({ TEXT("Hierro"), C->Inv.Iron });
	Rows.Add({ TEXT("Carbon"), C->Inv.Coal });
	Rows.Add({ TEXT("Ambar"), C->Inv.Amber });
	Rows.Add({ TEXT("Forraje"), C->Inv.Forage });
	Rows.Add({ TEXT("Semillas"), C->Inv.Seeds });
	Rows.Add({ TEXT("Balas de reserva"), C->Inv.Ammo });
	Rows.Add({ TEXT("Vendas"), C->Inv.Bandages });
	Rows.Add({ TEXT("Bengalas"), C->Inv.Flares });
	Rows.Add({ TEXT("Aceite de lampara"), C->Inv.Oil });

	for (int32 i = 0; i < Rows.Num(); ++i)
	{
		const float LineY = Y + 62.f * S + i * 32.f * S;
		DrawText(Rows[i].Name, FLinearColor(0.88f, 0.87f, 0.8f), X + 28.f * S, LineY, FontSmall, 1.f * S);
		DrawText(FString::Printf(TEXT("%d"), Rows[i].Value), FLinearColor(0.95f, 0.85f, 0.55f), X + 440.f * S, LineY, FontSmall, 1.f * S);
	}

	DrawText(FString::Printf(TEXT("Cubo: %d/3 riegos   ·   Herramienta: %s"), C->BucketCharges, *C->GetToolName()),
		FLinearColor(0.8f, 0.85f, 0.8f), X + 24.f * S, Y + H - 40.f * S, FontSmall, 0.9f * S);
}

void AENHUD::DrawSummary(AENGameState* GS)
{
	const float S = UIScale;
	const float W = 700.f * S;
	const float H = 420.f * S;
	const float X = (Canvas->SizeX - W) * 0.5f;
	const float Y = (Canvas->SizeY - H) * 0.5f;

	Panel(0.f, 0.f, Canvas->SizeX, Canvas->SizeY, FLinearColor(0.f, 0.f, 0.f, 0.65f));
	Panel(X, Y, W, H, FLinearColor(0.05f, 0.06f, 0.06f, 0.97f));
	DrawRect(FLinearColor(0.9f, 0.65f, 0.2f, 0.95f), X, Y, W, 5.f * S);

	DrawText(TEXT("FIN DEL DIA 1"), FLinearColor(0.98f, 0.85f, 0.55f), X + 40.f * S, Y + 32.f * S, FontBig, 1.5f * S);
	DrawText(TEXT("La Compania recibe tu telegrama. La estacion sigue en pie."), FLinearColor(0.85f, 0.85f, 0.78f), X + 40.f * S, Y + 100.f * S, FontMed, 0.9f * S);

	int32 Done = 0;
	for (const FENObjective& Obj : GS->Objectives)
	{
		if (Obj.bDone) { Done++; }
	}

	float LineY = Y + 150.f * S;
	DrawText(FString::Printf(TEXT("Objetivos completados: %d / %d"), Done, GS->Objectives.Num()), FLinearColor(0.75f, 0.95f, 0.7f), X + 40.f * S, LineY, FontMed, 1.f * S);
	LineY += 42.f * S;
	DrawText(FString::Printf(TEXT("Vetas picadas: %d    ·    Dinosaurios: %d"), GS->VeinsMined, GS->DinosaursKilled), FLinearColor(0.85f, 0.85f, 0.8f), X + 40.f * S, LineY, FontSmall, 1.f * S);
	LineY += 32.f * S;
	DrawText(FString::Printf(TEXT("Plata: $%d    ·    Deuda restante: $%d"), GS->Money, GS->Debt), FLinearColor(0.85f, 0.85f, 0.8f), X + 40.f * S, LineY, FontSmall, 1.f * S);
	LineY += 32.f * S;
	DrawText(FString::Printf(TEXT("Furia: %d / 10    ·    Estacion nivel %d"), GS->Furia, GS->StationLevel), FLinearColor(0.9f, 0.7f, 0.55f), X + 40.f * S, LineY, FontSmall, 1.f * S);

	DrawText(TEXT("Partida guardada. El dia 2 empieza en el huerto..."), FLinearColor(0.7f, 0.85f, 1.f), X + 40.f * S, Y + H - 50.f * S, FontSmall, 1.f * S);
}

void AENHUD::DrawDeath(AENCharacter* C)
{
	Panel(0.f, 0.f, Canvas->SizeX, Canvas->SizeY, FLinearColor(0.25f, 0.f, 0.f, 0.55f));
	const FString Text = TEXT("HAS MUERTO");
	float W = 0.f;
	float H = 0.f;
	GetTextSize(Text, W, H, FontBig, 2.f * UIScale);
	DrawText(Text, FLinearColor(0.95f, 0.25f, 0.2f), (Canvas->SizeX - W) * 0.5f, Canvas->SizeY * 0.44f, FontBig, 2.f * UIScale);
	DrawText(TEXT("Vuelves al ultimo checkpoint..."), FLinearColor(0.9f, 0.85f, 0.8f),
		Canvas->SizeX * 0.5f - 150.f * UIScale, Canvas->SizeY * 0.44f + 80.f * UIScale, FontMed, 1.f * UIScale);
}

void AENHUD::DrawFade(AENCharacter* C)
{
	const float Alpha = C->GetFadeAlpha();
	if (Alpha > 0.f)
	{
		DrawRect(FLinearColor(0.f, 0.f, 0.f, Alpha), 0.f, 0.f, Canvas->SizeX, Canvas->SizeY);
	}
}

void AENHUD::DrawHelp(AENCharacter* C)
{
	if (!bShowHelp)
	{
		return;
	}
	const float S = UIScale;
	const FString Help = C->bRiding
		? TEXT("WASD moverse · Shift galope · E desmontar · V silbar · F lampara · H vendarse")
		: TEXT("WASD moverse · Shift correr · Ctrl agacharse · E usar · Click disparar/picar/picar veta · R recargar · 1-5 herramientas · Q bengala · V silbar · F lampara · H vendas · Tab mochila");
	float W = 0.f;
	float H = 0.f;
	GetTextSize(Help, W, H, FontSmall, 0.75f * S);
	DrawText(Help, FLinearColor(0.75f, 0.75f, 0.7f, 0.75f), 42.f * S, Canvas->SizeY - 26.f * S, FontSmall, 0.75f * S);
}
