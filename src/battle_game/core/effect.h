#pragma once
#include "battle_game/core/unit.h"
#include "glm/glm.hpp"

namespace battle_game {

class Effect {
 public:
  virtual std::string Name() const = 0;
  virtual std::string Description() const {
    return "";
  }
  virtual void Influence(Unit::Status &) {
  }
  virtual uint32_t TickRemain() const = 0;
  virtual void TickPass() = 0;
  virtual bool ShouldRemove() const = 0;
  Effect(uint32_t src_player_id);

 protected:
  uint32_t src_player_id_;
};

}  // namespace battle_game