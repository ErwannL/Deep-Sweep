#include "Player/DSDiver.h"
#include "DeepSweep.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputModifiers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "HAL/IConsoleManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Pickups/DSPickup.h"
#include "Player/DSGrabComponent.h"
#include "World/DSInteractable.h"
#include "World/DSCounter.h"
#include "Machines/DSVacuum.h"
#include "Machines/DSVacuumNozzle.h"
#include "Machines/DSVacuumControls.h"
#include "Machines/DSConveyor.h"
#include "Giants/DSGiantPickup.h"
#include "Giants/DSCarryComponent.h"
#include "Upgrades/DSUpgradePad.h"
#include "Events/DSOctopus.h"
#include "GameFramework/GameModeBase.h"
#include "Net/DSSessionSubsystem.h"
#include "Audio/DSAudioSubsystem.h"
#include "FX/DSFX.h"
#include "Player/DSPlayerController.h"
#include "Core/DSUserSettings.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerState.h"
#include "Engine/GameInstance.h"
#include "Pickups/DSPickupManager.h"
#include "Pickups/DSPickupData.h"
#include "Core/DSGameState.h"
#include "GameFramework/GameStateBase.h"
#include "EngineUtils.h"
#include "Pickups/DSPickupSubsystem.h"

static TAutoConsoleVariable<int32> CVarAutoPilot(TEXT("ds.AutoPilot"), 0,
	TEXT("Debug : 1 = le plongeur local nage seul en spirale (captures automatiques)."));
static TAutoConsoleVariable<int32> CVarTestGrab(TEXT("ds.TestGrab"), 0,
	TEXT("Debug : scenario auto (attrape la cible a 3 s, capture, lance a 5 s, capture, quitte)."));
static TAutoConsoleVariable<int32> CVarTestSell(TEXT("ds.TestSell"), 0,
	TEXT("Debug : scenario auto attraper -> aller au comptoir -> vendre -> capture -> quitte."));
static TAutoConsoleVariable<int32> CVarTestVac(TEXT("ds.TestVac"), 0,
	TEXT("Debug : scenario auto prendre la buse -> aspirer un tas -> captures -> quitte."));
static TAutoConsoleVariable<int32> CVarTestBelt(TEXT("ds.TestBelt"), 0,
	TEXT("Debug : scenario auto cuve -> tapis (avec caddie qui bloque) -> deblocage -> vente."));
static TAutoConsoleVariable<int32> CVarTestGiant(TEXT("ds.TestGiant"), 0,
	TEXT("Debug : scenario auto porter le coffre jusqu au comptoir."));
static TAutoConsoleVariable<int32> CVarTestShop(TEXT("ds.TestShop"), 0,
	TEXT("Debug : scenario auto credits -> achat des 5 ameliorations -> verification des effets."));
static TAutoConsoleVariable<int32> CVarTestOcto(TEXT("ds.TestOcto"), 0,
	TEXT("Debug : scenario auto poulpe voleur -> capture -> ecran de fin."));
static TAutoConsoleVariable<int32> CVarTestNet(TEXT("ds.TestNet"), 0,
	TEXT("Debug hote : attend N joueurs puis coffre a plusieurs + aspiration d un collegue + debit."));
static TAutoConsoleVariable<int32> CVarTestNetClient(TEXT("ds.TestNetClient"), 0,
	TEXT("Debug client : journalise la replication et pousse le coffre."));
static TAutoConsoleVariable<int32> CVarTestKeys(TEXT("ds.TestKeys"), 0,
	TEXT("Test : simule de vraies touches (Z/W, Echap) via Slate apres l arrivee en jeu."));
static TAutoConsoleVariable<float> CVarShotAt(TEXT("ds.ShotAt"), 0.f,
	TEXT("Debug : capture d'ecran a T secondes puis quitte (0 = desactive)."));

FLinearColor ADSDiver::GetPlayerColor(int32 Index)
{
	static const FLinearColor Colors[4] = {
		FLinearColor(1.f, 0.65f, 0.05f), FLinearColor(1.f, 0.15f, 0.3f),
		FLinearColor(0.1f, 0.75f, 1.f),  FLinearColor(0.4f, 1.f, 0.2f) };
	return Colors[FMath::Clamp(Index, 0, 3)];
}

