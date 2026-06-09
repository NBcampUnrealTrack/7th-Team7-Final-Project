// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "LadderTypeDataTableRow.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Ladder.generated.h"

UCLASS()
class GY_API ALadder : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ALadder();

	virtual void OnConstruction(const FTransform& Transform) override;

	// 콜리전 세팅
public:
	void CreateCollision();

	// 사다리 간격 세팅
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder", meta = (RowType = "LadderTypeDataTableRow"))
	FDataTableRowHandle LadderDataHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder")
	int32 LadderHeight = 250;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder")
	int32 RungsOffset = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder")
	int32 PoleOverStep = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder")
	int32 WallConnectionStep = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder")
	bool bNeedTopLadder = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder|Check")
	TObjectPtr<UTextRenderComponent> TextRender;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder|Check")
	FText CheckMessageText = FText::FromString(TEXT("Wrong Place"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder|Check")
	FName MaterialParameterName = FName(TEXT("Color"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder|Check")
	FVector MaterialParameterColor = FVector(1,1,1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder|Check")
	FVector MaterialParameterErrorColor = FVector(1,0,0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder|Collision")
	int32 TopBoxCollisionOffset = 32;


	// 데이터 테이블 메시 세팅
public:
	const FLadderTypeDataTableRow* LadderMesh = nullptr;

	UPROPERTY()
	TObjectPtr<UStaticMesh> RungsMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> PoleMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> TopLadderMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> WallConnectionMesh;

	// 콜리전
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder|Collision")
	TObjectPtr<UBoxComponent> TopBoxCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder|Collision")
	TObjectPtr<UBoxComponent> PlayerClimbCheckCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder|Collision")
	TObjectPtr<UBoxComponent> BottomBoxCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder|Collision")
	TObjectPtr<UBoxComponent> ClimbIntoFromTopBoxCollision;




	// 사다리생성함수
public:
	void CreateLadder();
	void CreateCheckText();
	void WallCheck();

	void DatatableSetup();
	void AddStaticMesh(UStaticMesh* Mesh, const int32 IndexNumber = 0, int32 MeshOffset = 1);
	void CalculatePole();
	void CreateRungs();
	void CreatePole();
	void CreateWallConnection();
	void CreateTopLadder();
	void UpdateCollision();

protected:
	int32 PoleMeshHeight;
	int32 LastRungsIndex;
	int32 LastPoleLocation;
	bool bWrongPlace = false;
};

