// Copyright Epic Games, Inc. All Rights Reserved.

#include "AIBehaviourTreeGameGameMode.h"
#include "EnemyAIController.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

AAIBehaviourTreeGameGameMode::AAIBehaviourTreeGameGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

void AAIBehaviourTreeGameGameMode::BeginPlay()
{
	Super::BeginPlay();

	ResetMatch();
}

void AAIBehaviourTreeGameGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	bool allGameBallsAttachedToPlayerPawn = true;
	bool specialBallAttachedToEnemy = false;

	for (int32 i = 0; i < GameBalls.Num(); i++)
	{
		if (GameBalls[i]->GetAttachParentActor() != GetWorld()->GetFirstPlayerController()->GetPawn())
		{
			allGameBallsAttachedToPlayerPawn = false;
			break;
		}
	}
	for (int32 i = 0; i < SpecialBall.Num(); i++)
	{
		if (SpecialBall[i]->GetAttachParentActor() != nullptr &&
			SpecialBall[i]->GetAttachParentActor() != GetWorld()->GetFirstPlayerController()->GetPawn())
		{
			specialBallAttachedToEnemy = true;
			break; 
		}
	}

	if (allGameBallsAttachedToPlayerPawn || specialBallAttachedToEnemy)
	{
		ResetMatch();
	}

}

const TArray<class ABall*>& AAIBehaviourTreeGameGameMode::GetBalls() const
{
	return GameBalls;
}

const TArray<class ASpecialBall*>& AAIBehaviourTreeGameGameMode::GetSpecialBall() const
{
	return SpecialBall;
}

void AAIBehaviourTreeGameGameMode::ResetMatch()
{
	TargetPoints.Empty();
	GameBalls.Empty();
	SpecialBall.Empty();
	
	for (TActorIterator<ASpecialBall> It(GetWorld()); It; ++It)
	{
		if (It->GetAttachParentActor())
		{
			It->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		}
		SpecialBall.Add(*It);
	}

	for (TActorIterator<ATargetPoint> It(GetWorld()); It; ++It)
	{
		TargetPoints.Add(*It);
	}

	for (TActorIterator<ABall> It(GetWorld()); It; ++It)
	{
		if (It->GetAttachParentActor())
		{
			It->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			
		}
		GameBalls.Add(*It);
	}



	TArray<ATargetPoint*> RandomTargetPoints = TargetPoints;

	for (int32 i = 0; i < GameBalls.Num(); i++)
	{
		const int32 RandomTargetIndex = FMath::RandRange(0, RandomTargetPoints.Num() - 1);

		GameBalls[i]->SetActorLocation(RandomTargetPoints[RandomTargetIndex]->GetActorLocation());

		RandomTargetPoints.RemoveAt(RandomTargetIndex);
	}
}



