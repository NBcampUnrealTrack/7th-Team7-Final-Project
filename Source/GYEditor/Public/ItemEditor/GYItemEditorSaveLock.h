#pragma once

#include "CoreMinimal.h"

class UPackage;

// 아이템 에디터 "모두 저장" 직전의 일괄 락 게이트.
// Poorforce의 락 클라이언트(redis)·LFS 클라이언트·경로 리졸버를 그대로 재사용하고,
// 여기서는 "여러 패키지 일괄 취득 + 실패 시 롤백" 순서만 제어한다.
namespace GYItemEditorSaveLock
{
	struct FBlockedPackage
	{
		FString PackageName;
		FString Reason;
	};

	struct FAcquireResult
	{
		bool bSuccess = true;
		TArray<FBlockedPackage> Blocked;
	};

	// 패키지들의 redis 락 + LFS 락(LockOnly 경로만)을 동기로 일괄 취득한다.
	// - 관리 경로 밖 패키지는 락 없이 통과
	// - 하나라도 실패하면 이번 호출에서 새로 취득한 락을 전부 되돌리고 bSuccess=false (all-or-nothing)
	// - 이미 내가 보유한 락은 TTL만 연장한다
	// - LFS 재락 실패 메시지("Lock exists")엔 소유자 정보가 없어 git lfs locks --verify(ours/theirs)로 판별한다
	// - Poorforce 비활성(설정 없음) 환경에서는 항상 성공
	FAcquireResult TryAcquireForPackages(const TArray<UPackage*>& Packages);
}
