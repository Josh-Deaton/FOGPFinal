#pragma once

#include <Canis/Entity.hpp>

#include <SuperPupUtilities/StateMachine.hpp>

#include <string>

namespace AICombat
{
    class MageStateMachine;

    class MageIdleState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "IdleState";

        explicit MageIdleState(SuperPupUtilities::StateMachine& _stateMachine);
        void Enter() override;
        void Update(float _dt) override;
    };

    class TargetState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "TargetState";
        float spellAimSpeed = 0.5f;
        float spellCastRange = 10.0f;
        float spellCastDuration = 1.5f;
        float spellDamageTime = 0.75f;

        explicit TargetState(SuperPupUtilities::StateMachine& _stateMachine);
        void Enter() override;
        void Update(float _dt) override;
        void Exit() override;

    private:
        float m_timeSinceEnter = 0.0f;
    };

    class MageStateMachine : public SuperPupUtilities::StateMachine
    {
    public:
        static constexpr const char* ScriptName = "AICombat::MageStateMachine"; 

        std::string targetTag = "";
        float detectionRange = 20.0f;
        Canis::Vector3 bodyColliderSize = Canis::Vector3(1.0f);
        int maxHealth = 30;
        bool logStateChanges = true;
        Canis::Entity* spellVisual = nullptr;
        Canis::AudioAssetHandle hitSfxPath1 = { .path = "assets/audio/sfx/hit_1.ogg" };
        float hitSfxVolume = 1.0f;
        Canis::SceneAssetHandle deathEffectPrefab = { .path = "assets/prefabs/brawler_death_effect.prefab" };

        explicit MageStateMachine(Canis::Entity& _entity);

        MageIdleState idleState;
        TargetState targetState;

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

        void ResetSpellPose();
        void TakeDamage(int _damage);
        bool IsAlive() const;
        int GetMaxHealth() const;
        void Heal(int _amount);

    private:
        int health = 0;
        float stateTime = 0.0f;
        Canis::Vector4 m_baseColor = Canis::Vector4(1.0f);
        bool m_hasBaseColor = false;
        bool m_useFirstHitSfx = true;
    };

    void RegisterMageStateMachineScript(Canis::App& _app);
    void UnregisterMageStateMachineScript(Canis::App& _app);
}