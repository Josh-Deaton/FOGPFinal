#include <AICombat/BrawlerStateMachine.hpp>

#include <Canis/App.hpp>
#include <Canis/AudioManager.hpp>
#include <Canis/ConfigHelper.hpp>
#include <Canis/Debug.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace AICombat
{
    namespace
    {
        ScriptConf brawlerStateMachineConf = {};
    }

    BrawlerIdleState::BrawlerIdleState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void BrawlerIdleState::Enter()
    {
        if (BrawlerStateMachine* brawlerStatMachine = dynamic_cast<BrawlerStateMachine*>(m_stateMachine))
            brawlerStatMachine->ResetHammerPose();
    }

    void BrawlerIdleState::Update(float)
    {
        if (BrawlerStateMachine* brawlerStatMachine = dynamic_cast<BrawlerStateMachine*>(m_stateMachine))
        {
            if (brawlerStatMachine->FindClosestTarget() != nullptr)
                brawlerStatMachine->ChangeState(ChaseState::Name);
        }
    }

    ChaseState::ChaseState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void ChaseState::Enter()
    {
        if (BrawlerStateMachine* brawlerStatMachine = dynamic_cast<BrawlerStateMachine*>(m_stateMachine))
            brawlerStatMachine->ResetHammerPose();
    }

    void ChaseState::Update(float _dt)
    {
        BrawlerStateMachine* brawlerStatMachine = dynamic_cast<BrawlerStateMachine*>(m_stateMachine);
        if (brawlerStatMachine == nullptr)
            return;

        Canis::Entity* target = brawlerStatMachine->FindClosestTarget();

        if (target == nullptr)
        {
            brawlerStatMachine->ChangeState(BrawlerIdleState::Name);
            return;
        }

        brawlerStatMachine->FaceTarget(*target);

        if (brawlerStatMachine->DistanceTo(*target) <= brawlerStatMachine->GetAttackRange())
        {
            brawlerStatMachine->ChangeState(HammerTimeState::Name);
            return;
        }

        brawlerStatMachine->MoveTowards(*target, moveSpeed, _dt);
    }

    HammerTimeState::HammerTimeState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void HammerTimeState::Enter()
    {
        if (BrawlerStateMachine* brawlerStatMachine = dynamic_cast<BrawlerStateMachine*>(m_stateMachine))
            brawlerStatMachine->SetHammerSwing(0.0f);
    }

    void HammerTimeState::Update(float)
    {
        BrawlerStateMachine* brawlerStatMachine = dynamic_cast<BrawlerStateMachine*>(m_stateMachine);
        if (brawlerStatMachine == nullptr)
            return;

        if (Canis::Entity* target = brawlerStatMachine->FindClosestTarget())
            brawlerStatMachine->FaceTarget(*target);

        const float duration = std::max(attackDuration, 0.001f);
        brawlerStatMachine->SetHammerSwing(brawlerStatMachine->GetStateTime() / duration);

        if (brawlerStatMachine->GetStateTime() < duration)
            return;

        if (brawlerStatMachine->FindClosestTarget() != nullptr)
            brawlerStatMachine->ChangeState(ChaseState::Name);
        else
            brawlerStatMachine->ChangeState(BrawlerIdleState::Name);
    }

    void HammerTimeState::Exit()
    {
        if (BrawlerStateMachine* brawlerStatMachine = dynamic_cast<BrawlerStateMachine*>(m_stateMachine))
            brawlerStatMachine->ResetHammerPose();
    }

    BrawlerStateMachine::BrawlerStateMachine(Canis::Entity& _entity) :
        SuperPupUtilities::StateMachine(_entity),
        idleState(*this),
        chaseState(*this),
        hammerTimeState(*this) {}

    void RegisterBrawlerStateMachineScript(Canis::App& _app)
    {
        REGISTER_PROPERTY(brawlerStateMachineConf, AICombat::BrawlerStateMachine, targetTag);
        REGISTER_PROPERTY(brawlerStateMachineConf, AICombat::BrawlerStateMachine, detectionRange);
        REGISTER_PROPERTY(brawlerStateMachineConf, AICombat::BrawlerStateMachine, bodyColliderSize);
        RegisterAccessorProperty(brawlerStateMachineConf, AICombat::BrawlerStateMachine, chaseState, moveSpeed);
        RegisterAccessorProperty(brawlerStateMachineConf, AICombat::BrawlerStateMachine, hammerTimeState, hammerRestDegrees);
        RegisterAccessorProperty(brawlerStateMachineConf, AICombat::BrawlerStateMachine, hammerTimeState, hammerSwingDegrees);
        RegisterAccessorProperty(brawlerStateMachineConf, AICombat::BrawlerStateMachine, hammerTimeState, attackRange);
        RegisterAccessorProperty(brawlerStateMachineConf, AICombat::BrawlerStateMachine, hammerTimeState, attackDuration);
        RegisterAccessorProperty(brawlerStateMachineConf, AICombat::BrawlerStateMachine, hammerTimeState, attackDamageTime);
        REGISTER_PROPERTY(brawlerStateMachineConf, AICombat::BrawlerStateMachine, maxHealth);
        REGISTER_PROPERTY(brawlerStateMachineConf, AICombat::BrawlerStateMachine, logStateChanges);
        REGISTER_PROPERTY(brawlerStateMachineConf, AICombat::BrawlerStateMachine, hammerVisual);
        REGISTER_PROPERTY(brawlerStateMachineConf, AICombat::BrawlerStateMachine, hitSfxPath1);
        REGISTER_PROPERTY(brawlerStateMachineConf, AICombat::BrawlerStateMachine, hitSfxPath2);
        REGISTER_PROPERTY(brawlerStateMachineConf, AICombat::BrawlerStateMachine, hitSfxVolume);
        REGISTER_PROPERTY(brawlerStateMachineConf, AICombat::BrawlerStateMachine, deathEffectPrefab);

        DEFAULT_CONFIG_AND_REQUIRED(
            brawlerStateMachineConf,
            AICombat::BrawlerStateMachine,
            Canis::Transform,
            Canis::Material,
            Canis::Model,
            Canis::Rigidbody,
            Canis::BoxCollider);

        brawlerStateMachineConf.DEFAULT_DRAW_INSPECTOR(AICombat::BrawlerStateMachine);

        _app.RegisterScript(brawlerStateMachineConf);
    }

    DEFAULT_UNREGISTER_SCRIPT(brawlerStateMachineConf, BrawlerStateMachine)

    void BrawlerStateMachine::Create()
    {
        entity.GetComponent<Canis::Transform>();

        Canis::Rigidbody& rigidbody = entity.GetComponent<Canis::Rigidbody>();
        rigidbody.motionType = Canis::RigidbodyMotionType::KINEMATIC;
        rigidbody.useGravity = false;
        rigidbody.allowSleeping = false;
        rigidbody.linearVelocity = Canis::Vector3(0.0f);
        rigidbody.angularVelocity = Canis::Vector3(0.0f);

        entity.GetComponent<Canis::BoxCollider>().size = bodyColliderSize;

        if (entity.HasComponent<Canis::Material>())
        {
            m_baseColor = entity.GetComponent<Canis::Material>().color;
            m_hasBaseColor = true;
        }
    }

    void BrawlerStateMachine::Ready()
    {
        if (entity.HasComponent<Canis::Material>())
        {
            m_baseColor = entity.GetComponent<Canis::Material>().color;
            m_hasBaseColor = true;
        }

        m_currentHealth = std::max(maxHealth, 1);
        m_stateTime = 0.0f;
        m_useFirstHitSfx = true;

        ClearStates();
        AddState(idleState);
        AddState(chaseState);
        AddState(hammerTimeState);

        ResetHammerPose();
        ChangeState(BrawlerIdleState::Name);
    }

    void BrawlerStateMachine::Destroy()
    {
        hammerVisual = nullptr;
        SuperPupUtilities::StateMachine::Destroy();
    }

    void BrawlerStateMachine::Update(float _dt)
    {
        if (!IsAlive())
            return;

        m_stateTime += _dt;
        SuperPupUtilities::StateMachine::Update(_dt);
    }

    Canis::Entity* BrawlerStateMachine::FindClosestTarget() const
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

            if (const BrawlerStateMachine* other = candidate->GetScript<BrawlerStateMachine>())
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

    float BrawlerStateMachine::DistanceTo(const Canis::Entity& _other) const
    {
        if (!entity.HasComponent<Canis::Transform>() || !_other.HasComponent<Canis::Transform>())
            return std::numeric_limits<float>::max();

        const Canis::Vector3 selfPosition = entity.GetComponent<Canis::Transform>().GetGlobalPosition();
        const Canis::Vector3 targetPosition = _other.GetComponent<Canis::Transform>().GetGlobalPosition();
        return glm::distance(selfPosition, targetPosition);
    }

    void BrawlerStateMachine::FaceTarget(const Canis::Entity& _target)
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

    void BrawlerStateMachine::MoveTowards(const Canis::Entity& _target, float _speed, float _dt)
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

    void BrawlerStateMachine::ChangeState(const std::string& _stateName)
    {
        if (SuperPupUtilities::StateMachine::GetCurrentStateName() == _stateName)
            return;

        if (!SuperPupUtilities::StateMachine::ChangeState(_stateName))
            return;

        m_stateTime = 0.0f;

        if (logStateChanges)
            Canis::Debug::Log("%s -> %s", entity.name.c_str(), _stateName.c_str());
    }

    const std::string& BrawlerStateMachine::GetCurrentStateName() const
    {
        return SuperPupUtilities::StateMachine::GetCurrentStateName();
    }

    float BrawlerStateMachine::GetStateTime() const
    {
        return m_stateTime;
    }

    float BrawlerStateMachine::GetAttackRange() const
    {
        return hammerTimeState.attackRange;
    }

    int BrawlerStateMachine::GetCurrentHealth() const
    {
        return m_currentHealth;
    }

    void BrawlerStateMachine::ResetHammerPose()
    {
        SetHammerSwing(0.0f);
    }

    void BrawlerStateMachine::SetHammerSwing(float _normalized)
    {
        if (hammerVisual == nullptr || !hammerVisual->HasComponent<Canis::Transform>())
            return;

        Canis::Transform& hammerTransform = hammerVisual->GetComponent<Canis::Transform>();
        const float normalized = Clamp01(_normalized);
        const float swingBlend = (normalized <= 0.5f)
            ? normalized * 2.0f
            : (1.0f - normalized) * 2.0f;

        hammerTransform.rotation.x = DEG2RAD *
            (hammerTimeState.hammerRestDegrees + (hammerTimeState.hammerSwingDegrees * swingBlend));
    }

    void BrawlerStateMachine::TakeDamage(int _damage)
    {
        if (!IsAlive())
            return;

        const int damageToApply = std::max(_damage, 0);
        if (damageToApply <= 0)
            return;

        m_currentHealth = std::max(0, m_currentHealth - damageToApply);
        PlayHitSfx();

        if (m_hasBaseColor && entity.HasComponent<Canis::Material>())
        {
            Canis::Material& material = entity.GetComponent<Canis::Material>();
            const float healthRatio = (maxHealth > 0)
                ? (static_cast<float>(m_currentHealth) / static_cast<float>(maxHealth))
                : 0.0f;

            material.color = Canis::Vector4(
                m_baseColor.x * (0.5f + (0.5f * healthRatio)),
                m_baseColor.y * (0.5f + (0.5f * healthRatio)),
                m_baseColor.z * (0.5f + (0.5f * healthRatio)),
                m_baseColor.w);
        }

        if (m_currentHealth > 0)
            return;

        if (logStateChanges)
            Canis::Debug::Log("%s was defeated.", entity.name.c_str());

        SpawnDeathEffect();
        entity.Destroy();
    }

    void BrawlerStateMachine::PlayHitSfx()
    {
        const Canis::AudioAssetHandle& selectedSfx = m_useFirstHitSfx ? hitSfxPath1 : hitSfxPath2;
        m_useFirstHitSfx = !m_useFirstHitSfx;

        if (selectedSfx.Empty())
            return;

        Canis::AudioManager::PlaySFX(selectedSfx, std::clamp(hitSfxVolume, 0.0f, 1.0f));
    }

    void BrawlerStateMachine::SpawnDeathEffect()
    {
        if (deathEffectPrefab.Empty() || !entity.HasComponent<Canis::Transform>())
            return;

        const Canis::Transform& sourceTransform = entity.GetComponent<Canis::Transform>();
        const Canis::Vector3 spawnPosition = sourceTransform.GetGlobalPosition();
        const Canis::Vector3 spawnRotation = sourceTransform.GetGlobalRotation();

        for (Canis::Entity* spawnedEntity : entity.scene.Instantiate(deathEffectPrefab))
        {
            if (spawnedEntity == nullptr || !spawnedEntity->HasComponent<Canis::Transform>())
                continue;

            Canis::Transform& spawnedTransform = spawnedEntity->GetComponent<Canis::Transform>();
            spawnedTransform.position = spawnPosition;
            spawnedTransform.rotation = spawnRotation;
        }
    }

    bool BrawlerStateMachine::IsAlive() const
    {
        return m_currentHealth > 0;
    }

    int BrawlerStateMachine::GetMaxHealth() const
    {
        return maxHealth;
    }

    void BrawlerStateMachine::Heal(int _amount)
    {
        m_currentHealth += _amount;
        if (m_currentHealth > maxHealth)
            m_currentHealth = maxHealth;
    }
}
