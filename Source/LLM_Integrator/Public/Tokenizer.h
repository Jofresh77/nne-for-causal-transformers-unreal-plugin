// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

THIRD_PARTY_INCLUDES_START
#include "tokenizers_cpp.h"
THIRD_PARTY_INCLUDES_END

#include "Tokenizer.generated.h"

class LLM_INTEGRATOR_API FQwenMessage
{
public:
	FString Role;
	FString Content;
    
	FQwenMessage(const FString& InRole, const FString& InContent)
		: Role(InRole), Content(InContent) {}
};

/*
 * The tokenization process was designed based on Alibaba Qwen's tokenization
 * https://qwen.readthedocs.io/en/latest/getting_started/concepts.html#tokens-tokenization
 */
UCLASS()
class LLM_INTEGRATOR_API UTokenizer : public UObject
{
	GENERATED_BODY()
	
	std::unique_ptr<tokenizers::Tokenizer> LoadedTokenizer;

	FString SystemMessage = TEXT("You are Qwen, created by Alibaba Cloud. You are a helpful assistant.");

public:
	UFUNCTION(BlueprintCallable, Category = "Tokenizer")
	TArray<int64> EncodeString(const FString& Input);
	
	FString FormatChatPrompt(const TArray<FQwenMessage>& Messages);
	TArray<int64> EncodeChatPrompt(const TArray<FQwenMessage>& Messages);
	FString DecodeModelOutput(const TArray<int64>& outputTokenIds);
	static FQwenMessage CreateMessage(const FString& Role, const FString& Content);
	void LoadTokenizer(const std::string& Path);
	int32 GetVocabSize();

private:
	std::string LoadTokenizerFile(const std::string& Path);
};
