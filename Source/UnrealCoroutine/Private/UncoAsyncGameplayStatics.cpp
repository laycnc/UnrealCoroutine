// Fill out your copyright notice in the Description page of Project Settings.

#include "UncoAsyncGameplayStatics.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"

namespace unco::details
{

	FGetGameModeAwaiter::FGetGameModeAwaiter(const UObject* InWorldContext)
	    : WorldContext(InWorldContext)
	{
		// コンストラクタでGameModeを取得する
		// ゲームモードがこの時点で無ければ待機が発生する
		GameMode = UGameplayStatics::GetGameMode(InWorldContext);
	}

	FGetGameModeAwaiter::~FGetGameModeAwaiter()
	{
		if ( Handle.IsValid() )
		{
			// 登録を解除
			FGameModeEvents::OnGameModeInitializedEvent().Remove(Handle);
		}
	}

	void FGetGameModeAwaiter::await_suspend(std::coroutine_handle<> coroutine)
	{
		// ゲームモードの初期化イベントが呼ばれる場合
		const auto OnGameModeInitialized = [=](AGameModeBase* NewGameMode)
		{
			const UWorld* SelfWorld = GEngine->GetWorldFromContextObject(
			    WorldContext.Get(), EGetWorldErrorMode::LogAndReturnNull);
			const UWorld* GameModeWorld = GEngine->GetWorldFromContextObject(
			    NewGameMode, EGetWorldErrorMode::LogAndReturnNull);

			// 作成されたGameModeのWorldが一致している場合有効
			if ( SelfWorld == GameModeWorld )
			{
				// 戻り値を保存して、コルーチンを再開
				GameMode = NewGameMode;
				coroutine.resume();
			}
		};

		// イベントを登録する
		Handle = FGameModeEvents::OnGameModeInitializedEvent().AddWeakLambda(
		    WorldContext.Get(), OnGameModeInitialized);
	}

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
	details::FGetGameModeAwaiter AsyncGetGameMode(const UObject* WorldContextObject)
	{
		return details::FGetGameModeAwaiter(WorldContextObject);
	}

} // namespace unco