ADSDiver::ADSDiver(const FObjectInitializer& OI) : Super(OI)
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	GetCapsuleComponent()->InitCapsuleSize(45.f, 70.f);

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Mov = GetCharacterMovement();
	Mov->DefaultLandMovementMode = MOVE_Flying;
	Mov->DefaultWaterMovementMode = MOVE_Flying;
	Mov->GravityScale = 0.f;
	Mov->MaxFlySpeed = SwimSpeed;
	Mov->MaxAcceleration = 1800.f;
	Mov->BrakingDecelerationFlying = 1100.f; // glisse legere, effet "eau"
	Mov->bOrientRotationToMovement = true;
	Mov->RotationRate = FRotator(0.f, 420.f, 0.f);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 650.f;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 120.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 8.f;
	CameraBoom->bEnableCameraRotationLag = true;
	CameraBoom->CameraRotationLagSpeed = 12.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
	Camera->SetFieldOfView(80.f);

	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(RootComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ColorMat(TEXT("/Game/Materials/M_DSColor.M_DSColor"));

	auto MakePart = [&](const TCHAR* Name, UStaticMesh* InMesh, FVector Loc, FVector Scale)
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(VisualRoot);
		C->SetStaticMesh(InMesh);
		if (ColorMat.Succeeded()) C->SetMaterial(0, ColorMat.Object);
		C->SetRelativeLocation(Loc);
		C->SetRelativeScale3D(Scale);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return C;
	};
	// Corps trapu + gros casque rond a hublot (GDD 13). Les meshes de base font 100 cm.
	BodyMesh   = MakePart(TEXT("Body"),   Sphere.Object,   FVector(0, 0, -20),   FVector(0.85f, 0.8f, 1.0f));
	TankMesh   = MakePart(TEXT("Tank"),   Cylinder.Object, FVector(-42, 0, -10), FVector(0.35f, 0.35f, 0.7f));
	HelmetMesh = MakePart(TEXT("Helmet"), Sphere.Object,   FVector(8, 0, 48),    FVector(0.95f));
	VisorMesh  = MakePart(TEXT("Visor"),  Sphere.Object,   FVector(40, 0, 50),   FVector(0.28f, 0.62f, 0.55f));
	EyeL       = MakePart(TEXT("EyeL"),   Sphere.Object,   FVector(52, -13, 55), FVector(0.12f, 0.12f, 0.16f));
	EyeR       = MakePart(TEXT("EyeR"),   Sphere.Object,   FVector(52, 13, 55),  FVector(0.12f, 0.12f, 0.16f));
	ClawMesh   = MakePart(TEXT("Claw"),   Cylinder.Object, FVector(45, 38, -15), FVector(0.12f, 0.12f, 0.55f));
	ClawMesh->SetRelativeRotation(FRotator(-70.f, 0.f, 0.f));

	Grab = CreateDefaultSubobject<UDSGrabComponent>(TEXT("Grab"));
}

void ADSDiver::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADSDiver, ColorIndex);
	DOREPLIFETIME(ADSDiver, StunUntil);
	DOREPLIFETIME(ADSDiver, CarryingGiant);
}

void ADSDiver::SetColorIndex(uint8 NewIndex)
{
	ColorIndex = NewIndex;
	ApplyColor();
}

void ADSDiver::OnRep_ColorIndex() { ApplyColor(); }

void ADSDiver::ApplyColor()
{
	auto Tint = [](UStaticMeshComponent* C, const FLinearColor& Col)
	{
		if (!C) return;
		if (UMaterialInstanceDynamic* MID = C->CreateDynamicMaterialInstance(0))
			MID->SetVectorParameterValue(TEXT("Color"), Col);
	};
	Tint(BodyMesh, GetPlayerColor(ColorIndex));
	Tint(TankMesh, FLinearColor(0.9f, 0.9f, 0.2f));
	Tint(HelmetMesh, FLinearColor(0.85f, 0.85f, 0.88f));
	Tint(VisorMesh, FLinearColor(0.02f, 0.08f, 0.12f));
	Tint(EyeL, FLinearColor::White);
	Tint(EyeR, FLinearColor::White);
	Tint(ClawMesh, FLinearColor(0.95f, 0.75f, 0.1f));
}

void ADSDiver::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	ApplyColor();
}

void ADSDiver::BuildInput()
{
	if (MappingContext) return;
	auto NewAction = [this](const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* A = NewObject<UInputAction>(this, Name);
		A->ValueType = Type;
		return A;
	};
	IA_Move = NewAction(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	IA_Look = NewAction(TEXT("IA_Look"), EInputActionValueType::Axis2D);
	IA_Vertical = NewAction(TEXT("IA_Vertical"), EInputActionValueType::Axis1D);
	IA_Grab = NewAction(TEXT("IA_Grab"), EInputActionValueType::Boolean);
	IA_Action = NewAction(TEXT("IA_Action"), EInputActionValueType::Boolean);
	IA_Menu = NewAction(TEXT("IA_Menu"), EInputActionValueType::Boolean);

	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Diver"));
	UInputMappingContext* M = MappingContext;

	auto Negate = [this]() { return NewObject<UInputModifierNegate>(this); };
	auto Swizzle = [this]() { UInputModifierSwizzleAxis* S = NewObject<UInputModifierSwizzleAxis>(this); S->Order = EInputAxisSwizzle::YXZ; return S; };
	auto Dead = [this]() { return NewObject<UInputModifierDeadZone>(this); };

	// Deplacement : X = droite, Y = avant. AZERTY (ZQSD) et QWERTY (WASD) en meme temps.
	for (const FKey& K : { EKeys::W, EKeys::Z, EKeys::Up }) M->MapKey(IA_Move, K).Modifiers.Add(Swizzle());
	for (const FKey& K : { EKeys::S, EKeys::Down }) { FEnhancedActionKeyMapping& Map = M->MapKey(IA_Move, K); Map.Modifiers.Add(Swizzle()); Map.Modifiers.Add(Negate()); }
	for (const FKey& K : { EKeys::D, EKeys::Right }) M->MapKey(IA_Move, K);
	for (const FKey& K : { EKeys::A, EKeys::Q, EKeys::Left }) M->MapKey(IA_Move, K).Modifiers.Add(Negate());
	M->MapKey(IA_Move, EKeys::Gamepad_Left2D).Modifiers.Add(Dead());

	// Regard : souris + stick droit
	M->MapKey(IA_Look, EKeys::Mouse2D);
	{
		FEnhancedActionKeyMapping& Map = M->MapKey(IA_Look, EKeys::Gamepad_Right2D);
		Map.Modifiers.Add(Dead());
		UInputModifierScalar* S = NewObject<UInputModifierScalar>(this);
		S->Scalar = FVector(3.f, 2.f, 1.f);
		Map.Modifiers.Add(S);
	}

	// Monter / descendre : Espace / Ctrl gauche ou C ; gachettes manette
	M->MapKey(IA_Vertical, EKeys::SpaceBar);
	M->MapKey(IA_Vertical, EKeys::Gamepad_RightTriggerAxis);
	M->MapKey(IA_Vertical, EKeys::LeftControl).Modifiers.Add(Negate());
	M->MapKey(IA_Vertical, EKeys::C).Modifiers.Add(Negate());
	M->MapKey(IA_Vertical, EKeys::Gamepad_LeftTriggerAxis).Modifiers.Add(Negate());

	// Attraper / action (branches aux etapes 4 et 6)
	M->MapKey(IA_Grab, EKeys::F);
	M->MapKey(IA_Grab, EKeys::LeftMouseButton);
	M->MapKey(IA_Grab, EKeys::Gamepad_FaceButton_Bottom);
	M->MapKey(IA_Action, EKeys::G);
	M->MapKey(IA_Action, EKeys::RightMouseButton);
	M->MapKey(IA_Action, EKeys::Gamepad_FaceButton_Left);
	M->MapKey(IA_Action, EKeys::Gamepad_FaceButton_Right);
	M->MapKey(IA_Menu, EKeys::Escape);
	M->MapKey(IA_Menu, EKeys::Gamepad_Special_Left);
	M->MapKey(IA_Menu, EKeys::Gamepad_Special_Right);
}

void ADSDiver::PawnClientRestart()
{
	Super::PawnClientRestart();
	// Pseudo choisi dans le menu (ou -DSName=) : envoye au serveur une fois connecte.
	if (const UDSSessionSubsystem* SS = GetGameInstance() ? GetGameInstance()->GetSubsystem<UDSSessionSubsystem>() : nullptr)
		if (const APlayerState* PS = GetPlayerState(); !PS || PS->GetPlayerName() != SS->GetPlayerName()) Server_SetDisplayName(SS->GetPlayerName());
	BuildInput();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
			if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				Sub->ClearAllMappings();
				Sub->AddMappingContext(MappingContext, 0);
			}
}

