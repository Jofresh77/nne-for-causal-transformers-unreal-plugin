#pragma once

#include "CoreMinimal.h"
#include "NNERuntimeCPU.h"
#include "NNEModelData.h"
#include "Tokenizer.h"
#include "GameFramework/Actor.h"
#include "InferencerQwenInstruct.generated.h"

struct FInferenceState
{
	TArray<int64> InputIDs;
	TArray<int64> AttentionMask;
	TArray<int64> PositionIDs;
	int32 CurrentLength;
	bool bIsGenerating;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInferenceCompleteDelegate, const FString&, OutputText);

UCLASS()
class LLM_INTEGRATOR_API AInferencerQwenInstruct : public AActor
{
	GENERATED_BODY()

public:
	AInferencerQwenInstruct();

	UPROPERTY(EditAnywhere, Category = "Model")
	TSoftObjectPtr<UNNEModelData> ModelDataAsset;

	UPROPERTY(EditAnywhere, Category = "Generation")
	int32 MaxGenerationLength = 2048;

	UPROPERTY(EditAnywhere, Category = "Generation", meta = (ClampMin = 0.1, ClampMax = 2.0))
	float Temperature = 0.7f;

	UPROPERTY(EditAnywhere, Category = "Generation", meta = (ClampMin = 0))
	int32 TopK = 20;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnInferenceCompleteDelegate OnInferenceComplete;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Inferencing")
	void ProcessChat(const FString& UserMessage);

private:
	TArray<int64> TokenIds;
	
	TSharedPtr<UE::NNE::IModelInstanceCPU> ModelInstance;
	TSharedPtr<UE::NNE::IModelCPU> Model;
	TWeakInterfacePtr<INNERuntimeCPU> Runtime;
    
	UPROPERTY()
	UTokenizer* Tokenizer;

	FInferenceState CurrentState;
	FCriticalSection StateMutex;
	bool bModelReady;

	void InitializeModel();
	void PrepareInputTensors(const TArray<int64>& TokenIDs);
	void RunGenerationStep();
	int32 SampleNextToken(const TArray<float>& Logits);
	void HandleEOS();
};