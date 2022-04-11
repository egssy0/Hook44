#pragma once
#include <Windows.h>
#include <UE4/UE4.h>
#include <string>

#ifdef _MSC_VER
#pragma pack(push, 0x8)
#endif

struct FNameEntry
{
	uint32_t Index;
	uint32_t pad;
	FNameEntry* HashNext;
	char AnsiName[1024];
	const int GetIndex() const { return Index >> 1; }
	const char* GetAnsiName() const { return AnsiName; }
};

class TNameEntryArray
{
public:
	bool IsValidIndex(uint32_t index) const { return index < NumElements; }
	FNameEntry const* GetById(uint32_t index) const { return *GetItemPtr(index); }
	FNameEntry const* const* GetItemPtr(uint32_t Index) const {
		const auto ChunkIndex = Index / 16384;
		const auto WithinChunkIndex = Index % 16384;
		const auto Chunk = Chunks[ChunkIndex];
		return Chunk + WithinChunkIndex;
	}
	FNameEntry** Chunks[128];
	uint32_t NumElements = 0;
	uint32_t NumChunks = 0;
};

struct FName
{
	int ComparisonIndex = 0;
	int Number = 0;
	static inline TNameEntryArray* GNames = nullptr;
	static const char* GetNameByIdFast(int Id) {
		auto NameEntry = GNames->GetById(Id);
		if (!NameEntry) return nullptr;
		return NameEntry->GetAnsiName();
	}
	static std::string GetNameById(int Id) {
		auto NameEntry = GNames->GetById(Id);
		if (!NameEntry) return std::string();
		return NameEntry->GetAnsiName();
	}
	const char* GetNameFast() const {
		auto NameEntry = GNames->GetById(ComparisonIndex);
		if (!NameEntry) return nullptr;
		return NameEntry->GetAnsiName();
	}
	const std::string GetName() const {
		auto NameEntry = GNames->GetById(ComparisonIndex);
		if (!NameEntry) return std::string();
		return NameEntry->GetAnsiName();
	};
	inline bool operator==(const FName& other) const {
		return ComparisonIndex == other.ComparisonIndex;
	};
	FName() {}
	FName(const char* find) {
		for (auto i = 6000u; i < GNames->NumElements; i++)
		{
			auto name = GetNameByIdFast(i);
			if (!name) continue;
			if (strcmp(name, find) == 0) {
				ComparisonIndex = i;
				return;
			};
		}
	}
};

struct FUObjectItem
{
	class UObject* Object;
	int Flags;
	int ClusterIndex;
	int SerialNumber;
	int pad;
};

struct TUObjectArray
{
	FUObjectItem* Objects;
	int MaxElements;
	int NumElements;
	class UObject* GetByIndex(int index) { return Objects[index].Object; }
};

class UClass;
class UObject
{
public:
	UObject(UObject* addr) { *this = addr; }
	static inline TUObjectArray* GObjects = nullptr;
	void* Vtable;
	int ObjectFlags;
	int InternalIndex;
	UClass* Class;
	FName Name;
	UObject* Outer;
	std::string GetName() const;
	const char* GetNameFast() const;
	std::string GetFullName() const;
	template<typename T>
	static T* FindObject(const std::string& name)
	{
		for (int i = 0; i < GObjects->NumElements; ++i)
		{
			auto object = GObjects->GetByIndex(i);

			if (object == nullptr)
			{
				continue;
			}
			if (object->GetFullName() == name)
			{
				return static_cast<T*>(object);
			}
		}
		return nullptr;
	}
	static UClass* FindClass(const std::string& name)
	{
		return FindObject<UClass>(name);
	}
	template<typename T>
	static T* GetObjectCasted(uint32_t index)
	{
		return static_cast<T*>(GObjects->GetByIndex(index));
	}
	bool IsA(UClass* cmp) const;
	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindObject<UClass>("Class CoreUObject.Object");
		return ptr;
	}
};

class UField : public UObject
{
public:
	using UObject::UObject;
	UField* Next;
};

class UProperty : public UField
{
public:
	int ArrayDim;
	int ElementSize;
	uint64_t PropertyFlags;
	char pad[0xC];
	int Offset_Internal;
	UProperty* PropertyLinkNext;
	UProperty* NextRef;
	UProperty* DestructorLinkNext;
	UProperty* PostConstructLinkNext;
};

class UStruct : public UField
{
public:
	using UField::UField;
	UStruct* SuperField;
	UField* Children;
	int PropertySize;
	int MinAlignment;
	TArray<uint8_t> Script;
	UProperty* PropertyLink;
	UProperty* RefLink;
	UProperty* DestructorLink;
	UProperty* PostConstructLink;
	TArray<UObject*> ScriptObjectReferences;
};

class UFunction : public UStruct
{
public:
	int FunctionFlags;
	uint16_t RepOffset;
	uint8_t NumParms;
	char pad;
	uint16_t ParmsSize;
	uint16_t ReturnValueOffset;
	uint16_t RPCId;
	uint16_t RPCResponseId;
	UProperty* FirstPropertyToInit;
	UFunction* EventGraphFunction;
	int EventGraphCallOffset;
	char pad2[0x4];
	void* Func;
};

