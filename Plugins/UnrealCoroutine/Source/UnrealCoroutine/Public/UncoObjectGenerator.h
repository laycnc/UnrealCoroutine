// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <coroutine>
#include <utility>

namespace unco
{

	/**
	 * オブジェクトジェネレーター
	 */
	struct FObjectGenerator
	{
	private:
		struct FPromise
		{
		public:
			FWeakObjectPtr HostObject;

			//getRetrunObjは必ず生成
			FObjectGenerator get_return_object()
			{
				return FObjectGenerator(*this, HostObject);
			}

			//初めにResume状態にする、neverはすぐに次に進む
			constexpr std::suspend_always initial_suspend() const noexcept
			{
				return {};
			}

			//終わったらResume状態、終了後自動破棄しない
			constexpr std::suspend_always final_suspend() const noexcept
			{
				return {};
			}

			//co_yieldのたびに呼ばれる、値をコピーするためのメソッド
			constexpr std::suspend_always yield_value(int32 value) noexcept
			{
				return {};
			}

			constexpr void return_void() const noexcept {}

			// 例外のハンドリング時
			constexpr void unhandled_exception() noexcept {}

			/**
			 * コンストラクタ
			 * @param InObject オブジェクト
			 */
			template<class... Args>
			FPromise(UObject& InObject, Args...)
			    : HostObject(&InObject)
			{
			}

			/**
			 * コンストラクタ
			 * @param InObject オブジェクト
			 */
			template<class... Args>
			FPromise(UObject* InObject, Args...)
			    : HostObject(InObject)
			{
			}
		};

	public:
		using promise_type = FPromise;

		FObjectGenerator()                        = default;
		FObjectGenerator(const FObjectGenerator&) = delete;
		void operator=(const FObjectGenerator&) = delete;

		//Move
		FObjectGenerator(FObjectGenerator&& other) noexcept
		    : CoroutineHandle(std::exchange(other.CoroutineHandle, nullptr))
		    , HostObject(std::exchange(other.HostObject, nullptr))
		{
		}

		FObjectGenerator& operator=(FObjectGenerator&& other) noexcept
		{
			if ( this != &other )
			{
				CoroutineHandle = std::exchange(other.CoroutineHandle, nullptr);
				HostObject      = std::exchange(other.HostObject, nullptr);
			}
			return *this;
		}

		~FObjectGenerator()
		{
			if ( CoroutineHandle != nullptr ) //generatorはHandleを消去する
			{
				CoroutineHandle.destroy();
			}
		}

		//Main内でhandle.resume()を呼ぶためのメソッドを定義
		//moveNext風
		bool MoveNext()
		{
			CoroutineHandle.resume();
			return !CoroutineHandle.done();
		}

		// コルーチンが最終サスペンドで中断されているか？
		bool Done() const noexcept
		{
			return CoroutineHandle.done();
		}

		// オブジェクトが有効か？
		bool IsValidObject() const
		{
			return HostObject.IsValid();
		}

	private:
		explicit FObjectGenerator(FPromise& InPromise, FWeakObjectPtr InHostObject)
		    : CoroutineHandle(
		          std::coroutine_handle<FPromise>::from_promise(InPromise))
		    , HostObject(InHostObject)
		{
		}

		std::coroutine_handle<FPromise> CoroutineHandle;
		FWeakObjectPtr                  HostObject;
	};

} // namespace unco
