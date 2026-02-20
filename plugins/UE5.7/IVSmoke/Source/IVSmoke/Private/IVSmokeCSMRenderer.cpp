// Copyright (c) 2026, Team SDB. All rights reserved.

#include "IVSmokeCSMRenderer.h"
#include "IVSmokeSettings.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogIVSmokeCSM, Log, All);

//~==============================================================================
// Constructor / Destructor

FIVSmokeCSMRenderer::FIVSmokeCSMRenderer()
{
}

FIVSmokeCSMRenderer::~FIVSmokeCSMRenderer()
{
	Shutdown();
}

//~==============================================================================
// Lifecycle

void FIVSmokeCSMRenderer::Initialize(UWorld* World, int32 NumCascades, int32 Resolution, float MaxDistance)
{
	if (!World)
	{
		UE_LOG(LogIVSmokeCSM, Error, TEXT("[FIVSmokeCSMRenderer::Initialize] World is null"));
		return;
	}

	// Clean up existing resources
	Shutdown();

	// Clamp parameters
	NumCascades = FMath::Clamp(NumCascades, 1, 8);
	Resolution = FMath::Clamp(Resolution, 256, 2048);
	MaxDistance = FMath::Max(MaxDistance, 1000.0f);

	CurrentResolution = Resolution;
	MaxShadowDistance = MaxDistance;

	// Load settings
	const UIVSmokeSettings* Settings = UIVSmokeSettings::Get();
	if (Settings)
	{
		LogLinearBlend = Settings->CascadeLogLinearBlend;
		bEnablePriorityUpdate = Settings->bEnablePriorityUpdate;
		NearCascadeUpdateInterval = Settings->NearCascadeUpdateInterval;
		FarCascadeUpdateInterval = Settings->FarCascadeUpdateInterval;
	}

	// Create owner actor for capture components
	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	// Don't specify a fixed name - let the engine generate a unique one to avoid conflicts
	// when transitioning between Editor Preview and PIE (both may have the level loaded)
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!Owner)
	{
		UE_LOG(LogIVSmokeCSM, Error, TEXT("[FIVSmokeCSMRenderer::Initialize] Failed to create capture owner actor"));
		return;
	}
	CaptureOwner = Owner;

	// Initialize cascades
	Cascades.SetNum(NumCascades);
	for (int32 i = 0; i < NumCascades; i++)
	{
		FIVSmokeCascadeData& Cascade = Cascades[i];
		Cascade.CascadeIndex = i;

		// Create capture component
		USceneCaptureComponent2D* CaptureComp = NewObject<USceneCaptureComponent2D>(Owner);
		CaptureComp->RegisterComponent();
		CaptureComp->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
		ConfigureCaptureComponent(CaptureComp);
		Cascade.CaptureComponent = CaptureComp;

		// Create render targets
		CreateCascadeRenderTargets(Cascade, Resolution);

		// Assign depth RT to capture component
		CaptureComp->TextureTarget = Cascade.DepthRT;
	}

	// Calculate initial cascade splits
	CalculateCascadeSplits(NearPlaneDistance, MaxDistance, LogLinearBlend);

	bIsInitialized = true;

	UE_LOG(LogIVSmokeCSM, Log, TEXT("[FIVSmokeCSMRenderer::Initialize] Initialized with %d cascades, %dx%d resolution, %.0f max distance"),
		NumCascades, Resolution, Resolution, MaxDistance);
}

void FIVSmokeCSMRenderer::Shutdown()
{
	for (FIVSmokeCascadeData& Cascade : Cascades)
	{
		// Clean up render targets
		if (Cascade.DepthRT)
		{
			Cascade.DepthRT->RemoveFromRoot();
			Cascade.DepthRT = nullptr;
		}
		if (Cascade.VSMRT)
		{
			Cascade.VSMRT->RemoveFromRoot();
			Cascade.VSMRT = nullptr;
		}

		// Capture component will be destroyed with owner
		Cascade.CaptureComponent = nullptr;
	}

	Cascades.Empty();

	// Destroy owner actor
	if (CaptureOwner.IsValid())
	{
		CaptureOwner->Destroy();
		CaptureOwner = nullptr;
	}

	bIsInitialized = false;

	UE_LOG(LogIVSmokeCSM, Log, TEXT("[FIVSmokeCSMRenderer::Shutdown] CSM renderer shut down"));
}

