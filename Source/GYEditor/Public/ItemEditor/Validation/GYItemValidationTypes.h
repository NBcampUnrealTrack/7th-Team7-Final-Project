#pragma once

#include "CoreMinimal.h"

enum class EGYItemValidationSeverity : uint8
{
	Info,
	Warning,
	Error,
};

struct FGYItemValidationMessage
{
	EGYItemValidationSeverity Severity = EGYItemValidationSeverity::Info;
	FName RuleId;
	FText Message;
};

struct FGYItemValidationReport
{
	TArray<FGYItemValidationMessage> Messages;

	bool HasAny() const
	{
		return Messages.Num() > 0;
	}

	bool HasSeverity(EGYItemValidationSeverity Severity) const
	{
		for (const FGYItemValidationMessage& Message : Messages)
		{
			if (Message.Severity == Severity)
			{
				return true;
			}
		}
		return false;
	}

	EGYItemValidationSeverity GetMaxSeverity() const
	{
		EGYItemValidationSeverity Max = EGYItemValidationSeverity::Info;
		for (const FGYItemValidationMessage& Message : Messages)
		{
			if (Message.Severity > Max)
			{
				Max = Message.Severity;
			}
		}
		return Max;
	}
};
