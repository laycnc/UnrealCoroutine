// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UncoObjectGenerator.h"
#include "UncoObjectTask.h"
#include "UncoScheduler.generated.h"

namespace unco
{

	struct FCacheObjectTask
	{

		friend class UUncoScheduler;
		using promise_type = unco::FObjectTaskPromise;

		explicit FCacheObjectTask(std::coroutine_handle<promise_type> p, FWeakObjectPtr InObjectPtr) noexcept
		    : CoroutineHandle(p)
		    , HostObject(InObjectPtr)
		{
		}

		~FCacheObjectTask();

		// コピー禁止+ムーブ禁止
		// メンバ変数に持たせない為
		FCacheObjectTask(const FCacheObjectTask&) = delete;
		void operator=(const FCacheObjectTask&) = delete;
		FCacheObjectTask(FCacheObjectTask&&)    = delete;
		void operator=(FCacheObjectTask&&) = delete;

		bool IsValid() const;

	private:
		std::coroutine_handle<promise_type> CoroutineHandle;
		FWeakObjectPtr                      HostObject;
	};

	struct FDistributedFrameInfo
	{
		FDistributedFrameInfo(FObjectGenerator&& InGenerator, float InFrameTime) noexcept
		    : Generator(std::move(InGenerator))
		    , FrameTime(InFrameTime)
		{
		}

		FObjectGenerator Generator;
		float            FrameTime;
	};

} // namespace unco

/**
 * 
 */
UCLASS()
class UUncoScheduler : public UTickableWorldSubsystem
{
	GENERATED_BODY()

private:
	friend struct unco::FObjectTask;
	friend struct unco::FObjectGenerator;

	// Begin USubsystem

	// サブシステムの初期化
	virtual void Initialize(FSubsystemCollectionBase& Collection);
	// サブシステムの終了
	virtual void Deinitialize();

	// End USubsystem

	// Begin UTickableWorldSubsystem
	virtual void    Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	// End UTickableWorldSubsystem

public:
	/**
	 * @brief コルーチンスケジューラーを取得する
	 * @param InWorldContext ワールドコンテキスト
	 * @return 
	*/
	static UUncoScheduler* Get(const UObject* InWorldContext);

	/**
	 * @brief 分散フレーム実行をリクエストする
	 * @param InWorldContext ワールドコンテキスト
	 * @param InFrameTime 1フレームに実行する時間(sec)
	 * @param Generator 実行するコルーチン
	*/
	static UNREALCOROUTINE_API void DistributedFrame(const UObject*           InWorldContext,
	                                                 float                    InFrameTime,
	                                                 unco::FObjectGenerator&& Generator);

public:
	void RegisterTask(FWeakObjectPtr InObject, std::coroutine_handle<unco::FObjectTaskPromise> InPromise);
	void UnregisterTask(FWeakObjectPtr InObject, std::coroutine_handle<unco::FObjectTaskPromise> InPromise);
private:
	TArray<unco::FCacheObjectTask>      Tasks;
	TArray<unco::FDistributedFrameInfo> DistributedFrameLists;
	TArray<unco::FDistributedFrameInfo> DelayDistributedFrameLists;
	bool                                bIsDistributedFrame = false;
};
