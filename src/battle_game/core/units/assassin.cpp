#include "assassin.h"

#include "battle_game/core/bullets/bullets.h"
#include "battle_game/core/game_core.h"
#include "battle_game/graphics/graphics.h"

namespace battle_game::unit {

const uint32_t baseTime_invisible = 40;
const uint32_t baseTime_teleporting = 120;

Assassin::Assassin(GameCore *game_core, uint32_t id, uint32_t player_id)
    : Unit(
          game_core,
          id,
          player_id,
          [=](glm::vec2 position) {
            return glm::length(WorldToLocal(position)) < 1.0f;
          },
          50.0f,
          1.0f,
          1.0f,
          .0f,
          15.0f) {
  Skill temp;
  temp.name = u8"隐身";
  temp.description = std::string(u8"十秒内无法被命中（冷却时间：") +
                     std::to_string(baseTime_invisible) + std::string(u8"秒）");
  temp.time_remain = 0;
  temp.time_total = InvisibleCoolDown() * kTickPerSecond;
  temp.type = E;
  temp.function = SKILL_ADD_FUNCTION(Assassin::InvisibleClick);
  skills_.push_back(temp);
  temp.name = u8"瞬移";
  temp.description = u8"下一次鼠标左键时传送至光标位置（冷却时间：" +
                     std::to_string(baseTime_teleporting) +
                     std::string(u8"秒）");
  temp.time_remain = 0;
  temp.time_total = TeleportingCoolDown() * kTickPerSecond;
  temp.type = Q;
  temp.function = SKILL_ADD_FUNCTION(Assassin::TeleportingClick);
  skills_.push_back(temp);
  temp.name = u8"匕首";
  temp.description = u8"对紫色范围内的所有单位造成小额伤害（冷却时间：0.1秒）";
  temp.time_remain = 0;
  temp.time_total = 0.1 * kTickPerSecond;
  temp.bullet_type = 1;
  temp.bullet_total_number = 1;
  temp.type = B;
  skills_.push_back(temp);
  PUSH_EFFECT(SpeedRegulation(GetId(), false, 0));
}

void Assassin::Render() {
  battle_game::SetTransformation(position_, rotation_);
  battle_game::SetTexture("../../textures/assassin_visible.png");
  battle_game::SetColor(game_core_->GetPlayerColor(player_id_));
  battle_game::DrawModel(0);
}

void Assassin::RenderHelper() {
  auto player = game_core_->GetPlayer(player_id_);
  auto &input_data = player->GetInputData();
  if (teleporting_) {
    if (!game_core_->IsOutOfRange(input_data.mouse_cursor_position) &&
        !game_core_->IsBlockedByObstacles(input_data.mouse_cursor_position)) {
      battle_game::SetTransformation(input_data.mouse_cursor_position,
                                     rotation_);
      battle_game::SetTexture("../../textures/assassin_invisible.png");
      battle_game::SetColor({1.0f, 1.0f, 1.0f, 0.5f});
      battle_game::DrawModel(0);
    }
  } else {
    battle_game::SetTransformation(
        position_ + float(sqrtf(2) * 3.0f / 2.0f) *
                        glm::vec2{-sin(rotation_), cos(rotation_)},
        rotation_ + glm::radians(135.0f), glm::vec2{0.5f});
    battle_game::SetTexture("../../textures/assassin_dagger.png");
    battle_game::SetColor(game_core_->GetPlayerColor(player_id_));
    battle_game::DrawModel(0);
    battle_game::SetTransformation(
        position_ + 2.0f * glm::vec2{-sin(rotation_), cos(rotation_)},
        rotation_ + glm::radians(-45.0f), glm::vec2{sqrtf(2)});
    battle_game::SetTexture("../../textures/assassin_range.png");
    battle_game::SetColor({1.0f, 1.0f, 1.0f, 0.1f});
    battle_game::DrawModel(0);
  }
}

void Assassin::Update() {
  Click();
  InvisibleDeal();
  Teleporting();
  AssassinMove(GetSpeed());
}

uint32_t Assassin::InvisibleCoolDown() const {
  return baseTime_invisible;
}

uint32_t Assassin::TeleportingCoolDown() const {
  return baseTime_teleporting;
}

void Assassin::AssassinMove(float speed) {
  auto player = game_core_->GetPlayer(player_id_);
  if (player) {
    auto &input_data = player->GetInputData();
    glm::vec2 offset{0.0f};
    if (input_data.key_down[GLFW_KEY_W]) {
      offset.y += 1.0f;
    }
    if (input_data.key_down[GLFW_KEY_S]) {
      offset.y -= 1.0f;
    }
    offset *= kSecondPerTick * speed;
    auto new_position =
        position_ + glm::vec2{glm::rotate(glm::mat4{1.0f}, rotation_,
                                          glm::vec3{0.0f, 0.0f, 1.0f}) *
                              glm::vec4{offset, 0.0f, 0.0f}};
    if (!game_core_->IsBlockedByObstacles(new_position)) {
      game_core_->PushEventMoveUnit(id_, new_position);
    }

    auto diff = input_data.mouse_cursor_position - position_;
    float new_rotation;
    if (glm::length(diff) < 1e-4) {
      new_rotation = rotation_;
    } else {
      new_rotation = std::atan2(diff.y, diff.x) - glm::radians(90.0f);
    }
    game_core_->PushEventRotateUnit(id_, new_rotation);
  }
}

void Assassin::InvisibleClick() {
  PUSH_EFFECT(Invisible(GetId(), 10 * kTickPerSecond));
  invisible_count_down_ = InvisibleCoolDown() * kTickPerSecond;
}

void Assassin::InvisibleDeal() {
  if (invisible_count_down_) {
    invisible_count_down_--;
  } else {
    auto player = game_core_->GetPlayer(player_id_);
    if (player) {
      auto &input_data = player->GetInputData();
      if (input_data.key_down[GLFW_KEY_E])
        InvisibleClick();
    }
  }
  skills_[0].time_remain = invisible_count_down_;
}

void Assassin::TeleportingClick() {
  teleporting_ = true;
  teleporting_count_down_ = TeleportingCoolDown() * kTickPerSecond;
}

void Assassin::Teleporting() {
  if (!teleporting_) {
    if (teleporting_count_down_)
      teleporting_count_down_--;
    else {
      auto player = game_core_->GetPlayer(player_id_);
      if (player) {
        auto &input_data = player->GetInputData();
        if (input_data.key_down[GLFW_KEY_Q])
          TeleportingClick();
      }
    }
  }
  skills_[1].time_remain = teleporting_count_down_;
}

void Assassin::Click() {
  if (attack_count_down_)
    attack_count_down_--;
  auto player = game_core_->GetPlayer(player_id_);
  auto &input_data = player->GetInputData();
  if (!input_data.mouse_button_down[GLFW_MOUSE_BUTTON_LEFT])
    return;
  if (!game_core_->IsOutOfRange(input_data.mouse_cursor_position) &&
      !game_core_->IsBlockedByObstacles(input_data.mouse_cursor_position) &&
      teleporting_) {
    position_ = input_data.mouse_cursor_position;
    teleporting_ = false;
  } else if (!attack_count_down_) {
    attack_count_down_ = 0.1 * kTickPerSecond;
    auto &units = game_core_->GetUnits();
    for (auto &unit : units) {
      auto diff = unit.second->GetPosition() - position_;
      float length = glm::length(diff);
      if (sqrtf(2) < length && length < 2.0f * sqrtf(2) &&
          (-sin(rotation_) * diff.x + cos(rotation_) * diff.y) / length >
              sqrtf(2) / 2.0f) {
        game_core_->PushEventDealDamage(unit.first, player_id_, 1);
      }
    }
  }
}

const char *Assassin::UnitName() const {
  return "Assassin";
}

const char *Assassin::Author() const {
  return "XieRujian";
}
}  // namespace battle_game::unit
