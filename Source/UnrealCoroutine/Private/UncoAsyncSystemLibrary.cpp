// Fill out your copyright notice in the Description page of Project Settings.

#include "UncoAsyncSystemLibrary.h"

#include "Engine/LatentActionManager.h"
#include "Engine/StreamableManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LatentActions.h"
#include "UObject/WeakObjectPtr.h"

namespace unco::details
{

	////////////////////////////////////////////////////////
	// FLoadAssetAwaiterBase

	/**
 * @KismetSystemLibrary.cpp
 * FLoadAssetActionBaseから引用
 */
	class FLoadAssetAwaiterBase::FDelayAction : public FPendingLatentAction
	{
	public:
		FSoftObjectPath               SoftObjectPath;
		FStreamableManager            StreamableManager;
		TSharedPtr<FStreamableHandle> Handle;

		FLoadAssetAwaiterBase*  Awaiter;
		std::coroutine_handle<> CoroutineHandle;

		FDelayAction(const FSoftObjectPath&  InSoftObjectPath,
		             FLoadAssetAwaiterBase*  InAwaiter,
		             std::coroutine_handle<> InCoroutineHandle)
		    : SoftObjectPath(InSoftObjectPath)
		    , StreamableManager()
		    , Handle()
		    , Awaiter(InAwaiter)
		    , CoroutineHandle(InCoroutineHandle)
		{
			Handle = StreamableManager.RequestAsyncLoad(SoftObjectPath);
		}

		~FDelayAction()
		{
			if ( Handle.IsValid() )
			{
				Handle->ReleaseHandle();
			}
		}

		virtual void UpdateOperation(FLatentResponse& Response) override
		{
			const bool bLoaded = !Handle.IsValid() || Handle->HasLoadCompleted() ||
			                     Handle->WasCanceled();
			if ( bLoaded && Awaiter )
			{
				UObject* LoadedObject = SoftObjectPath.ResolveObject();
				Awaiter->ResultObject = LoadedObject;
				// コルーチンを再開
				CoroutineHandle.resume();
			}
			Response.DoneIf(bLoaded);
		}
	};

	FLoadAssetAwaiterBase::FLoadAssetAwaiterBase(const UObject*  InWorldContext,
	                                             FSoftObjectPath InAsset)
	    : WorldContext(InWorldContext)
	    , Asset(InAsset)
	    , Action()
	{
	}
	FLoadAssetAwaiterBase::~FLoadAssetAwaiterBase()
	{
		if ( Action )
		{
			Action->Awaiter = nullptr;
		}
		Action = nullptr;
	}

	bool FLoadAssetAwaiterBase::await_ready() const noexcept
	{
		// エラー
		const bool bResult = Asset.IsNull();
		ensureAlwaysMsgf(!bResult, TEXT("Asset Null"));
		return bResult;
	}

	void FLoadAssetAwaiterBase::await_suspend(std::coroutine_handle<> coroutine)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(
		    WorldContext.Get(), EGetWorldErrorMode::LogAndReturnNull);
		if ( !IsValid(World) )
		{
			return;
		}
		FLatentActionManager& LatentManager = World->GetLatentActionManager();

		UObject* CallbackTarget = WorldContext.Get();
		// 識別用のUUIDをthisポインタから作成する
		const intptr_t uuidptr = reinterpret_cast<intptr_t>(this);
		const int32    UUID    = static_cast<int32>(uuidptr);

