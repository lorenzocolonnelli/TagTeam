#include "EnemyAIController.h"
#include "AIBehaviourTreeGameCharacter.h"
#include "Navigation/PathFollowingComponent.h"
#include "AIBehaviourTreeGameGameMode.h"
#include "Ball.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"


AEnemyAIController::AEnemyAIController()
{
    static ConstructorHelpers::FObjectFinder<UBehaviorTree> BTObject(TEXT("/Game/AI/EnemyBehaviourTree.EnemyBehaviourTree"));
    if (BTObject.Succeeded())
    {
        BehaviorTree = BTObject.Object;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find BehaviorTree'/Game/AI/EnemyBehaviourTree.EnemyBehaviourTree'"));
    }

    static ConstructorHelpers::FObjectFinder<UBlackboardData> BBObject(TEXT("/Game/AI/EnemyBlackboard.EnemyBlackboard"));
    if (BBObject.Succeeded())
    {
        BlackboardData = BBObject.Object;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find BlackboardData'/Game/AI/EnemyBlackboard.EnemyBlackboard'"));
    }

    bIsRunningAway = false;
}

void AEnemyAIController::BeginPlay()
{
    Super::BeginPlay();

    if (UseBlackboard(BlackboardData, BlackboardComponent))
    {
        if (!RunBehaviorTree(BehaviorTree))
        {
            UE_LOG(LogTemp, Warning, TEXT("Behavior tree failed to run"));
        }
    }

    if (BlackboardComponent)
    {
        BlackboardComponent->SetValueAsInt(FName("DeliveredBallsToPlayer"), 0);
    }

    GoToPlayer = MakeShared<FAivState>(
        [](AAIController* AIController) {
            UE_LOG(LogTemp, Log, TEXT("Entering GoToPlayer state"));
            AIController->MoveToActor(AIController->GetWorld()->GetFirstPlayerController()->GetPawn(), 10.0f);
        },
        nullptr,
        [this](AAIController* AIController, const float DeltaTime) -> TSharedPtr<FAivState> {
            EPathFollowingStatus::Type State = AIController->GetMoveStatus();

            if (State == EPathFollowingStatus::Moving)
            {
                return nullptr;
            }
            if (BestBall)
            {
                BestBall->SetActorLocation(FVector(0, 0, 0));
                BestBall->AttachToActor(AIController->GetWorld()->GetFirstPlayerController()->GetPawn(), FAttachmentTransformRules::KeepRelativeTransform);
                IncrementDeliveredBallsToPlayer();
                BestBall = nullptr;
            }
            return SearchForBall;
        }
    );

    RunAway = MakeShared<FAivState>(
        [this](AAIController* AIController) {
            UE_LOG(LogTemp, Log, TEXT("Entering RunAway state"));
            AIController->StopMovement();
        },
        nullptr,
        [this](AAIController* AIController, const float DeltaTime) -> TSharedPtr<FAivState> {
            
            FVector MinBounds = FVector(200.0f, 170.0f, 0.0f); // Esempio di limiti minimi
            FVector MaxBounds = FVector(2800.0f, 3200.0f, 0.0f); // Esempio di limiti massimi

            APawn* PlayerPawn = AIController->GetWorld()->GetFirstPlayerController()->GetPawn();
            
            if (PlayerPawn)
            {
                FVector Direction = PlayerPawn->GetActorLocation() - AIController->GetPawn()->GetActorLocation();
                Direction.Normalize();

                
                FVector RunToLocation = AIController->GetPawn()->GetActorLocation() - Direction * 500.0f;

                if (!(RunToLocation.X >= MinBounds.X && RunToLocation.X <= MaxBounds.X &&
                    RunToLocation.Y >= MinBounds.Y && RunToLocation.Y <= MaxBounds.Y &&
                    RunToLocation.Z >= MinBounds.Z && RunToLocation.Z <= MaxBounds.Z))
                {
                    RunToLocation = FVector(FMath::RandRange(MinBounds.X, MaxBounds.X),
                        FMath::RandRange(MinBounds.Y, MaxBounds.Y),
                        FMath::RandRange(MinBounds.Z, MaxBounds.Z));
                }

                
                EPathFollowingStatus::Type Status = AIController->GetPathFollowingComponent()->GetStatus();
                if (Status != EPathFollowingStatus::Moving)
                {
                    AIController->MoveToLocation(RunToLocation, 1.0f, true, true, true, false, 0, true);
                }
            }
            
            return nullptr;
        }
    );



    SearchForBall = MakeShared<FAivState>(
        [this](AAIController* AIController) {
            UE_LOG(LogTemp, Log, TEXT("Entering SearchForBall state"));
            AGameModeBase* GameMode = AIController->GetWorld()->GetAuthGameMode();
            AAIBehaviourTreeGameGameMode* AIGameMode = Cast<AAIBehaviourTreeGameGameMode>(GameMode);
            const TArray<ABall*>& BallsList = AIGameMode->GetBalls();

            ABall* NearestBall = nullptr;

            for (int32 i = 0; i < BallsList.Num(); i++)
            {
                if (!BallsList[i]->GetAttachParentActor() &&
                    (!NearestBall ||
                        FVector::Distance(AIController->GetPawn()->GetActorLocation(), BallsList[i]->GetActorLocation()) < FVector::Distance(AIController->GetPawn()->GetActorLocation(), NearestBall->GetActorLocation())))
                {
                    NearestBall = BallsList[i];
                }
            }

            BestBall = NearestBall;

            
            if (BlackboardComponent)
            {
                BlackboardComponent->SetValueAsObject(FName("BestBall"), BestBall);
            }
        },
        nullptr,
        [this](AAIController* AIController, const float DeltaTime) -> TSharedPtr<FAivState> {
            if (BestBall)
            {
                return GoToBall;
            }
            else
            {
                return SearchForBall;
            }
        }
    );

    GoToBall = MakeShared<FAivState>(
        [this](AAIController* AIController) {
            UE_LOG(LogTemp, Log, TEXT("Entering GoToBall state"));
            AIController->MoveToActor(BestBall, 10.0f);

        },
        nullptr,
        [this](AAIController* AIController, const float DeltaTime) -> TSharedPtr<FAivState> {
            EPathFollowingStatus::Type State = AIController->GetMoveStatus();

            if (State == EPathFollowingStatus::Moving)
            {
                return nullptr;
            }

            return GrabBall;
        }
    );

    GrabBall = MakeShared<FAivState>(
        [this](AAIController* AIController) {
            UE_LOG(LogTemp, Log, TEXT("Entering GrabBall state"));
            if (BestBall->GetAttachParentActor())
            {

                BestBall = nullptr;

                
                if (BlackboardComponent)
                {
                    BlackboardComponent->SetValueAsObject(FName("BestBall"), nullptr);
                }

            }
        },
        nullptr,
        [this](AAIController* AIController, const float DeltaTime) -> TSharedPtr<FAivState> {
            if (!BestBall)
            {
                return SearchForBall;
            }

            BestBall->SetActorLocation(FVector(0, 0, 0));
            BestBall->AttachToActor(AIController->GetPawn(), FAttachmentTransformRules::KeepRelativeTransform);
            
            
            if (BlackboardComponent)
            {
                BlackboardComponent->SetValueAsObject(FName("BestBall"), nullptr);
            }
            return GoToPlayer;
        }
    );

    CurrentState = SearchForBall;
    CurrentState->CallEnter(this);
}