void ADSDiver::SetupPlayerInputComponent(UInputComponent* PIC)
{
	Super::SetupPlayerInputComponent(PIC);
	BuildInput();
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PIC))
	{
		EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ADSDiver::Move);
		EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ADSDiver::Look);
		EIC->BindAction(IA_Vertical, ETriggerEvent::Triggered, this, &ADSDiver::Vertical);
		EIC->BindAction(IA_Grab, ETriggerEvent::Started, this, &ADSDiver::GrabPressed);
		EIC->BindAction(IA_Action, ETriggerEvent::Started, this, &ADSDiver::ActionPressed);
		EIC->BindAction(IA_Menu, ETriggerEvent::Started, this, &ADSDiver::MenuPressed);
	}
}

void ADSDiver::Move(const FInputActionValue& Value)
{
	if (IsStunned()) return;
	const FVector2D V = Value.Get<FVector2D>();
	if (!Controller) return;
	// Nage arcade : l'avant suit la camera (avec une partie de son inclinaison), la droite reste horizontale.
	const FRotator Ctrl = Controller->GetControlRotation();
	const FVector Fwd = FRotationMatrix(FRotator(FRotator::NormalizeAxis(Ctrl.Pitch) * 0.6f, Ctrl.Yaw, 0.f)).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(FRotator(0.f, Ctrl.Yaw, 0.f)).GetUnitAxis(EAxis::Y);
	if (CarryingGiant)
	{
		const FVector Flat = FRotationMatrix(FRotator(0.f, Ctrl.Yaw, 0.f)).GetUnitAxis(EAxis::X);
		PendingIntent = (Flat * V.Y + Right * V.X).GetClampedToMaxSize(1.f);
		bIntentThisFrame = true;
		return;
	}
	AddMovementInput(Fwd, V.Y);
	AddMovementInput(Right, V.X);
}

void ADSDiver::Look(const FInputActionValue& Value)
{
	const FVector2D V = Value.Get<FVector2D>();
	const UDSUserSettings* Set = UDSUserSettings::Get(this);
	const float Sens = Set ? Set->MouseSensitivity : 1.f;
	AddControllerYawInput(V.X * Sens);
	AddControllerPitchInput(-V.Y * Sens * (Set && Set->bInvertY ? -1.f : 1.f));
}

void ADSDiver::Vertical(const FInputActionValue& Value)
{
	if (IsStunned()) return;
	AddMovementInput(FVector::UpVector, Value.Get<float>() * VerticalSpeedScale);
}

void ADSDiver::SetCarrySpeedFactor(float Factor)
{
	CarrySpeedFactor = Factor;
}

void ADSDiver::Stun(float Duration)
{
	if (const AGameStateBase* GS = GetWorld()->GetGameState()) StunUntil = GS->GetServerWorldTimeSeconds() + Duration;
}

bool ADSDiver::IsStunned() const
{
	const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	return GS && GS->GetServerWorldTimeSeconds() < StunUntil;
}

void ADSDiver::Server_NozzleAction_Implementation()
{
	if (ADSVacuumNozzle* N = HeldNozzle.Get()) N->PrimaryAction(this);
}

void ADSDiver::SetCarryingGiant(ADSGiantPickup* G)
{
	CarryingGiant = G;
	OnRep_CarryingGiant();
}

ADSGiantPickup* ADSDiver::GetCarryingGiant() const { return CarryingGiant; }

void ADSDiver::OnRep_CarryingGiant()
{
	// En portant : plus de deplacement propre, on suit l'objet (attache par l'objet).
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(CarryingGiant ? MOVE_None : MOVE_Flying);
	if (!CarryingGiant) CarryIntent = FVector::ZeroVector;
}

void ADSDiver::Server_SetCarryIntent_Implementation(FVector_NetQuantize100 Intent)
{
	CarryIntent = FVector(Intent).GetClampedToMaxSize(1.f);
}

