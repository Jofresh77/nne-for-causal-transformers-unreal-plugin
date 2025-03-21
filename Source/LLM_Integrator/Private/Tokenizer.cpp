// Fill out your copyright notice in the Description page of Project Settings.


#include "Tokenizer.h"

#include <fstream>
#include <iostream>

// Inspired from: https://github.com/mlc-ai/tokenizers-cpp/blob/main/example/example.cc
std::string UTokenizer::LoadTokenizerFile(const std::string& RelativePath)
{
	FString AbsolutePath = FPaths::ProjectDir() / FString(RelativePath.c_str());

	std::ifstream fs(TCHAR_TO_UTF8(*AbsolutePath), std::ios::in | std::ios::binary);
	if (fs.fail())
	{
		std::cerr << "Cannot open " << RelativePath << std::endl;
		exit(1);
	}

	fs.seekg(0, std::ios::end);
	size_t size = fs.tellg();
	fs.seekg(0, std::ios::beg);

	std::string data(size, '\0');
	fs.read(data.data(), size);

	return data;
}

void UTokenizer::LoadTokenizer(const std::string& Path)
{
	try
	{
		std::string loadedJsonTokenizer = LoadTokenizerFile(Path);

		LoadedTokenizer = tokenizers::Tokenizer::FromBlobJSON(loadedJsonTokenizer);
		if (!LoadedTokenizer)
		{
			UE_LOG(LogTemp, Error, TEXT("Tokenizer failed to initialize!"));
		}
	}
	catch (const std::exception& e)
	{
		UE_LOG(LogTemp, Error, TEXT("Exception: %s"), UTF8_TO_TCHAR(e.what()));
	}
}

TArray<int64> UTokenizer::EncodeChatPrompt(const TArray<FQwenMessage>& Messages)
{
	FString FormattedPrompt = FormatChatPrompt(Messages);
	return EncodeString(FormattedPrompt);
}

TArray<int64> UTokenizer::EncodeString(const FString& Input)
{
	try
	{
		std::string InputStr = TCHAR_TO_UTF8(*Input);

		UE_LOG(LogTemp, Warning, TEXT("Input string to encode: %s"), *Input);

		std::vector<int> TokenIds;
		try
		{
			TokenIds = LoadedTokenizer->Encode(InputStr);
		}
		catch (const std::exception& e)
		{
			UE_LOG(LogTemp, Error, TEXT("Exception during tokenization: %s"), *FString(e.what()));
			return TArray<int64>();
		}

		TArray<int64> Result;
		for (int Id : TokenIds)
		{
			Result.Add(Id);
		}

		UE_LOG(LogTemp, Warning, TEXT("Input Token IDs: %s"),
		       *FString::JoinBy(Result, TEXT(" "), [](int32 Id) { return FString::FromInt(Id); }));

		return Result;
	}
	catch (const std::exception& e)
	{
		UE_LOG(LogTemp, Error, TEXT("Exception: %s"), UTF8_TO_TCHAR(e.what()));
	}

	return TArray<int64>();
}

FString UTokenizer::FormatChatPrompt(const TArray<FQwenMessage>& Messages)
{
	FString FormattedPrompt;

	for (const FQwenMessage& Message : Messages)
	{
		if (Message.Role == TEXT("system"))
		{
			FormattedPrompt.Append(FString::Printf(TEXT("<|im_start|>system\n%s<|im_end|>\n"), *Message.Content));
		}
		else if (Message.Role == TEXT("user"))
		{
			FormattedPrompt.Append(FString::Printf(TEXT("<|im_start|>user\n%s<|im_end|>\n"), *Message.Content));
		}
		else if (Message.Role == TEXT("assistant"))
		{
			FormattedPrompt.Append(FString::Printf(TEXT("<|im_start|>assistant\n%s<|im_end|>\n"), *Message.Content));
		}
	}

	FormattedPrompt.Append(FString::Printf(TEXT("<|im_start|>assistant")));

	return FormattedPrompt;
}

FString UTokenizer::DecodeModelOutput(const TArray<int64>& outputTokenIds)
{
	try
	{
		std::vector<int32_t> stdVector(outputTokenIds.GetData(), outputTokenIds.GetData() + outputTokenIds.Num());
		return LoadedTokenizer->Decode(stdVector).data();
	}
	catch (const std::exception& e)
	{
		UE_LOG(LogTemp, Error, TEXT("Exception: %s"), UTF8_TO_TCHAR(e.what()));
	}
}

FQwenMessage UTokenizer::CreateMessage(const FString& Role, const FString& Content)
{
	return FQwenMessage(Role, Content);
}

int32 UTokenizer::GetVocabSize()
{
	return LoadedTokenizer->GetVocabSize();
}
