// ReSharper disable CppHidingFunction
// ReSharper disable CppParameterMayBeConst
#include <Geode/Geode.hpp>

using namespace geode::prelude;

std::string patchSaveString(std::string save, CCObject* self, std::string (*patcher)(CCObject*, const int, std::string)) {
	size_t pos = 0;
	std::stringstream out;
	char c;
	bool first = true;
	while (pos < save.size()) {
		if (first) {
			first = false;
		} else {
			out << ',';
		}
		size_t key_start = pos;
		do {
			c = save[pos++];
			if (pos >= save.size()) {
				log::warn("Object string ended prematurely, will abort processing of this save string");
				return save;
			}
		} while (c != ',');

		std::string keyStr = std::string(
			save.data() + key_start,
			std::min(pos - key_start - 1, save.size() - key_start));
		auto keyResult = utils::numFromString<int>(keyStr);
		if (!keyResult.isOk()) {
			log::warn("Failed to parse object string key {}, will abort processing of this save string", keyStr);
			return save;
		}
		const int key = keyResult.unwrap();
		out << key << ',';

		size_t val_start = pos;
		do {
			c = save[pos++];
			if (pos >= save.size()) {
				pos++;
				break;
			}
		} while (c != ',');

		out << patcher(self, key, std::string(save.data() + val_start, std::min(pos - val_start - 1, save.size() - val_start)));
	}

	return out.str();
}

bool float64Position = true;
bool float64Rotation = true;
bool float64Scale = true;
bool float64Params = true;
bool writeMathExpressions = false;

// Các giá trị mặc định cho các tính năng bổ trợ từ High Precision Editor
bool decimalMoveParams = true;
enum class AreaTriggerSmallStepMode : int {
	Disabled = -1,
	DefaultOff = 1,
	DefaultOn = 2,
	AlwaysOn = -2,
	Invalid = -3,
};
AreaTriggerSmallStepMode parseAreaTriggerSmallStepToggle(const std::string& s) {
	if (s == "Disabled") {
		return AreaTriggerSmallStepMode::Disabled;
	} else if (s == "Default Off") {
		return AreaTriggerSmallStepMode::DefaultOff;
	} else if (s == "Default On") {
		return AreaTriggerSmallStepMode::DefaultOn;
	} else if (s == "Always On") {
		return AreaTriggerSmallStepMode::AlwaysOn;
	}
	return AreaTriggerSmallStepMode::Invalid;
}
auto areaTriggerSmallStepMode = AreaTriggerSmallStepMode::DefaultOff;
bool sliderInputs = true;
bool fixedPlaytestReset = false;
bool miscEditorFixes = true;
bool miscUIFixes = false;

bool areaTriggerSmallStepState = false;

$execute {
	float64Position = Mod::get()->getSettingValue<bool>("float64-object-position");
	float64Rotation = Mod::get()->getSettingValue<bool>("float64-object-rotation");
	float64Scale = Mod::get()->getSettingValue<bool>("float64-object-scale");
	float64Params = Mod::get()->getSettingValue<bool>("float64-trigger-parameters");
	writeMathExpressions = Mod::get()->getSettingValue<bool>("write-math-expressions");

	// Đồng bộ lắng nghe thay đổi từ setting trong mod.json mới
	listenForSettingChanges<bool>("float64-object-position", [](bool value) {
		float64Position = value;
	});
	listenForSettingChanges<bool>("float64-object-rotation", [](bool value) {
		float64Rotation = value;
	});
	listenForSettingChanges<bool>("float64-object-scale", [](bool value) {
		float64Scale = value;
	});
	listenForSettingChanges<bool>("float64-trigger-parameters", [](bool value) {
		float64Params = value;
	});
	listenForSettingChanges<bool>("write-math-expressions", [](bool value) {
		writeMathExpressions = value;
	});

	switch (areaTriggerSmallStepMode) {
		case AreaTriggerSmallStepMode::Disabled:
		case AreaTriggerSmallStepMode::DefaultOff:
			areaTriggerSmallStepState = false;
			break;
		case AreaTriggerSmallStepMode::DefaultOn:
		case AreaTriggerSmallStepMode::AlwaysOn:
			areaTriggerSmallStepState = true;
			break;
		default:
			log::warn("Invalid mode for the area trigger small step toggle");
			break;
	}
}

