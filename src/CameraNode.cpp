#include "CameraNode.h"
#include <ncine/IInputManager.h>
#include <ncine/Application.h>

CameraNode::CameraNode(SceneNode *parent)
    : nc::SceneNode(parent), ignoreEvents_(false), snapMovement_(true),
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

void CameraNode::update(float interval)
{
	if (ignoreEvents_)
	{
		nc::SceneNode::update(interval);
		return;
	}

	const nc::KeyboardState &keyState = nc::theApplication().inputManager().keyboardState();
	if (keyState.isKeyDown(nc::KeySym::D))
		x -= moveSpeed_ * interval;
	else if (keyState.isKeyDown(nc::KeySym::A))
		x += moveSpeed_ * interval;
	if (keyState.isKeyDown(nc::KeySym::W))
		y -= moveSpeed_ * interval;
	else if (keyState.isKeyDown(nc::KeySym::S))
		y += moveSpeed_ * interval;

	if (keyState.isKeyDown(nc::KeySym::RIGHT))
		rotation_ += rotateSpeed_ * interval;
	else if (keyState.isKeyDown(nc::KeySym::LEFT))
		rotation_ -= rotateSpeed_ * interval;

	if (keyState.isKeyDown(nc::KeySym::UP))
		scaleFactor_ += scaleSpeed_ * interval;
	else if (keyState.isKeyDown(nc::KeySym::DOWN))
		scaleFactor_ -= scaleSpeed_ * interval;

	if (joyVectorLeft_.length() > nc::IInputManager::LeftStickDeadZone)
	{
		x -= joyVectorLeft_.x * moveSpeed_ * interval;
		y -= joyVectorLeft_.y * moveSpeed_ * interval;
	}
	if (joyVectorRight_.length() > nc::IInputManager::RightStickDeadZone)
	{
		rotation_ += joyVectorRight_.x * rotateSpeed_ * interval;
		scaleFactor_ += joyVectorRight_.y * scaleSpeed_ * interval;
	}

	const nc::Vector2f scrollDiff = scrollMove_ - scrollOrigin_;
	if (scrollDiff.sqrLength() > 2.0f)
	{
		x += scrollDiff.x;
		y += scrollDiff.y;
		scrollOrigin_ = scrollMove_;
	}
	const nc::Vector2f scrollDiff2 = scrollMove2_ - scrollOrigin2_;
	if (scrollDiff2.sqrLength() > 2.0f)
	{
		rotation_ += scrollDiff2.x * 0.1f;
		scaleFactor_ += scrollDiff2.y * 0.001f;
		scrollOrigin2_ = scrollMove2_;
	}

	if (scaleFactor_.x > maxCameraScale_)
		scaleFactor_.set(maxCameraScale_, maxCameraScale_);
	else if (scaleFactor_.x < minCameraScale_)
		scaleFactor_.set(minCameraScale_, minCameraScale_);

	if (rotation_ > 0.01f || rotation_ < -0.01f)
		rotation_ = fmodf(rotation_, 360.0f);

	if (snapMovement_)
	{
		x = roundf(x);
		y = roundf(y);
	}

	nc::SceneNode::update(interval);
}

void CameraNode::reset()
{
	x = nc::theApplication().width() * 0.5f;
	y = nc::theApplication().height() * 0.5f;
	rotation_ = 0.0f;
	scaleFactor_.set(1.0f, 1.0f);
}

void CameraNode::onTouchDown(const nc::TouchEvent &event)
{
	if (ignoreEvents_)
		return;

	scrollOrigin_.x = event.pointers[0].x;
	scrollOrigin_.y = event.pointers[0].y;
	scrollMove_ = scrollOrigin_;
}

void CameraNode::onTouchMove(const nc::TouchEvent &event)
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

void CameraNode::onPointerDown(const nc::TouchEvent &event)
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

void CameraNode::onMouseButtonPressed(const nc::MouseEvent &event)
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

void CameraNode::onMouseMoved(const nc::MouseState &state)
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

void CameraNode::onScrollInput(const nc::ScrollEvent &event)
{
	if (ignoreEvents_)
		return;

	rotation_ += 10.0f * event.x;
	scaleFactor_ += 0.1f * event.y;
}

void CameraNode::onJoyMappedButtonReleased(const nc::JoyMappedButtonEvent &event)
{
	if (ignoreEvents_)
		return;

	if (event.buttonName == nc::ButtonName::LSTICK)
	{
		x = nc::theApplication().width() * 0.5f;
		y = nc::theApplication().height() * 0.5f;
	}
	else if (event.buttonName == nc::ButtonName::RSTICK)
	{
		rotation_ = 0.0f;
		scaleFactor_.set(1.0f, 1.0f);
	}
	else if (event.buttonName == nc::ButtonName::B)
		reset();
}

void CameraNode::onJoyMappedAxisMoved(const nc::JoyMappedAxisEvent &event)
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

void CameraNode::onJoyDisconnected(const nc::JoyConnectionEvent &event)
{
	joyVectorLeft_ = nc::Vector2f::Zero;
	joyVectorRight_ = nc::Vector2f::Zero;
}