void ADSDiver::Server_SetDisplayName_Implementation(const FString& NewName)
{
	FString N = NewName.Left(16).TrimStartAndEnd();
	if (N.IsEmpty() || N.Contains(FPlatformProcess::ComputerName())) return;
	if (AGameModeBase* GM = GetWorld()->GetAuthGameMode())
		if (APlayerController* PC = Cast<APlayerController>(Controller)) GM->ChangeName(PC, N, false);
}

void ADSDiver::RunKeysTest()
{
	// Passe par la vraie chaine d'entree (Slate -> viewport -> Enhanced Input), comme un joueur.
	static int32 Step = 0;
	static float T0 = 0.f;
	static FVector Start;
	const float T = GetWorld()->GetTimeSeconds();
	ADSPlayerController* PC = Cast<ADSPlayerController>(Controller);
	if (!PC || !FSlateApplication::IsInitialized()) return;
	FSlateApplication& App = FSlateApplication::Get();
	auto Key = [&App](const FKey& K, bool bDown)
	{
		const FKeyEvent E(K, FModifierKeysState(), 0, false, 0, 0);
		if (bDown) App.ProcessKeyDownEvent(E); else App.ProcessKeyUpEvent(E);
	};
	if (Step == 0) { T0 = T; Step = 1; }
	else if (Step == 1 && T > T0 + 2.f) { Start = GetActorLocation(); Key(EKeys::W, true); Step = 2; }
	else if (Step == 2 && T > T0 + 4.f)
	{
		Key(EKeys::W, false);
		UE_LOG(LogDeepSweep, Log, TEXT("TEST touches : deplacement %.0f cm avec Z/W pendant 2 s"), FVector::Dist(Start, GetActorLocation()));
		Step = 3;
	}
	else if (Step == 3 && T > T0 + 4.5f) { Key(EKeys::Escape, true); Step = 31; }
	else if (Step == 31 && T > T0 + 4.7f) { Key(EKeys::Escape, false); Step = 4; }
	else if (Step == 4 && T > T0 + 5.5f) { UE_LOG(LogDeepSweep, Log, TEXT("TEST touches : menu pause ouvert=%d"), PC->IsPauseMenuOpen() ? 1 : 0); PC->ConsoleCommand(TEXT("shot showui")); Step = 5; }
	else if (Step == 5 && T > T0 + 6.5f) { Key(EKeys::Escape, true); Step = 51; }
	else if (Step == 51 && T > T0 + 6.7f) { Key(EKeys::Escape, false); Step = 6; }
	else if (Step == 6 && T > T0 + 7.f)
	{
		UE_LOG(LogDeepSweep, Log, TEXT("TEST touches : menu pause ferme=%d"), PC->IsPauseMenuOpen() ? 0 : 1);
		Start = GetActorLocation(); Key(EKeys::W, true); Step = 7;
	}
	else if (Step == 7 && T > T0 + 8.5f)
	{
		Key(EKeys::W, false);
		UE_LOG(LogDeepSweep, Log, TEXT("TEST touches : apres pause, deplacement %.0f cm"), FVector::Dist(Start, GetActorLocation()));
		PC->ConsoleCommand(TEXT("shot showui"));
		Step = 8;
	}
	else if (Step == 8 && T > T0 + 9.5f) { PC->ConsoleCommand(TEXT("quit")); Step = 9; }
}

void ADSDiver::MenuPressed()
{
	// Echap / Start : menu pause (reprendre, options, quitter la partie, quitter le jeu)
	if (ADSPlayerController* PC = Cast<ADSPlayerController>(Controller)) PC->TogglePauseMenu();
}

void ADSDiver::Server_RequestRestart_Implementation()
{
	const ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>();
	if (GS && GS->IsMatchOver())
		GetWorld()->ServerTravel(TEXT("?restart"), false); // nouvelle plongee pour tout le monde (clients inclus)
}

void ADSDiver::RunOctoScenario()
{
	static int32 Step = 0;
	static float StepT = 0.f;
	const float T = GetWorld()->GetTimeSeconds();
	APlayerController* PC = Cast<APlayerController>(Controller);
	ADSOctopus* Octo = nullptr;
	for (TActorIterator<ADSOctopus> It(GetWorld()); It; ++It) Octo = *It;
	ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>();
	if (!PC || !Octo || !GS) return;
	if (Step == 0 && T > 2.f) { Octo->TriggerNow(); Step = 1; }
	else if (Step == 1 && Octo->Loot.Num() >= 1)
	{
		// Se place pour voir le poulpe avec son butin
		const FVector O = Octo->GetActorLocation();
		SetActorLocation(O + FVector(-700.f, 0.f, 250.f));
		PC->SetControlRotation(FRotator(-12.f, 0.f, 0.f));
		StepT = T; Step = 2;
	}
	else if (Step == 2)
	{
		const FVector O = Octo->GetActorLocation();
		SetActorLocation(O + FVector(-700.f, 0.f, 250.f));
		if (T > StepT + 0.4f) { PC->ConsoleCommand(TEXT("shot showui")); Step = 3; }
	}
	else if (Step == 3)
	{
		SetActorLocation(Octo->GetActorLocation() + FVector(-150.f, 0.f, 0.f));
		UE_LOG(LogDeepSweep, Log, TEXT("TEST poulpe butin=%d etat=%d"), Octo->Loot.Num(), int32(Octo->State));
		GrabPressed();
		StepT = T; Step = 4;
	}
	else if (Step == 4 && T > StepT + 1.f)
	{
		UE_LOG(LogDeepSweep, Log, TEXT("TEST apres capture : etat=%d credits=%d"), int32(Octo->State), GS->GetCredits());
		GS->EndMatch();
		StepT = T; Step = 5;
	}
	else if (Step == 5 && T > StepT + 1.5f) { PC->ConsoleCommand(TEXT("shot showui")); StepT = T; Step = 6; }
	else if (Step == 6 && T > StepT + 1.f) { PC->ConsoleCommand(TEXT("quit")); Step = 7; }
	if (T > 60.f && Step < 7) { UE_LOG(LogDeepSweep, Log, TEXT("TEST timeout step=%d"), Step); PC->ConsoleCommand(TEXT("quit")); Step = 7; }
}

