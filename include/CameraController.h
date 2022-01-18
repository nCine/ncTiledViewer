#ifndef CLASS_CAMERACONTROLLER
#define CLASS_CAMERACONTROLLER

#include <ncine/IInputEventHandler.h>
#include <ncine/Camera.h>

namespace nc = ncine;

// TODO: RENAME CLASS

/// A controller class for the camera
class CameraController :
    public nc::IInputEventHandler
{
  public:
	CameraController();

	void update(float interval);
	void reset();

	inline bool isIgnoringEvents() const { return ignoreEvents_; }
	inline void setIgnoreEvents(bool ignoreEvents) { ignoreEvents_ = ignoreEvents; }

	inline bool isSnappingMovement() const { return snapMovement_; }
	inline void setSnapMovement(bool snapMovement) { snapMovement_ = snapMovement; }

	inline float maxCameraScale() const { return maxCameraScale_; }
	inline float minCameraScale() const { return minCameraScale_; }

	void onTouchDown(const nc::TouchEvent &event) override;
	void onTouchMove(const nc::TouchEvent &event) override;
	void onPointerDown(const nc::TouchEvent &event) override;

	void onMouseButtonPressed(const nc::MouseEvent &event) override;
	void onMouseMoved(const nc::MouseState &state) override;
	void onScrollInput(const nc::ScrollEvent &event) override;

	void onJoyMappedButtonReleased(const nc::JoyMappedButtonEvent &event) override;
	void onJoyMappedAxisMoved(const nc::JoyMappedAxisEvent &event) override;
	void onJoyDisconnected(const nc::JoyConnectionEvent &event) override;

	nc::Camera &camera() { return camera_; }
	nc::Camera::ViewValues &viewValues() { return viewValues_; }

  private:
	bool ignoreEvents_;
	bool snapMovement_;

	float moveSpeed_;
	float rotateSpeed_;
	float scaleSpeed_;

	float maxCameraScale_;
	float minCameraScale_;

	nc::Vector2f scrollOrigin_;
	nc::Vector2f scrollMove_;
	nc::Vector2f scrollOrigin2_;
	nc::Vector2f scrollMove2_;
	nc::Vector2f joyVectorLeft_;
	nc::Vector2f joyVectorRight_;

	nc::Camera camera_;
	nc::Camera::ViewValues viewValues_;
};

#endif
