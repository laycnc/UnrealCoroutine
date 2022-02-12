// Fill out your copyright notice in the Description page of Project Settings.

#include "UncoObjectTask.h"
#include "UncoScheduler.h"

namespace unco
{

	void FObjectTaskPromise::FFinalSuspend::await_suspend(
	    std::coroutine_handle<>) noexcept
	{
		// 終了フラグを建てる
		Promise.bFinalized = true;

		if ( Promise.bRegister )
		{
			// スケジューラーに登録されている場合には
			// スケジューラーに解除させる
			// スケジューラー経由でdestoryを呼ばせる
			UUncoScheduler* Scheduler =
			    UUncoScheduler::Get(Promise.HostObject.Get());
			if ( IsValid(Scheduler) )
			{
				Scheduler->UnregisterTask(
				    Promise.HostObject,
				    std::coroutine_handle<FObjectTaskPromise>::from_promise(
				        Promise));
			}
		}
	}

	FObjectTask::~FObjectTask()
	{
		if ( CoroutineHandle.promise().bFinalized )
		{
			// すでに終了している場合には
			if ( CoroutineHandle )
			{
				// コルーチンがすでに終了しているのでコルーチンハンドルを削除する必要がアリ
				CoroutineHandle.destroy();
			}
			CoroutineHandle = nullptr;
			HostObject      = nullptr;
			return;
		}

		// 実行中で
		if ( HostObject.IsValid() )
		{
			// スケジューラーに保持させる
			// スケジューラー経由でdestoryを呼ばせる
			UUncoScheduler* Scheduler = UUncoScheduler::Get(HostObject.Get());
			if ( IsValid(Scheduler) )
			{
				Scheduler->RegisterTask(HostObject, CoroutineHandle);
				CoroutineHandle.promise().bRegister = true;
			}
		}
		else
		{
			// Objectが無効値になっているので終了させる
			CoroutineHandle.destroy();
		}

		CoroutineHandle = nullptr;
		HostObject      = nullptr;
	}

	FObjectTask FObjectTaskPromise::get_return_object() noexcept
	{
		return FObjectTask{
		    std::coroutine_handle<FObjectTaskPromise>::from_promise(*this),
		    HostObject};
	}

} // namespace unco