void AEnemyAIController::IncrementDeliveredBallsToPlayer()
{
    
    if (BlackboardComponent)
    {
        int32 DeliveredBallsToPlayer = BlackboardComponent->GetValueAsInt(FName("DeliveredBallsToPlayer"));
        DeliveredBallsToPlayer++;
        UE_LOG(LogTemp, Warning, TEXT("BallDelivered ++ : %d" ), DeliveredBallsToPlayer);
        BlackboardComponent->SetValueAsInt(FName("DeliveredBallsToPlayer"), DeliveredBallsToPlayer);

        
        if (DeliveredBallsToPlayer >= 1)
        {
            
            ShootPlayerInAir();
            UE_LOG(LogTemp, Warning, TEXT("Player ha perso!"));
        }
    }
}




void AEnemyAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    bool bHasSpecialBallBool = AAIBehaviourTreeGameCharacter::GetSpecialBallBool();
    DetachSpecialBall(this);

    if (CurrentState)
    {
        CurrentState = CurrentState->CallTick(this, DeltaTime);
    }

    if (BestBall && BestBall->IsPendingKillPending())
    {
        ResetBestBall();
    }

    if (bHasSpecialBallBool && CurrentState != RunAway)
    {
        CurrentState = RunAway;
        CurrentState->CallEnter(this);
        UE_LOG(LogTemp, Warning, TEXT("Is player have specialball: %s"), bHasSpecialBallBool ? TEXT("true") : TEXT("false"));
        
    }
    if (bIsRunningAway)
    {
        UE_LOG(LogTemp, Warning, TEXT("JUMPING IN AIR"));
        bIsRunningAway = false;
        CurrentState = SearchForBall;
        CurrentState->CallEnter(this);
        ShootEnemyInAir(this);
    }
}