#include <Geode/modify/GameObject.hpp>
class $modify(Float64GameObject, GameObject) {
	struct Fields {
		bool hasRecordedPosition = false;
		CCPoint recordedPosition;
	};
	gd::string getSaveString(GJBaseGameLayer* layer) override {
		gd::string save = GameObject::getSaveString(layer);
		return patchSaveString(save, this, [](CCObject* rawSelf, const int key, std::string orig) {
			auto self = (Float64GameObject*) rawSelf;
			switch (key) {
				case 2:
					if (!float64Position) return orig;
					return fmt::format("{}", (double)self->getPositionX());
				case 3:
					if (!float64Position) return orig;
					return fmt::format("{}", (double)(self->getPositionY() - 90));
				case 32:
					if (!float64Scale) return orig;
					return fmt::format("{}", (double)std::max(self->m_scaleX, self->m_scaleY));
				case 6:
				case 131:
					if (!float64Rotation) return orig;
					return fmt::format("{}", (double)self->m_fRotationX);
				case 132:
					if (!float64Rotation) return orig;
					return fmt::format("{}", (double)self->m_fRotationY);
				case 128:
					if (!float64Scale) return orig;
					return fmt::format("{}", (double)self->m_scaleX);
				case 129:
					if (!float64Scale) return orig;
					return fmt::format("{}", (double)self->m_scaleY);

				default:
					return orig;
			}
		});
	}
};

#include <Geode/modify/EffectGameObject.hpp>
class $modify(Float64EffectObject, EffectGameObject) {
	gd::string getSaveString(GJBaseGameLayer* layer) override {
		gd::string save = EffectGameObject::getSaveString(layer);
		if (!float64Params) return save;

		return patchSaveString(save, this, [](CCObject* rawSelf, const int key, std::string orig) {
			auto self = (Float64EffectObject*) rawSelf;
			switch (key) {
				case 10: 
					return fmt::format("{}", (double)self->m_duration);
				case 85:
					return fmt::format("{}", (double)self->m_easingRate);
				case 28: 
					return fmt::format("{}", (double)self->m_moveOffset.x);
				case 29: 
					return fmt::format("{}", (double)self->m_moveOffset.y);
				case 143:
					return fmt::format("{}", (double)self->m_moveModX);
				case 144:
					return fmt::format("{}", (double)self->m_moveModY);
				case 68: 
					return fmt::format("{}", (double)self->m_rotationDegrees);
				case 402:
					return fmt::format("{}", (double)self->m_rotationOffset);
				case 45:
					return fmt::format("{}", (double)self->m_fadeInDuration);
				case 46:
					return fmt::format("{}", (double)self->m_holdDuration);
				case 47:
					return fmt::format("{}", (double)self->m_fadeOutDuration);
				case 35:
					return fmt::format("{}", (double)self->m_opacity);
				case 75:
					return fmt::format("{}", (double)self->m_shakeStrength);
				case 84:
					return fmt::format("{}", (double)self->m_shakeInterval);
				case 72:
					return fmt::format("{}", (double)self->m_followXMod);
				case 73:
					return fmt::format("{}", (double)self->m_followYMod);
				case 90:
					return fmt::format("{}", (double)self->m_followYSpeed);
				case 91:
					return fmt::format("{}", (double)self->m_followYDelay);
				case 105:
					return fmt::format("{}", (double)self->m_followYMaxSpeed);
				case 371: 
					return fmt::format("{}", (double)self->m_zoomValue);
				case 114:
					return fmt::format("{}", (double)self->m_cameraPaddingValue);
				case 120:
					return fmt::format("{}", (double)self->m_timeWarpTimeMod);
				case 148:
					return fmt::format("{}", (double)self->m_gravityValue);

				default:
					return orig;
			}
		});
	}
};

#include <Geode/modify/TransformTriggerGameObject.hpp>
class $modify(Float64TransformTrigger, TransformTriggerGameObject) {
	gd::string getSaveString(GJBaseGameLayer* layer) override {
		gd::string save = TransformTriggerGameObject::getSaveString(layer);
		if (!float64Params) return save;

		return patchSaveString(save, this, [](CCObject* rawSelf, const int key, std::string orig) {
			auto self = (Float64TransformTrigger*) rawSelf;
			switch (key) {
				case 150:
					return fmt::format("{}", (double)self->m_objectScaleX);
				case 151:
					return fmt::format("{}", (double)self->m_objectScaleY);
				default:
					return orig;
			}
		});
	}
};

