// Copyright (c) 2026, Team SDB. All rights reserved.

#include "IVSmokeHoleRequestComponent.h"

#include "IVSmoke.h"
#include "IVSmokeHoleGeneratorComponent.h"
#include "IVSmokeHolePreset.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UIVSmokeHoleRequestComponent::UIVSmokeHoleRequestComponent()
{
	SetIsReplicatedByDefault(true);
}

//~============================================================================
// Public API
#pragma region API

void UIVSmokeHoleRequestComponent::RequestPenetrationHole(AActor* Caller, AActor* IVSmokeVoxelVolume, const FVector3f& BulletOrigin, const FVector3f& BulletDirection, UIVSmokeHolePreset* BulletPreset)
{
	if (!Caller)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestPenetrationHole] Caller is null"));
		return;
	}

	if (!IVSmokeVoxelVolume)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestPenetrationHole] IVSmokeVoxelVolume is null"));
		return;
	}

	if (!BulletPreset)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestPenetrationHole] BulletPreset is null"));
		return;
	}

	UIVSmokeHoleGeneratorComponent* Generator = IVSmokeVoxelVolume->FindComponentByClass<UIVSmokeHoleGeneratorComponent>();
	if (!Generator)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestPenetrationHole] Generator not found on IVSmokeVoxelVolume"));
		return;
	}

	// Server/AI → Direct call
	if (Generator->GetOwner() && Generator->GetOwner()->HasAuthority())
	{
		Generator->CreatePenetrationHole(BulletOrigin, BulletDirection, BulletPreset->GetPresetID());
		return;
	}

	// Client → RPC via Caller's Instigator chain
	APawn* InstigatorPawn = Caller->GetInstigator();
	if (!InstigatorPawn)
	{
		InstigatorPawn = Cast<APawn>(Caller);
	}

	if (!InstigatorPawn)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestPenetrationHole] InstigatorPawn not found"));
		return;
	}

	APlayerController* PC = Cast<APlayerController>(InstigatorPawn->GetController());
	if (!PC)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestPenetrationHole] PlayerController not found"));
		return;
	}

	UIVSmokeHoleRequestComponent* Requester = PC->FindComponentByClass<UIVSmokeHoleRequestComponent>();
	if (!Requester)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestPenetrationHole] RequestComponent not found on PlayerController"));
		return;
	}

	Requester->Internal_RequestPenetrationHole(Generator, BulletOrigin, BulletDirection, BulletPreset);
}

void UIVSmokeHoleRequestComponent::RequestExplosionHole(AActor* Caller, AActor* IVSmokeVoxelVolume, const FVector3f& ExplosionOrigin, UIVSmokeHolePreset* ExplosionPreset)
{
	if (!Caller)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestExplosionHole] Caller is null"));
		return;
	}

	if (!IVSmokeVoxelVolume)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestExplosionHole] IVSmokeVoxelVolume is null"));
		return;
	}

	if (!ExplosionPreset)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestExplosionHole] ExplosionPreset is null"));
		return;
	}

	UIVSmokeHoleGeneratorComponent* Generator = IVSmokeVoxelVolume->FindComponentByClass<UIVSmokeHoleGeneratorComponent>();
	if (!Generator)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestExplosionHole] Generator not found on IVSmokeVoxelVolume"));
		return;
	}

	// Server/AI → Direct call
	if (Generator->GetOwner() && Generator->GetOwner()->HasAuthority())
	{
		Generator->CreateExplosionHole(ExplosionOrigin, ExplosionPreset->GetPresetID());
		return;
	}

	// Client → RPC via Caller's Instigator chain
	APawn* InstigatorPawn = Caller->GetInstigator();
	if (!InstigatorPawn)
	{
		InstigatorPawn = Cast<APawn>(Caller);
	}

	if (!InstigatorPawn)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestExplosionHole] InstigatorPawn not found"));
		return;
	}

	APlayerController* PC = Cast<APlayerController>(InstigatorPawn->GetController());
	if (!PC)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestExplosionHole] PlayerController not found"));
		return;
	}

	UIVSmokeHoleRequestComponent* Requester = PC->FindComponentByClass<UIVSmokeHoleRequestComponent>();
	if (!Requester)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestExplosionHole] RequestComponent not found on PlayerController"));
		return;
	}

	Requester->Internal_RequestExplosionHole(Generator, ExplosionOrigin, ExplosionPreset);
}