inline void ProcessEvent(void* obj, UFunction* function, void* parms)
{
	auto vtable = *reinterpret_cast<void***>(obj);
	reinterpret_cast<void(*)(void*, UFunction*, void*)>(vtable[59])(obj, function, parms);
}

class UClass : public UStruct
{
public:
	using UStruct::UStruct;
	unsigned char  UnknownData00[0x138];
	template<typename T>
	inline T* CreateDefaultObject()
	{
		return static_cast<T*>(CreateDefaultObject());
	}
};

class FString : public TArray<wchar_t>
{
public:
	inline FString()
	{
	};
	FString(const wchar_t* other)
	{
		Max = Count = *other ? static_cast<int>(std::wcslen(other)) + 1 : 0;
		if (Count)
		{
			Data = const_cast<wchar_t*>(other);
		}
	};
	FString(const wchar_t* other, int count)
	{
		Data = const_cast<wchar_t*>(other);;
		Max = Count = count;
	};
	inline bool IsValid() const
	{
		return Data != nullptr;
	}
	inline const wchar_t* wide() const
	{
		return Data;
	}
	int multi(char* name, int size) const
	{
		return WideCharToMultiByte(CP_UTF8, 0, Data, Count, name, size, nullptr, nullptr) - 1;
	}
};

struct AController_K2_GetPawn_Params
{
	class APawn* ReturnValue;
};

struct APlayerState
{
	char pad[0x03D0];
	float Score;
	float Ping;
	FString	PlayerName;
};

struct FMinimalViewInfo {
	FVector Location;
	FRotator Rotation; 
	char UnknownData00[0x10];
	float FOV; 
};

struct UFOVHandlerFunctions_GetTargetFOV_Params {
	class AAthenaPlayerCharacter* Character; 
	float ReturnValue;    
};

struct FCameraCacheEntry {
	float TimeStamp;
	char pad[0x0010];
	FMinimalViewInfo POV; // 0x10(0x5a0)
};

struct FTViewTarget
{
	class AActor* Target;
	struct FMinimalViewInfo POV;
	class APlayerState* PlayerState;
};

struct APlayerCameraManager { 
	char pad[0x0440];
	FCameraCacheEntry CameraCache;  // 0x0440(0x05B0)
	FCameraCacheEntry LastFrameCameraCache;  // 0x09F0(0x05B0)
	FTViewTarget ViewTarget;  // 0x0FA0(0x05C0)
	FTViewTarget PendingViewTarget;  // 0x1560(0x05C0)
	FVector GetCameraLocation() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerCameraManager.GetCameraLocation");
		FVector location;
		ProcessEvent(this, fn, &location);
		return location;
	};
	FRotator GetCameraRotation() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerCameraManager.GetCameraRotation");
		FRotator rotation;
		ProcessEvent(this, fn, &rotation);
		return rotation;
	}
};

struct FKey 
{
	FName KeyName; // 0x00(0x08)
	unsigned char UnknownData00[0x18] = {}; // char UnknownData_8[0x18]; // 0x08(0x18)
	FKey() {};
	FKey(const char* InName) : KeyName(FName(InName)) {}
};

struct AController { 
	char pad1[0x03E8];
	class ACharacter* Character; // 0x3e8(0x08)
	char pad2[0x70];
	APlayerCameraManager* PlayerCameraManager; // 0x460(0x08)
	void SendToConsole(FString& cmd) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerController.SendToConsole");
		ProcessEvent(this, fn, &cmd);
	}
	bool WasInputKeyJustPressed(const FKey& Key) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerController.WasInputKeyJustPressed");
		struct
		{
			FKey Key;
			bool ReturnValue = false;
		} params;
		params.Key = Key;
		ProcessEvent(this, fn, &params);
		return params.ReturnValue;
	}
	bool ProjectWorldLocationToScreen(const FVector& WorldLocation, FVector2D& ScreenLocation) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerController.ProjectWorldLocationToScreen");
		struct
		{
			FVector WorldLocation;
			FVector2D ScreenLocation;
			bool ReturnValue = false;
		} params;
		params.WorldLocation = WorldLocation;
		ProcessEvent(this, fn, &params);
		ScreenLocation = params.ScreenLocation;
		return params.ReturnValue;
	}
	FRotator GetControlRotation() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Pawn.GetControlRotation");
		struct FRotator rotation;
		ProcessEvent(this, fn, &rotation);
		return rotation;
	}
	FRotator GetDesiredRotation() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Pawn.GetDesiredRotation");
		struct FRotator rotation;
		ProcessEvent(this, fn, &rotation);
		return rotation;
	}
	void AddYawInput(float Val) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerController.AddYawInput");
		ProcessEvent(this, fn, &Val);
	}
	void AddPitchInput(float Val) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerController.AddPitchInput");
		ProcessEvent(this, fn, &Val);
	}
	void FOV(float NewFOV) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerController.FOV");
		ProcessEvent(this, fn, &NewFOV);
	}
	APawn* K2_GetPawn() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Controller.K2_GetPawn");
		AController_K2_GetPawn_Params params;
		class APawn* ReturnValue;
		ProcessEvent(this, fn, &params);
		return params.ReturnValue;
	}
	bool LineOfSightTo(ACharacter* Other, const FVector& ViewPoint, const bool bAlternateChecks) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Controller.LineOfSightTo");
		struct {
			ACharacter* Other = nullptr;
			FVector ViewPoint;
			bool bAlternateChecks = false;
			bool ReturnValue = false;
		} params;
		params.Other = Other;
		params.ViewPoint = ViewPoint;
		params.bAlternateChecks = bAlternateChecks;
		ProcessEvent(this, fn, &params);
		return params.ReturnValue;
	}
};

