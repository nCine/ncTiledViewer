#ifndef CLASS_CAMERANODE
#define CLASS_CAMERANODE

#include <ncine/IInputEventHandler.h>
#include <ncine/SceneNode.h>

namespace nc = ncine;

/// My nCine event handler
class CameraNode :
    public nc::SceneNode,
    public nc::IInputEventHandler
{
  public:
	explicit CameraNode(SceneNode *parent);

	void update(float interval) override;
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
};

#endif