void AEnemyAIController::SetBestBall(ABall* Ball)
{
    BestBall = Ball;

    if (BlackboardComponent)
    {
        BlackboardComponent->SetValueAsObject(FName("BestBall"), Ball);
    }
}

void AEnemyAIController::ResetBestBall()
{
    BestBall = nullptr;

    if (BlackboardComponent)
    {
        BlackboardComponent->SetValueAsObject(FName("BestBall"), nullptr);
    }

    
    CurrentState = SearchForBall;
    CurrentState->CallEnter(this);
}


void AEnemyAIController::ShootPlayerInAir()
{
    
    ACharacter* PlayerCharacter = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
    if (PlayerCharacter)
    {
        
        FVector LaunchDirection = FVector(0, 0, 1); 

        
        float LaunchForce = 10000.0f; 
        PlayerCharacter->LaunchCharacter(LaunchDirection * LaunchForce, true, true);
        StartRestartLevelTimer();
        
    }
}

void AEnemyAIController::ShootEnemyInAir(AAIController* AIController)
{
    ACharacter* EnemyCharacter = AIController->GetCharacter();
    
    if (EnemyCharacter)
    {
        
        FVector LaunchDirection = FVector(0, 0, 1); 

        
        float LaunchForce = 10000.0f; 
        EnemyCharacter->LaunchCharacter(LaunchDirection * LaunchForce, true, true);
        StartRestartLevelTimer();

    }
}

void AEnemyAIController::DetachSpecialBall(AAIController* AIController)
{
    AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
    AAIBehaviourTreeGameGameMode* BallGameMode = Cast<AAIBehaviourTreeGameGameMode>(GameMode);
    const TArray<ASpecialBall*>& SpecialBall = BallGameMode->GetSpecialBall();

    AAIBehaviourTreeGameCharacter* PlayerMode = Cast<AAIBehaviourTreeGameCharacter>(GameMode);
    
    

    for (size_t i = 0; i < SpecialBall.Num(); i++)
    {
        if (SpecialBall[i]->GetDistanceTo(AIController->GetPawn()) <= 100.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("SpecialBall Detached"));

            SpecialBall[i]->AttachToActor(AIController->GetPawn(), FAttachmentTransformRules::KeepRelativeTransform);
            bIsRunningAway = true;
            PlayerMode->SetSpecialBallBool(false);
        }
    }
}

void AEnemyAIController::RestartLevel()
{
    UGameplayStatics::OpenLevel(GetWorld(),"ThirdPersonMap");
}

void AEnemyAIController::StartRestartLevelTimer()
{
    GetWorldTimerManager().SetTimer(RestartLevelTimer, this, &AEnemyAIController::RestartLevel, 2.0f, false);
}