struct UHealthComponent {
	float GetMaxHealth() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.HealthComponent.GetMaxHealth");
		float health = 0.f;
		ProcessEvent(this, fn, &health);
		return health;
	}
	float GetCurrentHealth() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.HealthComponent.GetCurrentHealth");
		float health = 0.f;
		ProcessEvent(this, fn, &health);
		return health;
	};
};

struct USkeletalMeshComponent {
	TArray<FTransform> SpaceBasesArray[2];
	int CurrentEditableSpaceBases;
	int CurrentReadSpaceBases;
	FName GetBoneName(int BoneIndex)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.SkinnedMeshComponent.GetBoneName");
		struct
		{
			int BoneIndex = 0;
			FName ReturnValue;
		} params;
		params.BoneIndex = BoneIndex;
		ProcessEvent(this, fn, &params);
		return params.ReturnValue;
	}
	FTransform K2_GetComponentToWorld() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.SceneComponent.K2_GetComponentToWorld");
		FTransform CompToWorld;
		ProcessEvent(this, fn, &CompToWorld);
		return CompToWorld;
	}
	bool GetBone(const uint32_t id, const FMatrix& componentToWorld, FVector& pos) {
		try {
			auto bones = SpaceBasesArray[CurrentReadSpaceBases];
			if (id >= bones.Count) return false;
			const auto& bone = bones[id];
			auto boneMatrix = bone.ToMatrixWithScale();
			auto world = boneMatrix * componentToWorld;
			pos = { world.M[3][0], world.M[3][1], world.M[3][2] };
			return true;
		}
		catch (...)
		{
			printf("error %d\n", __LINE__);
			return false;
		}
	}
};

struct AShipInternalWater {
	float GetNormalizedWaterAmount() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.ShipInternalWater.GetNormalizedWaterAmount");
		float params = 0.f;
		ProcessEvent(this, fn, &params);
		return params;
	}
};

struct AHullDamage {
	char pad[0x0428];
	TArray<class ACharacter*> ActiveHullDamageZones;
};

struct UDrowningComponent {
	float GetOxygenLevel() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.DrowningComponent.GetOxygenLevel");
		float oxygen;
		ProcessEvent(this, fn, &oxygen);
		return oxygen;
	}
};

struct FAIEncounterSpecification 
{
	char pad[0x80];
	FString* LocalisableName;  // 0x80(0x38)
};

struct UItemDesc { 
	char pad[0x0028];
	FString* Title; // 0x0028(0x0038)
};

struct AItemInfo {
	char pad[0x0430];
	UItemDesc* Desc; // 0x430
};

struct UWieldedItemComponent {
	char pad[0x02F0];
	ACharacter* CurrentlyWieldedItem; // 0x2f0(0x08)
};

struct FWeaponProjectileParams
{
	float Damage;
	float DamageMultiplierAtMaximumRange;
	float LifeTime;
	float TrailFadeOutTime;
	float Velocity;
};

struct FProjectileShotParams
{
	int Seed;
	float ProjectileDistributionMaxAngle;
	int NumberOfProjectiles;
	float ProjectileMaximumRange;
	float ProjectileDamage;
	float ProjectileDamageMultiplierAtMaximumRange;
};

struct FProjectileWeaponParameters {
	int AmmoClipSize;
	int AmmoCostPerShot;
	float EquipDuration;
	float IntoAimingDuration;
	float RecoilDuration;
	float ReloadDuration;
	struct FProjectileShotParams HipFireProjectileShotParams;
	struct FProjectileShotParams AimDownSightsProjectileShotParams;
	int Seed;
	float ProjectileDistributionMaxAngle;
	int NumberOfProjectiles;
	float ProjectileMaximumRange;
	float ProjectileDamage;
	float ProjectileDamageMultiplierAtMaximumRange;
	class UClass* DamagerType;
	class UClass* ProjectileId;
	struct FWeaponProjectileParams AmmoParams;
	bool UsesScope;
	float ZoomedRecoilDurationIncrease;
	float SecondsUntilZoomStarts;
	float SecondsUntilPostStarts;
	float WeaponFiredAINoiseRange;
	float MaximumRequestPositionDelta;
	float MaximumRequestAngleDelta;
	float TimeoutTolerance;
	float AimingMoveSpeedScalar;
	float InAimFOV;
	float BlendSpeed;
};

