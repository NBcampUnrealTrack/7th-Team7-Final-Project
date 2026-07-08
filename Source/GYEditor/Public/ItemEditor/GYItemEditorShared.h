#pragma once

#include "ItemEditor/Validation/GYItemValidationTypes.h"
#include "Styling/AppStyle.h"

namespace GYItemEditorUI
{
	inline const FSlateBrush* GetSeverityIcon(EGYItemValidationSeverity Severity)
	{
		switch (Severity)
		{
		case EGYItemValidationSeverity::Error:
			return FAppStyle::GetBrush("Icons.ErrorWithColor");
		case EGYItemValidationSeverity::Warning:
			return FAppStyle::GetBrush("Icons.WarningWithColor");
		default:
			return FAppStyle::GetBrush("Icons.InfoWithColor");
		}
	}

	inline const FSlateBrush* GetPassIcon()
	{
		return FAppStyle::GetBrush("Icons.SuccessWithColor");
	}
}
