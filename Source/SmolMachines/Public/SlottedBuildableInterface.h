/* SPDX-License-Identifier: MPL-2.0 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SlottedBuildableInterface.generated.h"

USTRUCT(BlueprintType)
struct FSlottedBuildableParams
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	double SlotSnapSize;

	UPROPERTY(BlueprintReadWrite)
	FIntVector PerDimSnapSlots;

	UPROPERTY(BlueprintReadWrite)
	FVector BoxCenter;

	UPROPERTY(BlueprintReadWrite)
	FVector BoxExtent;

	UPROPERTY(BlueprintReadWrite)
	int32 RotationStep;
};

UINTERFACE(Blueprintable)
class SMOLMACHINES_API USlottedBuildableInterface : public UInterface
{
	GENERATED_BODY()
};

class SMOLMACHINES_API ISlottedBuildableInterface
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, Category = "Slotted Buildable")
	FSlottedBuildableParams GetSlotParams();
};