struct AProjectileWeapon {
	char pad[0x07D0];
	FProjectileWeaponParameters WeaponParameters; // 0x7d0(0x1e0)
	bool CanFire()
	{
		static auto fn = UObject::FindObject<UFunction>("Function Athena.ProjectileWeapon.CanFire");
		bool canfire;
		ProcessEvent(this, fn, &canfire);
		return canfire;
	}
};

struct FHitResult
{
	unsigned char bBlockingHit : 1; // 0x0000(0x0001)
	unsigned char bStartPenetrating : 1; // 0x0000(0x0001)
	unsigned char UnknownData00[0x3]; // char UnknownData_1[0x3]; 0x01(0x03)
	float Time; // 0x0004(0x0004)
	float Distance; // 0x0008(0x0004)
	char pad1[0x48];
	float PenetrationDepth; // 0x0054(0x0004)
	int Item; // 0x0058(0x0004)
	char pad2[0x18];
	struct FName BoneName; // 0x0074(0x0008)
	int FaceIndex; // 0x007C(0x0004)
};

enum class EDrawDebugTrace : uint8_t
{
	EDrawDebugTrace__None = 0,
	EDrawDebugTrace__ForOneFrame = 1,
	EDrawDebugTrace__ForDuration = 2,
	EDrawDebugTrace__Persistent = 3,
	EDrawDebugTrace__EDrawDebugTrace_MAX = 4
};

enum class ETraceTypeQuery : uint8_t
{
	TraceTypeQuery1 = 0,
	TraceTypeQuery2 = 1,
	TraceTypeQuery3 = 2,
	TraceTypeQuery4 = 3,
	TraceTypeQuery5 = 4,
	TraceTypeQuery6 = 5,
	TraceTypeQuery7 = 6,
	TraceTypeQuery8 = 7,
	TraceTypeQuery9 = 8,
	TraceTypeQuery10 = 9,
	TraceTypeQuery11 = 10,
	TraceTypeQuery12 = 11,
	TraceTypeQuery13 = 12,
	TraceTypeQuery14 = 13,
	TraceTypeQuery15 = 14,
	TraceTypeQuery16 = 15,
	TraceTypeQuery17 = 16,
	TraceTypeQuery18 = 17,
	TraceTypeQuery19 = 18,
	TraceTypeQuery20 = 19,
	TraceTypeQuery21 = 20,
	TraceTypeQuery22 = 21,
	TraceTypeQuery23 = 22,
	TraceTypeQuery24 = 23,
	TraceTypeQuery25 = 24,
	TraceTypeQuery26 = 25,
	TraceTypeQuery27 = 26,
	TraceTypeQuery28 = 27,
	TraceTypeQuery29 = 28,
	TraceTypeQuery30 = 29,
	TraceTypeQuery31 = 30,
	TraceTypeQuery32 = 31,
	TraceTypeQuery_MAX = 32,
	ETraceTypeQuery_MAX = 33
};

struct USceneComponent {
	FVector K2_GetComponentLocation() {
		FVector location;
		static auto fn = UObject::FindObject<UFunction>("Function Engine.SceneComponent.K2_GetComponentLocation");
		ProcessEvent(this, fn, &location);
		return location;
	}
};

struct AShip
{
	FVector GetCurrentVelocity() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.Ship.GetCurrentVelocity");
		FVector params;
		ProcessEvent(this, fn, &params);
		return params;
	}
	FVector GetVelocity() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.GetVelocity");
		FVector velocity;
		ProcessEvent(this, fn, &velocity);
		return velocity;
	}
};