void ADSDiver::RunShopScenario()
{
	static int32 Step = 0;
	static int32 PadIndex = 0;
	const float T = GetWorld()->GetTimeSeconds();
	APlayerController* PC = Cast<APlayerController>(Controller);
	ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>();
	if (!PC || !GS) return;
	TArray<ADSUpgradePad*> Pads;
	for (TActorIterator<ADSUpgradePad> It(GetWorld()); It; ++It) Pads.Add(*It);
	Pads.Sort([](const ADSUpgradePad& A, const ADSUpgradePad& B) { return A.Cost < B.Cost; });
	if (Pads.Num() == 0) return;
	if (Step == 0 && T > 2.f)
	{
		// Vue d'ensemble des pads, avec juste assez pour 3 achats
		GS->AddCredits(140);
		SetActorLocation(Pads[2]->GetActorLocation() + FVector(900.f, 0.f, 350.f));
		PC->SetControlRotation(FRotator(-22.f, 180.f, 0.f));
		Step = 1;
	}
	else if (Step == 1 && T > 3.5f) { PC->ConsoleCommand(TEXT("shot showui")); Step = 2; }
	else if (Step == 2 && T > 4.f)
	{
		GS->AddCredits(1000);
		Step = 3;
	}
	else if (Step == 3 && T > 4.2f + PadIndex * 0.6f)
	{
		if (PadIndex < Pads.Num())
		{
			SetActorLocation(Pads[PadIndex]->GetActorLocation() + FVector(0, 0, 120.f));
			Step = 4;
		}
		else Step = 5;
	}
	else if (Step == 4 && T > 4.5f + PadIndex * 0.6f) { GrabPressed(); ++PadIndex; Step = 3; }
	else if (Step == 5)
	{
		int32 MaxP = -1; float BeltSpeed = -1.f; bool bBalloons = false;
		for (TActorIterator<ADSVacuum> It(GetWorld()); It; ++It) MaxP = It->MaxPower;
		for (TActorIterator<ADSConveyor> It(GetWorld()); It; ++It) BeltSpeed = It->Speed;
		for (TActorIterator<ADSGiantPickup> It(GetWorld()); It; ++It) bBalloons = It->GetCarry()->bBalloons;
		UE_LOG(LogDeepSweep, Log, TEXT("TEST achats : credits=%d capacite=%d portee=%.0f puissanceMax=%d tapis=%.0f ballons=%d"),
			GS->GetCredits(), Grab->Capacity, Grab->Range, MaxP, BeltSpeed, bBalloons ? 1 : 0);
		SetActorLocation(Pads[2]->GetActorLocation() + FVector(900.f, 0.f, 350.f));
		PC->SetControlRotation(FRotator(-22.f, 180.f, 0.f));
		Step = 6;
	}
	else if (Step == 6 && T > 9.f) { PC->ConsoleCommand(TEXT("shot showui")); Step = 7; }
	else if (Step == 7 && T > 10.f) { PC->ConsoleCommand(TEXT("quit")); Step = 8; }
}