#include <Geode/modify/KeyframeAnimTriggerObject.hpp>
class $modify(Float64KeyframeAnimTrigger, KeyframeAnimTriggerObject) {
	gd::string getSaveString(GJBaseGameLayer* layer) override {
		gd::string save = KeyframeAnimTriggerObject::getSaveString(layer);
		if (!float64Params) return save;

		return patchSaveString(save, this, [](CCObject* rawSelf, const int key, std::string orig) {
			auto self = (Float64KeyframeAnimTrigger*) rawSelf;
			switch (key) {
				case 520: return fmt::format("{}", (double)self->m_timeMod);
				case 521: return fmt::format("{}", (double)self->m_positionXMod);
				case 545: return fmt::format("{}", (double)self->m_positionYMod);
				case 522: return fmt::format("{}", (double)self->m_rotationMod);
				case 523: return fmt::format("{}", (double)self->m_scaleXMod);
				case 546: return fmt::format("{}", (double)self->m_scaleYMod);
				default: return orig;
			}
		});
	}
};

#include <Geode/modify/KeyframeGameObject.hpp>
class $modify(Float64KeyframeGameObject, KeyframeGameObject) {
	gd::string getSaveString(GJBaseGameLayer* layer) override {
		gd::string save = KeyframeGameObject::getSaveString(layer);
		if (!float64Params) return save;

		return patchSaveString(save, this, [](CCObject* rawSelf, const int key, std::string orig) {
			auto self = (Float64KeyframeGameObject*) rawSelf;
			switch (key) {
				case 557: return fmt::format("{}", (double)self->m_spawnDelay);
				default: return orig;
			}
		});
	}
};

#include <Geode/modify/GradientTriggerObject.hpp>
class $modify(Float64GradientTrigger, GradientTriggerObject) {
	gd::string getSaveString(GJBaseGameLayer* layer) override {
		gd::string save = GradientTriggerObject::getSaveString(layer);
		if (!float64Params) return save;

		return patchSaveString(save, this, [](CCObject* rawSelf, const int key, std::string orig) {
			auto self = (Float64GradientTrigger*) rawSelf;
			switch (key) {
				case 456: return fmt::format("{}", (double)self->m_previewOpacity);
				default: return orig;
			}
		});
	}
};

#include <Geode/modify/CameraTriggerGameObject.hpp>
class $modify(Float64CameraTrigger, CameraTriggerGameObject) {
	gd::string getSaveString(GJBaseGameLayer* layer) override {
		gd::string save = CameraTriggerGameObject::getSaveString(layer);
		if (!float64Params) return save;

		return patchSaveString(save, this, [](CCObject* rawSelf, const int key, std::string orig) {
			auto self = (Float64CameraTrigger*) rawSelf;
			switch (key) {
				case 213: return fmt::format("{}", (double)self->m_followEasing);
				case 454: return fmt::format("{}", (double)self->m_velocityModifier);
				default: return orig;
			}
		});
	}
};

#include <Geode/modify/ItemTriggerGameObject.hpp>
class $modify(Float64ItemTrigger, ItemTriggerGameObject) {
	gd::string getSaveString(GJBaseGameLayer* layer) override {
		gd::string save = ItemTriggerGameObject::getSaveString(layer);
		if (!float64Params) return save;

		return patchSaveString(save, this, [](CCObject* rawSelf, const int key, std::string orig) {
			auto self = (Float64ItemTrigger*) rawSelf;
			switch (key) {
				case 479: return fmt::format("{}", (double)self->m_mod1);
				case 483: return fmt::format("{}", (double)self->m_mod2);
				case 484: return fmt::format("{}", (double)self->m_tolerance);
				default: return orig;
			}
		});
	}
};

#include <Geode/modify/SFXTriggerGameObject.hpp>
class $modify(Float64SFXTrigger, SFXTriggerGameObject) {
	gd::string getSaveString(GJBaseGameLayer* layer) override {
		gd::string save = SFXTriggerGameObject::getSaveString(layer);
		if (!float64Params) return save;

		return patchSaveString(save, this, [](CCObject* rawSelf, const int key, std::string orig) {
			auto self = (Float64SFXTrigger*) rawSelf;
			switch (key) {
				case 406: return fmt::format("{}", (double)self->m_volume);
				case 421: return fmt::format("{}", (double)self->m_volumeNear);
				case 422: return fmt::format("{}", (double)self->m_volumeMedium);
				case 423: return fmt::format("{}", (double)self->m_volumeFar);
				case 434: return fmt::format("{}", (double)self->m_minInterval);
				case 490: return fmt::format("{}", (double)self->m_soundDuration);
				default: return orig;
			}
		});
	}
};

