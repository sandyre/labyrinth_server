//
//  unit.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "unit.hpp"

#include "../effect.hpp"
#include "../gameworld.hpp"
#include "../../GameMessage.h"

#include <chrono>
using namespace std::chrono_literals;


Unit::Unit(GameWorld& world, uint32_t uid)
: GameObject(world, uid),
  _logger(("Unit" + std::to_string(uid)), NamedLogger::Mode::STDIO),
  _unitType(Unit::Type::UNDEFINED),
  _state(Unit::State::UNDEFINED),
  _orientation(Unit::Orientation::DOWN),
  _damage(10, 0, 100),
  _health(50, 0, 50),
  _armor(2, 0, 100),
  _resistance(2, 0, 100),
  _moveSpeed(0.5, 0.0, 1.0)
{
    _name = "Unit";
    _objType = GameObject::Type::UNIT;
    _objAttributes |= GameObject::Attributes::DAMAGABLE | GameObject::Attributes::MOVABLE;
    _unitAttributes = Unit::Attributes::INPUT | Unit::Attributes::ATTACK | Unit::Attributes::DUELABLE;
}


void
Unit::ApplyEffect(std::shared_ptr<Effect> effect)
{
        // Log item drop event
    _logger.Info() << effect->GetName() << " effect is applied";
    _effectsManager.AddEffect(effect);
}


void
Unit::update(std::chrono::microseconds delta)
{
    _effectsManager.Update(delta);
    _cdManager.Update(delta);
}


void
Unit::TakeItem(std::shared_ptr<Item> item)
{
        // Log item drop event
    _logger.Info() << "Took item " << item->GetName();
    _inventory.push_back(item);
    
    flatbuffers::FlatBufferBuilder builder;
    auto take = GameMessage::CreateSVActionItem(builder,
                                                this->GetUID(),
                                                item->GetUID(),
                                                GameMessage::ActionItemType_TAKE);
    auto msg = GameMessage::CreateMessage(builder,
                                          0,
                                          GameMessage::Messages_SVActionItem,
                                          take.Union());
    builder.Finish(msg);
    _world._outputMessages.push_back(std::make_shared<MessageBuffer>(builder.GetCurrentBufferPointer(),
                                                                     builder.GetBufferPointer() + builder.GetSize()));
}


void
Unit::Spawn(const Point<>& pos)
{
        // Log spawn event
    _logger.Info() << "Spawned at " << pos;
    
    _state = Unit::State::WALKING;
    _objAttributes = GameObject::Attributes::MOVABLE | GameObject::Attributes::VISIBLE | GameObject::Attributes::DAMAGABLE;
    _unitAttributes = Unit::Attributes::INPUT | Unit::Attributes::ATTACK | Unit::Attributes::DUELABLE;
    _health = _health.Max();
    
    _pos = pos;
    
    flatbuffers::FlatBufferBuilder builder;
    auto spawn = GameMessage::CreateSVSpawnPlayer(builder,
                                                  this->GetUID(),
                                                  pos.x,
                                                  pos.y);
    auto msg = GameMessage::CreateMessage(builder,
                                          0,
                                          GameMessage::Messages_SVSpawnPlayer,
                                          spawn.Union());
    builder.Finish(msg);
    _world._outputMessages.push_back(std::make_shared<MessageBuffer>(builder.GetCurrentBufferPointer(),
                                                                     builder.GetBufferPointer() + builder.GetSize()));
}


void
Unit::Respawn(const Point<>& pos)
{
        // Log respawn event
    _logger.Info() << "Respawned at " << pos;
    
    _state = Unit::State::WALKING;
    _objAttributes = GameObject::Attributes::MOVABLE | GameObject::Attributes::VISIBLE | GameObject::Attributes::DAMAGABLE;
    _unitAttributes = Unit::Attributes::INPUT | Unit::Attributes::ATTACK | Unit::Attributes::DUELABLE;
    _health = _health.Max();
    
    _pos = pos;

    auto respBuff = std::make_shared<RespawnInvulnerability>(5s);
    respBuff->SetTargetUnit(std::static_pointer_cast<Unit>(shared_from_this()));
    this->ApplyEffect(respBuff);
    
    flatbuffers::FlatBufferBuilder builder;
    auto resp = GameMessage::CreateSVRespawnPlayer(builder,
                                                   this->GetUID(),
                                                   pos.x,
                                                   pos.y);
    auto msg = GameMessage::CreateMessage(builder,
                                          0,
                                          GameMessage::Messages_SVRespawnPlayer,
                                          resp.Union());
    builder.Finish(msg);
    _world._outputMessages.push_back(std::make_shared<MessageBuffer>(builder.GetCurrentBufferPointer(),
                                                                     builder.GetBufferPointer() + builder.GetSize()));}


std::shared_ptr<Item>
Unit::DropItem(int32_t uid)
{
    auto item = std::find_if(_inventory.begin(),
                             _inventory.end(),
                             [uid](const std::shared_ptr<Item>& item)
                             {
                                 return item->GetUID() == uid;
                             });

    if(item == _inventory.end())
    {
        _logger.Error() << "Failed to drop item with uid = " << uid << " (reason: does not have it)";
        return nullptr;
    }
    
        // Log item drop event
    _logger.Info() << "Drop item " << (*item)->GetName();
    (*item)->SetPosition(_pos);
    _world._objectsStorage.PushObject(*item);

    auto item_ptr = *item;
    _inventory.erase(item);
    return item_ptr;
}