//~==============================================================================
// Update

void FIVSmokeCSMRenderer::Update(
	const FVector& CameraPosition,
	const FVector& CameraForward,
	const FVector& LightDirection,
	uint32 FrameNumber)
{
	if (!bIsInitialized || Cascades.Num() == 0)
	{
		return;
	}

	// Store main camera position for camera-relative calculations
	MainCameraPosition = CameraPosition;

	//~==========================================================================
	// Validity Check - PIE restart can invalidate capture components
	// Check CaptureOwner first (all components are attached to it).
	// If owner is destroyed, all components are invalid.
	if (!CaptureOwner.IsValid())
	{
		UE_LOG(LogIVSmokeCSM, Warning, TEXT("[FIVSmokeCSMRenderer::Update] CaptureOwner invalidated (PIE restart?), shutting down for reinitialization"));
		Shutdown();
		return;
	}

	// Also verify at least one capture component is valid
	bool bAnyValidComponent = false;
	for (const FIVSmokeCascadeData& Cascade : Cascades)
	{
		if (IsValid(Cascade.CaptureComponent))
		{
			bAnyValidComponent = true;
			break;
		}
	}
	if (!bAnyValidComponent)
	{
		UE_LOG(LogIVSmokeCSM, Warning, TEXT("[FIVSmokeCSMRenderer::Update] All CaptureComponents invalidated, shutting down for reinitialization"));
		Shutdown();
		return;
	}

	// Determine which cascades need update
	UpdateCascadePriorities(FrameNumber);

	// Update each cascade that needs it
	for (int32 i = 0; i < Cascades.Num(); i++)
	{
		if (Cascades[i].bNeedsCapture)
		{
			UpdateCascadeCapture(i, CameraPosition, LightDirection);
			Cascades[i].LastCaptureFrame = FrameNumber;
		}
	}
}

void FIVSmokeCSMRenderer::UpdateCascadePriorities(uint32 FrameNumber)
{
	// TODO: Priority update system disabled due to texel snapping synchronization issues.
	// When a cascade is not updated, its texel-snapped position becomes stale relative to
	// the current camera position, causing shadow flickering.
	// Re-implement with proper synchronization: either update VP matrix every frame
	// (even when not capturing), or use per-cascade camera positions for cascade selection.
	for (FIVSmokeCascadeData& Cascade : Cascades)
	{
		Cascade.bNeedsCapture = true;
	}
}