void ADSDiver::RunGiantScenario()
{
	static int32 Step = 0;
	const float T = GetWorld()->GetTimeSeconds();
	APlayerController* PC = Cast<APlayerController>(Controller);
	ADSGiantPickup* Chest = nullptr;
	for (TActorIterator<ADSGiantPickup> It(GetWorld()); It; ++It) if (It->GetCarry()->DesignCarriers == 2) Chest = *It;
	ADSCounter* Counter = nullptr;
	for (TActorIterator<ADSCounter> It(GetWorld()); It; ++It) Counter = *It;
	if (!PC || !Counter) return;
	if (Step == 0 && T > 2.f && Chest)
	{
		SetActorLocation(Chest->GetActorLocation() + FVector(-250.f, 0.f, 60.f));
		Step = 1;
	}
	else if (Step == 1 && T > 2.5f && Chest)
	{
		GrabPressed();
		UE_LOG(LogDeepSweep, Log, TEXT("TEST porte geant=%s requis=%d facteur=%.2f"), *GetNameSafe(CarryingGiant), Chest->GetCarry()->GetRequiredCarriers(), Chest->GetCarry()->GetSpeedFactor());
		Step = 2;
	}
	else if (Step >= 2 && Step < 5)
	{
		// Pousse vers le comptoir ; la camera regarde dans la meme direction
		if (CarryingGiant)
		{
			const FVector To = (Counter->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
			PC->SetControlRotation(FRotator(-20.f, To.Rotation().Yaw, 0.f));
			PendingIntent = To;
			bIntentThisFrame = true;
		}
		if (Step == 2 && T > 7.f) { PC->ConsoleCommand(TEXT("shot showui")); Step = 3; }
		if (Step == 3 && !Chest) { Step = 4; }
		if (Step == 4)
		{
			const ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>();
			UE_LOG(LogDeepSweep, Log, TEXT("TEST coffre livre a t=%.1f, credits=%d proprete=%.3f"), T, GS ? GS->GetCredits() : -1, GS ? GS->GetCleanliness() : -1.f);
			PC->ConsoleCommand(TEXT("shot showui"));
			Step = 5;
		}
	}
	else if (Step == 5 && T > 0.f) { Step = 6; }
	else if (Step == 6) { PC->ConsoleCommand(TEXT("quit")); Step = 7; }
	if (T > 90.f && Step < 7) { UE_LOG(LogDeepSweep, Log, TEXT("TEST timeout step=%d"), Step); PC->ConsoleCommand(TEXT("quit")); Step = 7; }
}

void ADSDiver::RunBeltScenario()
{
	static int32 Step = 0;
	const float T = GetWorld()->GetTimeSeconds();
	APlayerController* PC = Cast<APlayerController>(Controller);
	ADSConveyor* Belt = nullptr;
	for (TActorIterator<ADSConveyor> It(GetWorld()); It; ++It) Belt = *It;
	ADSVacuum* Vac = nullptr;
	for (TActorIterator<ADSVacuum> It(GetWorld()); It; ++It) Vac = *It;
	const UDSPickupSubsystem* Sub = GetWorld()->GetSubsystem<UDSPickupSubsystem>();
	ADSPickupManager* M = Sub ? Sub->GetManager() : nullptr;
	if (!PC || !Belt || !Vac || !M) return;
	const float L = Belt->GetLength();
	auto Watch = [&](float D)
	{
		const FVector P = Belt->GetActorLocation(); // vue de cote du tapis
		SetActorLocation(FVector(-2350.f, -300.f + D, 350.f));
		PC->SetControlRotation(FRotator(-18.f, 200.f, 0.f));
	};
	if (Step == 0 && T > 2.f)
	{
		// Remplit la cuve (2 canettes, 1 bouteille, 1 caddie, 3 ferrailles) puis tire le levier.
		for (const TCHAR* N : { TEXT("Canette"), TEXT("Canette"), TEXT("Bouteille"), TEXT("Caddie"), TEXT("Ferraille"), TEXT("Ferraille"), TEXT("Vase") })
			for (UDSPickupData* Ty : M->Types) if (Ty && Ty->TypeId == FName(N)) { Vac->StoreItem(Ty); break; }
		Vac->DumpTank(this);
		Watch(0.f);
		Step = 1;
	}
	else if (Step == 1 && Belt->bJammed) { Step = 2; }
	else if (Step == 2) { PC->ConsoleCommand(TEXT("shot showui")); UE_LOG(LogDeepSweep, Log, TEXT("TEST tapis bloque, %d objets"), Belt->NumItems()); Step = 3; }
	else if (Step == 3)
	{
		// Va au point de blocage et debloque
		SetActorLocation(FVector(-2850.f, 0.f, 200.f));
		for (TActorIterator<ADSConveyor> It(GetWorld()); It; ++It) if (It->IsInRange(this)) { GrabPressed(); Step = 4; }
		if (Step == 3) { UE_LOG(LogDeepSweep, Log, TEXT("TEST hors de portee du blocage")); Step = 4; }
	}
	else if (Step == 4 && !Belt->bJammed && T > 0.f) { Watch(500.f); Step = 5; }
	else if (Step == 5 && Belt->NumItems() <= 3) { PC->ConsoleCommand(TEXT("shot showui")); Step = 6; }
	else if (Step == 6 && Belt->NumItems() == 0)
	{
		const ADSGameState* GS = GetWorld()->GetGameState<ADSGameState>();
		UE_LOG(LogDeepSweep, Log, TEXT("TEST tapis vide, credits=%d proprete=%.3f"), GS ? GS->GetCredits() : -1, GS ? GS->GetCleanliness() : -1.f);
		PC->ConsoleCommand(TEXT("quit"));
		Step = 7;
	}
	if (T > 40.f && Step < 7) { UE_LOG(LogDeepSweep, Log, TEXT("TEST timeout step=%d items=%d jam=%d"), Step, Belt->NumItems(), Belt->bJammed); PC->ConsoleCommand(TEXT("quit")); Step = 7; }
}

void ADSDiver::RunVacScenario()
{
	static int32 Step = 0;
	const float T = GetWorld()->GetTimeSeconds();
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC) return;
	ADSVacuumNozzle* Noz = nullptr;
	for (TActorIterator<ADSVacuumNozzle> It(GetWorld()); It; ++It) Noz = *It;
	if (!Noz) return;
	if (Step == 0 && T > 2.f)
	{
		SetActorLocation(Noz->GetActorLocation() + FVector(-120.f, 0.f, 80.f));
		Step = 1;
	}
	else if (Step == 1 && T > 2.5f) { GrabPressed(); Step = 2; }
	else if (Step == 2 && T > 3.f)
	{
		// Devant le tas pres de la base, face au tas, aspiration
		SetActorLocation(FVector(-1350.f, 450.f, 160.f));
		PC->SetControlRotation(FRotator(-8.f, 0.f, 0.f));
		ActionPressed();
		Step = 3;
	}
	else if (Step == 3) { AddMovementInput(FVector(1, 0, 0), 0.6f); if (T > 6.f) { PC->ConsoleCommand(TEXT("shot showui")); Step = 4; } }
	else if (Step == 4) { AddMovementInput(FVector(1, 0.2f * FMath::Sin(T), 0), 0.6f); if (T > 11.f) { PC->ConsoleCommand(TEXT("shot showui")); Step = 5; } }
	else if (Step == 5 && T > 12.f)
	{
		for (TActorIterator<ADSVacuum> It(GetWorld()); It; ++It) UE_LOG(LogDeepSweep, Log, TEXT("TEST cuve=%d bouchon=%d"), It->TankCount, It->Clog);
		PC->ConsoleCommand(TEXT("quit"));
		Step = 6;
	}
}

