#include "CameraController.h"
#include <ncine/IInputManager.h>
#include <ncine/Camera.h>
#include <ncine/Application.h>

CameraController::CameraController()
    : ignoreEvents_(false), snapMovement_(true),
      moveSpeed_(500.0f), rotateSpeed_(50.0f), scaleSpeed_(0.5f),
      maxCameraScale_(4.0f), minCameraScale_(0.25f)
{
	reset();

	scrollOrigin_ = nc::Vector2f::Zero;
	scrollMove_ = nc::Vector2f::Zero;
	scrollOrigin2_ = nc::Vector2f::Zero;
	scrollMove2_ = nc::Vector2f::Zero;
	joyVectorLeft_ = nc::Vector2f::Zero;
	joyVectorRight_ = nc::Vector2f::Zero;
}

void CameraController::update(float interval)
{
	if (ignoreEvents_)
	{
		camera_.setView(viewValues_);
		return;
	}

	const nc::KeyboardState &keyState = nc::theApplication().inputManager().keyboardState();
	if (keyState.isKeyDown(nc::KeySym::D))
		viewValues_.position.x -= moveSpeed_ * interval;
	else if (keyState.isKeyDown(nc::KeySym::A))
		viewValues_.position.x += moveSpeed_ * interval;
	if (keyState.isKeyDown(nc::KeySym::W))
		viewValues_.position.y -= moveSpeed_ * interval;
	else if (keyState.isKeyDown(nc::KeySym::S))
		viewValues_.position.y += moveSpeed_ * interval;

	if (keyState.isKeyDown(nc::KeySym::RIGHT))
		viewValues_.rotation += rotateSpeed_ * interval;
	else if (keyState.isKeyDown(nc::KeySym::LEFT))
		viewValues_.rotation -= rotateSpeed_ * interval;

	if (keyState.isKeyDown(nc::KeySym::UP))
		viewValues_.scale += scaleSpeed_ * interval;
	else if (keyState.isKeyDown(nc::KeySym::DOWN))
		viewValues_.scale -= scaleSpeed_ * interval;

	if (joyVectorLeft_.length() > nc::IInputManager::LeftStickDeadZone)
	{
		viewValues_.position.x -= joyVectorLeft_.x * moveSpeed_ * interval;
		viewValues_.position.y -= joyVectorLeft_.y * moveSpeed_ * interval;
	}
	if (joyVectorRight_.length() > nc::IInputManager::RightStickDeadZone)
	{
		viewValues_.rotation += joyVectorRight_.x * rotateSpeed_ * interval;
		viewValues_.scale += joyVectorRight_.y * scaleSpeed_ * interval;
	}

	const nc::Vector2f scrollDiff = scrollMove_ - scrollOrigin_;
	if (scrollDiff.sqrLength() > 2.0f)
	{
		viewValues_.position.x += scrollDiff.x;
		viewValues_.position.y += scrollDiff.y;
		scrollOrigin_ = scrollMove_;
	}
	const nc::Vector2f scrollDiff2 = scrollMove2_ - scrollOrigin2_;
	if (scrollDiff2.sqrLength() > 2.0f)
	{
		viewValues_.rotation += scrollDiff2.x * 0.1f;
		viewValues_.scale += scrollDiff2.y * 0.001f;
		scrollOrigin2_ = scrollMove2_;
	}

	if (viewValues_.scale > maxCameraScale_)
		viewValues_.scale = maxCameraScale_;
	else if (viewValues_.scale < minCameraScale_)
		viewValues_.scale = minCameraScale_;

	if (viewValues_.rotation > 0.01f || viewValues_.rotation < -0.01f)
		viewValues_.rotation = fmodf(viewValues_.rotation, 360.0f);

	if (snapMovement_)
	{
		viewValues_.position.x = roundf(viewValues_.position.x);
		viewValues_.position.y = roundf(viewValues_.position.y);
	}

	camera_.setView(viewValues_);
}

