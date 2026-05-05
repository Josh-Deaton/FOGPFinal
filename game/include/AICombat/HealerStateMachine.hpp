#pragma once

#include <Canis/Entity.hpp>
#include <SuperPupUtilities/StateMachine.hpp>

#include <string>

namespace AICombat
{
    class HealerIdleState;
    class HealerSupportState;
    class HealerHealingState;

    class HealerStateMachine : public SuperPupUtilities::StateMachine
    {
    public:
        static constexpr const char* ScriptName = "AICombat::HealerStateMachine";

        std::string allyTag = "Blue";
        float detectionRange = 24.0f;
        float supportDistance = 2.0f;
        float moveSpeed = 3.2f;
        float healRate = 2.0f;
        Canis::AudioAssetHandle healSfxPath = { .path = "assets/audio/sfx/hit_1.ogg" };
        float healSfxVolume = 0.14f;
        int maxHealth = 26;

        explicit HealerStateMachine(Canis::Entity& _entity);

        void Create() override;
        void Ready() override;
        void Destroy() override;
        void Update(float _dt) override;

        Canis::Entity* FindSupportTarget() const;
        bool IsTargetBeingHealedByOther(const Canis::Entity& _target) const;
        float DistanceTo(const Canis::Entity& _target) const;
        Canis::Vector3 GetSupportPosition(const Canis::Entity& _target) const;
        void MoveTowardsPosition(const Canis::Vector3& _position, float _speed, float _dt);
        void FaceTarget(const Canis::Entity& _target);

        bool IsAlive() const;
        void TakeDamage(int _amount);
        int GetCurrentHealth() const;
        int GetMaxHealth() const;
        void Heal(int _amount);
        const Canis::Entity* GetTargetEntity() const;

    public:
        friend class HealerIdleState;
        friend class HealerSupportState;
        friend class HealerHealingState;

    private:
        int m_currentHealth = 0;
        float m_healAccumulator = 0.0f;
        Canis::Entity* m_target = nullptr;
        bool m_hasBaseColor = false;
        Canis::Vector4 m_baseColor = Canis::Vector4(1.0f);
    };

    class HealerIdleState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "Idle";

        HealerIdleState(SuperPupUtilities::StateMachine& _stateMachine);
        void Update(float _dt) override;
    };

    class HealerSupportState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "Support";

        HealerSupportState(SuperPupUtilities::StateMachine& _stateMachine);
        void Enter() override;
        void Update(float _dt) override;
    };

    class HealerHealingState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "Healing";

        HealerHealingState(SuperPupUtilities::StateMachine& _stateMachine);
        void Enter() override;
        void Update(float _dt) override;

    private:
        float m_secondsSinceSound = 0.0f;
    };

    void RegisterHealerStateMachineScript(Canis::App& _app);
    void UnRegisterHealerStateMachineScript(Canis::App& _app);
}
