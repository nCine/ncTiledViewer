#ifndef CLASS_MYEVENTHANDLER
#define CLASS_MYEVENTHANDLER

#include <ncine/IAppEventHandler.h>
#include <ncine/IInputEventHandler.h>

#include <nctl/Array.h>

namespace ncine {

class AppConfiguration;
class Texture;
class Sprite;
class MeshSprite;
class AnimatedSprite;
class SceneNode;

}

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

	void onKeyReleased(const nc::KeyboardEvent &event) override;

  private:
	nctl::UniquePtr<nc::SceneNode> parent_;
	nctl::Array<nctl::UniquePtr<nc::Texture>> textures_;
	nctl::Array<nctl::UniquePtr<nc::Sprite>> sprites_;
	nctl::Array<nctl::UniquePtr<nc::MeshSprite>> meshSprites_;
	nctl::Array<nctl::UniquePtr<nc::AnimatedSprite>> animSprites_;
};

#endif