void CameraController::reset()
{
	viewValues_.position.x = nc::theApplication().width() * 0.5f;
	viewValues_.position.y = nc::theApplication().height() * 0.5f;
	viewValues_.rotation = 0.0f;
	viewValues_.scale = 1.0f;
}

void CameraController::onTouchDown(const nc::TouchEvent &event)
{
	if (ignoreEvents_)
		return;

	scrollOrigin_.x = event.pointers[0].x;
	scrollOrigin_.y = event.pointers[0].y;
	scrollMove_ = scrollOrigin_;
}

void CameraController::onTouchMove(const nc::TouchEvent &event)
{
	if (ignoreEvents_)
		return;

	scrollMove_.x = event.pointers[0].x;
	scrollMove_.y = event.pointers[0].y;

	if (event.count > 1)
	{
		scrollMove2_.x = event.pointers[1].x;
		scrollMove2_.y = event.pointers[1].y;
	}
}

void CameraController::onPointerDown(const nc::TouchEvent &event)
{
	if (ignoreEvents_)
		return;

	if (event.count == 2)
	{
		scrollOrigin2_.x = event.pointers[1].x;
		scrollOrigin2_.y = event.pointers[1].y;
		scrollMove2_ = scrollOrigin2_;
	}
}

void CameraController::onMouseButtonPressed(const nc::MouseEvent &event)
{
	if (ignoreEvents_)
		return;

	if (event.isLeftButton())
	{
		scrollOrigin_.x = static_cast<float>(event.x);
		scrollOrigin_.y = static_cast<float>(event.y);
		scrollMove_ = scrollOrigin_;
	}
	else if (event.isRightButton())
	{
		scrollOrigin2_.x = static_cast<float>(event.x);
		scrollOrigin2_.y = static_cast<float>(event.y);
		scrollMove2_ = scrollOrigin2_;
	}
}

void CameraController::onMouseMoved(const nc::MouseState &state)
{
	if (ignoreEvents_)
		return;

	if (state.isLeftButtonDown())
	{
		scrollMove_.x = static_cast<float>(state.x);
		scrollMove_.y = static_cast<float>(state.y);
	}
	else if (state.isRightButtonDown())
	{
		scrollMove2_.x = static_cast<float>(state.x);
		scrollMove2_.y = static_cast<float>(state.y);
	}
}

void CameraController::onScrollInput(const nc::ScrollEvent &event)
{
	if (ignoreEvents_)
		return;

	viewValues_.rotation += 10.0f * event.x;
	viewValues_.scale += 0.1f * event.y;
}

void CameraController::onJoyMappedButtonReleased(const nc::JoyMappedButtonEvent &event)
{
	if (ignoreEvents_)
		return;

	if (event.buttonName == nc::ButtonName::LSTICK)
	{
		viewValues_.position.x = nc::theApplication().width() * 0.5f;
		viewValues_.position.y = nc::theApplication().height() * 0.5f;
	}
	else if (event.buttonName == nc::ButtonName::RSTICK)
	{
		viewValues_.rotation = 0.0f;
		viewValues_.scale = 1.0f;
	}
	else if (event.buttonName == nc::ButtonName::B)
		reset();
}

void CameraController::onJoyMappedAxisMoved(const nc::JoyMappedAxisEvent &event)
{
	if (ignoreEvents_)
		return;

	if (event.axisName == nc::AxisName::LX)
		joyVectorLeft_.x = event.value;
	else if (event.axisName == nc::AxisName::LY)
		joyVectorLeft_.y = -event.value;

	if (event.axisName == nc::AxisName::RX)
		joyVectorRight_.x = event.value;
	else if (event.axisName == nc::AxisName::RY)
		joyVectorRight_.y = -event.value;
}

void CameraController::onJoyDisconnected(const nc::JoyConnectionEvent &event)
{
	joyVectorLeft_ = nc::Vector2f::Zero;
	joyVectorRight_ = nc::Vector2f::Zero;
}
