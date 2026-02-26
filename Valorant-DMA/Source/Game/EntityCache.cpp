#include "EntityCache.h"

EntityCache::FrameData EntityCache::Update(const MemoryReader& reader, const WorldSnapshot& snapshot) {
    FrameData frame;
    frame.localPawn = snapshot.localPawn;

    if (snapshot.playerCount <= 0 || !snapshot.persistentLevel || !snapshot.playerCameraManager)
        return frame;

    // --- Pass 1: Camera + player array pointer + count ---
    auto scatter = reader.CreateScatter();
    if (!scatter.IsValid()) return frame;

    Camera camera{};
    uintptr_t playerArrayPtr = 0;
    int playerCount = 0;

    scatter.PrepareRead(snapshot.playerCameraManager + Offsets::Player::CameraCache, &camera);
    scatter.PrepareRead(snapshot.persistentLevel + Offsets::Level::PlayerArray, &playerArrayPtr);
    scatter.PrepareRead(snapshot.persistentLevel + Offsets::Level::PlayerCount, &playerCount);
    scatter.Execute();

    frame.camera = camera;

    if (playerCount <= 0 || playerCount > 256 || !IsValidVA(playerArrayPtr))
        return frame;

    // --- Read actor array ---
    m_actorArray.resize(playerCount);
    reader.Read(playerArrayPtr, m_actorArray.data(), playerCount * sizeof(uintptr_t));

    // --- Pass 2: Per-actor basic data ---
    m_playersData.clear();
    m_playersData.resize(playerCount);

    auto scatter2 = reader.CreateScatter();
    if (!scatter2.IsValid()) return frame;

    for (int i = 0; i < playerCount; i++) {
        uintptr_t actor = m_actorArray[i];
        if (IsValidVA(actor)) {
            scatter2.PrepareRead(actor + Offsets::Actor::PawnPrivate, &m_playersData[i].pawnPrivate);
            scatter2.PrepareRead(actor + Offsets::Actor::Mesh, &m_playersData[i].mesh);
            scatter2.PrepareRead(actor + Offsets::Damage::DamageHandler, &m_playersData[i].damageHandler);
            scatter2.PrepareRead(actor + Offsets::Actor::Dormant, &m_playersData[i].dormant);
        }
    }
    scatter2.Execute();

    // --- Pass 3: Mesh data for valid, active, non-local players ---
    auto scatter3 = reader.CreateScatter();
    if (!scatter3.IsValid()) return frame;

    for (int i = 0; i < playerCount; i++) {
        auto& p = m_playersData[i];
        if (IsValidVA(p.mesh) && p.pawnPrivate != snapshot.localPawn && p.IsActive()) {
            scatter3.PrepareRead(p.mesh + Offsets::Mesh::BoneArray, &p.boneArray);
            scatter3.PrepareRead(p.mesh + Offsets::Mesh::BoneArray + 0x10, &p.boneArrayCache);
            scatter3.PrepareRead(p.mesh + Offsets::Mesh::ComponentToWorld, &p.comp2World);
            scatter3.PrepareRead(p.damageHandler + Offsets::Damage::Health, &p.health);
            scatter3.PrepareRead(p.mesh + Offsets::Mesh::LastSubmitTime, &p.lastSubmitTime);
            scatter3.PrepareRead(p.mesh + Offsets::Mesh::LastRenderTimeOnScreen, &p.lastRenderTime);
        }
    }
    scatter3.Execute();

    // --- Pass 4: Bone transforms ---
    auto scatter4 = reader.CreateScatter();
    if (!scatter4.IsValid()) return frame;

    for (int i = 0; i < playerCount; i++) {
        auto& p = m_playersData[i];
        if (!p.boneArray)
            p.boneArray = p.boneArrayCache;

        if (p.boneArray) {
            scatter4.PrepareRead(p.boneArray + (Offsets::Bone::Head * 0x30), &p.headTransform);
            scatter4.PrepareRead(p.boneArray + (Offsets::Bone::Root * 0x30), &p.rootTransform);
        }
    }
    scatter4.Execute();

    frame.players = std::move(m_playersData);
    return frame;
}