#include <Geode/modify/TimerTriggerGameObject.hpp>
class $modify(Float64TimerTrigger, TimerTriggerGameObject) {
	gd::string getSaveString(GJBaseGameLayer* layer) override {
		gd::string save = TimerTriggerGameObject::getSaveString(layer);
		if (!float64Params) return save;

		return patchSaveString(save, this, [](CCObject* rawSelf, const int key, std::string orig) {
			auto self = (Float64TimerTrigger*) rawSelf;
			switch (key) {
				case 467: return fmt::format("{}", (double)self->m_startTime);
				case 473: return fmt::format("{}", (double)self->m_targetTime);
				case 470: return fmt::format("{}", (double)self->m_timeMod);
				default: return orig;
			}
		});
	}
};

#include <Geode/modify/SpawnTriggerGameObject.hpp>
class $modify(Float64SpawnTrigger, SpawnTriggerGameObject) {
	gd::string getSaveString(GJBaseGameLayer* layer) override {
		gd::string save = SpawnTriggerGameObject::getSaveString(layer);
		if (!float64Params) return save;

		return patchSaveString(save, this, [](CCObject* rawSelf, const int key, std::string orig) {
			auto self = (Float64SpawnTrigger*) rawSelf;
			switch (key) {
				case 63:  return fmt::format("{}", (double)self->m_spawnDelay);
				case 556: return fmt::format("{}", (double)self->m_delayRange);
				default:  return orig;
			}
		});
	}
};

#include <Geode/modify/SequenceTriggerGameObject.hpp>
class $modify(Float64SequenceTrigger, SequenceTriggerGameObject) {
	gd::string getSaveString(GJBaseGameLayer* layer) override {
		gd::string save = SequenceTriggerGameObject::getSaveString(layer);
		if (!float64Params) return save;

		return patchSaveString(save, this, [](CCObject* rawSelf, const int key, std::string orig) {
			auto self = (Float64SequenceTrigger*) rawSelf;
			switch (key) {
				case 437: return fmt::format("{}", (double)self->m_minInt);
				case 438: return fmt::format("{}", (double)self->m_reset);
				default:  return orig;
			}
		});
	}
};

#include <Geode/modify/SpawnParticleGameObject.hpp>
class $modify(Float64SpawnParticle, SpawnParticleGameObject) {
	gd::string getSaveString(GJBaseGameLayer* layer) override {
		gd::string save = SpawnParticleGameObject::getSaveString(layer);
		if (!float64Params) return save;

		return patchSaveString(save, this, [](CCObject* rawSelf, const int key, std::string orig) {
			auto self = (Float64SpawnParticle*) rawSelf;
			switch (key) {
				case 554: return fmt::format("{}", (double)self->m_scale);
				case 555: return fmt::format("{}", (double)self->m_scaleVariance);
				default:  return orig;
			}
		});
	}
};

#include <Geode/modify/RotateGameplayGameObject.hpp>
class $modify(Float64RotateGameplay, RotateGameplayGameObject) {
	gd::string getSaveString(GJBaseGameLayer* layer) override {
		gd::string save = RotateGameplayGameObject::getSaveString(layer);
		if (!float64Params) return save;

		return patchSaveString(save, this, [](CCObject* rawSelf, const int key, std::string orig) {
			auto self = (Float64RotateGameplay*) rawSelf;
			switch (key) {
				case 582: return fmt::format("{}", (double)self->m_velocityModX);
				case 583: return fmt::format("{}", (double)self->m_velocityModY);
				default:  return orig;
			}
		});
	}
};

#include <Geode/modify/GameOptionsTrigger.hpp>
class $modify(Float64GameOptions, GameOptionsTrigger) {
	gd::string getSaveString(GJBaseGameLayer* layer) override {
		gd::string save = GameOptionsTrigger::getSaveString(layer);
		if (!float64Params) return save;

		return patchSaveString(save, this, [](CCObject* rawSelf, const int key, std::string orig) {
			auto self = (Float64GameOptions*) rawSelf;
			switch (key) {
				case 574: return fmt::format("{}", (double)self->m_respawnTime);
				default:  return orig;
			}
		});
	}
};