struct AAthenaGameState {
	char pad[0x05b8];
	class AWindService* WindService;                                             // 0x05B8(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class APlayerManagerService* PlayerManagerService;                                    // 0x05C0(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class AShipService* ShipService;                                             // 0x05C8(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class AWatercraftService* WatercraftService;                                       // 0x05D0(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class ATimeService* TimeService;                                             // 0x05D8(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class UHealthCustomizationService* HealthService;                                           // 0x05E0(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class UCustomWeatherService* CustomWeatherService;                                    // 0x05E8(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class UCustomStatusesService* CustomStatusesService;                                   // 0x05F0(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class AFFTWaterService* WaterService;                                            // 0x05F8(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class AStormService* StormService;                                            // 0x0600(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class ACrewService* CrewService;                                             // 0x0608(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class AContestZoneService* ContestZoneService;                                      // 0x0610(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class AContestRowboatsService* ContestRowboatsService;                                  // 0x0618(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class AIslandService* IslandService;                                           // 0x0620(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class ANPCService* NPCService;                                              // 0x0628(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class ASkellyFortService* SkellyFortService;                                       // 0x0630(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class ADeepSeaRegionService* DeepSeaRegionService;                                    // 0x0638(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class AAIDioramaService* AIDioramaService;                                        // 0x0640(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class AAshenLordEncounterService* AshenLordEncounterService;                               // 0x0648(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class AAggressiveGhostShipsEncounterService* AggressiveGhostShipsEncounterService;                    // 0x0650(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class ATallTaleService* TallTaleService;                                         // 0x0658(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class AAIShipObstacleService* AIShipObstacleService;                                   // 0x0660(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class AAIShipService* AIShipService;                                           // 0x0668(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class AAITargetService* AITargetService;                                         // 0x0670(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class UShipLiveryCatalogueService* ShipLiveryCatalogueService;                              // 0x0678(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class AContestManagerService* ContestManagerService;                                   // 0x0680(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class ADrawDebugService* DrawDebugService;                                        // 0x0688(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class AWorldEventZoneService* WorldEventZoneService;                                   // 0x0690(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class UWorldResourceRegistry* WorldResourceRegistry;                                   // 0x0698(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class AKrakenService* KrakenService;                                           // 0x06A0(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class UPlayerNameService* PlayerNameService;                                       // 0x06A8(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class ATinySharkService* TinySharkService;                                        // 0x06B0(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class AProjectileService* ProjectileService;                                       // 0x06B8(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class ULaunchableProjectileService* LaunchableProjectileService;                             // 0x06C0(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class UServerNotificationsService* ServerNotificationsService;                              // 0x06C8(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class AAIManagerService* AIManagerService;                                        // 0x06D0(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class AAIEncounterService* AIEncounterService;                                      // 0x06D8(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class AAIEncounterGenerationService* AIEncounterGenerationService;                            // 0x06E0(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class UEncounterService* EncounterService;                                        // 0x06E8(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class UGameEventSchedulerService* GameEventSchedulerService;                               // 0x06F0(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class UHideoutService* HideoutService;                                          // 0x06F8(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class UAthenaStreamedLevelService* StreamedLevelService;                                    // 0x0700(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class ULocationProviderService* LocationProviderService;                                 // 0x0708(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class AHoleService* HoleService;                                             // 0x0710(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class APlayerBuriedItemService* PlayerBuriedItemService;                                 // 0x0718(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class ULoadoutService* LoadoutService;                                          // 0x0720(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class UOcclusionService* OcclusionService;                                        // 0x0728(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class UPetsService* PetsService;                                             // 0x0730(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class UAthenaAITeamsService* AthenaAITeamsService;                                    // 0x0738(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class AAllianceService* AllianceService;                                         // 0x0740(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class UMaterialAccessibilityService* MaterialAccessibilityService;                            // 0x0748(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class AReapersMarkService* ReapersMarkService;                                      // 0x0750(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class AEmissaryLevelService* EmissaryLevelService;                                    // 0x0758(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class ACampaignService* CampaignService;                                         // 0x0760(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class AStoryService* StoryService;                                            // 0x0768(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class AStorySpawnedActorsService* StorySpawnedActorsService;                               // 0x0770(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class AFlamesOfFateSettingsService* FlamesOfFateSettingsService;                             // 0x0778(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class AServiceStatusNotificationsService* ServiceStatusNotificationsService;                       // 0x0780(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class UMigrationService* MigrationService;                                        // 0x0788(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class AShroudBreakerService* ShroudBreakerService;                                    // 0x0790(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class UServerUpdateReportingService* ServerUpdateReportingService;                            // 0x0798(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class AGenericMarkerService* GenericMarkerService;                                    // 0x07A0(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class AMechanismsService* MechanismsService;                                       // 0x07A8(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class UMerchantContractsService* MerchantContractsService;                                // 0x07B0(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class UShipFactory* ShipFactory;                                             // 0x07B8(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class URewindPhysicsService* RewindPhysicsService;                                    // 0x07C0(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class UNotificationMessagesDataAsset* NotificationMessagesDataAsset;                           // 0x07C8(0x0008) Edit, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class AProjectileCooldownService* ProjectileCooldownService;                               // 0x07D0(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class UIslandReservationService* IslandReservationService;                                // 0x07D8(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class APortalService* PortalService;                                           // 0x07E0(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class UMeshMemoryConstraintService* MeshMemoryConstraintService;                             // 0x07E8(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class ABootyStorageService* BootyStorageService;                                     // 0x07F0(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class ASpireService* SpireService;                                            // 0x07F8(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class AFireworkService* FireworkService;                                         // 0x0800(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class UAirGivingService* AirGivingService;                                        // 0x0808(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class UCutsceneService* CutsceneService;                                         // 0x0810(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class ACargoRunService* CargoRunService;                                         // 0x0818(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class ACommodityDemandService* CommodityDemandService;                                  // 0x0820(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class ADebugTeleportationDestinationService* DebugTeleportationDestinationService;                    // 0x0828(0x0008) Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash
	class ASeasonProgressionUIService* SeasonProgressionUIService;                              // 0x0830(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class UTransientActorService* TransientActorService;                                   // 0x0838(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class UTunnelsOfTheDamnedService* TunnelsOfTheDamnedService;                               // 0x0840(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class UWorldSequenceService* WorldSequenceService;                                    // 0x0848(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class UItemLifetimeManagerService* ItemLifetimeManagerService;                              // 0x0850(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class USeaFortsService* SeaFortsService;                                         // 0x0858(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class ABeckonService* BeckonService;                                           // 0x0860(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class UVolcanoService* VolcanoService;                                          // 0x0868(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash
	class UShipAnnouncementService* ShipAnnouncementService;                                 // 0x0870(0x0008) ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash

};

