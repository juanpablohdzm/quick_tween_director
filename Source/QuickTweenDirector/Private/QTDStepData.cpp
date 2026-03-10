// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#include "QTDStepData.h"

FLinearColor FQTDStepData::GetTypeColor() const
{
	switch (StepType)
	{
		case EQTDStepType::Vector:
			switch (VectorProperty)
			{
				case EQTDVectorProperty::WorldScale3D:
				case EQTDVectorProperty::RelativeScale3D:
					return FLinearColor(1.0f, 0.62f, 0.11f); // amber – scale
				default:
					return FLinearColor(1.0f, 0.42f, 0.21f); // orange – location
			}
		case EQTDStepType::Rotator:
			return FLinearColor(0.18f, 0.77f, 0.71f); // teal
		case EQTDStepType::Float:
			return FLinearColor(0.23f, 0.53f, 1.0f);  // blue
		case EQTDStepType::LinearColor:
			return FLinearColor(0.51f, 0.22f, 0.93f); // purple
		case EQTDStepType::Vector2D:
			return FLinearColor(0.20f, 0.80f, 0.40f); // green
		case EQTDStepType::Int:
			return FLinearColor(1.00f, 0.29f, 0.42f); // coral
		case EQTDStepType::Empty:
		default:
			return FLinearColor(0.42f, 0.46f, 0.49f); // gray
	}
}
