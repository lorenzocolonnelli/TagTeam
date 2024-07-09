// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/TargetPoint.h"
#include "SpecialBall.h"
#include "Ball.h"
#include "AIBehaviourTreeGameGameMode.generated.h"

UCLASS(minimalapi)
class AAIBehaviourTreeGameGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:
	TArray<ATargetPoint*> TargetPoints;
	TArray<ABall*> GameBalls;
	TArray<ASpecialBall*> SpecialBall;

	void ResetMatch();

public:
	AAIBehaviourTreeGameGameMode();
	
	void BeginPlay() override;
	void Tick(float DeltaTime) override;

	const TArray<class ABall*>& GetBalls() const;

	const TArray<class ASpecialBall*>& GetSpecialBall() const;

};



