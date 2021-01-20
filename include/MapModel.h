#ifndef MAPMODEL_H
#define MAPMODEL_H

#include <nctl/Array.h>
#include <nctl/String.h>
#include <ncine/Color.h>
#include <ncine/Vector2.h>

namespace nc = ncine;

/// The class that holds all the information contained in a Tiled map
class MapModel
{
  public:
	static const unsigned int MaxSourceLength = 256;
	static const unsigned int MaxNameLength = 64;

	enum class PropertyType
	{
		StringType,
		IntType,
		FloatType,
		BoolType,
		ColorType,
		FileType,
		ObjectType
	};

	struct Property
	{
		static const unsigned int MaxStringLength = 1024;

		char name[MaxNameLength];
		PropertyType type = PropertyType::StringType;
		union
		{
			int intValue;
			float floatValue;
			bool boolValue;
			int color;
			int object;
		} value;
		char string[MaxStringLength];

		Property()
		{
			name[0] = '\0';
			value.intValue = 0;
			string[0] = '\0';
		}

		inline int intValue() const { return (type == PropertyType::IntType) ? value.intValue : 0; }
		inline float floatValue() const { return (type == PropertyType::FloatType) ? value.floatValue : 0.0f; }
		inline bool boolValue() const { return (type == PropertyType::BoolType) ? value.boolValue : false; }
		inline int intColor() const { return (type == PropertyType::ColorType) ? value.color : 0; }
		inline nc::Color color() const { return (type == PropertyType::ColorType) ? nc::Color(value.color) : nc::Color::Black; }
		inline int object() const { return (type == PropertyType::ObjectType) ? value.object : 0; }
	};

	struct Image
	{
		static const unsigned int MaxFormatLength = 8;

		char format[MaxFormatLength];
		char source[MaxSourceLength];
		/// True if a transparent color has been specified in the TMX file
		bool hasTransparency = false;
		nc::Color trans;
		int width;
		int height;
	};

	struct TileOffset
	{
		float x = 0.0f;
		float y = 0.0f;
	};

	enum class GridOrientation
	{
		Orthogonal,
		Isometric
	};

	struct Grid
	{
		GridOrientation orientation = GridOrientation::Orthogonal;
		int width;
		int height;
	};

	struct Terrain
	{
		char name[MaxNameLength];
		int tile;

		nctl::Array<Property> properties;
	};

	struct Frame
	{
		int tileId;
		/// Frame duration expressed in milliseconds
		int duration;
	};

	struct Tile
	{
		int id;
		int type = -1;
		int terrain[4] = { -1, -1, -1, -1 };
		float probability = 0.0f;

		nctl::Array<Frame> frames;
		nctl::Array<Property> properties;
	};

	enum class ObjectAlignment
	{
		Unspecified,
		TopLeft,
		Top,
		TopRight,
		Left,
		Center,
		Right,
		BottomLeft,
		Bottom,
		BottomRight
	};

	struct TileSet
	{
		unsigned int firstGid;
		char source[MaxSourceLength];
		char name[MaxNameLength];
		int tileWidth;
		int tileHeight;
		int spacing = 0;
		int margin = 0;
		int tileCount;
		int columns;
		ObjectAlignment objectAlignment = ObjectAlignment::Unspecified;

		Image image;
		TileOffset tileOffset;
		Grid grid;
		nctl::Array<Terrain> terrainTypes;
		nctl::Array<Tile> tiles;
		nctl::Array<Property> properties;
	};

	enum class Encoding
	{
		Base64,
		CSV
	};

	enum class Compression
	{
		Uncompressed,
		gzip,
		zlib,
		zstd
	};

	struct Data
	{
		static const unsigned int MaxEncodingLength = 7; // "base64" and "csv"
		static const unsigned int MaxCompressionLength = 5; // "gzip", "zlib", "zstd".
		static const unsigned int MaxDataLength = 1024 * 64;

		Encoding encoding;
		Compression compression = Compression::Uncompressed;
		/// It contains the data string if it was not possible to extract tile gids
		nctl::UniquePtr<char[]> string;

		nctl::Array<unsigned int> tileGids;
	};

