#ifndef MAPFACTORY_H
#define MAPFACTORY_H

#include <nctl/Array.h>

namespace ncine {

class Texture;
class Sprite;
class MeshSprite;
class AnimatedSprite;
class SceneNode;

template <class T> class Matrix4x4;
using Matrix4x4f = Matrix4x4<float>;

}

namespace nc = ncine;

class MapModel;

/// The class responsible for instantiating scene nodes from a Tiled map model
class MapFactory
{
  public:
	class Configuration
	{
	  public:
		Configuration()
		    : textures(nullptr), sprites(nullptr), meshSprites(nullptr), animSprites(nullptr), parent(nullptr),
		      firstLayerDepth(0), nearestFilter(true), snapObjectsToPixel(true), useMeshSprites(true)
		{}

		/// An array of textures where all tileset images will be appended
		nctl::Array<nctl::UniquePtr<nc::Texture>> *textures;
		/// An array of sprites where all tiles and objects will be appended
		nctl::Array<nctl::UniquePtr<nc::Sprite>> *sprites;
		/// An array of mesh sprites where all layers of tiles will be appended
		nctl::Array<nctl::UniquePtr<nc::MeshSprite>> *meshSprites;
		/// An array of sprites where all tiles and objects will be appended
		nctl::Array<nctl::UniquePtr<nc::AnimatedSprite>> *animSprites;
		/// The optional parent node of all kind of sprites
		nc::SceneNode *parent;
		/// The depth value of the first layer of the map
		unsigned short firstLayerDepth;
		/// Applies a nearest filter to all textures
		bool nearestFilter;
		/// Snaps objects position to the nearest pixel coordinate
		bool snapObjectsToPixel;
		/// Creates mesh sprites if possible
		bool useMeshSprites;

	  private:
		bool check() const;

		friend class MapFactory;
	};

	struct TileFlip
	{
		static const unsigned HorizontalFlipFlag = 0x80000000;
		static const unsigned VerticalFlipFlag = 0x40000000;
		static const unsigned DiagonalFlipFlag = 0x20000000;

		const bool isHorizontallyFlipped = false;
		const bool isVerticallyFlipped = false;
		const bool isDiagonallyFlipped = false;
		const unsigned int gid = 0;

		TileFlip(unsigned int preFlippingGid)
		    : isHorizontallyFlipped(preFlippingGid & HorizontalFlipFlag),
		      isVerticallyFlipped(preFlippingGid & VerticalFlipFlag),
		      isDiagonallyFlipped(preFlippingGid & DiagonalFlipFlag),
		      gid(~(HorizontalFlipFlag | VerticalFlipFlag | DiagonalFlipFlag) & preFlippingGid)
		{}
	};

	static const unsigned int MaxOverlayPoints = 64;

	static bool instantiate(const MapModel &mapModel, const Configuration &config);
	static bool drawObjectsWithImGui(const ncine::SceneNode &node, const MapModel &mapModel, unsigned int objectGroupIdx);
};

#endif