		// We always spawn a new load even if this node already queued one, the outside node handles this case
		FDelayAction* NewAction = new FDelayAction(Asset, this, coroutine);
		LatentManager.AddNewAction(CallbackTarget, UUID, NewAction);
	}

	////////////////////////////////////////////////////////
	// FDelayAwaiter

	class FDelayAwaiter::FDelayAction : public FPendingLatentAction
	{
	public:
		FDelayAwaiter*          Awaiter;
		std::coroutine_handle<> CoroutineHandle;
		FWeakObjectPtr          CallbackTarget;
		float                   TimeRemaining;

		FDelayAction(FDelayAwaiter*          InAwaiter,
		             std::coroutine_handle<> InCoroutineHandle,
		             UObject*                CallbackTarget,
		             float                   Duration)
		    : Awaiter(InAwaiter)
		    , CoroutineHandle(InCoroutineHandle)
		    , CallbackTarget(CallbackTarget)
		    , TimeRemaining(Duration)
		{
		}

		virtual void UpdateOperation(FLatentResponse& Response) override
		{
			TimeRemaining -= Response.ElapsedTime();
			const bool bFinish = TimeRemaining <= 0.0f;
			if ( bFinish && Awaiter )
			{
				// コルーチンを再開する
				CoroutineHandle.resume();
			}
			Response.DoneIf(bFinish);
		}
	};

	FDelayAwaiter::FDelayAwaiter(UObject* InWorldContext, float InDuration)
	    : WorldContext(InWorldContext)
	    , Duration(InDuration)
	    , Action()
	{
	}

	FDelayAwaiter::~FDelayAwaiter()
	{
		if ( Action )
		{
			Action->Awaiter = nullptr;
		}
		Action = nullptr;
	}

	bool FDelayAwaiter::await_ready() const noexcept
	{
		// 有効期限がある場合には中断をする
		return Duration <= 0.f;
	}

	void FDelayAwaiter::await_suspend(std::coroutine_handle<> coroutine)
	{
		UObject* CallbackTarget = WorldContext.Get();
		// 識別用のUUIDをthisポインタから作成する
		const intptr_t uuidptr = reinterpret_cast<intptr_t>(this);
		const int32    UUID    = static_cast<int32>(uuidptr);

		UWorld* World = GEngine->GetWorldFromContextObject(
		    CallbackTarget, EGetWorldErrorMode::LogAndReturnNull);
		if ( !IsValid(World) )
		{
			return;
		}
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if ( LatentActionManager.FindExistingAction<FDelayAction>(CallbackTarget,
		                                                          UUID) == nullptr )
		{
			Action = new FDelayAction(this, coroutine, CallbackTarget, Duration);
			LatentActionManager.AddNewAction(CallbackTarget, UUID, Action);
		}
	}

	////////////////////////////////////////////////////////
	// FDelayUntilNextTickAwaiter

	class FDelayUntilNextTickAwaiter::FDelayAction : public FPendingLatentAction
	{
	public:
		FDelayUntilNextTickAwaiter* Awaiter;
		std::coroutine_handle<>     CoroutineHandle;
		FWeakObjectPtr              CallbackTarget;

		FDelayAction(FDelayUntilNextTickAwaiter* InAwaiter,
		             std::coroutine_handle<>     InCoroutineHandle,
		             UObject*                    CallbackTarget)
		    : Awaiter(InAwaiter)
		    , CoroutineHandle(InCoroutineHandle)
		    , CallbackTarget(CallbackTarget)
		{
		}

		virtual void UpdateOperation(FLatentResponse& Response) override
		{
			if ( Awaiter )
			{
				// コルーチンを再開する
				CoroutineHandle.resume();
			}
			Response.DoneIf(true);
		}
	};

	FDelayUntilNextTickAwaiter::FDelayUntilNextTickAwaiter(UObject* InWorldContext)
	    : WorldContext(InWorldContext)
	    , Action()
	{
	}

	FDelayUntilNextTickAwaiter::~FDelayUntilNextTickAwaiter()
	{
		if ( Action )
		{
			Action->Awaiter = nullptr;
		}
		Action = nullptr;
	}

	void FDelayUntilNextTickAwaiter::await_suspend(std::coroutine_handle<> coroutine)
	{
		UObject* CallbackTarget = WorldContext.Get();
		// 識別用のUUIDをthisポインタから作成する
		const intptr_t uuidptr = reinterpret_cast<intptr_t>(this);
		const int32    UUID    = static_cast<int32>(uuidptr);

		UWorld* World = GEngine->GetWorldFromContextObject(
		    CallbackTarget, EGetWorldErrorMode::LogAndReturnNull);
		if ( !IsValid(World) )
		{
			return;
		}
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if ( LatentActionManager.FindExistingAction<FDelayAction>(CallbackTarget,
		                                                          UUID) == nullptr )
		{
			Action = new FDelayAction(this, coroutine, CallbackTarget);
			LatentActionManager.AddNewAction(CallbackTarget, UUID, Action);
		}
	}

	////////////////////////////////////////////////////////
	// FTimerAwaiter

	FTimerAwaiter::FTimerAwaiter(UObject* InWorldContext,
	                             float    InTime,
	                             float    InitialStartDelay,
	                             float    InitialStartDelayVariance)
	    : WorldContext(InWorldContext)
	    , TimerHandle()
	    , Time(InTime)
	    , InitialStartDelay(InitialStartDelay)
	    , InitialStartDelayVariance(InitialStartDelayVariance)
	{
	}

	FTimerAwaiter::~FTimerAwaiter()
	{
		// タイマーハンドルが有効の場合にはまだ完了していないので削除処理が必要
		if ( TimerHandle.IsValid() && WorldContext.IsValid() )
		{
			UWorld* world = WorldContext->GetWorld();
			world->GetTimerManager().ClearTimer(TimerHandle);
		}
	}

	bool FTimerAwaiter::await_ready() const noexcept
	{
		// Timeが0の場合には中断しない
		return Time == 0;
	}

	void FTimerAwaiter::await_suspend(std::coroutine_handle<> coroutine)
	{
		const UWorld* World =
		    WorldContext.IsValid() ? WorldContext->GetWorld() : nullptr;
		if ( !IsValid(World) )
		{
			return;
		}

		InitialStartDelay +=
		    FMath::RandRange(-InitialStartDelayVariance, InitialStartDelayVariance);

		const auto OnDelayFinish = [this, coroutine]()
		{
			// タイマーハンドルを無効値にする
			TimerHandle.Invalidate();

			// コルーチンを再開する
			coroutine.resume();
		};

		FTimerManager& TimerManager = World->GetTimerManager();
		TimerManager.SetTimer(
		    TimerHandle, OnDelayFinish, Time, false, (Time + InitialStartDelay));
	}

} // namespace unco::details

namespace unco
{
	unco::details::FDelayAwaiter AsyncDelay(UObject* WorldContextObject,
	                                        float    Duration)
	{
		return details::FDelayAwaiter(WorldContextObject, Duration);
	}

	unco::details::FDelayUntilNextTickAwaiter DelayUntilNextTick(
	    UObject* WorldContextObject)
	{
		return details::FDelayUntilNextTickAwaiter(WorldContextObject);
	}

	unco::details::FTimerAwaiter AsyncSetTimer(UObject* InWorldContext,
	                                           float    Time,
	                                           float    InitialStartDelay,
	                                           float    InitialStartDelayVariance)
	{
		return details::FTimerAwaiter(
		    InWorldContext, Time, InitialStartDelay, InitialStartDelayVariance);
	}

} // namespace unco