	struct Layer
	{
		int id;
		char name[MaxNameLength];
		int x = 0;
		int y = 0;
		int width;
		int height;
		float opacity = 1.0f;
		bool visible = true;
		nc::Color tintColor = nc::Color::White;
		float offsetX = 0.0f;
		float offsetY = 0.0f;

		Data data;
		nctl::Array<Property> properties;
	};

	enum class HorizontalAlign
	{
		Left,
		Center,
		Right,
		Justify
	};

	enum class VerticalAlign
	{
		Top,
		Center,
		Bottom,
	};

	struct Text
	{
		static const unsigned int MaxDataLength = 256;

		char data[MaxDataLength];
		char fontFamily[MaxNameLength];
		int pixelSize = 16;
		bool wrap = false;
		nc::Color color = nc::Color::Black;
		bool bold = false;
		bool italic = false;
		bool underline = false;
		bool strikeout = false;
		bool kerning = true;
		HorizontalAlign hAlign = HorizontalAlign::Left;
		VerticalAlign vAlign = VerticalAlign::Top;
	};

	enum class ObjectType
	{
		Tile,
		Rectangle,
		Ellipse,
		Point,
		Polygon,
		Polyline,
		Text
	};

	struct Object
	{
		static const unsigned int MaxTypeLength = 256;

		int id;
		char name[MaxNameLength];
		char type[MaxTypeLength];
		float x = 0.0f;
		float y = 0.0f;
		float width = 0.0f;
		float height = 0.0f;
		float rotation = 0.0f;
		unsigned int gid = 0;
		bool visible = true;
		char templateFile[MaxSourceLength];

		ObjectType objectType = ObjectType::Tile;
		nctl::Array<nc::Vector2i> points;
		struct Text text;
		nctl::Array<Property> properties;
	};

	enum class DrawOrder
	{
		Index,
		TopDown
	};

	struct ObjectGroup
	{
		int id;
		char name[MaxNameLength];
		nc::Color color = nc::Color(0xa0a0a4);
		int x = 0;
		int y = 0;
		int width;
		int height;
		float opacity = 1.0f;
		bool visible = true;
		nc::Color tintColor = nc::Color::White;
		float offsetX = 0.0f;
		float offsetY = 0.0f;
		DrawOrder drawOrder;

		nctl::Array<Object> objects;
		nctl::Array<Property> properties;
	};

	struct ImageLayer
	{
		int id;
		char name[MaxNameLength];
		float offsetX = 0.0f;
		float offsetY = 0.0f;
		int x = 0;
		int y = 0;
		float opacity = 1.0f;
		bool visible = true;
		nc::Color tintColor = nc::Color::White;

		Image image;
		nctl::Array<Property> properties;
	};

	enum class Orientation
	{
		Orthogonal,
		Isometric,
		Staggered,
		Hexagonal
	};

	enum class RenderOrder
	{
		Right_Down,
		Right_Up,
		Left_Down,
		Left_Up
	};

	enum class StaggerAxis
	{
		X,
		Y
	};

	enum class StaggerIndex
	{
		Even,
		Odd
	};

	struct Map
	{
		static const unsigned int MaxVersionLength = 8;

		char version[MaxVersionLength];
		char tiledVersion[MaxVersionLength];
		Orientation orientation;
		RenderOrder renderOrder = RenderOrder::Right_Down;
		int compressionLevel = -1;
		int width;
		int height;
		int tileWidth;
		int tileHeight;
		int hexSideLength;
		StaggerAxis staggerAxis;
		StaggerIndex staggerIndex;
		nc::Color backgroundColor;
		int nextLayerId;
		int nextObjectId;
		bool infinite = false;

		nctl::Array<TileSet> tileSets;
		nctl::Array<Layer> layers;
		nctl::Array<ObjectGroup> objectGroups;
		nctl::Array<ImageLayer> imageLayers;
		nctl::Array<Property> properties;
	};

	void reset();

	inline const Map &map() const { return map_; }
	inline Map &map() { return map_; }

	inline const nctl::String &tmxDirName() const { return tmxDirName_; }
	inline nctl::String &tmxDirName() { return tmxDirName_; }
	inline const nctl::String &tsxDirName() const { return tmxDirName_; }
	inline nctl::String &tsxDirName() { return tmxDirName_; }

  private:
	Map map_;
	nctl::String tmxDirName_;
	nctl::String tsxDirName_; // TODO: inside tileset
};

#endif