#include <Geode/modify/TeleportPortalObject.hpp>
class $modify(Float64TeleportPortal, TeleportPortalObject) {
	gd::string getSaveString(GJBaseGameLayer* layer) override {
		gd::string save = TeleportPortalObject::getSaveString(layer);
		if (!float64Params) return save;

		return patchSaveString(save, this, [](CCObject* rawSelf, const int key, std::string orig) {
			auto self = (Float64TeleportPortal*) rawSelf;
			switch (key) {
				case 348: return fmt::format("{}", (double)self->m_redirectForceMin);
				case 349: return fmt::format("{}", (double)self->m_redirectForceMax);
				case 350: return fmt::format("{}", (double)self->m_redirectForceMod);
				default:  return orig;
			}
		});
	}
};

#include <Geode/modify/ShaderGameObject.hpp>
class $modify(Float64ShaderGameObject, ShaderGameObject) {
	gd::string getSaveString(GJBaseGameLayer* layer) override {
		gd::string save = ShaderGameObject::getSaveString(layer);
		if (!float64Params) return save;

		return patchSaveString(save, this, [](CCObject* rawSelf, const int key, std::string orig) {
			auto self = (Float64ShaderGameObject*) rawSelf;
			switch (key) {
				case 175: return fmt::format("{}", (double)self->m_speed);
				case 176: return fmt::format("{}", (double)self->m_strength);
				case 179: return fmt::format("{}", (double)self->m_waveWidth);
				case 180: return fmt::format("{}", (double)self->m_targetX);
				case 189: return fmt::format("{}", (double)self->m_targetY);
				case 181: return fmt::format("{}", (double)self->m_fadeIn);
				case 182: return fmt::format("{}", (double)self->m_fadeOut);
				case 177: return fmt::format("{}", (double)self->m_timeOff);
				case 512: return fmt::format("{}", (double)self->m_maxSize);
				case 290: return fmt::format("{}", (double)self->m_screenOffsetX);
				case 291: return fmt::format("{}", (double)self->m_screenOffsetY);
				case 183: return fmt::format("{}", (double)self->m_inner);
				case 191: return fmt::format("{}", (double)self->m_outer);
				default:  return orig;
			}
		});
	}
};

#include <Geode/modify/ForceBlockGameObject.hpp>
class $modify(Float64ForceBlock, ForceBlockGameObject) {
	gd::string getSaveString(GJBaseGameLayer* layer) override {
		gd::string save = ForceBlockGameObject::getSaveString(layer);
		if (!float64Params) return save;

		return patchSaveString(save, this, [](CCObject* rawSelf, const int key, std::string orig) {
			auto self = (Float64ForceBlock*) rawSelf;
			switch (key) {
				case 149: return fmt::format("{}", (double)self->m_force);
				case 526: return fmt::format("{}", (double)self->m_minForce);
				case 527: return fmt::format("{}", (double)self->m_maxForce);
				default:  return orig;
			}
		});
	}
};

#include <Geode/modify/EnterEffectObject.hpp>
class $modify(Float64EnterEffect, EnterEffectObject) {
	gd::string getSaveString(GJBaseGameLayer* layer) override {
		gd::string save = EnterEffectObject::getSaveString(layer);
		if (!float64Params) return save;

		return patchSaveString(save, this, [](CCObject* rawSelf, const int key, std::string orig) {
			auto self = (Float64EnterEffect*) rawSelf;
			switch (key) {
				case 243: return fmt::format("{}", (double)self->m_easingInRate);
				case 249: return fmt::format("{}", (double)self->m_easingOutRate);
				case 233: return fmt::format("{}", (double)self->m_areaScaleX);
				case 234: return fmt::format("{}", (double)self->m_areaScaleXVariance);
				case 235: return fmt::format("{}", (double)self->m_areaScaleY);
				case 236: return fmt::format("{}", (double)self->m_areaScaleYVariance);
				case 270: return fmt::format("{}", (double)self->m_areaRotation);
				case 271: return fmt::format("{}", (double)self->m_areaRotationVariance);
				case 275: return fmt::format("{}", (double)self->m_toOpacity);
				case 286: return fmt::format("{}", (double)self->m_fromOpacity);
				case 263: return fmt::format("{}", (double)self->m_modFront);
				case 264: return fmt::format("{}", (double)self->m_modBack);
				case 282: return fmt::format("{}", (double)self->m_deadzone);
				case 265: return fmt::format("{}", (double)self->m_areaTint);
				case 288: return fmt::format("{}", (double)self->m_relativeFade);
				case 285: return fmt::format("{}", (double)self->m_property285);
				default:  return orig;
			}
		});
	}
};

