/* SPDX-License-Identifier: MPL-2.0 */

#pragma once

#include "CoreMinimal.h"
#include "SmolMachines.h"
#include "SlottedBuildableInterface.h"
#include "Hologram/FGFactoryHologram.h"
#include "SlottedFactoryHologram.generated.h"

UCLASS()
class SMOLMACHINES_API ASlottedFactoryHologram : public AFGFactoryHologram
{
	GENERATED_BODY()
public:
	ASlottedFactoryHologram();
	virtual void BeginPlay() override;

	// Begin AFGHologram interface
	virtual bool IsValidHitResult(const FHitResult& hitResult) const override;
	virtual bool TrySnapToActor(const FHitResult& hitResult) override;
	// End AFGHologram interface

protected:
	// Begin AFGHologram interface
	virtual int32 GetRotationStep() const override;
	// End of AFGHologram interface

private:
	static FORCEINLINE bool IsSlottedBuildable(AActor* Actor)
	{
		return IsValid(Actor) and Actor->Implements<USlottedBuildableInterface>();
	}

	bool Internal_TrySnapToActor(const FHitResult& hitResult);
private:
	FSlottedBuildableParams mSlotParams;
};
