// Fill out your copyright notice in the Description page of Project Settings.

#include "UncoScheduler.h"

#include "UnrealCoroutine.h"
#include "UnrealEngine.h"

DECLARE_CYCLE_STAT(TEXT("Unco_DistributedFramePhase"),
                   STAT_DistributedFramePhase,
                   STATGROUP_Unco);
DECLARE_CYCLE_STAT(TEXT("Unco_DistributedFrame"),
                   STAT_DistributedFrame,
                   STATGROUP_Unco);

namespace unco
{
	FCacheObjectTask::~FCacheObjectTask()
	{
		if ( CoroutineHandle )
		{
			CoroutineHandle.destroy();
		}
		CoroutineHandle = nullptr;
	}

	bool FCacheObjectTask::IsValid() const
	{
		if ( CoroutineHandle )
		{
			return true;
		}
		return false;
	}

} // namespace unco

// Begin USubsystem

// サブシステムの初期化
void UUncoScheduler::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

// サブシステムの終了
void UUncoScheduler::Deinitialize()
{
	Super::Deinitialize();

	Tasks.Empty();
}

// End USubsystem

// Begin UTickableWorldSubsystem

void UUncoScheduler::Tick(float DeltaTime)
{
	// フレーム分散が存在している場合実行
	if ( DistributedFrameLists.Num() > 0 )
	{
		SCOPE_CYCLE_COUNTER(STAT_DistributedFramePhase);

		bIsDistributedFrame = true;

		float ElapsedTimeArray[100] = {};
		int32 index                 = 0;

		//
		for ( unco::FDistributedFrameInfo& FrameInfo : DistributedFrameLists )
		{
			SCOPE_CYCLE_COUNTER(STAT_DistributedFrame);

			unco::FObjectGenerator& Generator   = FrameInfo.Generator;
			const float             FrameTime   = FrameInfo.FrameTime;
			const FDateTime         StartTime   = FDateTime::Now();
			float                   ElapsedTime = 0.0f;

			do
			{
				// コルーチンの内部処理を実行
				Generator.MoveNext();

				// 経過時間を保存
				ElapsedTime = static_cast<float>(
				    (FDateTime::Now() - StartTime).GetTotalMilliseconds());

				// コルーチンが終わっている or 経過時間が指定時間を超えるかチェックする
			} while ( !Generator.Done() && ElapsedTime < FrameTime );
		}

		// 分散フレームが終了したものを削除
		DistributedFrameLists.RemoveAll(
		    [&](const unco::FDistributedFrameInfo& Cache)
		    {
			    // コルーチンが終了した物
			    if ( Cache.Generator.Done() )
			    {
				    return true;
			    }
			    // 呼び出し元のオブジェクトが破棄されたか？
			    if ( !Cache.Generator.IsValidObject() )
			    {
				    return true;
			    }

			    return false;
		    });

		// コルーチン実行中に追加された物はループ後に追加する
		for ( unco::FDistributedFrameInfo& Info : DelayDistributedFrameLists )
		{
			DistributedFrameLists.Emplace(std::move(Info));
		}

		DelayDistributedFrameLists.Empty();

		// フラグを無効化する
		bIsDistributedFrame = false;
	}
}

TStatId UUncoScheduler::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UUncoScheduler, STATGROUP_Tickables);
}

// End UTickableWorldSubsystem

UUncoScheduler* UUncoScheduler::Get(const UObject* InWorldContext)
{
	const UWorld* World = GEngine->GetWorldFromContextObject(
	    InWorldContext, EGetWorldErrorMode::ReturnNull);
	if ( IsValid(World) )
	{
		return Cast<UUncoScheduler>(
		    World->GetSubsystemBase(UUncoScheduler::StaticClass()));
	}
	return nullptr;
}

/**
	 * @brief 分散フレーム実行をリクエストする
	 * @param InWorldContext ワールドコンテキスト
	 * @param InFrameTime 1フレームに実行する時間(sec)
	 * @param Generator 実行するコルーチン
	*/
void UUncoScheduler::DistributedFrame(const UObject*           InWorldContext,
                                      float                    InFrameTime,
                                      unco::FObjectGenerator&& Generator)
{
	UUncoScheduler* Scheduler = Get(InWorldContext);
	if ( IsValid(InWorldContext) )
	{
		if ( Scheduler->bIsDistributedFrame )
		{
			// 実行中に追加はさせたくないので遅延で追加させる
			Scheduler->DelayDistributedFrameLists.Emplace(std::move(Generator),
			                                              InFrameTime);
		}
		else
		{
			Scheduler->DistributedFrameLists.Emplace(std::move(Generator),
			                                         InFrameTime);
		}
	}
}

void UUncoScheduler::RegisterTask(
    FWeakObjectPtr                                  InObject,
    std::coroutine_handle<unco::FObjectTaskPromise> InPromise)
{
	// 念の為事前に終わっているものを削除しておく
	Tasks.RemoveAll(
	    [](const unco::FCacheObjectTask& Cache)
	    {
		    return !Cache.IsValid();
	    });

	Tasks.Emplace(InPromise, InObject);
}

void UUncoScheduler::UnregisterTask(
    FWeakObjectPtr                                  InObject,
    std::coroutine_handle<unco::FObjectTaskPromise> InPromise)
{
	// 念の為事前に終わっているものを削除しておく
	Tasks.RemoveAll(
	    [&](const unco::FCacheObjectTask& Cache)
	    {
		    return Cache.HostObject == InObject &&
		           Cache.CoroutineHandle == InPromise;
	    });
}