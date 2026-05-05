#include <AICombat/HealerStateMachine.hpp>

#include <AICombat/BrawlerStateMachine.hpp>
#include <AICombat/MageStateMachine.hpp>
#include <Canis/App.hpp>
#include <Canis/AudioManager.hpp>
#include <Canis/ConfigHelper.hpp>
#include <Canis/Debug.hpp>
#include <SuperPupUtilities/Bullet.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace AICombat
{
    namespace
    {
        ScriptConf healerStateMachineConf = {};
    }

    HealerIdleState::HealerIdleState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void HealerIdleState::Update(float _dt)
    {
        if (HealerStateMachine* healer = dynamic_cast<HealerStateMachine*>(m_stateMachine))
        {
            Canis::Entity* target = healer->FindSupportTarget();
            if (target != nullptr)
            {
                healer->m_target = target;
                healer->ChangeState(HealerSupportState::Name);
            }
        }
    }

    HealerSupportState::HealerSupportState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void HealerSupportState::Enter()
    {
        if (HealerStateMachine* healer = dynamic_cast<HealerStateMachine*>(m_stateMachine))
        {
            if (healer->m_target == nullptr)
            {
                healer->ChangeState(HealerIdleState::Name);
                return;
            }
        }
    }

    void HealerSupportState::Update(float _dt)
    {
        HealerStateMachine* healer = dynamic_cast<HealerStateMachine*>(m_stateMachine);
        if (healer == nullptr)
            return;

        if (healer->m_target == nullptr || !healer->m_target->active || !healer->m_target->HasComponent<Canis::Transform>())
        {
            healer->m_target = nullptr;
            healer->ChangeState(HealerIdleState::Name);
            return;
        }

        if (!healer->m_target->HasScript<AICombat::BrawlerStateMachine>() &&
            !healer->m_target->HasScript<AICombat::MageStateMachine>() &&
            !healer->m_target->HasScript<AICombat::HealerStateMachine>())
        {
            healer->m_target = nullptr;
            healer->ChangeState(HealerIdleState::Name);
            return;
        }

        if (healer->IsTargetBeingHealedByOther(*healer->m_target))
        {
            healer->m_target = nullptr;
            healer->ChangeState(HealerIdleState::Name);
            return;
        }

        const float distanceToSupport = glm::distance(healer->entity.GetComponent<Canis::Transform>().GetGlobalPosition(), healer->GetSupportPosition(*healer->m_target));
        if (distanceToSupport > 0.3f)
        {
            healer->MoveTowardsPosition(healer->GetSupportPosition(*healer->m_target), healer->moveSpeed, _dt);
            return;
        }

        healer->ChangeState(HealerHealingState::Name);
    }

    HealerHealingState::HealerHealingState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void HealerHealingState::Enter()
    {
        m_secondsSinceSound = 0.0f;
    }

    void HealerHealingState::Update(float _dt)
    {
        HealerStateMachine* healer = dynamic_cast<HealerStateMachine*>(m_stateMachine);
        if (healer == nullptr)
            return;

        if (healer->m_target == nullptr || !healer->m_target->active || !healer->m_target->HasComponent<Canis::Transform>())
        {
            healer->m_target = nullptr;
            healer->ChangeState(HealerIdleState::Name);
            return;
        }

        if (healer->IsTargetBeingHealedByOther(*healer->m_target) && healer->GetTargetEntity() != healer->m_target)
        {
            healer->m_target = nullptr;
            healer->ChangeState(HealerIdleState::Name);
            return;
        }

        AICombat::BrawlerStateMachine* brawler = healer->m_target->GetScript<AICombat::BrawlerStateMachine>();
        AICombat::MageStateMachine* mage = healer->m_target->GetScript<AICombat::MageStateMachine>();
        AICombat::HealerStateMachine* otherHealer = healer->m_target->GetScript<AICombat::HealerStateMachine>();
        int targetCurrentHealth = 0;
        int targetMaxHealth = 0;

        if (brawler != nullptr)
        {
            targetCurrentHealth = brawler->GetCurrentHealth();
            targetMaxHealth = brawler->maxHealth;
        }
        else if (mage != nullptr)
        {
            targetCurrentHealth = mage->GetCurrentHealth();
            targetMaxHealth = mage->maxHealth;
        }
        else if (otherHealer != nullptr)
        {
            targetCurrentHealth = otherHealer->GetCurrentHealth();
            targetMaxHealth = otherHealer->GetMaxHealth();
        }

        if (targetCurrentHealth >= targetMaxHealth)
        {
            healer->m_target = nullptr;
            healer->ChangeState(HealerIdleState::Name);
            return;
        }

        healer->m_healAccumulator += healer->healRate * _dt;
        int healAmount = static_cast<int>(std::floor(healer->m_healAccumulator));
        if (healAmount > 0)
        {
            healer->m_healAccumulator -= healAmount;
            if (brawler != nullptr)
                brawler->Heal(healAmount);
            else if (mage != nullptr)
                mage->Heal(healAmount);
            else if (otherHealer != nullptr)
                otherHealer->Heal(healAmount);
        }

        m_secondsSinceSound += _dt;
        if (m_secondsSinceSound >= 1.0f)
        {
            if (healer->healSfxPath.uuid != Canis::UUID(0) || !healer->healSfxPath.path.empty())
                Canis::AudioManager::PlaySFX(healer->healSfxPath, std::clamp(healer->healSfxVolume, 0.0f, 1.0f));
            m_secondsSinceSound = 0.0f;
        }

        if (glm::distance(healer->entity.GetComponent<Canis::Transform>().GetGlobalPosition(), healer->GetSupportPosition(*healer->m_target)) > 0.85f)
        {
            healer->ChangeState(HealerSupportState::Name);
            return;
        }
    }

    HealerStateMachine::HealerStateMachine(Canis::Entity& _entity) :
        SuperPupUtilities::StateMachine(_entity) {}

    void HealerStateMachine::Create()
    {
        entity.GetComponent<Canis::Transform>();
    }

    void HealerStateMachine::Ready()
    {
        m_currentHealth = maxHealth;
        m_healAccumulator = 0.0f;
        m_hasBaseColor = entity.HasComponent<Canis::Material>();

        if (m_hasBaseColor)
            m_baseColor = entity.GetComponent<Canis::Material>().color;

        AddState(*new HealerIdleState(*this));
        AddState(*new HealerSupportState(*this));
        AddState(*new HealerHealingState(*this));
        ChangeState(HealerIdleState::Name);
    }

    void HealerStateMachine::Destroy() {}

    void HealerStateMachine::Update(float _dt)
    {
        SuperPupUtilities::StateMachine::Update(_dt);
    }

    Canis::Entity* HealerStateMachine::FindSupportTarget() const
    {
        Canis::Entity* lowestTarget = nullptr;
        int lowestHealth = std::numeric_limits<int>::max();

        for (Canis::Entity* candidate : entity.scene.GetEntitiesWithTag(allyTag))
        {
            if (candidate == nullptr || candidate == &entity || !candidate->active || !candidate->HasComponent<Canis::Transform>())
                continue;

            AICombat::BrawlerStateMachine* brawler = candidate->GetScript<AICombat::BrawlerStateMachine>();
            AICombat::MageStateMachine* mage = candidate->GetScript<AICombat::MageStateMachine>();
            AICombat::HealerStateMachine* healer = candidate->GetScript<AICombat::HealerStateMachine>();
            int currentHealth = 0;
            int maxHealthValue = 0;

            if (brawler != nullptr)
            {
                if (!brawler->IsAlive())
                    continue;
                currentHealth = brawler->GetCurrentHealth();
                maxHealthValue = brawler->GetMaxHealth();
            }
            else if (mage != nullptr)
            {
                if (!mage->IsAlive())
                    continue;
                currentHealth = mage->GetCurrentHealth();
                maxHealthValue = mage->GetMaxHealth();
            }
            else if (healer != nullptr)
            {
                if (!healer->IsAlive())
                    continue;
                currentHealth = healer->GetCurrentHealth();
                maxHealthValue = healer->GetMaxHealth();
            }
            else
            {
                continue;
            }

            if (currentHealth >= maxHealthValue)
                continue;

            const float distance = glm::distance(candidate->GetComponent<Canis::Transform>().GetGlobalPosition(), entity.GetComponent<Canis::Transform>().GetGlobalPosition());
            if (distance > detectionRange)
                continue;

            if (IsTargetBeingHealedByOther(*candidate))
                continue;

            if (currentHealth < lowestHealth)
            {
                lowestHealth = currentHealth;
                lowestTarget = candidate;
            }
        }

        return lowestTarget;
    }

    bool HealerStateMachine::IsTargetBeingHealedByOther(const Canis::Entity& _target) const
    {
        for (Canis::Entity* candidate : entity.scene.GetEntitiesWithTag(allyTag))
        {
            if (candidate == nullptr || candidate == &entity)
                continue;

            if (!candidate->HasScript<AICombat::HealerStateMachine>())
                continue;

            HealerStateMachine* otherHealer = candidate->GetScript<AICombat::HealerStateMachine>();
            if (otherHealer == nullptr)
                continue;

            if (otherHealer->GetCurrentStateName() != HealerHealingState::Name)
                continue;

            const Canis::Entity* targetEntity = otherHealer->GetTargetEntity();
            if (targetEntity == nullptr)
                continue;

            if (targetEntity == &_target)
                return true;
        }

        return false;
    }

    float HealerStateMachine::DistanceTo(const Canis::Entity& _target) const
    {
        return glm::distance(_target.GetComponent<Canis::Transform>().GetGlobalPosition(), entity.GetComponent<Canis::Transform>().GetGlobalPosition());
    }

    Canis::Vector3 HealerStateMachine::GetSupportPosition(const Canis::Entity& _target) const
    {
        Canis::Vector3 forward = _target.GetComponent<Canis::Transform>().GetForward();
        if (glm::length(forward) < 0.0001f)
            forward = Canis::Vector3(0.0f, 0.0f, 1.0f);

        const Canis::Vector3 targetPosition = _target.GetComponent<Canis::Transform>().GetGlobalPosition();
        return targetPosition - glm::normalize(forward) * supportDistance;
    }

    void HealerStateMachine::MoveTowardsPosition(const Canis::Vector3& _position, float _speed, float _dt)
    {
        Canis::Transform& transform = entity.GetComponent<Canis::Transform>();
        const Canis::Vector3 currentPosition = transform.GetGlobalPosition();
        Canis::Vector3 desiredDirection = _position - currentPosition;
        desiredDirection.y = 0.0f;

        if (glm::length(desiredDirection) <= 0.001f)
            return;

        desiredDirection = glm::normalize(desiredDirection);
        transform.position += desiredDirection * _speed * _dt;
        FaceTarget(*m_target);
    }

    void HealerStateMachine::FaceTarget(const Canis::Entity& _target)
    {
        Canis::Transform& transform = entity.GetComponent<Canis::Transform>();
        Canis::Vector3 direction = _target.GetComponent<Canis::Transform>().GetGlobalPosition() - transform.GetGlobalPosition();
        direction.y = 0.0f;

        if (glm::length(direction) <= 0.001f)
            return;

        const Canis::Vector3 forward = glm::normalize(direction);
        transform.rotation.y = std::atan2(-forward.x, -forward.z);
    }

    bool HealerStateMachine::IsAlive() const
    {
        return m_currentHealth > 0;
    }

    void HealerStateMachine::TakeDamage(int _amount)
    {
        if (_amount <= 0)
            return;

        m_currentHealth = std::max(0, m_currentHealth - _amount);
    }

    int HealerStateMachine::GetCurrentHealth() const
    {
        return m_currentHealth;
    }

    int HealerStateMachine::GetMaxHealth() const
    {
        return maxHealth;
    }

    void HealerStateMachine::Heal(int _amount)
    {
        if (!IsAlive() || _amount <= 0)
            return;

        m_currentHealth = std::min(maxHealth, m_currentHealth + _amount);
    }

    const Canis::Entity* HealerStateMachine::GetTargetEntity() const
    {
        return m_target;
    }

    void RegisterHealerStateMachineScript(Canis::App& _app)
    {
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, allyTag);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, detectionRange);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, supportDistance);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, moveSpeed);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, healRate);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, healSfxPath);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, healSfxVolume);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, maxHealth);

        DEFAULT_CONFIG_AND_REQUIRED(healerStateMachineConf, AICombat::HealerStateMachine, Canis::Transform);
        healerStateMachineConf.DEFAULT_DRAW_INSPECTOR(AICombat::HealerStateMachine);

        _app.RegisterScript(healerStateMachineConf);
    }

    DEFAULT_UNREGISTER_SCRIPT(healerStateMachineConf, HealerStateMachine)
}