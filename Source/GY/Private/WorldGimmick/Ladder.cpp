// Fill out your copyright notice in the Description page of Project Settings.

#include "WorldGimmick/Ladder.h"

ALadder::ALadder()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* RootScene = CreateDefaultSubobject<USceneComponent>("Scene");
	RootComponent = RootScene;

	CreateCollision();
}

void ALadder::CreateCollision()
{
	TopBoxCollision = CreateDefaultSubobject<UBoxComponent>("TopBox");
	TopBoxCollision->SetupAttachment(RootComponent);

	PlayerClimbCheckCollision = CreateDefaultSubobject<UBoxComponent>("ClimbCheck");
	PlayerClimbCheckCollision->SetupAttachment(RootComponent);

	BottomBoxCollision = CreateDefaultSubobject<UBoxComponent>("BottomBox");
	BottomBoxCollision->SetupAttachment(RootComponent);
}

// 에디터에서 액터의 transform이 변할때마다 호출됨. 액터 변경시마다 생성자의 초기화처럼 작동함.
// 게임 실행중 런타임중에는 작동하지 않음
void ALadder::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	CreateLadder();
}

void ALadder::CreateLadder()
{
	CreateCheckText();
	WallCheck();

	DatatableSetup();
	if (LadderMesh == nullptr)
	{
		return;
	}

	CalculatePole();
	CreateRungs();
	CreatePole();
	CreateWallConnection();
	CreateTopLadder();
	UpdateCollision();
}

void ALadder::CreateCheckText()
{
	TextRender = NewObject<UTextRenderComponent>(this);
	if (TextRender == nullptr)
	{
		return;
	}

	TextRender->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	TextRender->SetText(CheckMessageText);

	TextRender->SetRelativeLocation(FVector(0, 0, LadderHeight / 2));
	TextRender->SetupAttachment(RootComponent);
	TextRender->RegisterComponent();
}

void ALadder::WallCheck()
{
	if (TextRender == nullptr)
	{
		return;
	}

	FHitResult HitResult;
	const FVector StartLocation = GetActorLocation() + FVector(0, 0, 50);
	const FVector Backward = GetRootComponent()->GetForwardVector() * -1;
	const FVector EndLocation = StartLocation + (Backward * 30);

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);

	bool bOnHit = GetWorld()->LineTraceSingleByChannel(
		HitResult, StartLocation, EndLocation,
		ECC_Visibility, CollisionParams);

	bWrongPlace = !bOnHit;

	TextRender->SetVisibility(bWrongPlace);
}


void ALadder::DatatableSetup()
{
	const FLadderTypeDataTableRow* LadderData = LadderDataHandle.GetRow<FLadderTypeDataTableRow>(TEXT("Rung Setup"));

	if (LadderData == nullptr)
	{
		return;
	}

	LadderMesh = LadderData;
	RungsMesh = LadderMesh->Rungs;
	PoleMesh = LadderMesh->SideRungs;
	TopLadderMesh = LadderMesh->TopLadder;
	WallConnectionMesh = LadderMesh->WallConnection;
}

void ALadder::AddStaticMesh(UStaticMesh* Mesh, const int32 IndexNumber, int32 MeshOffset)
{
	//NewObject는 GC에 들어가서 따로 해제 안해줘도 됨.
	UStaticMeshComponent* MeshPart = NewObject<UStaticMeshComponent>(this);
	if (MeshPart == nullptr)
	{
		return;
	}

	// 메시에 construction script 태그 추가. OnConstruction으로 셍성시 (에디터에서 움직일때마다 동적생성) 기존 메시를 자동으로 지워주는 태그부착.
	// 이 태그가 없으면 움직일때마다 메시가 계속 추가됨.
	MeshPart->CreationMethod = EComponentCreationMethod::UserConstructionScript;

	// 1. 메시 세팅
	MeshPart->SetRelativeLocation(FVector(0, 0, IndexNumber * MeshOffset));
	MeshPart->SetStaticMesh(Mesh);

	// 2. 부착부모 예약
	// SetupAttachment는 초기화시 / AttachTo는 이미 등록후 런타임중 사용.
	// OnConstruction에서 NewObject로 동적생성시 초기화로 취급함.
	MeshPart->SetupAttachment(RootComponent);
	//RungMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	// 3. 세팅 완료후 월드 등록 -> 2,3,1 순서로 하면 레지스터 파괴후 재계산, 231->3 다시 실행함.
	MeshPart->RegisterComponent();

	FVector MaterialColor;
	if (bWrongPlace == false)
	{
		MaterialColor = MaterialParameterColor;
	}
	else
	{
		MaterialColor = MaterialParameterErrorColor;
	}
	MeshPart->SetVectorParameterValueOnMaterials(MaterialParameterName, MaterialColor);
}

void ALadder::CalculatePole()
{
	// 봉 메시 높이 가져오기
	if (LadderMesh->SideRungs == nullptr)
	{
		return;
	}
	const FBoxSphereBounds Bounds = LadderMesh->SideRungs->GetBounds();
	const double BoundsZSize = Bounds.BoxExtent.Z * 2.0;

	PoleMeshHeight = FMath::TruncToInt(BoundsZSize);
}

void ALadder::CreateRungs()
{
	if (RungsMesh == nullptr || RungsOffset == 0)
	{
		return;
	}

	LastRungsIndex = (LadderHeight / RungsOffset) + 1;

	for (int32 i = 1; i <= LastRungsIndex; i++)
	{
		AddStaticMesh(RungsMesh, i, RungsOffset);
	}
}

void ALadder::CreatePole()
{
	if (PoleMesh == nullptr || PoleMeshHeight == 0)
	{
		return;
	}

	const int32 LastPoleIndex = (LastRungsIndex * RungsOffset / PoleMeshHeight) + PoleOverStep;

	for (int32 i = 0; i <= LastPoleIndex; i++)
	{
		AddStaticMesh(PoleMesh, i, PoleMeshHeight);
	}

	LastPoleLocation = LastPoleIndex * PoleMeshHeight;
}

void ALadder::CreateWallConnection()
{
	if (WallConnectionMesh == nullptr || WallConnectionStep == 0)
	{
		return;
	}

	const int32 WallOffset = LadderHeight / WallConnectionStep;

	for (int32 i = 1; i <= WallConnectionStep; i++)
	{
		AddStaticMesh(WallConnectionMesh, i, WallOffset);
	}
}

void ALadder::CreateTopLadder()
{
	if (TopLadderMesh == nullptr || bNeedTopLadder == false)
	{
		return;
	}

	AddStaticMesh(TopLadderMesh, 1, LastPoleLocation);
}


void ALadder::UpdateCollision()
{
	if (PlayerClimbCheckCollision == nullptr)
	{
		return;
	}

	const double HalfHeight = LadderHeight / 2.0;

	PlayerClimbCheckCollision->SetBoxExtent(FVector(25.f, 25.f, HalfHeight), true);
	PlayerClimbCheckCollision->SetRelativeLocation(FVector(32.f, 0.f, HalfHeight));


	float TopBoxLocation = LadderHeight + TopBoxCollisionOffset;
	TopBoxCollision->SetRelativeLocation(FVector(0, 0, TopBoxLocation));
}