struct UCharacterMovementComponent {
	FVector GetCurrentAcceleration() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.CharacterMovementComponent.GetCurrentAcceleration");
		FVector acceleration;
		ProcessEvent(this, fn, &acceleration);
		return acceleration;
	}
};

struct UInventoryManipulatorComponent {
	bool ConsumeItem(ACharacter* item) {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.InventoryManipulatorComponent.ConsumeItem");
		struct
		{
			ACharacter* item;
			bool ReturnValue;
		} params;
		params.item = item;
		params.ReturnValue = false;
		ProcessEvent(this, fn, &params);
		return params.ReturnValue;
	}
};

class ACharacter : public UObject {
public:
	char pad1[0x3C8];
	APlayerState* PlayerState;  // 0x3f0(0x08)
	char pad2[0x10];
	AController* Controller; // 0x408(0x08)
	char pad3[0x38];
	USkeletalMeshComponent* Mesh; // 0x0448
	UCharacterMovementComponent* CharacterMovement; // 0x450(0x08)
	char pad4[0x3D8];
	UWieldedItemComponent* WieldedItemComponent; // 0x830(0x08)
	char pad43[0x8];
	UInventoryManipulatorComponent* InventoryManipulatorComponent; // 0x840(0x08)
	char pad5[0x10];
	UHealthComponent* HealthComponent; // 0x858(0x08)
	char pad6[0x4A8];
	UDrowningComponent* DrowningComponent; // 0xd08(0x08)
	void ReceiveTick(float DeltaSeconds)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.ActorComponent.ReceiveTick");
		ProcessEvent(this, fn, &DeltaSeconds);
	}
	void GetActorBounds(bool bOnlyCollidingComponents, FVector& Origin, FVector& BoxExtent) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.GetActorBounds");
		struct
		{
			bool bOnlyCollidingComponents = false;
			FVector Origin;
			FVector BoxExtent;
		} params;
		params.bOnlyCollidingComponents = bOnlyCollidingComponents;
		ProcessEvent(this, fn, &params);
		Origin = params.Origin;
		BoxExtent = params.BoxExtent;
	}
	ACharacter* GetCurrentShip() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.AthenaCharacter.GetCurrentShip");
		ACharacter* ReturnValue;
		ProcessEvent(this, fn, &ReturnValue);
		return ReturnValue;
	}
	ACharacter* GetAttachParentActor() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.GetAttachParentActor");
		ACharacter* ReturnValue;
		ProcessEvent(this, fn, &ReturnValue);
		return ReturnValue;
	};
	ACharacter* GetParentActor() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.GetParentActor");
		ACharacter* ReturnValue;
		ProcessEvent(this, fn, &ReturnValue);
		return ReturnValue;
	};
	ACharacter* GetWieldedItem() {
		if (!WieldedItemComponent) return nullptr;
		return WieldedItemComponent->CurrentlyWieldedItem;
	}
	FVector GetVelocity() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.GetVelocity");
		FVector velocity;
		ProcessEvent(this, fn, &velocity);
		return velocity;
	}
	AItemInfo* GetItemInfo() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.ItemProxy.GetItemInfo");
		AItemInfo* info = nullptr;
		ProcessEvent(this, fn, &info);
		return info;
	}
	void CureAllAilings() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.AthenaCharacter.CureAllAilings");
		ProcessEvent(this, fn, nullptr);
	}
	float GetTargetFOV(class AAthenaPlayerCharacter* Character) {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.FOVHandlerFunctions.GetTargetFOV");
		UFOVHandlerFunctions_GetTargetFOV_Params params;
		params.Character = Character;
		ProcessEvent(this, fn, &params);
		return params.ReturnValue;
	}
	bool IsLoading() {
		static auto fn = UObject::FindObject<UFunction>("Function AthenaLoadingScreen.AthenaLoadingScreenBlueprintFunctionLibrary.IsLoadingScreenVisible");
		bool isLoading = true;
		ProcessEvent(this, fn, &isLoading);
		return isLoading;
	}
	void Kill(uint8_t DeathType) {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.AthenaCharacter.Kill");
		ProcessEvent(this, fn, &DeathType);
	}
	bool IsDead() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.AthenaCharacter.IsDead");
		bool isDead = true;
		ProcessEvent(this, fn, &isDead);
		return isDead;
	}
	FVector GetForwardVelocity() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.GetActorForwardVector");
		FVector ForwardVelocity;
		ProcessEvent(this, fn, &ForwardVelocity);
		return ForwardVelocity;
	}

	bool IsInWater() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.AthenaCharacter.IsInWater");
		bool isInWater = false;
		ProcessEvent(this, fn, &isInWater);
		return isInWater;
	}
	FRotator K2_GetActorRotation() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.K2_GetActorRotation");
		FRotator params;
		ProcessEvent(this, fn, &params);
		return params;
	}
	FVector K2_GetActorLocation() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.K2_GetActorLocation");
		FVector params;
		ProcessEvent(this, fn, &params);
		return params;
	}
	FVector GetActorForwardVector() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.GetActorForwardVector");
		FVector params;
		ProcessEvent(this, fn, &params);
		return params;
	}
	FVector GetActorUpVector() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.GetActorUpVector");
		FVector params;
		ProcessEvent(this, fn, &params);
		return params;
	}
	inline bool isPlayer() {
		static auto obj = UObject::FindClass("Class Athena.AthenaPlayerCharacter");
		return IsA(obj);
	}
	inline bool isShip() {
		static auto obj = UObject::FindClass("Class Athena.Ship");
		return IsA(obj);
	}
	inline bool isWheel() {
		static auto obj = UObject::FindClass("Class Athena.Wheel");
		return IsA(obj);
	}
	inline bool isCannonProjectile() {
		static auto obj = UObject::FindClass("Class Athena.CannonProjectile");
		return IsA(obj);
	}
	inline bool isEvent() {
		static auto obj = UObject::FindClass("Class Athena.GameplayEventSignal");
		return IsA(obj);
	}
	inline bool isMapTable() {
		static auto obj = UObject::FindClass("Class Athena.MapTable");
		return IsA(obj);
	}
	inline bool isCannon() {
		static auto obj = UObject::FindClass("Class Athena.Cannon");
		return IsA(obj);
	}
	inline bool isHarpoon() {
		static auto obj = UObject::FindClass("Class Athena.HarpoonLauncher");
		return IsA(obj);
	}
	inline bool isMermaid() {
		static auto obj = UObject::FindClass("Class Athena.Mermaid");
		return IsA(obj);
	}
	inline bool isFarShip() {
		static auto obj = UObject::FindClass("Class Athena.ShipNetProxy");
		return IsA(obj);
	}
	inline bool isItem() {
		static auto obj = UObject::FindClass("Class Athena.ItemProxy");
		return IsA(obj);
	}
	bool isWeapon() {
		static auto obj = UObject::FindClass("Class Athena.ProjectileWeapon");
		return IsA(obj);
	}
	bool isWorldSettings() {
		static auto obj = UObject::FindClass("Class Engine.WorldSettings");
		return IsA(obj);
	}
	FAIEncounterSpecification GetAIEncounterSpec() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.AthenaAICharacter.GetAIEncounterSpec");
		FAIEncounterSpecification spec;
		ProcessEvent(this, fn, &spec);
		return spec;
	}
	AHullDamage* GetHullDamage() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.Ship.GetHullDamage");
		AHullDamage* params = nullptr;
		ProcessEvent(this, fn, &params);
		return params;
	}
	AShipInternalWater* GetInternalWater() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.Ship.GetInternalWater");
		AShipInternalWater* params = nullptr;
		ProcessEvent(this, fn, &params);
		return params;
	}
	float GetMinWheelAngle() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.Wheel.GetMinWheelAngle");
		float angle = 0.f;
		ProcessEvent(this, fn, &angle);
		return angle;
	}
	float GetMaxWheelAngle() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.Wheel.GetMaxWheelAngle");
		float angle = 0.f;
		ProcessEvent(this, fn, &angle);
		return angle;
	}
	void ForceSetWheelAngle(float Angle) {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.Wheel.ForceSetWheelAngle");
		ProcessEvent(this, fn, &Angle);
	}
	bool CanJump() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Character.CanJump");
		bool can_jump = false;
		ProcessEvent(this, fn, &can_jump);
		return can_jump;
	}
	void Jump() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Character.Jump");
		ProcessEvent(this, fn, nullptr);
	}
	void LaunchCharacter(FVector& LaunchVelocity, bool bXYOverride, bool bZOverride) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Character.LaunchCharacter");
		struct
		{
			FVector LaunchVelocity;
			bool bXYOverride;
			bool bZOverride;
		} params;
		params.LaunchVelocity = LaunchVelocity;
		params.bXYOverride = bXYOverride;
		params.bZOverride = bZOverride;
		ProcessEvent(this, fn, &params);
	}
};

