#include "InferencerQwenInstruct.h"

#include "Engine/AssetManager.h"
#include "Async/Async.h"
#include "NNE.h"
#include "Tokenizer.h"

const int32 EOS_TOKEN_ID = 151643;
const int32 MAX_SEQ_LENGTH = 2048;

AInferencerQwenInstruct::AInferencerQwenInstruct(): CurrentState()
{
	PrimaryActorTick.bCanEverTick = true;
	bModelReady = false;
}

void AInferencerQwenInstruct::BeginPlay()
{
	Super::BeginPlay();

	if (!Tokenizer)
	{
		Tokenizer = NewObject<UTokenizer>();
		if (!Tokenizer)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create Tokenizer!"));
			return;
		}
		Tokenizer->AddToRoot();
		Tokenizer->LoadTokenizer("Plugins/LLM_Integrator/Source/LLM_Integrator/Tokenizers/tokenizer-qwen-02.json");
	}

	if (ModelDataAsset.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("Model data asset not set!"));
		return;
	}

	UAssetManager::GetStreamableManager().RequestAsyncLoad(ModelDataAsset.ToSoftObjectPath(),
	                                                       FStreamableDelegate::CreateUObject(
		                                                       this, &AInferencerQwenInstruct::InitializeModel));
}

void AInferencerQwenInstruct::InitializeModel()
{
	Runtime = UE::NNE::GetRuntime<INNERuntimeCPU>(TEXT("NNERuntimeORTCpu"));

	if (!Runtime.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get NNE runtime"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Can%hs create model"),
	       Runtime->CanCreateModelCPU(ModelDataAsset.Get()) == UE::NNE::EResultStatus::Ok ? "" : "not");


	Model = Runtime->CreateModelCPU(ModelDataAsset.Get());
	if (!Model.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create model"));
		return;
	}

	ModelInstance = Model->CreateModelInstanceCPU();
	if (!ModelInstance.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create model instance"));
		return;
	}

	bModelReady = true;

	ProcessChat(FString("Hello how are you ?"));
	
	UE_LOG(LogTemp, Log, TEXT("Model initialized successfully"));
}

void AInferencerQwenInstruct::ProcessChat(const FString& UserMessage)
{
	if (!bModelReady) return;

	TArray<FQwenMessage> Messages;
	Messages.Add(UTokenizer::CreateMessage(TEXT("system"),
	                                       "You are Qwen, created by Alibaba Cloud. You are a helpful assistant."));
	Messages.Add(UTokenizer::CreateMessage(TEXT("user"), UserMessage));

	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, Messages]()
	{
		TArray<int64> TokenIds = Tokenizer->EncodeChatPrompt(Messages);
		FString Decoded = Tokenizer->DecodeModelOutput(TokenIds);
		
		UE_LOG(LogTemp, Log, TEXT("Decoded token ids: %s"), *Decoded);

		AsyncTask(ENamedThreads::AnyNormalThreadNormalTask, [this, TokenIds]()
		{
			FScopeLock Lock(&StateMutex);
			CurrentState = FInferenceState();
			PrepareInputTensors(TokenIds);
			RunGenerationStep();
		});
	});
}

void AInferencerQwenInstruct::PrepareInputTensors(const TArray<int64>& TokenIDs)
{
	TConstArrayView<UE::NNE::FTensorDesc> InputTensorDescs = ModelInstance->GetInputTensorDescs();
	
	UE_LOG(LogTemp, Warning, TEXT("Expected number of input tensors: %d"), InputTensorDescs.Num());
	for (auto Desc : InputTensorDescs)
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("Expected input tensor shape of %s: %hs (rank: %d & dimension: %d) with type %s and byte size %d"),
		       *Desc.GetName(),
		       Desc.GetShape().IsConcrete() ? "Concrete" : "Not concrete",
		       Desc.GetShape().Rank(),
		       Desc.GetShape().GetData().Num(),
		       *StaticEnum<ENNETensorDataType>()->GetNameStringByValue(static_cast<int64>(Desc.GetDataType())),
		       Desc.GetElementByteSize());
	}

	const int32 NumTokens = FMath::Min(TokenIDs.Num(), MAX_SEQ_LENGTH);

	CurrentState.InputIDs.SetNumZeroed(NumTokens);
	CurrentState.AttentionMask.SetNumZeroed(NumTokens);
	CurrentState.PositionIDs.SetNumZeroed(NumTokens);

	for (int32 i = NumTokens; i < MAX_SEQ_LENGTH; i++)
	{
		CurrentState.InputIDs[i] = 0; //TODO: MAYBE GIVE THE ACTUAL TOKEN IDS ??????? WTF
		CurrentState.AttentionMask[i] = 0;
		CurrentState.PositionIDs[i] = 0;
	}

	CurrentState.CurrentLength = NumTokens;
	CurrentState.bIsGenerating = true;
}