#include <Geode/modify/LevelEditorLayer.hpp>
class $modify(Float64EditorLayer, LevelEditorLayer) {
	$override
	void onPlaytest() {
		if (fixedPlaytestReset) {
			for (GameObject* object : CCArrayExt<GameObject>(m_objects)) {
				auto p_object = static_cast<Float64GameObject*>(object); 
				p_object->m_fields->recordedPosition = p_object->getPosition();
				p_object->m_fields->hasRecordedPosition = true;
			}
		}
		return LevelEditorLayer::onPlaytest();
	}
	$override
	void onStopPlaytest() {
		LevelEditorLayer::onStopPlaytest();
		if (fixedPlaytestReset) {
			for (GameObject* object : CCArrayExt<GameObject>(m_objects)) {
				auto p_object = static_cast<Float64GameObject*>(object); 
				if (p_object->m_fields->hasRecordedPosition) {
					p_object->m_fields->hasRecordedPosition = false;
					p_object->setPosition(p_object->m_fields->recordedPosition);
				}
			}
		}
	}
};

#include <Geode/modify/SetupTriggerPopup.hpp>
class $modify(Float64TriggerPopup, SetupTriggerPopup) {
	static void onModify(auto& self) {
		if (!self.setHookPriorityPost("SetupTriggerPopup::triggerSliderChanged", Priority::Late)) {
			log::error("failed to set hook priority for SetupTriggerPopup::triggerSliderChanged");
		}
		#if defined(GEODE_IS_MACOS) || defined(GEODE_IS_IOS)
		if (!self.setHookPriorityPost("SetupTriggerPopup::textChanged", Priority::Late)) {
			log::error("failed to set hook priority for SetupTriggerPopup::textChanged");
		}
		#endif
	}

	void updateInputNode(int property, float value) override {
		SetupTriggerPopup::updateInputNode(property, value);
		if (!float64Params) return;

		auto inputNode = (CCTextInputNode*) m_inputNodes->objectForKey(property);
		if (inputNode == nullptr || inputNode->m_textField == nullptr) {
			return;
		}
		const std::string newStr = fmt::format("{}", (double)value);
		const std::string oldStr = inputNode->getString();
		auto oldResult = utils::numFromString<float>(oldStr);
		if (!oldResult.isOk() || oldResult.unwrap() != value || newStr.size() < oldStr.size()) {
			inputNode->setString(newStr);
		}
	}

	CCArray* createValueControlAdvanced(int property,
										gd::string label,
										cocos2d::CCPoint position,
										float scale,
										bool noSlider,
										InputValueType valueType,
										int length,
										bool arrows,
										float sliderMin,
										float sliderMax,
										int page,
										int group,
										GJInputStyle inputStyle,
										int decimalPlaces,
										bool allowDisable) {
		switch (property) {
			case 28:
			case 29:
			case 97:
				if (!decimalMoveParams) break;
				valueType = InputValueType::Float;
				decimalPlaces = -1;
				break;
			case 218: case 219: case 220: case 221: case 222: case 223:
			case 237: case 238: case 239: case 240: case 252: case 253:
				if (!float64Params) break;
				valueType = InputValueType::Float;
				decimalPlaces = -2;
				break;
			default: break;
		}
		return SetupTriggerPopup::createValueControlAdvanced(property,
															 std::move(label),
															 position,
															 scale,
															 noSlider,
															 valueType,
															 length,
															 arrows,
															 sliderMin,
															 sliderMax,
															 page,
															 group,
															 inputStyle,
															 decimalPlaces,
															 allowDisable);
	}