void FIVSmokeCSMRenderer::UpdateCascadeCapture(
	int32 CascadeIndex,
	const FVector& CameraPosition,
	const FVector& LightDirection)
{
	if (!Cascades.IsValidIndex(CascadeIndex))
	{
		return;
	}

	FIVSmokeCascadeData& Cascade = Cascades[CascadeIndex];
	if (!IsValid(Cascade.CaptureComponent))
	{
		UE_LOG(LogIVSmokeCSM, Warning, TEXT("[FIVSmokeCSMRenderer::UpdateCascadeCapture] CaptureComponent for cascade %d is invalid"), CascadeIndex);
		return;
	}

	// Normalize light direction
	FVector NormalizedLightDir = LightDirection.GetSafeNormal();
	if (NormalizedLightDir.IsNearlyZero())
	{
		NormalizedLightDir = FVector(0.0f, 0.0f, 1.0f);
	}

	//~==========================================================================
	// CSM Camera Positioning
	//
	// OrthoWidth covers the cascade's view frustum at its far distance.
	// For a reasonable FOV (~90°), width at distance D is roughly 2*D.
	float NewOrthoWidth = Cascade.FarPlane * 2.0f;

	// Calculate light view axes
	FVector LightForward = -NormalizedLightDir; // Camera looks opposite to light direction
	FVector LightRight = FVector::CrossProduct(NormalizedLightDir, FVector::UpVector);
	if (LightRight.IsNearlyZero())
	{
		LightRight = FVector::CrossProduct(NormalizedLightDir, FVector::ForwardVector);
	}
	LightRight.Normalize();
	FVector LightUp = FVector::CrossProduct(LightRight, LightForward).GetSafeNormal();

	// Position shadow camera far enough to see all shadow casters
	float CaptureDistance = MaxShadowDistance * 1.5f;
	FVector BaseCapturePosition = CameraPosition + NormalizedLightDir * CaptureDistance;

	//~==========================================================================
	// Texel Snapping - Use SMALLEST cascade's texel size for ALL cascades
	//
	// CRITICAL: All cascades must snap to the SAME grid to ensure:
	// 1. Same WorldPos maps to same relative UV across cascades
	// 2. Minimal shadow shimmer during camera movement
	// 3. Smooth cascade transitions without edge artifacts
	//
	// Uses cascade 0's texel size (smallest = finest grid) for consistency.
	double SmallestOrthoWidth = (double)Cascades[0].FarPlane * 2.0;
	double TexelSize = SmallestOrthoWidth / (double)CurrentResolution;

	// Project PLAYER CAMERA position onto light view axes
	double PlayerRightOffset = FVector::DotProduct(CameraPosition, LightRight);
	double PlayerUpOffset = FVector::DotProduct(CameraPosition, LightUp);

	// Snap to unified grid (same for all cascades)
	double SnappedRight = FMath::FloorToDouble(PlayerRightOffset / TexelSize) * TexelSize;
	double SnappedUp = FMath::FloorToDouble(PlayerUpOffset / TexelSize) * TexelSize;

	// Calculate adjustment
	FVector SnapAdjustment = LightRight * (float)(SnappedRight - PlayerRightOffset)
	                       + LightUp * (float)(SnappedUp - PlayerUpOffset);

	FVector SnappedPosition = BaseCapturePosition + SnapAdjustment;

	//~==========================================================================
	// Store current frame values (used by both capture and shader)
	Cascade.OrthoWidth = NewOrthoWidth;
	Cascade.LightCameraPosition = SnappedPosition;
	Cascade.LightCameraForward = LightForward;

	// Apply to capture component
	FRotator CaptureRotation = LightForward.Rotation();
	Cascade.CaptureComponent->SetWorldLocationAndRotation(SnappedPosition, CaptureRotation);
	Cascade.CaptureComponent->OrthoWidth = NewOrthoWidth;

	// Calculate view-projection matrix
	CalculateViewProjectionMatrix(Cascade);

	//~==========================================================================
	// Synchronous Capture - ensures VP matrix and depth texture match
	Cascade.CaptureComponent->CaptureScene();

	UE_LOG(LogIVSmokeCSM, Verbose, TEXT("[FIVSmokeCSMRenderer::UpdateCascadeCapture] Cascade %d: Near=%.0f, Far=%.0f, OrthoWidth=%.0f"),
		CascadeIndex, Cascade.NearPlane, Cascade.FarPlane, NewOrthoWidth);
}

//~==============================================================================
// Cascade Split Calculation