class UKismetMathLibrary {
private:
	static inline UClass* defaultObj;
public:
	static bool Init() {
		return defaultObj = UObject::FindObject<UClass>("Class Engine.KismetMathLibrary");
	}
	static FRotator NormalizedDeltaRotator(const struct FRotator& A, const struct FRotator& B) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.KismetMathLibrary.NormalizedDeltaRotator");
		struct
		{
			struct FRotator A;
			struct FRotator B;
			struct FRotator ReturnValue;
		} params;
		params.A = A;
		params.B = B;
		ProcessEvent(defaultObj, fn, &params);
		return params.ReturnValue;
	};
	static FRotator FindLookAtRotation(const FVector& Start, const FVector& Target) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.KismetMathLibrary.FindLookAtRotation");
		struct {
			FVector Start;
			FVector Target;
			FRotator ReturnValue;
		} params;
		params.Start = Start;
		params.Target = Target;
		ProcessEvent(defaultObj, fn, &params);
		return params.ReturnValue;
	}
	static FVector Conv_RotatorToVector(const struct FRotator& InRot) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.KismetMathLibrary.Conv_RotatorToVector");
		struct
		{
			struct FRotator InRot;
			struct FVector ReturnValue;
		} params;
		params.InRot = InRot;
		ProcessEvent(defaultObj, fn, &params);
		return params.ReturnValue;
	}
	static bool LineTraceSingle_NEW(class UObject* WorldContextObject, const struct FVector& Start, const struct FVector& End, ETraceTypeQuery TraceChannel, bool bTraceComplex, TArray<class AActor*> ActorsToIgnore, EDrawDebugTrace DrawDebugType, bool bIgnoreSelf, struct FHitResult* OutHit)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.KismetSystemLibrary.LineTraceSingle_NEW");
		struct
		{
			class UObject* WorldContextObject;
			struct FVector Start;
			struct FVector End;
			ETraceTypeQuery	TraceChannel;
			bool bTraceComplex;
			TArray<class AActor*> ActorsToIgnore;
			EDrawDebugTrace DrawDebugType;
			struct FHitResult OutHit;
			bool bIgnoreSelf;
			bool ReturnValue;
		} params;
		params.WorldContextObject = WorldContextObject;
		params.Start = Start;
		params.End = End;
		params.TraceChannel = TraceChannel;
		params.bTraceComplex = bTraceComplex;
		params.ActorsToIgnore = ActorsToIgnore;
		params.DrawDebugType = DrawDebugType;
		params.bIgnoreSelf = bIgnoreSelf;
		ProcessEvent(defaultObj, fn, &params);
		if (OutHit != nullptr)
			*OutHit = params.OutHit;
		return params.ReturnValue;
	}
	static void DrawDebugBox(UObject* WorldContextObject, const FVector& Center, const FVector& Extent, const FLinearColor& LineColor, const FRotator& Rotation, float Duration) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.KismetSystemLibrary.DrawDebugBox");
		struct
		{
			UObject* WorldContextObject = nullptr;
			FVector Center;
			FVector Extent;
			FLinearColor LineColor;
			FRotator Rotation;
			float Duration = INFINITY;
		} params;
		params.WorldContextObject = WorldContextObject;
		params.Center = Center;
		params.Extent = Extent;
		params.LineColor = LineColor;
		params.Rotation = Rotation;
		params.Duration = Duration;
		ProcessEvent(defaultObj, fn, &params);
	}
	static void DrawDebugArrow(UObject* WorldContextObject, const FVector& LineStart, const FVector& LineEnd, float ArrowSize, const FLinearColor& LineColor, float Duration) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.KismetSystemLibrary.DrawDebugBox");
		struct
		{
			class UObject* WorldContextObject = nullptr;
			struct FVector LineStart;
			struct FVector LineEnd;
			float ArrowSize = 1.f;
			struct FLinearColor LineColor;
			float Duration = 1.f;
		} params;
		params.WorldContextObject = WorldContextObject;
		params.LineStart = LineStart;
		params.LineEnd = LineEnd;
		params.ArrowSize = ArrowSize;
		params.LineColor = LineColor;
		params.Duration = Duration;
		ProcessEvent(defaultObj, fn, &params);
	}
};

