# UnrealCoroutine

[C++20のコルーチン](https://cpprefjp.github.io/lang/cpp20/coroutines.html)学習の為に作ったライブラリです。  
UnrealEngineでコルーチンが使えるようにする為のライブラリになります。  

# 組み込み方法

## 対応しているエンジンバージョンに関して

コルーチンはC++20の機能の為指定モジュールをC++20でビルドする必要があります。  
C++20にはUnrealEngine5から対応をしている為、[UnrealEngine5の最新版をGithub](https://github.com/EpicGames/UnrealEngine/tree/5.0)からダウンロードして自前でビルドを行う必要があります。  
※ 執筆時点ではUnrealEngine5は未リリース。EA版ではC++20のビルドは通りませんでした。  
UnrealEngineをGithubからダウンロードしてビルドする方法に関しては記述しません。  

## プラグインを組み込み

> ${プロジェクト名}/Plugins/

↑のディレクトリにダウンロードしたプラグインを配置してください。

## 利用モジュールのビルド設定を変える 

```C#:モジュール名.Build.cs
public class モジュール名 : ModuleRules
{
	public モジュール名(ReadOnlyTargetRules Target) : base(Target)
	{
		// コルーチンはC++20の機能の為
		// このモジュールはC++20でビルドする必要がある
		CppStandard = CppStandardVersion.Cpp20;

		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// 他モジュールへの依存関係
		PublicDependencyModuleNames.AddRange(new string[] 
		{ 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			// 本プラグインへの依存を追加します。
			"UnrealCoroutine"
		});
	}
}
```

ゲームロジックを記述するBuild.csファイルにUnrealCoroutineへの依存関係を追加とC++20でのビルドする為の記述を追加します。 

# 基本的な使い方

## Task

```cpp:ExsampleActor.h
// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UncoObjectTask.h"
#include "ExsampleActor.generated.h"

UCLASS()
class AExsampleActor : public AActor
{
	GENERATED_BODY()

public:
	// アクターの初期化
	virtual void BeginPlay() override;

	// 非同期で初期化する
	unco::FObjectTask AsyncBeginPlay();

private:
	// ロードするオブジェクトパス
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UStaticMesh> LoadMeshPath;
	// ロードされたメッシュ
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> Mesh;
};

```

```cpp:ExsampleActor.cpp
// Copyright Epic Games, Inc. All Rights Reserved.
#include "ExsampleActor.h"
#include "UncoAsyncSystemLibrary.h"

// アクターの初期化
void AExsampleActor::BeginPlay()
{
	Super::BeginPlay();

	// 非同期関数を呼び出すと自動的に実行される
	AsyncBeginPlay();
}

// 非同期で初期化する
unco::FObjectTask AExsampleActor::AsyncBeginPlay()
{
	// 非同期でメッシュをロードする
	Mesh = co_await unco::AsyncLoadAsset(this, LoadMeshPath);

	// 5秒間待機する
	co_await unco::AsyncDelay(this, 5.f);

	// 終了
	co_return;
}

```


## 分散フレーム実行

```cpp:ExsampleActor.h
// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UncoObjectTask.h"
#include "ExsampleActor.generated.h"

UCLASS()
class AExsampleActor : public AActor
{
	GENERATED_BODY()

public:
	// アクターの初期化
	virtual void BeginPlay() override;

	// 非同期で初期化する
	unco::FObjectGenerator AsyncBeginPlay();

private:
	UPROPERTY(Transient)
	TArray<AActor*> SpawnActors;
};

```

```cpp:ExsampleActor.cpp
// Copyright Epic Games, Inc. All Rights Reserved.
#include "ExsampleActor.h"
#include "UncoAsyncSystemLibrary.h"

// アクターの初期化
void AExsampleActor::BeginPlay()
{
	Super::BeginPlay();

	// 処理負荷の高い処理を分散フレーム実行
	// 1フレームに実行出来るのは0.05ms
	UUncoScheduler::DistributedFrame(this, 0.05f, AsyncBeginPlay());
}

// 非同期で初期化する
unco::FObjectTask AExsampleActor::AsyncBeginPlay()
{
	// 200体アクターをスポーンする
	// 重い処理
	for ( int32 i = 0; i < 200; ++i )
	{

		AActor* spawn_actor = GetWorld()->SpawnActor<AActor>();
		SpawnActors.Add(spawn_actor);

		// ここまでの経過時間を見てしきい値を超えている場合には次のフレームに実行を分割されます。
		co_yield i;
	}

	co_return;
}

```