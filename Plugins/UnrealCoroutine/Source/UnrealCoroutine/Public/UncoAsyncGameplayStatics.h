// Fill out your copyright notice in the Description page of Project Settings.
// GameplayStaticsの非同期関数を記述する
#pragma once

#include "CoreMinimal.h"
#include <coroutine>

class UObject;
class AGameModeBase;

namespace unco::details
{
	/**
	 * @brief 非同期GameMode取得待機オブジェクト
	*/
	struct UNREALCOROUTINE_API FGetGameModeAwaiter
	{
		FGetGameModeAwaiter(const UObject* InWorldContext);
		~FGetGameModeAwaiter();
		bool await_ready() const noexcept
		{
			// ゲームモードが無い場合には待機が発生します。
			return GameMode != nullptr;
		}
		void await_suspend(std::coroutine_handle<> coroutine);
		constexpr [[nodiscard]] AGameModeBase* await_resume() const noexcept
		{
			return GameMode;
		}

	private:
		FWeakObjectPtr  WorldContext = nullptr;
		AGameModeBase*  GameMode     = nullptr;
		FDelegateHandle Handle;
	};

} // namespace unco::details

namespace unco
{

	/**
	 * @brief ゲームモードを非同期で取得します
	 *
	 * AGameModeはUWorldSubsystemなどの一部の初期化タイミングでは生成されていない為待機が必要になります。
	 * 大体のActorからのアクセスでは通常のUGameplayStatics経由での取得を推奨します。
	 * @param WorldContextObject ワールドコンテキスト
	 * @return ゲームモード
	 */
	UNREALCOROUTINE_API details::FGetGameModeAwaiter AsyncGetGameMode(
	    const UObject* WorldContextObject);

} // namespace unco