struct FFloatRange {
	float pad1;
	float min;
	float pad2;
	float max;
};

struct UCrewFunctions {
private:
	static inline UClass* defaultObj;
public:
	static bool Init() {
		return defaultObj = UObject::FindObject<UClass>("Class Athena.CrewFunctions");
	}
	static bool AreCharactersInSameCrew(ACharacter* Player1, ACharacter* Player2) {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.CrewFunctions.AreCharactersInSameCrew");
		struct
		{
			ACharacter* Player1;
			ACharacter* Player2;
			bool ReturnValue;
		} params;
		params.Player1 = Player1;
		params.Player2 = Player2;
		ProcessEvent(defaultObj, fn, &params);
		return params.ReturnValue;
	}
};

struct UPlayer { 
	char UnknownData00[0x30];
	AController* PlayerController; // APlayerController* PlayerController; // 0x30(0x08)
};

struct UGameInstance {
	char UnknownData00[0x38];
	TArray<UPlayer*> LocalPlayers; // ULocalPlayer*> LocalPlayers; // 0x38(0x10)
};

struct ULevel {
	char UnknownData00[0xA0];
	TArray<ACharacter*> AActors; // char UnknownData_28[0xa0]; 0x28(0xa0)
};

struct UWorld {
	static inline UWorld** GWorld = nullptr;
	char pad1[0x30];
	ULevel* PersistentLevel; // 0x0030(0x0008) 
	char pad2[0x20];
	AAthenaGameState* GameState; //0x0058(0x0008)
	char pad3[0xF0];
	TArray<ULevel*> Levels; //0x0150(0x0010)
	char pad4[0x50];
	ULevel* CurrentLevel; //0x01B0(0x0008)
	char pad5[0x8];
	UGameInstance* GameInstance; //0x01C0(0x0008)
};

#ifdef _MSC_VER
#pragma pack(pop)
#endif