void AInferencerQwenInstruct::RunGenerationStep()
{
	if (!CurrentState.bIsGenerating || CurrentState.CurrentLength >= MAX_SEQ_LENGTH) return;

	TArray<UE::NNE::FTensorShape> InputShapes;
	InputShapes.SetNum(3);

	TArray<uint32> ShapeData = {1, static_cast<uint32>(CurrentState.CurrentLength)};

	for (int32 i = 0; i < 3; i++)
	{
		InputShapes[i] = UE::NNE::FTensorShape::Make(ShapeData);
	}

	UE::NNE::EResultStatus ShapeStatus = ModelInstance->SetInputTensorShapes(InputShapes);
	if (ShapeStatus != UE::NNE::EResultStatus::Ok)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to set input tensor shapes"));
		return;
	}

	//Model input bindings
	TArray<UE::NNE::FTensorBindingCPU> InputBindings;
	InputBindings.SetNum(3);

	InputBindings[0].Data = CurrentState.InputIDs.GetData();
	InputBindings[0].SizeInBytes = CurrentState.CurrentLength * sizeof(int64);

	InputBindings[1].Data = CurrentState.AttentionMask.GetData();
	InputBindings[1].SizeInBytes = CurrentState.CurrentLength * sizeof(int64);

	InputBindings[2].Data = CurrentState.PositionIDs.GetData();
	InputBindings[2].SizeInBytes = CurrentState.CurrentLength * sizeof(int64);

	//Model output bindings
	TArray<float> OutputLogits;
	OutputLogits.SetNumZeroed(CurrentState.CurrentLength * Tokenizer->GetVocabSize());

	TArray<UE::NNE::FTensorBindingCPU> OutputBindings;
	OutputBindings.SetNum(1);
	OutputBindings[0].Data = OutputLogits.GetData();
	OutputBindings[0].SizeInBytes = OutputLogits.Num() * sizeof(float);

	// Run inference
	UE::NNE::EResultStatus Status = ModelInstance->RunSync(InputBindings, OutputBindings);
	if (Status != UE::NNE::EResultStatus::Ok)
	{
		UE_LOG(LogTemp, Error, TEXT("Inference failed"));
		return;
	}

	// Process output
	const int32 NextToken = SampleNextToken(OutputLogits);

	TokenIds.Add(NextToken);
	const FString Output = Tokenizer->DecodeModelOutput(TokenIds);
	
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Output);

	if (NextToken == EOS_TOKEN_ID || CurrentState.CurrentLength >= MaxGenerationLength)
	{
		HandleEOS();
		return;
	}

	// Update state for next token
	FScopeLock Lock(&StateMutex);
	CurrentState.InputIDs.Add(NextToken);
	CurrentState.AttentionMask.Add(0); //decoder-only models need the future to be masked
	CurrentState.PositionIDs.Add(CurrentState.CurrentLength);
	CurrentState.CurrentLength++;

	// Schedule next generation step
	AsyncTask(ENamedThreads::AnyNormalThreadNormalTask, [this]()
	{
		RunGenerationStep();
	});
}

int32 AInferencerQwenInstruct::SampleNextToken(const TArray<float>& Logits)
{
	UE_LOG(LogTemp, Warning, TEXT("Input length: %d | Output length: %d"), CurrentState.InputIDs.Num(),
	       Logits.Num() / Tokenizer->GetVocabSize());

	const int32 StartIdx = (CurrentState.CurrentLength - 1) * Tokenizer->GetVocabSize();
	TArrayView<const float> StepLogits(&Logits[StartIdx], Tokenizer->GetVocabSize());

	for (int32 i = 0; i < 5; i++)
	{
		UE_LOG(LogTemp, Warning, TEXT("%f"), StepLogits[i]);
	}

	TArray<TPair<float, int32>> Probabilities;
	Probabilities.Reserve(Tokenizer->GetVocabSize());

	// Apply temperature
	for (int32 i = 0; i < StepLogits.Num(); i++)
	{
		Probabilities.Emplace(FMath::Exp(StepLogits[i] / Temperature), i);
	}

	// Top-K filtering
	if (TopK > 0 && TopK < Probabilities.Num())
	{
		Probabilities.Sort([](const TPair<float, int32>& A, const TPair<float, int32>& B)
		{
			return A.Key > B.Key;
		});
		Probabilities.SetNum(TopK);
	}

	// Normalize and sample
	float Sum = 0.0f;
	for (const auto& Pair : Probabilities)
	{
		Sum += Pair.Key;
	}

	//const float Threshold = 0.8f;
	const float Threshold = FMath::FRand() * Sum;
	float Accumulator = 0.0f;

	for (const auto& Pair : Probabilities)
	{
		Accumulator += Pair.Key;
		if (Accumulator >= Threshold)
		{
			return Pair.Value;
		}
	}

	return Probabilities.Last().Value;
}

void AInferencerQwenInstruct::HandleEOS()
{
	FScopeLock Lock(&StateMutex);
	CurrentState.bIsGenerating = false;

	TArray<int64> GeneratedTokens;
	for (int64 i = 0; i < CurrentState.CurrentLength; i++)
	{
		GeneratedTokens.Add(CurrentState.InputIDs[i]);
	}

	FString OutputText = Tokenizer->DecodeModelOutput(GeneratedTokens);
	OnInferenceComplete.Broadcast(OutputText);
}

void AInferencerQwenInstruct::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	FScopeLock Lock(&StateMutex);
	ModelInstance.Reset();
	Model.Reset();
}
