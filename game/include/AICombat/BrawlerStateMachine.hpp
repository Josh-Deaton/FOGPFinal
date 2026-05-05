#pragma once

#include <Canis/Entity.hpp>

#include <SuperPupUtilities/StateMachine.hpp>

#include <string>

namespace AICombat
{
    class BrawlerStateMachine;

    class BrawlerIdleState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "IdleState";

        explicit BrawlerIdleState(SuperPupUtilities::StateMachine& _stateMachine);
        void Enter() override;
        void Update(float _dt) override;
    };

    class ChaseState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "ChaseState";
        float moveSpeed = 4.0f;

        explicit ChaseState(SuperPupUtilities::StateMachine& _stateMachine);
        void Enter() override;
        void Update(float _dt) override;
    };

    class HammerTimeState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "HammerTimeState";
        float hammerRestDegrees = 140.0f;
        float hammerSwingDegrees = -120.0f;
        float attackRange = 2.25f;
        float attackDuration = 0.75f;
        float attackDamageTime = 0.25f;

        explicit HammerTimeState(SuperPupUtilities::StateMachine& _stateMachine);
        void Enter() override;
        void Update(float _dt) override;
        void Exit() override;
    };

    class BrawlerStateMachine : public SuperPupUtilities::StateMachine
    {
    public:
        static constexpr const char* ScriptName = "AICombat::BrawlerStateMachine";

        std::string targetTag = "";
        float detectionRange = 20.0f;
        Canis::Vector3 bodyColliderSize = Canis::Vector3(1.0f);
        int maxHealth = 40;
        bool logStateChanges = true;
        Canis::Entity* hammerVisual = nullptr;
        Canis::AudioAssetHandle hitSfxPath1 = { .path = "assets/audio/sfx/hit_1.ogg" };
        Canis::AudioAssetHandle hitSfxPath2 = { .path = "assets/audio/sfx/hit_2.ogg" };
        float hitSfxVolume = 1.0f;
        Canis::SceneAssetHandle deathEffectPrefab = { .path = "assets/prefabs/brawler_death_particles.scene" };

        explicit BrawlerStateMachine(Canis::Entity& _entity);

        BrawlerIdleState idleState;
        ChaseState chaseState;
        HammerTimeState hammerTimeState;

        void Create() override;
        void Ready() override;
        void Destroy() override;
        void Update(float _dt) override;

        Canis::Entity* FindClosestTarget() const;
        float DistanceTo(const Canis::Entity& _other) const;
        void FaceTarget(const Canis::Entity& _target);
        void MoveTowards(const Canis::Entity& _target, float _speed, float _dt);
        void ChangeState(const std::string& _stateName);
        const std::string& GetCurrentStateName() const;
        float GetStateTime() const;
        float GetAttackRange() const;
        int GetCurrentHealth() const;

        void ResetHammerPose();
        void SetHammerSwing(float _normalized);
        void TakeDamage(int _damage);
        bool IsAlive() const;
        int GetMaxHealth() const;
        void Heal(int _amount);

    private:
        void PlayHitSfx();
        void SpawnDeathEffect();

        int m_currentHealth = 0;
        float m_stateTime = 0.0f;
        Canis::Vector4 m_baseColor = Canis::Vector4(1.0f);
        bool m_hasBaseColor = false;
        bool m_useFirstHitSfx = true;
    };

    void RegisterBrawlerStateMachineScript(Canis::App& _app);
    void UnRegisterBrawlerStateMachineScript(Canis::App& _app);
}
