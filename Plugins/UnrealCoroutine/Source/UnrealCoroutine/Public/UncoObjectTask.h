#pragma once

#include "CoreMinimal.h"
#include <coroutine>
#include <utility>

namespace unco
{

	struct FObjectTask;

	struct UNREALCOROUTINE_API FObjectTaskPromise
	{
		/**
		 * @brief タスク終了時の処理
		*/
		struct UNREALCOROUTINE_API FFinalSuspend
		{
			constexpr bool await_ready() const noexcept
			{
				return false;
			}

			void           await_suspend(std::coroutine_handle<>) noexcept;
			constexpr void await_resume() const noexcept {}

			FObjectTaskPromise& Promise;
		};

		FObjectTask get_return_object() noexcept;

		// コルーチン本体処理の開始前に無条件サスペンド
		constexpr auto initial_suspend() noexcept
		{
			// suspend_neverの場合には即時コルーチン関数内部が実行される
			// suspend_alwaysの場合には明示的にコルーチン再開処理を呼ぶ必要があり
			return std::suspend_never{};
		}
		// コルーチン本体終処理の終了後に無条件サスペンド
		constexpr auto final_suspend() noexcept
		{
			return FFinalSuspend{*this};
		}

		inline void unhandled_exception() noexcept
		{
			// 一先ずは強制終了
			// 例外は一先ず無視
		}

		// co_return時に呼ばれる処理
		inline void return_void() const noexcept {}

		/**
		 * コンストラクタ
		 * @param InObject オブジェクト
		 */
		template<class... Args>
		FObjectTaskPromise(UObject& InObject, Args...)
		    : HostObject(&InObject)
		{
		}

		/**
		 * コンストラクタ
		 * @param InObject オブジェクト
		 */
		template<class... Args>
		FObjectTaskPromise(UObject* InObject, Args...)
		    : HostObject(InObject)
		{
		}

		// タスクの呼び出し者
		// このオブジェクトが無効になったらコルーチンも破棄させる為に保持させる
		FWeakObjectPtr HostObject;
		// Promise をスケジューラーに登録したか？
		bool bRegister = false;
		// コルーチンが終了したか？
		bool bFinalized = false;
	};

	/**
	 * @brief オブジェクトタスク
	*/
	struct UNREALCOROUTINE_API FObjectTask
	{
		friend struct FObjectTaskPromise;
		using promise_type = FObjectTaskPromise;

		~FObjectTask();

		// コピー禁止+ムーブ禁止
		// メンバ変数に持たせない為
		// コルーチンの保持はスケジューラーにさせる為
		FObjectTask(const FObjectTask&) = delete;
		FObjectTask(FObjectTask&&)      = delete;
		void operator=(const FObjectTask&) = delete;
		void operator=(FObjectTask&&) = delete;

		// co_awaitは禁止
		template<typename U>
		std::suspend_never await_transform(U&&) = delete;

	private:
		explicit FObjectTask(std::coroutine_handle<promise_type> p,
		                     FWeakObjectPtr InObjectPtr) noexcept
		    : CoroutineHandle(p)
		    , HostObject(InObjectPtr)
		{
		}

	private:
		std::coroutine_handle<promise_type> CoroutineHandle;
		FWeakObjectPtr                      HostObject;
	};
} // namespace unco
