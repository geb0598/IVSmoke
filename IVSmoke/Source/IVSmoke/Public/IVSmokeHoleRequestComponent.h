// Copyright (c) 2026, Team SDB. All rights reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "IVSmokeHoleRequestComponent.generated.h"

class UIVSmokeHoleGeneratorComponent;
class UIVSmokeHolePreset;

/**
 * @brief Handles network routing for hole requests.
 *        This component enables clients to request holes on VoxelVolumes
 *        by routing Server RPCs through the PlayerController's connection.
 */
UCLASS(ClassGroup = (IVSmoke), meta = (BlueprintSpawnableComponent))
class IVSMOKE_API UIVSmokeHoleRequestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIVSmokeHoleRequestComponent();

	//~============================================================================
	// Public API
#pragma region API
	UFUNCTION(BlueprintCallable, Category = "IVSmoke | Hole | API", meta = (DefaultToSelf = "Caller", HidePin = "Caller"))
	static void RequestPenetrationHole(AActor* Caller, AActor* IVSmokeVoxelVolume, const FVector3f& BulletOrigin, const FVector3f& BulletDirection, UIVSmokeHolePreset* BulletPreset);

	UFUNCTION(BlueprintCallable, Category = "IVSmoke | Hole | API", meta = (DefaultToSelf = "Caller", HidePin = "Caller"))
	static void RequestExplosionHole(AActor* Caller, AActor* IVSmokeVoxelVolume, const FVector3f& ExplosionOrigin, UIVSmokeHolePreset* ExplosionPreset);

	UFUNCTION(BlueprintCallable, Category = "IVSmoke | Hole | API", meta = (DefaultToSelf = "Caller", HidePin = "Caller"))
	static void RequestDynamicHole(AActor* Caller, AActor* IVSmokeVoxelVolume, UIVSmokeHolePreset* DynamicPreset);
#pragma endregion

	//~============================================================================
	// Server RPC
#pragma region RPC
private:
	/** Request a penetration hole. Always executed on server. */
	UFUNCTION(Server, Reliable, Category = "IVSmoke | Hole | RPC")
	void Internal_RequestPenetrationHole(UIVSmokeHoleGeneratorComponent* IVSmokeHoleGeneratorComponent, const FVector3f& Origin, const FVector3f& Direction, UIVSmokeHolePreset* Preset);

	/** Request an explosion hole. Always executed on server. */
	UFUNCTION(Server, Reliable, Category = "IVSmoke | Hole | RPC")
	void Internal_RequestExplosionHole(UIVSmokeHoleGeneratorComponent* IVSmokeHoleGeneratorComponent, const FVector3f& Origin, UIVSmokeHolePreset* Preset);

	/** Request a dynamic hole. Always executed on server. */
	UFUNCTION(Server, Reliable, Category = "IVSmoke | Hole | RPC")
	void Internal_RequestDynamicHole(UIVSmokeHoleGeneratorComponent* IVSmokeHoleGeneratorComponent, AActor* TargetActor, UIVSmokeHolePreset* Preset);
#pragma endregion
};