void
Unit::Die(const std::string& killerName)
{
        // Log death event
    _logger.Info() << "Killed by " << killerName << " at " << _pos;
    
        // drop items
    auto items = _inventory;
    for(auto item : items)
        this->DropItem(item->GetUID());

    if (const auto enemy = _duelTarget.lock())
    {
        enemy->EndDuel();
        EndDuel();
    }
    
    flatbuffers::FlatBufferBuilder builder;
    auto move = GameMessage::CreateSVActionDeath(builder,
                                               this->GetUID(),
                                               0);
    auto msg = GameMessage::CreateMessage(builder,
                                        0,
                                        GameMessage::Messages_SVActionDeath,
                                        move.Union());
    builder.Finish(msg);
    _world._outputMessages.push_back(std::make_shared<MessageBuffer>(builder.GetCurrentBufferPointer(),
                                                                     builder.GetBufferPointer() + builder.GetSize()));
    
    _state = Unit::State::DEAD;
    _objAttributes = GameObject::Attributes::PASSABLE;
    _unitAttributes = 0;
    _health = 0;
    _world._respawner.Enqueue(std::static_pointer_cast<Unit>(shared_from_this()), 3s);
    _world._objectsStorage.DeleteObject(std::static_pointer_cast<Unit>(shared_from_this())); // FIXME: update wont be called when unit is dead. so CDs wont be updated. Discuss with Sergey
}


void
Unit::Move(MoveDirection dir)
{
    Point<> new_coord = _pos;
    if(dir == MoveDirection::LEFT)
        --new_coord.x;
    else if(dir == MoveDirection::RIGHT)
        ++new_coord.x;
    else if(dir == MoveDirection::UP)
        ++new_coord.y;
    else if(dir == MoveDirection::DOWN)
        --new_coord.y;
    
        // firstly - check if it can go there (no unpassable objects)
    for (auto iter = _world._objectsStorage.Begin(); iter != _world._objectsStorage.End(); ++iter)
    {
        if((*iter)->GetPosition() == new_coord &&
           !((*iter)->GetAttributes() & GameObject::Attributes::PASSABLE))
            return; // there is an unpassable object
    }

        // Log move event
    _logger.Debug() << "Move to " << new_coord;

    GameObject::Move(new_coord);
    
    flatbuffers::FlatBufferBuilder builder;
    auto move = GameMessage::CreateSVActionMove(builder,
                                                this->GetUID(),
                                                (char)dir,
                                                new_coord.x,
                                                new_coord.y);
    auto msg = GameMessage::CreateMessage(builder,
                                          0,
                                          GameMessage::Messages_SVActionMove,
                                          move.Union());
    builder.Finish(msg);
    _world._outputMessages.push_back(std::make_shared<MessageBuffer>(builder.GetCurrentBufferPointer(),
                                                                     builder.GetBufferPointer() + builder.GetSize()));
}


void
Unit::TakeDamage(const DamageDescriptor& dmg)
{
    int16_t damage_taken = dmg.Value;

    if(dmg.Type == DamageDescriptor::DamageType::PHYSICAL)
        damage_taken -= _armor;
    else if(dmg.Type == DamageDescriptor::DamageType::MAGICAL)
        damage_taken -= _resistance;
    
    _health -= damage_taken;
    
        // Log damage take event
    _logger.Info() << "HP: " << _health + damage_taken << " -> " << _health << " (reason: attack for " << damage_taken << " from " << dmg.DealerName << ")";
    
    if(_health == _health.Min())
        Die(dmg.DealerName);
}


void
Unit::StartDuel(std::shared_ptr<Unit> enemy)
{
    enemy->AcceptDuel(std::dynamic_pointer_cast<Unit>(shared_from_this()));

        // Log duel start event
    _logger.Info() << "Initiates duel with " << enemy->GetName();
    
    _state = Unit::State::DUEL;
    _unitAttributes &= ~Unit::Attributes::DUELABLE;
    
    _duelTarget = enemy;
    
    flatbuffers::FlatBufferBuilder builder;
    auto duel = GameMessage::CreateSVActionDuel(builder,
                                                this->GetUID(),
                                                enemy->GetUID(),
                                                GameMessage::ActionDuelType_STARTED);
    auto msg = GameMessage::CreateMessage(builder,
                                          0,
                                          GameMessage::Messages_SVActionDuel,
                                          duel.Union());
    builder.Finish(msg);
    
    _world._outputMessages.push_back(std::make_shared<MessageBuffer>(builder.GetCurrentBufferPointer(),
                                                                     builder.GetBufferPointer() + builder.GetSize()));
}

void
Unit::AcceptDuel(std::shared_ptr<Unit> enemy)
{
    // Log duel start event
    _logger.Info() << "Accept duel from " << enemy->GetName();

    _state = Unit::State::DUEL;
    _unitAttributes &= ~Unit::Attributes::DUELABLE;

    _duelTarget = enemy;
}


void
Unit::EndDuel()
{
        // Log duel-end event
    _logger.Info() << "Duel with " << _duelTarget.lock()->GetName() << " ended";
    
    _state = Unit::State::WALKING;
    _unitAttributes |= Unit::Attributes::INPUT | Unit::Attributes::ATTACK | Unit::Attributes::DUELABLE;
    
    _duelTarget.reset();
}
