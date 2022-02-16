// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <coroutine>

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
class UObject;

namespace unco::details
{

	struct UNREALCOROUTINE_API FLoadAssetAwaiterBase
	{
		FLoadAssetAwaiterBase(const UObject*  InWorldContext,
		                      FSoftObjectPath InAsset);
		virtual ~FLoadAssetAwaiterBase();
		bool await_ready() const noexcept;
		void await_suspend(std::coroutine_handle<> coroutine);

	protected:
		class FDelayAction;
		FWeakObjectPtr  WorldContext;
		FSoftObjectPath Asset;
		UObject*        ResultObject = nullptr;
		FDelayAction*   Action       = nullptr;
	};

	/**
	 * @brief アセット読み込み待機
	*/
	template<class T>
	struct TLoadAssetAwaiter : public FLoadAssetAwaiterBase
	{
		TLoadAssetAwaiter(const UObject* InWorldContext, TSoftObjectPtr<T> InAsset)
		    : FLoadAssetAwaiterBase(InWorldContext, InAsset.ToSoftObjectPath())
		{
		}
		[[nodiscard]] constexpr T* await_resume() const noexcept
		{
			return Cast<T>(ResultObject);
		}
	};

	/**
	 * @brief クラス読み込み待機
	*/
	template<class T>
	struct TLoadClassAwaiter : public FLoadAssetAwaiterBase
	{
		TLoadClassAwaiter(const UObject* InWorldContext, TSoftClassPtr<T> InAsset)
		    : FLoadAssetAwaiterBase(InWorldContext, InAsset.ToSoftObjectPath())
		{
		}
		[[nodiscard]] constexpr UClass* await_resume() const noexcept
		{
			return Cast<UClass>(ResultObject);
		}
	};

	/**
	 * @brief 遅延待機
	*/
	struct UNREALCOROUTINE_API FDelayAwaiter
	{

		FDelayAwaiter(UObject* InWorldContext, float InDuration);
		~FDelayAwaiter();
		bool           await_ready() const noexcept;
		void           await_suspend(std::coroutine_handle<> coroutine);
		constexpr void await_resume() const noexcept {}

	private:
		class FDelayAction;
		FWeakObjectPtr      WorldContext;
		FTimerHandle        TimerHandle;
		float               Duration;
		class FDelayAction* Action = nullptr;
	};

	struct UNREALCOROUTINE_API FDelayUntilNextTickAwaiter
	{

		FDelayUntilNextTickAwaiter(UObject* InWorldContext);
		~FDelayUntilNextTickAwaiter();
		constexpr bool await_ready() const noexcept
		{
			return false;
		}
		void           await_suspend(std::coroutine_handle<> coroutine);
		constexpr void await_resume() const noexcept {}

	private:
		class FDelayAction;
		FWeakObjectPtr WorldContext;
		FTimerHandle   TimerHandle;
		FDelayAction*  Action = nullptr;
	};

	struct UNREALCOROUTINE_API FTimerAwaiter
	{

		FTimerAwaiter(UObject* InWorldContext,
		              float    InTime,
		              float    InitialStartDelay,
		              float    InitialStartDelayVariance);
		~FTimerAwaiter();
		bool           await_ready() const noexcept;
		void           await_suspend(std::coroutine_handle<> coroutine);
		constexpr void await_resume() noexcept {}

	private:
		TWeakObjectPtr<> WorldContext;
		FTimerHandle     TimerHandle;
		float            Time;
		float            InitialStartDelay;
		float            InitialStartDelayVariance;
	};

} // namespace unco::details

namespace unco
{

	/**
	 * アセットをロードする
	 */
	template<class T = UObject>
	static unco::details::TLoadAssetAwaiter<T> AsyncLoadAsset(
	    const UObject*    WorldContextObject,
	    TSoftObjectPtr<T> Asset)
	{
		return unco::details::TLoadAssetAwaiter(WorldContextObject, Asset);
	}

	template<class T = UObject>
	static unco::details::TLoadClassAwaiter<T> AsyncLoadClass(
	    const UObject*   WorldContextObject,
	    TSoftClassPtr<T> AssetClass)
	{
		return unco::details::TLoadClassAwaiter(WorldContextObject, AssetClass);
	}

	/**
	 * 非同期で一定時間待機します
	 *
	 * @param WorldContext	ワールドコンテキスト
	 * @param Duration 		待機時間(秒).
	 */
	UNREALCOROUTINE_API unco::details::FDelayAwaiter AsyncDelay(
	    UObject* WorldContext,
	    float    Duration);

	/**
	 * 非同期で次のフレームまで待機します
	 *
	 * @param WorldContext	ワールドコンテキスト
	 */
	UNREALCOROUTINE_API unco::details::FDelayUntilNextTickAwaiter DelayUntilNextTick(
	    UObject* WorldContext);

	/**
	 *デリゲートを実行するタイマーを設定します。 既存のタイマーを設定すると、更新されたパラメーターでそのタイマーがリセットされます。
	 * @param WorldContext ワールドコンテキスト。
	 * @param Time デリゲートを実行するまでの待機時間（秒単位）。 タイマーを<= 0秒に設定の場合には遅延を行いません
	 * @param InitialStartDelay タイマーマネージャーに渡される初期遅延（秒単位）。
	 * @param InitialStartDelayVariance これを使用して、ランダムを実行する代わりにタイマーが開始するタイミングに分散を追加します
	 * InitialStartDelay 入力の範囲（秒単位）。
	 */
	UNREALCOROUTINE_API unco::details::FTimerAwaiter AsyncSetTimer(
	    UObject* WorldContext,
	    float    Time,
	    float    InitialStartDelay         = 0.f,
	    float    InitialStartDelayVariance = 0.f);

} // namespace unco