void FIVSmokeCSMRenderer::CalculateCascadeSplits(float NearPlane, float FarPlane, float LogLinearBlendFactor)
{
	const int32 NumCascades = Cascades.Num();
	if (NumCascades == 0)
	{
		return;
	}

	// Ensure minimum near plane
	NearPlane = FMath::Max(NearPlane, 1.0f);

	for (int32 i = 0; i < NumCascades; i++)
	{
		float t = (float)(i + 1) / (float)NumCascades;

		// Linear distribution
		float Linear = NearPlane + (FarPlane - NearPlane) * t;

		// Logarithmic distribution (better near/far balance)
		float Log = NearPlane * FMath::Pow(FarPlane / NearPlane, t);

		// Blend between linear and logarithmic
		float SplitDistance = FMath::Lerp(Linear, Log, LogLinearBlendFactor);

		Cascades[i].FarPlane = SplitDistance;
		Cascades[i].NearPlane = (i == 0) ? NearPlane : Cascades[i - 1].FarPlane;
	}

	UE_LOG(LogIVSmokeCSM, Log, TEXT("[FIVSmokeCSMRenderer::CalculateCascadeSplits] Splits calculated (LogLinear=%.2f):"), LogLinearBlendFactor);
	for (int32 i = 0; i < NumCascades; i++)
	{
		UE_LOG(LogIVSmokeCSM, Log, TEXT("  Cascade %d: %.0f - %.0f cm"), i, Cascades[i].NearPlane, Cascades[i].FarPlane);
	}
}

//~==============================================================================
// Texel Snapping

FVector FIVSmokeCSMRenderer::ApplyTexelSnapping(
	const FVector& LightViewOrigin,
	float OrthoWidth,
	int32 Resolution,
	const FVector& LightRight,
	const FVector& LightUp) const
{
	// Calculate world-space texel size
	// Use double precision for large coordinates to avoid float precision issues
	double TexelSize = (double)OrthoWidth / (double)Resolution;

	// Project position onto light view axes using double precision
	// This is critical when coordinates are large (e.g., 100,000 cm)
	double RightOffset = FVector::DotProduct(LightViewOrigin, LightRight);
	double UpOffset = FVector::DotProduct(LightViewOrigin, LightUp);

	// Snap to texel grid using Floor (not Round) for stability
	// Round can cause jittering when value oscillates around X.5
	double SnappedRight = FMath::FloorToDouble(RightOffset / TexelSize) * TexelSize;
	double SnappedUp = FMath::FloorToDouble(UpOffset / TexelSize) * TexelSize;

	// Calculate adjustment delta
	double DeltaRight = SnappedRight - RightOffset;
	double DeltaUp = SnappedUp - UpOffset;

	// Apply snapped offset
	FVector Snapped = LightViewOrigin;
	Snapped += LightRight * (float)DeltaRight;
	Snapped += LightUp * (float)DeltaUp;

	return Snapped;
}

//~==============================================================================
// View-Projection Matrix

void FIVSmokeCSMRenderer::CalculateViewProjectionMatrix(FIVSmokeCascadeData& Cascade)
{
	//~==========================================================================
	// Calculate VP Matrix to MATCH SceneCaptureComponent2D's actual rendering
	//
	// We must use the EXACT same method that UE uses for SceneCaptureComponent2D
	// to ensure our VP matrix matches the captured texture.
	//
	// This is read from CaptureComponent to ensure we use the exact same values
	// that the capture will use (after any engine-side adjustments).

	if (!IsValid(Cascade.CaptureComponent))
	{
		return;
	}

	// Get the actual transform that will be used for capture
	FVector CameraLocation = Cascade.CaptureComponent->GetComponentLocation();
	FRotator CameraRotation = Cascade.CaptureComponent->GetComponentRotation();

	//~==========================================================================
	// View Matrix - Match UE's FSceneView calculation
	//
	// UE calculates view matrix from component transform using:
	// 1. Translation matrix (negative location)
	// 2. Rotation matrix (inverse rotation + axis swap for UE coordinate system)
	FMatrix ViewRotationMatrix = FInverseRotationMatrix(CameraRotation) * FMatrix(
		FPlane(0, 0, 1, 0),
		FPlane(1, 0, 0, 0),
		FPlane(0, 1, 0, 0),
		FPlane(0, 0, 0, 1)
	);
	FMatrix ViewMatrix = FTranslationMatrix(-CameraLocation) * ViewRotationMatrix;

	//~==========================================================================
	// Projection Matrix - Orthographic
	float OrthoWidth = Cascade.CaptureComponent->OrthoWidth;
	float HalfWidth = OrthoWidth * 0.5f;
	float HalfHeight = HalfWidth; // Square projection

	// Near/far planes - use large range to capture all shadow casters
	float NearZ = 1.0f;
	float FarZ = MaxShadowDistance * 3.0f;

	// Build orthographic projection matrix (UE uses reversed-Z)
	FMatrix ProjectionMatrix = FReversedZOrthoMatrix(HalfWidth, HalfHeight, 1.0f / (FarZ - NearZ), 0.0f);

	// Combined view-projection matrix
	Cascade.ViewProjectionMatrix = ViewMatrix * ProjectionMatrix;

	// Update camera position/forward from component (ensure consistency)
	Cascade.LightCameraPosition = CameraLocation;
	Cascade.LightCameraForward = CameraRotation.Vector();
}

