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
#include "Pickups/DSPickupSubsystem.h"

static TAutoConsoleVariable<int32> CVarAutoPilot(TEXT("ds.AutoPilot"), 0,
	TEXT("Debug : 1 = le plongeur local nage seul en spirale (captures automatiques)."));
static TAutoConsoleVariable<int32> CVarTestGrab(TEXT("ds.TestGrab"), 0,
	TEXT("Debug : scenario auto (attrape la cible a 3 s, capture, lance a 5 s, capture, quitte)."));
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
}

void ADSDiver::PawnClientRestart()
{
	Super::PawnClientRestart();
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
	}
}

void ADSDiver::Move(const FInputActionValue& Value)
{
	const FVector2D V = Value.Get<FVector2D>();
	if (!Controller) return;
	// Nage arcade : l'avant suit la camera (avec une partie de son inclinaison), la droite reste horizontale.
	const FRotator Ctrl = Controller->GetControlRotation();
	const FVector Fwd = FRotationMatrix(FRotator(FRotator::NormalizeAxis(Ctrl.Pitch) * 0.6f, Ctrl.Yaw, 0.f)).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(FRotator(0.f, Ctrl.Yaw, 0.f)).GetUnitAxis(EAxis::Y);
	AddMovementInput(Fwd, V.Y);
	AddMovementInput(Right, V.X);
}

void ADSDiver::Look(const FInputActionValue& Value)
{
	const FVector2D V = Value.Get<FVector2D>();
	AddControllerYawInput(V.X);
	AddControllerPitchInput(-V.Y);
}

void ADSDiver::Vertical(const FInputActionValue& Value)
{
	AddMovementInput(FVector::UpVector, Value.Get<float>() * VerticalSpeedScale);
}

void ADSDiver::SetCarrySpeedFactor(float Factor)
{
	CarrySpeedFactor = Factor;
	GetCharacterMovement()->MaxFlySpeed = SwimSpeed * CarrySpeedFactor;
}

void ADSDiver::GrabPressed()
{
	// Attraper l'objet vise ; sinon (ou sac plein) lacher ce qu'on porte.
	ADSPickup* Target = HoverTarget.Get();
	if (Target && !Grab->IsFull()) Grab->RequestGrab(Target);
	else if (Grab->NumHeld() > 0) Grab->RequestDrop();
}

void ADSDiver::ActionPressed()
{
	// Lancer dans la direction de la camera.
	if (Grab->NumHeld() > 0)
	{
		const FVector Aim = Controller ? Controller->GetControlRotation().Vector() : GetActorForwardVector();
		Grab->RequestThrow(Aim);
	}
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

	// Cosmetique : inclinaison vers la direction de nage + leger flottement
	BobTime += DeltaSeconds;
	const FVector Vel = GetVelocity();
	const FVector LocalVel = GetActorRotation().UnrotateVector(Vel);
	const float Speed01 = FMath::Clamp(Vel.Size() / SwimSpeed, 0.f, 1.f);
	const float TargetPitch = FMath::Clamp(Vel.Z / SwimSpeed, -1.f, 1.f) * 35.f - Speed01 * 25.f;
	const float TargetRoll = FMath::Clamp(LocalVel.Y / SwimSpeed, -1.f, 1.f) * 25.f;
	VisualTilt = FMath::RInterpTo(VisualTilt, FRotator(TargetPitch, 0.f, TargetRoll), DeltaSeconds, 5.f);
	VisualRoot->SetRelativeRotation(VisualTilt);
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