void ADSDiver::GrabPressed()
{
	if (const ADSGameState* EndGS = GetWorld()->GetGameState<ADSGameState>(); EndGS && EndGS->IsMatchOver()) { Server_RequestRestart(); return; }
	if (IsStunned()) return;
	// 1) element de la base a portee (comptoir, levier...), 2) objet vise, 3) lacher.
	if (ADSInteractable* I = ADSInteractable::FindBest(this)) { Server_Interact(I); return; }
	ADSPickup* Target = HoverTarget.Get();
	if (Target && !Grab->IsFull())
	{
		Grab->RequestGrab(Target);
		if (UDSAudioSubsystem* A = GetWorld()->GetSubsystem<UDSAudioSubsystem>()) A->PlaySfx(EDSSfx::Grab, FVector::ZeroVector); // retour immediat
		ADSFXBurst::Play(this, EDSFX::Bubbles, Target->GetActorLocation(), 0.5f);
	}
	else if (Grab->NumHeld() > 0) Grab->RequestDrop();
}

void ADSDiver::ActionPressed()
{
	if (IsStunned()) return;
	// Buse en main : allumer/eteindre ou taper sur le bouchon. Sinon lancer.
	if (HeldNozzle.IsValid()) { Server_NozzleAction(); return; }
	if (Grab->NumHeld() > 0)
	{
		const FVector Aim = Controller ? Controller->GetControlRotation().Vector() : GetActorForwardVector();
		Grab->RequestThrow(Aim);
		if (UDSAudioSubsystem* A = GetWorld()->GetSubsystem<UDSAudioSubsystem>()) A->PlaySfx(EDSSfx::Throw, FVector::ZeroVector);
	}
}

void ADSDiver::Server_Interact_Implementation(ADSInteractable* Target)
{
	// Revalide cote serveur (portee + conditions).
	if (IsValid(Target) && Target->IsInRange(this) && Target->CanInteract(this)) Target->Interact(this);
}

void ADSDiver::RunSellScenario()
{
	static int32 Step = 0;
	const float T = GetWorld()->GetTimeSeconds();
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC) return;
	if (Step == 0 && T > 3.f) { GrabPressed(); Step = 1; }
	else if (Step == 1 && T > 3.5f)
	{
		// Teleporte devant le comptoir (debug) et le regarde
		for (TActorIterator<ADSCounter> It(GetWorld()); It; ++It)
		{
			const FVector C = It->GetActorLocation();
			SetActorLocation(C + FVector(-350.f, -250.f, 150.f));
			PC->SetControlRotation((C - GetActorLocation()).Rotation() + FRotator(-10.f, 0.f, 0.f));
		}
		Step = 2;
	}
	else if (Step == 2 && T > 5.f) { UE_LOG(LogDeepSweep, Log, TEXT("TEST vente, porte=%d"), Grab->NumHeld()); GrabPressed(); Step = 3; }
	else if (Step == 3 && T > 5.25f) { PC->ConsoleCommand(TEXT("shot showui")); Step = 4; }
	else if (Step == 4 && T > 6.5f) { UE_LOG(LogDeepSweep, Log, TEXT("TEST fin, porte=%d"), Grab->NumHeld()); PC->ConsoleCommand(TEXT("quit")); Step = 5; }
}

void ADSDiver::RunDebugScenario()
{
	static int32 Step = 0;
	const float T = GetWorld()->GetTimeSeconds();
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC) return;
	if (Step == 0 && T > 3.f) { GrabPressed(); Step = 1; UE_LOG(LogDeepSweep, Log, TEXT("TEST grab, cible=%s"), *GetNameSafe(HoverTarget.Get())); }
	else if (Step == 1 && T > 4.5f) { PC->ConsoleCommand(TEXT("shot showui")); Step = 2; UE_LOG(LogDeepSweep, Log, TEXT("TEST porte=%d"), Grab->NumHeld()); }
	else if (Step == 2 && T > 5.f) { ActionPressed(); Step = 3; }
	else if (Step == 3 && T > 5.35f) { PC->ConsoleCommand(TEXT("shot showui")); Step = 4; }
	else if (Step == 4 && T > 7.f) { UE_LOG(LogDeepSweep, Log, TEXT("TEST fin, porte=%d"), Grab->NumHeld()); PC->ConsoleCommand(TEXT("quit")); Step = 5; }
}