//~==============================================================================
// Render Target Creation

void FIVSmokeCSMRenderer::CreateCascadeRenderTargets(FIVSmokeCascadeData& Cascade, int32 Resolution)
{
	// Create depth render target (R32F)
	Cascade.DepthRT = NewObject<UTextureRenderTarget2D>();
	Cascade.DepthRT->AddToRoot(); // Prevent GC
	Cascade.DepthRT->RenderTargetFormat = ETextureRenderTargetFormat::RTF_R32f;
	Cascade.DepthRT->InitAutoFormat(Resolution, Resolution);
	Cascade.DepthRT->AddressX = TextureAddress::TA_Clamp;
	Cascade.DepthRT->AddressY = TextureAddress::TA_Clamp;
	Cascade.DepthRT->ClearColor = FLinearColor::Black;
	Cascade.DepthRT->UpdateResourceImmediate(true);

	// Create VSM render target (RG32F) with UAV support for compute shader processing
	const UIVSmokeSettings* Settings = UIVSmokeSettings::Get();
	if (Settings && Settings->bEnableVSM)
	{
		Cascade.VSMRT = NewObject<UTextureRenderTarget2D>();
		Cascade.VSMRT->AddToRoot();
		Cascade.VSMRT->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RG32f;
		Cascade.VSMRT->bCanCreateUAV = true;  // Required for VSM compute shader processing
		Cascade.VSMRT->InitAutoFormat(Resolution, Resolution);
		Cascade.VSMRT->AddressX = TextureAddress::TA_Clamp;
		Cascade.VSMRT->AddressY = TextureAddress::TA_Clamp;
		Cascade.VSMRT->ClearColor = FLinearColor::Black;
		Cascade.VSMRT->UpdateResourceImmediate(true);
	}

	UE_LOG(LogIVSmokeCSM, Verbose, TEXT("[FIVSmokeCSMRenderer::CreateCascadeRenderTargets] Created RTs for cascade %d: %dx%d"),
		Cascade.CascadeIndex, Resolution, Resolution);
}