void UIVSmokeHoleRequestComponent::RequestDynamicHole(
	AActor* Caller,
	AActor* IVSmokeVoxelVolume,
	UIVSmokeHolePreset* DynamicPreset
)
{
	if (!Caller)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestDynamicHole] Caller is null"));
		return;
	}

	if (!IVSmokeVoxelVolume)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestDynamicHole] IVSmokeVoxelVolume is null"));
		return;
	}

	if (!DynamicPreset)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestDynamicHole] DynamicPreset is null"));
		return;
	}

	UIVSmokeHoleGeneratorComponent* Generator = IVSmokeVoxelVolume->FindComponentByClass<UIVSmokeHoleGeneratorComponent>();
	if (!Generator)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestDynamicHole] Generator not found on IVSmokeVoxelVolume"));
		return;
	}

	// Server/AI → Direct call (Caller = TargetActor)
	if (Generator->GetOwner() && Generator->GetOwner()->HasAuthority())
	{
		Generator->RegisterTrackDynamicHole(Caller, DynamicPreset->GetPresetID());
		return;
	}

	// Client → RPC via Caller's Instigator chain (Caller = TargetActor = Pawn)
	APawn* InstigatorPawn = Cast<APawn>(Caller);
	if (!InstigatorPawn)
	{
		InstigatorPawn = Caller->GetInstigator();
	}

	if (!InstigatorPawn)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestDynamicHole] InstigatorPawn not found"));
		return;
	}

	APlayerController* PC = Cast<APlayerController>(InstigatorPawn->GetController());
	if (!PC)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestDynamicHole] PlayerController not found"));
		return;
	}

	UIVSmokeHoleRequestComponent* Requester = PC->FindComponentByClass<UIVSmokeHoleRequestComponent>();
	if (!Requester)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestDynamicHole] RequestComponent not found on PlayerController"));
		return;
	}

	Requester->Internal_RequestDynamicHole(Generator, Caller, DynamicPreset);
}
#pragma endregion

//~============================================================================
// Server RPC
#pragma region RPC
void UIVSmokeHoleRequestComponent::Internal_RequestPenetrationHole_Implementation(UIVSmokeHoleGeneratorComponent* IVSmokeHoleGeneratorComponent, const FVector3f& Origin, const FVector3f& Direction, UIVSmokeHolePreset* Preset)
{
	if (!IVSmokeHoleGeneratorComponent)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestPenetrationHole] IVSmokeHoleGeneratorComponent is null"));
		return;
	}

	if (!Preset)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestPenetrationHole] Preset is null"));
		return;
	}

	if (Preset->HoleType != EIVSmokeHoleType::Penetration)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestPenetrationHole] Preset type mismatch"));
		return;
	}

	IVSmokeHoleGeneratorComponent->CreatePenetrationHole(Origin, Direction, Preset->GetPresetID());
}

void UIVSmokeHoleRequestComponent::Internal_RequestExplosionHole_Implementation(UIVSmokeHoleGeneratorComponent* IVSmokeHoleGeneratorComponent, const FVector3f& Origin, UIVSmokeHolePreset* Preset)
{
	if (!IVSmokeHoleGeneratorComponent)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestExplosionHole] IVSmokeHoleGeneratorComponent is null"));
		return;
	}

	if (!Preset)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestExplosionHole] Preset is null"));
		return;
	}

	if (Preset->HoleType != EIVSmokeHoleType::Explosion)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestExplosionHole] Preset type mismatch"));
		return;
	}

	IVSmokeHoleGeneratorComponent->CreateExplosionHole(Origin, Preset->GetPresetID());
}

void UIVSmokeHoleRequestComponent::Internal_RequestDynamicHole_Implementation(UIVSmokeHoleGeneratorComponent* IVSmokeHoleGeneratorComponent, AActor* TargetActor, UIVSmokeHolePreset* Preset)
{
	if (!IVSmokeHoleGeneratorComponent)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestDynamicHole] IVSmokeHoleGeneratorComponent is null"));
		return;
	}

	if (!Preset)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestDynamicHole] Preset is null"));
		return;
	}

	if (Preset->HoleType != EIVSmokeHoleType::Dynamic)
	{
		UE_LOG(LogIVSmoke, Warning, TEXT("[UIVSmokeHoleRequestComponent::RequestDynamicHole] Preset type mismatch"));
		return;
	}

	IVSmokeHoleGeneratorComponent->RegisterTrackDynamicHole(TargetActor, Preset->GetPresetID());
}
#pragma endregion
