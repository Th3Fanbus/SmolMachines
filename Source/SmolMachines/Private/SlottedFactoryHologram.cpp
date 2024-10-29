/* SPDX-License-Identifier: MPL-2.0 */

#include "SlottedFactoryHologram.h"
#include "SmolMachines.h"
#include "Buildables/FGBuildableManufacturer.h"
#include "FGFactoryLegsComponent.h"

ASlottedFactoryHologram::ASlottedFactoryHologram() : Super()
{
}

void ASlottedFactoryHologram::BeginPlay()
{
	Super::BeginPlay();
	
	AActor* ActorCDO = this->GetBuildClass().GetDefaultObject();
	if (IsSlottedBuildable(ActorCDO)) {
		UE_LOG(LogSmolMachinesCpp, Log, TEXT("Slotted holo for class: %s"), *ActorCDO->GetClass()->GetName());
		this->mSlotParams = ISlottedBuildableInterface::Execute_GetSlotParams(ActorCDO);
		UE_LOG(LogSmolMachinesCpp, Log, TEXT("BoxCenter = %s, BoxExtent = %s"), *this->mSlotParams.BoxCenter.ToString(), *this->mSlotParams.BoxExtent.ToString());
		UE_LOG(LogSmolMachinesCpp, Log, TEXT("RotationStep = %d"), this->mSlotParams.RotationStep);
	} else {
		UE_LOG(LogSmolMachinesCpp, Error, TEXT("INVALID BUILD CLASS: %s"), IsValid(ActorCDO) ? *ActorCDO->GetClass()->GetName() : TEXT("nullptr"));
	}
}

bool ASlottedFactoryHologram::IsValidHitResult(const FHitResult& hitResult) const
{
	AActor* HitActor = hitResult.GetActor();
	if (IsSlottedBuildable(HitActor)) {
		return true;
	}
	return Super::IsValidHitResult(hitResult);
}

template<typename T>
static FORCEINLINE int32 VecSign(const T& Vec, const int32 Idx)
{
	return Idx + Idx + int32(Vec[Idx] >= 0);
}

static FORCEINLINE int32 GetClosestAxisIndex(const FVector& Vec)
{
	const FVector AbsVec = Vec.GetAbs().GetSafeNormal();
	if (AbsVec.X > AbsVec.Y) {
		if (AbsVec.X > AbsVec.Z) {
			return VecSign(Vec, 0);
		}
	} else {
		if (AbsVec.Y > AbsVec.Z) {
			return VecSign(Vec, 1);
		}
	}
	return VecSign(Vec, 2);
}

template<typename T, typename U>
static FORCEINLINE T PositiveMod(const T& i, const U& n)
{
	return (i % n + T(n)) % n;
}

template<typename T>
static FORCEINLINE T DoVecSwizzle(const T& Vec, const int32 Swizzle)
{
	const FIntVector Idx = PositiveMod(FIntVector(Swizzle, Swizzle + 1, Swizzle + 2), 3);
	return T(Vec[Idx.X], Vec[Idx.Y], Vec[Idx.Z]);
}

static FORCEINLINE double ExtFactor(const double x, const double n)
{
	return (2 * x + 1 - n) / n;
}

static bool GetClosestSnapPoint(FVector& OutResult, const FSlottedBuildableParams& Params, const FVector& Origin, int32 SignedAxis)
{
	const int32 Axis = SignedAxis / 2;
	const int32 Sign = SignedAxis % 2 ? 1 : -1;

	/* Project 3D space so that X is forward and Y, Z represent 2D space */
	const FIntVector DimSnapSlots = DoVecSwizzle(Params.PerDimSnapSlots, Axis);
	if (DimSnapSlots.X <= 0) {
		return false;
	}
	const int32 NumU = FMath::Max(DimSnapSlots.Y, 1);
	const int32 NumV = FMath::Max(DimSnapSlots.Z, 1);

	//UE_LOG(LogSmolMachinesCpp, Error, TEXT("NumU = %d, NumV = %d"), NumU, NumV);

	/* Yes, these loops could use the remapped indices, but it would be less readable */
	double MinDist = DBL_MAX;
	for (int32 u = 0; u < NumU; u++) {
		const double FacU = ExtFactor(u, NumU);
		for (int32 v = 0; v < NumV; v++) {
			const double FacV = ExtFactor(v, NumV);
			const FVector Vec = Params.BoxCenter + Params.BoxExtent * DoVecSwizzle(FVector(Sign, FacU, FacV), -Axis);
			const double Dist = FVector::DistSquared(Vec, Origin);
			if (Dist < MinDist) {
				MinDist = Dist;
				OutResult = Vec;
			}
		}
	}
	return MinDist < DBL_MAX;
}

static FORCEINLINE int32 FlipAxis(const int32 SignedAxis)
{
	return SignedAxis ^ 1;
}

bool ASlottedFactoryHologram::Internal_TrySnapToActor(const FHitResult& hitResult)
{
	AFGBuildable* HitActor = Cast<AFGBuildable>(hitResult.GetActor());
	if (not IsSlottedBuildable(HitActor)) {
		return false;
	}

	const FVector LocalHitPos = HitActor->GetTransform().InverseTransformPosition(hitResult.Location);
	const FSlottedBuildableParams Params = ISlottedBuildableInterface::Execute_GetSlotParams(HitActor);

	const FVector AxisLimit = FVector(Params.PerDimSnapSlots).GetClampedToSize(0, 1);
	const int32 SignedAxis = GetClosestAxisIndex((LocalHitPos - Params.BoxCenter) * AxisLimit);

	FVector ClosestSlot = FVector::ZeroVector;
	if (not GetClosestSnapPoint(ClosestSlot, Params, LocalHitPos, SignedAxis)) {
		return false;
	}
	FVector OurSnapSlot = FVector::ZeroVector;
	if (not GetClosestSnapPoint(OurSnapSlot, mSlotParams, FVector::ZeroVector, FlipAxis(SignedAxis))) {
		UE_LOG(LogSmolMachinesCpp, Error, TEXT("COULD NOT GET OUR OWN SNAP POINT?"));
		return false;
	}

	this->mUseBuildClearanceOverlapSnapp = false;

	const FRotator ActorRotation = HitActor->GetActorRotation();
	const float FinalYaw = this->ApplyScrollRotationTo(ActorRotation.Yaw);
	const FVector FinalLocation = HitActor->GetTransform().TransformPosition(ClosestSlot - OurSnapSlot);
	this->SetActorLocationAndRotation(FinalLocation, FRotator(ActorRotation.Pitch, FinalYaw, ActorRotation.Roll));

	if (IsValid(this->mLegs)) {
		this->mLegs->ClearFeetOffsets();
	}
	this->mSnappedBuilding = HitActor;
	return true;
}

bool ASlottedFactoryHologram::TrySnapToActor(const FHitResult& hitResult)
{
	const bool Result = this->Internal_TrySnapToActor(hitResult);
	if (not Result) {
		this->mSnappedBuilding = nullptr;
		this->mUseBuildClearanceOverlapSnapp = true;
	}
	return Result;
}

int32 ASlottedFactoryHologram::GetRotationStep() const
{
	if (IsSlottedBuildable(this->mSnappedBuilding)) {
		return mSlotParams.RotationStep ? mSlotParams.RotationStep : 180;
	}
	return Super::GetRotationStep();
}