void FIVSmokeCSMRenderer::ConfigureCaptureComponent(USceneCaptureComponent2D* CaptureComponent)
{
	if (!CaptureComponent)
	{
		return;
	}

	// Orthographic projection for directional light
	CaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;

	// Capture scene depth
	CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_SceneDepth;

	// Disable auto-capture - we call CaptureScene() manually in UpdateCascadeCapture()
	// for synchronous VP matrix and depth texture synchronization
	CaptureComponent->bCaptureEveryFrame = false;
	CaptureComponent->bCaptureOnMovement = false;

	// Disable auto-calculate for consistency
	CaptureComponent->bAutoCalculateOrthoPlanes = false;

	// Use scene primitives
	CaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;

	// Persist rendering state for quality
	CaptureComponent->bAlwaysPersistRenderingState = true;

	//~==========================================================================
	// ShowFlags Optimization for Depth-Only Shadow Capture
	//
	// NOTE: Nanite must stay ENABLED - fallback meshes don't write depth properly.
	// Disable only rendering features that don't affect depth output.

	// --- Disable lighting/shading (not needed for depth) ---
	CaptureComponent->ShowFlags.SetLighting(false);
	CaptureComponent->ShowFlags.SetGlobalIllumination(false);
	CaptureComponent->ShowFlags.SetLumenGlobalIllumination(false);
	CaptureComponent->ShowFlags.SetLumenReflections(false);
	CaptureComponent->ShowFlags.SetReflectionEnvironment(false);
	CaptureComponent->ShowFlags.SetAmbientOcclusion(false);
	CaptureComponent->ShowFlags.SetScreenSpaceReflections(false);

	// --- Disable post-processing (not needed for depth) ---
	CaptureComponent->ShowFlags.SetPostProcessing(false);
	CaptureComponent->ShowFlags.SetBloom(false);
	CaptureComponent->ShowFlags.SetMotionBlur(false);
	CaptureComponent->ShowFlags.SetToneCurve(false);
	CaptureComponent->ShowFlags.SetEyeAdaptation(false);
	CaptureComponent->ShowFlags.SetColorGrading(false);
	CaptureComponent->ShowFlags.SetDepthOfField(false);
	CaptureComponent->ShowFlags.SetVignette(false);
	CaptureComponent->ShowFlags.SetGrain(false);

	// --- Disable atmosphere/fog (not needed for depth) ---
	CaptureComponent->ShowFlags.SetAtmosphere(false);
	CaptureComponent->ShowFlags.SetFog(false);
	CaptureComponent->ShowFlags.SetVolumetricFog(false);

	// --- Disable shadows (we're creating shadows, not receiving) ---
	CaptureComponent->ShowFlags.SetDynamicShadows(false);
	CaptureComponent->ShowFlags.SetContactShadows(false);

	// --- Disable non-shadow-casting elements ---
	CaptureComponent->ShowFlags.SetTranslucency(false);
	CaptureComponent->ShowFlags.SetParticles(false);
	CaptureComponent->ShowFlags.SetDecals(false);

	// --- Optionally disable skeletal meshes (characters) ---
	const UIVSmokeSettings* Settings = UIVSmokeSettings::Get();
	if (Settings && !Settings->bCaptureSkeletalMeshes)
	{
		CaptureComponent->ShowFlags.SetSkeletalMeshes(false);
	}
}

//~==============================================================================
// Accessors

TArray<float> FIVSmokeCSMRenderer::GetSplitDistances() const
{
	TArray<float> Splits;
	Splits.Reserve(Cascades.Num());

	for (const FIVSmokeCascadeData& Cascade : Cascades)
	{
		Splits.Add(Cascade.FarPlane);
	}

	return Splits;
}

FTextureRHIRef FIVSmokeCSMRenderer::GetVSMTexture(int32 CascadeIndex) const
{
	if (!Cascades.IsValidIndex(CascadeIndex) || !Cascades[CascadeIndex].VSMRT)
	{
		return nullptr;
	}

	FTextureRenderTargetResource* Resource = Cascades[CascadeIndex].VSMRT->GameThread_GetRenderTargetResource();
	return Resource ? Resource->TextureRHI : nullptr;
}

FTextureRHIRef FIVSmokeCSMRenderer::GetDepthTexture(int32 CascadeIndex) const
{
	if (!Cascades.IsValidIndex(CascadeIndex) || !Cascades[CascadeIndex].DepthRT)
	{
		return nullptr;
	}

	FTextureRenderTargetResource* Resource = Cascades[CascadeIndex].DepthRT->GameThread_GetRenderTargetResource();
	return Resource ? Resource->TextureRHI : nullptr;
}

bool FIVSmokeCSMRenderer::HasValidShadowData() const
{
	if (!bIsInitialized || Cascades.Num() == 0)
	{
		return false;
	}

	// Check if at least cascade 0 has a valid render target
	return Cascades[0].DepthRT != nullptr && Cascades[0].DepthRT->GetResource() != nullptr;
}

FVector FIVSmokeCSMRenderer::GetLightCameraPosition(int32 CascadeIndex) const
{
	if (!Cascades.IsValidIndex(CascadeIndex))
	{
		return FVector::ZeroVector;
	}

	return Cascades[CascadeIndex].LightCameraPosition;
}

