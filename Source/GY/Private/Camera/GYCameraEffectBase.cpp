#include "Camera/GYCameraEffectBase.h"


void UGYCameraEffectBase::Initialize(const FGYCameraEffectContext& InContext)
{
	Context = InContext;
	ElapsedTime=0.f;
}

void UGYCameraEffectBase::UpdateEffect(float DeltaTime, FGYCameraView& InOutView)
{
	ElapsedTime += DeltaTime;
}

bool UGYCameraEffectBase::IsFinished() const
{
	return ElapsedTime >= Context.Duration;
}