void ADSDiver::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocallyControlled() && Controller && CVarAutoPilot.GetValueOnGameThread() > 0)
	{
		const float T = GetWorld()->GetTimeSeconds();
		// Nage vers le centre de la crique en serpentant
		AddMovementInput(FVector(1.f, 0.6f * FMath::Sin(T * 0.7f), 0.25f * FMath::Sin(T * 0.5f)), 1.f);
		Controller->SetControlRotation(FRotator(-12.f, 20.f * FMath::Sin(T * 0.7f), 0.f));
	}

	if (IsLocallyControlled() && CVarShotAt.GetValueOnGameThread() > 0.f)
	{
		static int32 ShotState = 0;
		const float T = GetWorld()->GetTimeSeconds();
		APlayerController* PC = Cast<APlayerController>(Controller);
		if (PC && ShotState == 0 && T > CVarShotAt.GetValueOnGameThread()) { PC->ConsoleCommand(TEXT("shot showui")); ShotState = 1; }
		else if (PC && ShotState == 1 && T > CVarShotAt.GetValueOnGameThread() + 2.f) { ShotState = 2; PC->ConsoleCommand(TEXT("quit")); }
	}

	if (IsLocallyControlled()) UpdateHover();
	if (IsLocallyControlled() && CVarTestGrab.GetValueOnGameThread() > 0) RunDebugScenario();
	if (IsLocallyControlled() && CVarTestSell.GetValueOnGameThread() > 0) RunSellScenario();
	if (IsLocallyControlled() && CVarTestVac.GetValueOnGameThread() > 0) RunVacScenario();
	if (IsLocallyControlled() && CVarTestBelt.GetValueOnGameThread() > 0) RunBeltScenario();
	if (IsLocallyControlled() && CVarTestGiant.GetValueOnGameThread() > 0) RunGiantScenario();
	if (IsLocallyControlled() && CVarTestShop.GetValueOnGameThread() > 0) RunShopScenario();
	if (IsLocallyControlled() && CVarTestOcto.GetValueOnGameThread() > 0) RunOctoScenario();
	if (IsLocallyControlled() && CVarTestKeys.GetValueOnGameThread() > 0) RunKeysTest();
	if (IsLocallyControlled() && HasAuthority() && CVarTestNet.GetValueOnGameThread() > 0) RunNetHostScenario(CVarTestNet.GetValueOnGameThread());
	if (IsLocallyControlled() && !HasAuthority() && CVarTestNetClient.GetValueOnGameThread() > 0) RunNetClientScenario();

	// Intention de portage : envoyee au serveur ~10 fois/s (ou des qu'elle change).
	if (IsLocallyControlled())
	{
		const FVector Now = (CarryingGiant && bIntentThisFrame) ? PendingIntent : FVector::ZeroVector;
		bIntentThisFrame = false;
		CarryIntent = Now;
		IntentSendTimer -= DeltaSeconds;
		if (CarryingGiant && (IntentSendTimer <= 0.f || !Now.Equals(LastSentIntent, 0.05f)))
		{
			IntentSendTimer = 0.1f;
			LastSentIntent = Now;
			if (!HasAuthority()) Server_SetCarryIntent(Now);
		}
	}

	// Vitesse : calculee pareil sur serveur et client (prediction) a partir d'etats repliques.
	{
		float F = CarrySpeedFactor;
		if (HeldNozzle.IsValid()) F *= 0.72f;
		if (IsStunned()) F = 0.f;
		GetCharacterMovement()->MaxFlySpeed = SwimSpeed * F;
	}

	// Secousse d'ecran (locale) qui s'amortit
	if (IsLocallyControlled())
	{
		Shake = FMath::Max(0.f, Shake - DeltaSeconds * 2.5f);
		const float T = GetWorld()->GetTimeSeconds();
		Camera->SetRelativeLocation(Shake > 0.f ? FVector(0.f, FMath::Sin(T * 61.f), FMath::Sin(T * 47.f + 1.f)) * Shake * Shake * 30.f : FVector::ZeroVector);
	}

	// Bulles du casque (respiration) et sillage quand on nage vite (cosmetique, chez tout le monde)
	BubbleTimer -= DeltaSeconds;
	if (BubbleTimer <= 0.f && GetNetMode() != NM_DedicatedServer)
	{
		const float Speed = GetVelocity().Size();
		BubbleTimer = Speed > 500.f ? 0.25f : FMath::FRandRange(1.4f, 2.4f);
		ADSFXBurst::Play(this, EDSFX::Bubbles, GetActorLocation() + GetActorForwardVector() * 30.f + FVector(0, 0, 70.f), Speed > 500.f ? 0.35f : 0.25f);
	}

	// Cosmetique : inclinaison vers la direction de nage + leger flottement
	BobTime += DeltaSeconds;
	const FVector Vel = GetVelocity();
	const FVector LocalVel = GetActorRotation().UnrotateVector(Vel);
	const float Speed01 = FMath::Clamp(Vel.Size() / SwimSpeed, 0.f, 1.f);
	const float TargetPitch = FMath::Clamp(Vel.Z / SwimSpeed, -1.f, 1.f) * 35.f - Speed01 * 25.f;
	const float TargetRoll = FMath::Clamp(LocalVel.Y / SwimSpeed, -1.f, 1.f) * 25.f;
	VisualTilt = FMath::RInterpTo(VisualTilt, FRotator(TargetPitch, 0.f, TargetRoll), DeltaSeconds, 5.f);
	VisualRoot->SetRelativeRotation(IsStunned() ? FRotator(VisualTilt.Pitch, 0.f, GetWorld()->GetTimeSeconds() * 720.f) : VisualTilt);
	VisualRoot->SetRelativeLocation(FVector(0, 0, FMath::Sin(BobTime * 2.f + ColorIndex) * 6.f));
}

void ADSDiver::UpdateHover()
{
	ADSPickup* Best = nullptr;
	if (const UDSPickupSubsystem* Sub = GetWorld()->GetSubsystem<UDSPickupSubsystem>())
	{
		const FVector Fwd = Controller ? Controller->GetControlRotation().Vector() : GetActorForwardVector();
		Best = Grab->IsFull() ? nullptr : Sub->FindBestPickup(GetActorLocation(), (Fwd + GetActorForwardVector()).GetSafeNormal(), Grab->Range);
	}
	if (Best != HoverTarget.Get())
	{
		if (ADSPickup* Old = HoverTarget.Get()) Old->SetHighlighted(false);
		HoverTarget = Best;
		if (Best) Best->SetHighlighted(true);
	}
}

void ADSDiver::SetHeldNozzle(ADSVacuumNozzle* N) { HeldNozzle = N; }
ADSVacuumNozzle* ADSDiver::GetHeldNozzle() const { return HeldNozzle.Get(); }