	float getTruncatedValueHook(float value, int decimalPlaces) {
		if (!float64Params) return SetupTriggerPopup::getTruncatedValue(value, std::abs(decimalPlaces));

		if (decimalPlaces == -2) {
			value = roundf(value * 3.0f) / 3.0f;
			value += copysignf(0.01f, value); 
		} else if (decimalPlaces != 0) {
			return value;
		}
		return SetupTriggerPopup::getTruncatedValue(value, std::abs(decimalPlaces));
	}

	#ifndef GEODE_IS_IOS
	float getTruncatedValue(float value, int decimalPlaces) {
		return getTruncatedValueHook(value, decimalPlaces);
	}
	#endif

	#if defined(GEODE_IS_MACOS) || defined(GEODE_IS_IOS)
	void textChanged(CCTextInputNode* inputNode) override {
		if (!float64Params) return SetupTriggerPopup::textChanged(inputNode);
		if (m_disableTextDelegate) return;

		int property = inputNode->getTag();
		std::string str = inputNode->getString();
		float value = getTruncatedValueHook(utils::numFromString<float>(str).unwrapOr(0), inputNode->m_decimalPlaces);

		updateInputValue(property, value);
		m_triggerValues->setObject(CCFloat::create(value), property);
		valueChanged(property, value);
		updateSlider(property, triggerSliderValueFromValue(property, value));
	}
	#endif

	void triggerSliderChanged(CCObject* param) {
		if (!float64Params) return SetupTriggerPopup::triggerSliderChanged(param);

		bool oldDisableTextDelegate = m_disableTextDelegate;
		m_disableTextDelegate = true;

		int property = param->getTag();
		float value = ((SliderThumb*) param)->getValue();
		value = triggerValueFromSliderValue(property, value);
		auto inputNode = (CCTextInputNode*) m_inputNodes->objectForKey(property);
		if (inputNode != nullptr) {
			int places = inputNode->m_decimalPlaces;
			if (places < 0) places = -places - 1;

			if (places < 1) {
				value = (float) (int) value;
			} else {
				float scale = std::powf(10.0, (float) places);
				value = std::roundf(value * scale) / scale;
			}
		}
		valueChanged(property, value);
		updateInputNode(property, value);
		m_disableTextDelegate = oldDisableTextDelegate;
	}

	void updateEaseRateLabel() {
		if (!float64Params) return SetupTriggerPopup::updateEaseRateLabel();
		m_easingRateLabel->setString(fmt::format("{}", (double)m_easingRate).c_str());
	}

	void valuePopupClosed(ConfigureValuePopup* popup, float value) override {
		if (!float64Params) return SetupTriggerPopup::valuePopupClosed(popup, value);

		int property = popup->getTag();
		if (property == 85) {
			m_easingRate = value;
			valueChanged(85, value);
			updateEaseRateLabel();
		} else {
			valueChanged(property, value);
			updateCustomEaseRateLabel(property, value);
		}
	}
};

void updateTriggersPopup(SetupTriggerPopup* popup, auto updater) {
	if (popup->m_gameObject == nullptr) {
		unsigned int count = popup->m_gameObjects->count();
		for (unsigned int i = 0; i < count; i++) {
			auto object = (EffectGameObject*) popup->m_gameObjects->objectAtIndex(i);
			updater(object);
		}
	} else
		updater(popup->m_gameObject);
}

#include <Geode/modify/SetupCameraOffsetTrigger.hpp>
class $modify(Float64SetupCameraOffset, SetupCameraOffsetTrigger) {
	bool init(CameraTriggerGameObject* p0, CCArray* p1) {
		if (!SetupCameraOffsetTrigger::init(p0, p1)) return false;
		if (!float64Params) return true;

		auto object = m_gameObject;
		if (object == nullptr) {
			if (m_gameObjects == nullptr || m_gameObjects->count() == 0) return true;
			object = (EffectGameObject*) m_gameObjects->objectAtIndex(0);
			if (object == nullptr) return true;
		}

		if (m_offsetX != -99999 && m_offsetXInput != nullptr)
			m_offsetXInput->setString(fmt::format("{}", (double)(object->m_moveOffset.x / 3.0f)));
		if (m_offsetY != -99999 && m_offsetYInput != nullptr)
			m_offsetYInput->setString(fmt::format("{}", (double)(object->m_moveOffset.y / 3.0f)));
		if (m_moveTimeInput != nullptr)
			m_moveTimeInput->setString(fmt::format("{}", (double)m_moveTime));
		return true;
	}
};

