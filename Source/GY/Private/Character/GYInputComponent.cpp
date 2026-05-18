// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/GYInputComponent.h"


UGYInputComponent::UGYInputComponent(const FObjectInitializer& ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

// Called when the game starts
void UGYInputComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...

}

