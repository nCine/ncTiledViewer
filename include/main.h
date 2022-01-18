#ifndef CLASS_MYEVENTHANDLER
#define CLASS_MYEVENTHANDLER

#include <ncine/IAppEventHandler.h>
#include <ncine/IInputEventHandler.h>

#include <nctl/Array.h>

namespace ncine {

class AppConfiguration;
class Texture;
class SceneNode;
class Sprite;
class MeshSprite;
class AnimatedSprite;

}

class CameraController;

namespace nc = ncine;

/// My nCine event handler
class MyEventHandler :
    public nc::IAppEventHandler,
    public nc::IInputEventHandler
{
  public:
	void onPreInit(nc::AppConfiguration &config) override;
	void onInit() override;
	void onFrameStart() override;
	void onPostUpdate() override;

	void onKeyReleased(const nc::KeyboardEvent &event) override;

	void onTouchDown(const nc::TouchEvent &event) override;
	void onTouchMove(const nc::TouchEvent &event) override;
	void onPointerDown(const nc::TouchEvent &event) override;

	void onMouseButtonPressed(const nc::MouseEvent &event) override;
	void onMouseMoved(const nc::MouseState &state) override;
	void onScrollInput(const nc::ScrollEvent &event) override;

	void onJoyMappedAxisMoved(const nc::JoyMappedAxisEvent &event) override;
	void onJoyMappedButtonReleased(const nc::JoyMappedButtonEvent &event) override;
	void onJoyDisconnected(const nc::JoyConnectionEvent &event) override;

  private:
	nctl::UniquePtr<CameraController> cameraCtrl_;
	nctl::UniquePtr<nc::SceneNode> parent_;
	nctl::Array<nctl::UniquePtr<nc::Texture>> textures_;
	nctl::Array<nctl::UniquePtr<nc::Sprite>> sprites_;
	nctl::Array<nctl::UniquePtr<nc::MeshSprite>> meshSprites_;
	nctl::Array<nctl::UniquePtr<nc::AnimatedSprite>> animSprites_;
};

#endif
