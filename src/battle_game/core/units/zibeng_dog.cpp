#include "zibeng_dog.h"

#include "battle_game/core/bullets/bullets.h"
#include "battle_game/core/game_core.h"
#include "battle_game/graphics/graphics.h"

namespace battle_game::unit {

namespace {
uint32_t dog_body_model_index = 0xffffffffu;
uint32_t dog_muzzle_model_index = 0xffffffffu;
}  // namespace

ZibengDog::ZibengDog(GameCore *game_core, uint32_t id, uint32_t player_id)
    : Unit(
          game_core,
          id,
          player_id,
          [=](glm::vec2 position) {
            position = WorldToLocal(position);
            return position.x > -0.8f && position.x < 0.8f &&
                   position.y > -1.0f && position.y < 1.0f;
          },
          50.0f,
          1.0f,
          0.8f,
          0.0f,
          3.0f) {
  if (!~dog_body_model_index) {
    auto mgr = AssetsManager::GetInstance();
    {
      /* Dog Body */
      dog_body_model_index = mgr->RegisterModel(
          {{{-0.8f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
           {{-0.8f, -1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
           {{0.8f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
           {{0.8f, -1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}},
          {0, 1, 2, 1, 2, 3});
    }

    {
      /* Dog Muzzle */
      std::vector<ObjectVertex> muzzle_vertices;
      std::vector<uint32_t> muzzle_indices;
      const int precision = 60;
      const float inv_precision = 1.0f / float(precision);
      for (int i = 0; i < precision; i++) {
        auto theta = (float(i) + 0.5f) * inv_precision;
        theta *= glm::pi<float>() * 2.0f;
        auto sin_theta = std::sin(theta);
        auto cos_theta = std::cos(theta);
        muzzle_vertices.push_back({{sin_theta * 0.5f, cos_theta * 0.5f},
                                   {0.0f, 0.0f},
                                   {0.7f, 0.7f, 0.7f, 1.0f}});
        muzzle_indices.push_back(i);
        muzzle_indices.push_back((i + 1) % precision);
        muzzle_indices.push_back(precision);
      }
      muzzle_vertices.push_back(
          {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.7f, 0.7f, 0.7f, 1.0f}});
      muzzle_vertices.push_back(
          {{-0.1f, 0.0f}, {0.0f, 0.0f}, {0.7f, 0.7f, 0.7f, 1.0f}});
      muzzle_vertices.push_back(
          {{0.1f, 0.0f}, {0.0f, 0.0f}, {0.7f, 0.7f, 0.7f, 1.0f}});
      muzzle_vertices.push_back(
          {{-0.1f, 1.2f}, {0.0f, 0.0f}, {0.7f, 0.7f, 0.7f, 1.0f}});
      muzzle_vertices.push_back(
          {{0.1f, 1.2f}, {0.0f, 0.0f}, {0.7f, 0.7f, 0.7f, 1.0f}});
      muzzle_indices.push_back(precision + 1 + 0);
      muzzle_indices.push_back(precision + 1 + 1);
      muzzle_indices.push_back(precision + 1 + 2);
      muzzle_indices.push_back(precision + 1 + 1);
      muzzle_indices.push_back(precision + 1 + 2);
      muzzle_indices.push_back(precision + 1 + 3);
      dog_muzzle_model_index =
          mgr->RegisterModel(muzzle_vertices, muzzle_indices);
    }
  }
}

void ZibengDog::Render() {
  battle_game::SetTransformation(position_, rotation_);
  battle_game::SetTexture("../../textures/doge.png");
  battle_game::SetColor(game_core_->GetPlayerColor(player_id_));
  battle_game::DrawModel(0);
  battle_game::SetRotation(muzzle_rotation_);
  battle_game::SetTexture("../../textures/sweaty_soybean.png");
  battle_game::DrawModel(dog_muzzle_model_index);
}

void ZibengDog::Update() {
  DogMove(3.0f, glm::radians(180.0f));
  MuzzleRotate();
  Fire();
}

void ZibengDog::DogMove(float move_speed, float rotate_angular_speed) {
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
    float speed = move_speed * GetSpeedScale();
    offset *= kSecondPerTick * speed;
    auto new_position =
        position_ + glm::vec2{glm::rotate(glm::mat4{1.0f}, rotation_,
                                          glm::vec3{0.0f, 0.0f, 1.0f}) *
                              glm::vec4{offset, 0.0f, 0.0f}};
    if (!game_core_->IsBlockedByObstacles(new_position)) {
      game_core_->PushEventMoveUnit(id_, new_position);
    }
    float rotation_offset = 0.0f;
    if (input_data.key_down[GLFW_KEY_A]) {
      rotation_offset += 1.0f;
    }
    if (input_data.key_down[GLFW_KEY_D]) {
      rotation_offset -= 1.0f;
    }
    rotation_offset *= kSecondPerTick * rotate_angular_speed * GetSpeedScale();
    game_core_->PushEventRotateUnit(id_, rotation_ + rotation_offset);
  }
}

void ZibengDog::MuzzleRotate() {
  auto player = game_core_->GetPlayer(player_id_);
  if (player) {
    auto &input_data = player->GetInputData();
    auto diff = input_data.mouse_cursor_position - position_;
    if (glm::length(diff) < 1e-4) {
      muzzle_rotation_ = rotation_;
    }
    muzzle_rotation_ = std::atan2(diff.y, diff.x) - glm::radians(90.0f);
  }
}

void ZibengDog::Fire() {
  if (fire_count_down_) {
    fire_count_down_--;
    if (zibeng_line_time_) {
      zibeng_line_time_--;
      auto velocity = Rotate(glm::vec2{0.0f, 30.0f}, muzzle_rotation_);
      GenerateBullet<bullet::SweatySoybean>(
          position_ + Rotate({0.0f, 1.2f}, muzzle_rotation_), muzzle_rotation_,
          0.02f, velocity);
    } else {
      if (should_beng_) {
        auto velocity = Rotate(glm::vec2{0.0f, 30.0f}, muzzle_rotation_);
        GenerateBullet<bullet::SweatySoybean>(
            position_ + Rotate({0.0f, 1.2f}, muzzle_rotation_),
            muzzle_rotation_, GetDamageScale(), velocity);
        should_beng_ = false;
      }
    }
  } else {
    auto player = game_core_->GetPlayer(player_id_);
    if (player) {
      auto &input_data = player->GetInputData();
      if (input_data.mouse_button_down[GLFW_MOUSE_BUTTON_LEFT]) {
        auto velocity = Rotate(glm::vec2{0.0f, 30.0f}, muzzle_rotation_);
        GenerateBullet<bullet::SweatySoybean>(
            position_ + Rotate({0.0f, 1.2f}, muzzle_rotation_),
            muzzle_rotation_, 0.02f, velocity);
        fire_count_down_ = kTickPerSecond + 1;
        zibeng_line_time_ = kTickPerSecond;
        should_beng_ = true;  // Fire interval 1 second.
      }
    }
  }
}

const char *ZibengDog::UnitName() const {
  return "Zibeng Dog";
}

const char *ZibengDog::Author() const {
  return "Aoarashi";
}
}  // namespace battle_game::unit