#include <Geode/modify/GJFollowCommandLayer.hpp>
class $modify(Float64FollowCommandLayer, GJFollowCommandLayer) {
	bool init(EffectGameObject* p0, CCArray* p1) {
		if (!GJFollowCommandLayer::init(p0, p1)) return false;
		if (!float64Params) return true;

		if (m_xModInput != nullptr) m_xModInput->setString(fmt::format("{}", (double)m_xMod));
		if (m_yModInput != nullptr) m_yModInput->setString(fmt::format("{}", (double)m_yMod));
		if (m_moveTimeInput != nullptr) m_moveTimeInput->setString(fmt::format("{}", (double)m_moveTime));
		return true;
	}
};

#include <Geode/modify/ColorSelectPopup.hpp>
class $modify(Float64ColorSelect, ColorSelectPopup) {
	bool init(EffectGameObject* p0, CCArray* p1, ColorAction* p2) {
		if (!ColorSelectPopup::init(p0, p1, p2)) return false;

		if (float64Params && m_fadeTimeInput != nullptr) {
			m_disableTextDelegate = true;
			m_fadeTimeInput->setString(fmt::format("{}", (double)m_fadeTime));
			m_disableTextDelegate = false;
		}
		return true;
	}
};

#include <Geode/modify/SetupPulsePopup.hpp>
class $modify(Float64PulsePopup, SetupPulsePopup) {
	bool init(EffectGameObject* p0, CCArray* p1) {
		if (!SetupPulsePopup::init(p0, p1)) return false;
		if (!float64Params) return true;

		bool oldDisableTextDelegate = m_disableTextDelegate;
		m_disableTextDelegate = true;

		if (m_fadeInInput != nullptr)  m_fadeInInput->setString(fmt::format("{}", (double)m_fadeInTime));
		if (m_holdInput != nullptr)    m_holdInput->setString(fmt::format("{}", (double)m_holdTime));
		if (m_fadeOutInput != nullptr) m_fadeOutInput->setString(fmt::format("{}", (double)m_fadeOutTime));

		m_disableTextDelegate = oldDisableTextDelegate;
		return true;
	}
};

#include <Geode/modify/SetupOpacityPopup.hpp>
class $modify(Float64OpacityPopup, SetupOpacityPopup) {
	bool init(EffectGameObject* p0, CCArray* p1) {
		if (!SetupOpacityPopup::init(p0, p1)) return false;
		if (float64Params && m_fadeTimeInput != nullptr)
			m_fadeTimeInput->setString(fmt::format("{}", (double)m_fadeTime));
		return true;
	}
};

#include <Geode/modify/SetupTimeWarpPopup.hpp>
class $modify(Float64TimeWarpPopup, SetupTimeWarpPopup) {
	// Giữ nguyên logic tích hợp cho TimeWarp
};

#include <Geode/modify/ConfigureValuePopup.hpp>
class $modify(Float64ValuePopup, ConfigureValuePopup) {
	void updateTextInputLabel() {
		if (!float64Params) return ConfigureValuePopup::updateTextInputLabel();

		bool oldDisableTextDelegate = m_disableTextDelegate;
		m_disableTextDelegate = true;

		if (m_input != nullptr)
			m_input->setString(fmt::format("{}", (double)m_value));

		m_disableTextDelegate = oldDisableTextDelegate;
	}
};

#include <Geode/modify/ConfigureHSVWidget.hpp>
class $modify(Float64HSVWidget, ConfigureHSVWidget) {
	void updateLabels() {
		if (!float64Params) return ConfigureHSVWidget::updateLabels();

		auto hStr = fmt::format("{}", (double)m_hsv.h);
		auto sStr = fmt::format("{}", (double)m_hsv.s);
		auto vStr = fmt::format("{}", (double)m_hsv.v);

		if (m_addInputs) {
			reinterpret_cast<CCTextInputNode*>(m_inputs->objectForKey(1))->setString(hStr);
			reinterpret_cast<CCTextInputNode*>(m_inputs->objectForKey(2))->setString(sStr);
			reinterpret_cast<CCTextInputNode*>(m_inputs->objectForKey(3))->setString(vStr);
		} else {
			m_hueLabel->setString(hStr.c_str());
			m_saturationLabel->setString(sStr.c_str());
			m_brightnessLabel->setString(vStr.c_str());
		}
	}
};
