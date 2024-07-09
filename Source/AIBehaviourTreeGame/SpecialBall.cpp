// Fill out your copyright notice in the Description page of Project Settings.


#include "SpecialBall.h"
#include "AIBehaviourTreeGameCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
ASpecialBall::ASpecialBall()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = false;

    SpecialBallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpecialBallMesh"));
    RootComponent = SpecialBallMesh;

}

// Called when the game starts or when spawned
void ASpecialBall::BeginPlay()
{
	Super::BeginPlay();


}

// Called every frame
void ASpecialBall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}





