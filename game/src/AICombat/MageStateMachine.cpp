#include <AICombat/MageStateMachine.hpp>

#include <Canis/App.hpp>
#include <Canis/AudioManager.hpp>
#include <Canis/ConfigHelper.hpp>
#include <Canis/Debug.hpp>
#include <SuperPupUtilities/Bullet.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

using namespace Canis;

namespace AICombat
{
    namespace
    {
        ScriptConf mageStateMachineConf = {};
    }

    MageIdleState::MageIdleState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}
    
    void MageIdleState::Enter()
    {
        if (MageStateMachine* mageStateMachine = dynamic_cast<MageStateMachine*>(m_stateMachine))
            mageStateMachine->ResetSpellPose();
    }

    void MageIdleState::Update(float)
    {
        if (MageStateMachine* mageStateMachine = dynamic_cast<MageStateMachine*>(m_stateMachine))
        {
            if (mageStateMachine->FindClosestTarget() != nullptr)
                mageStateMachine->ChangeState(TargetState::Name);
        }
    }

    TargetState::TargetState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void TargetState::Enter()
    {
        m_timeSinceEnter = 0.0f;
    }

    void TargetState::Update(float _dt)
    {
        if (MageStateMachine* mageStateMachine = dynamic_cast<MageStateMachine*>(m_stateMachine))
        {
            m_timeSinceEnter += _dt;

            if (m_timeSinceEnter >= 0.5f && m_timeSinceEnter - _dt < 0.5f)
            {
                Entity* target = mageStateMachine->FindClosestTarget();
                if (target != nullptr && target->HasComponent<Transform>())
                {
                    const Transform& sourceTransform = mageStateMachine->entity.GetComponent<Transform>();
                    const Transform& targetTransform = target->GetComponent<Transform>();
                    
                    Vector3 toTarget = targetTransform.GetGlobalPosition() - sourceTransform.GetGlobalPosition();
                    toTarget = glm::normalize(toTarget);

                    if (mageStateMachine->spellVisual != nullptr && 
                        mageStateMachine->spellVisual->HasComponent<Transform>())
                    {
                        Transform& bulletTransform = mageStateMachine->spellVisual->GetComponent<Transform>();
                        bulletTransform.position = sourceTransform.GetGlobalPosition();
                        
                        if (glm::length(toTarget) > 0.001f)
                        {
                            float yaw = std::atan2(-toTarget.x, -toTarget.z);
                            bulletTransform.rotation = Vector3(0.0f, yaw, 0.0f);
                        }

                        if (SuperPupUtilities::Bullet* bullet = mageStateMachine->spellVisual->GetScript<SuperPupUtilities::Bullet>())
                        {
                            bullet->Launch();
                        }
                    }
                }
            }

            if (m_timeSinceEnter >= spellCastDuration)
            {
                mageStateMachine->ChangeState(MageIdleState::Name);
            }
        }
    }

    void TargetState::Exit()
    {}

    int MageStateMachine::GetMaxHealth() const
    {
        return maxHealth;
    }

    void MageStateMachine::Heal(int _amount)
    {
        health += _amount;
        if (health > maxHealth)
            health = maxHealth;
    }

    Canis::Entity* MageStateMachine::FindClosestTarget() const
    {
        if (targetTag.empty() || !entity.HasComponent<Canis::Transform>())
            return nullptr;

        const Canis::Transform& transform = entity.GetComponent<Canis::Transform>();
        const Canis::Vector3 origin = transform.GetGlobalPosition();
        Canis::Entity* closestTarget = nullptr;
        float closestDistance = detectionRange;

        for (Canis::Entity* candidate : entity.scene.GetEntitiesWithTag(targetTag))
        {
            if (candidate == nullptr || candidate == &entity || !candidate->active)
                continue;

            if (!candidate->HasComponent<Canis::Transform>())
                continue;

            if (const MageStateMachine* other = candidate->GetScript<MageStateMachine>())
            {
                if (!other->IsAlive())
                    continue;
            }

            const Canis::Vector3 candidatePosition = candidate->GetComponent<Canis::Transform>().GetGlobalPosition();
            const float distance = glm::distance(origin, candidatePosition);

            if (distance > detectionRange || distance >= closestDistance)
                continue;

            closestDistance = distance;
            closestTarget = candidate;
        }

        return closestTarget;
    }

    float MageStateMachine::DistanceTo(const Canis::Entity& _other) const
    {
        if (!entity.HasComponent<Canis::Transform>() || !_other.HasComponent<Canis::Transform>())
            return std::numeric_limits<float>::max();

        const Canis::Vector3 selfPosition = entity.GetComponent<Canis::Transform>().GetGlobalPosition();
        const Canis::Vector3 targetPosition = _other.GetComponent<Canis::Transform>().GetGlobalPosition();
        return glm::distance(selfPosition, targetPosition);
    }

    void MageStateMachine::FaceTarget(const Canis::Entity& _target)
    {
        if (!entity.HasComponent<Canis::Transform>() || !_target.HasComponent<Canis::Transform>())
            return;

        Canis::Transform& transform = entity.GetComponent<Canis::Transform>();
        const Canis::Vector3 selfPosition = transform.GetGlobalPosition();
        Canis::Vector3 direction = _target.GetComponent<Canis::Transform>().GetGlobalPosition() - selfPosition;
        direction.y = 0.0f;

        if (glm::dot(direction, direction) <= 0.0001f)
            return;

        direction = glm::normalize(direction);
        transform.rotation.y = std::atan2(-direction.x, -direction.z);
    }

    void MageStateMachine::MoveTowards(const Canis::Entity& _target, float _speed, float _dt)
    {
        if (!entity.HasComponent<Canis::Transform>() || !_target.HasComponent<Canis::Transform>())
            return;

        Canis::Transform& transform = entity.GetComponent<Canis::Transform>();
        const Canis::Vector3 selfPosition = transform.GetGlobalPosition();
        Canis::Vector3 direction = _target.GetComponent<Canis::Transform>().GetGlobalPosition() - selfPosition;
        direction.y = 0.0f;

        if (glm::dot(direction, direction) <= 0.0001f)
            return;

        direction = glm::normalize(direction);
        transform.position += direction * _speed * _dt;
    }

    void MageStateMachine::ChangeState(const std::string& _stateName)
    {
        if (SuperPupUtilities::StateMachine::GetCurrentStateName() == _stateName)
            return;

        if (!SuperPupUtilities::StateMachine::ChangeState(_stateName))
            return;

        stateTime = 0.0f;

        if (logStateChanges)
            Canis::Debug::Log("%s -> %s", entity.name.c_str(), _stateName.c_str());
    }

    const std::string& MageStateMachine::GetCurrentStateName() const
    {
        return SuperPupUtilities::StateMachine::GetCurrentStateName();
    }

    float MageStateMachine::GetStateTime() const
    {
        return stateTime;
    }

    float MageStateMachine::GetAttackRange() const
    {
        return targetState.spellCastRange;
    }

    int MageStateMachine::GetCurrentHealth() const
    {
        return health;
    }

    void MageStateMachine::ResetSpellPose()
    {
        // TODO: implement
    }

    void MageStateMachine::TakeDamage(int _damage)
    {
        if (!IsAlive())
            return;

        const int damageToApply = std::max(_damage, 0);
        if (damageToApply <= 0)
            return;

        health = std::max(0, health - damageToApply);
        // TODO: play hit sfx, change color, etc.
    }

    bool MageStateMachine::IsAlive() const
    {
        return health > 0;
    }
}