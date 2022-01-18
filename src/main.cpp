#include "main.h"
#include <nctl/CString.h>
#include <ncine/imgui.h>
#include <ncine/Application.h>
#include <ncine/AppConfiguration.h>
#include <ncine/Colorf.h>
#include <ncine/Viewport.h>
#include <ncine/Texture.h>
#include <ncine/SceneNode.h>
#include <ncine/TimeStamp.h>

#include <ncine/Sprite.h>
#include <ncine/MeshSprite.h>
#include <ncine/AnimatedSprite.h>

#include "MapModel.h"
#include "TmxParser.h"
#include "MapFactory.h"
#include "FileDialog.h"
#include "CameraController.h"

namespace {

const char *propertyTypeToString(MapModel::PropertyType propertyType)
{
	switch (propertyType)
	{
		case MapModel::PropertyType::StringType:
			return "String";
		case MapModel::PropertyType::IntType:
			return "Int";
		case MapModel::PropertyType::FloatType:
			return "Float";
		case MapModel::PropertyType::BoolType:
			return "Bool";
		case MapModel::PropertyType::ColorType:
			return "Color";
		case MapModel::PropertyType::FileType:
			return "File";
		case MapModel::PropertyType::ObjectType:
			return "Object";
		default:
			return "Unknown";
	}
}

const char *gridOrientationToString(MapModel::GridOrientation orientation)
{
	switch (orientation)
	{
		case MapModel::GridOrientation::Orthogonal:
			return "Orthogonal";
		case MapModel::GridOrientation::Isometric:
			return "Isometric";
		default:
			return "Unknown";
	}
}

const char *orientationToString(MapModel::Orientation orientation)
{
	switch (orientation)
	{
		case MapModel::Orientation::Orthogonal:
			return "Orthogonal";
		case MapModel::Orientation::Isometric:
			return "Isometric";
		case MapModel::Orientation::Staggered:
			return "Staggered";
		case MapModel::Orientation::Hexagonal:
			return "Hexagonal";
		default:
			return "Unknown";
	}
}

const char *renderOrderToString(MapModel::RenderOrder renderOrder)
{
	switch (renderOrder)
	{
		case MapModel::RenderOrder::Right_Down:
			return "Right-Down";
		case MapModel::RenderOrder::Right_Up:
			return "Right-Up";
		case MapModel::RenderOrder::Left_Down:
			return "Left-Down";
		case MapModel::RenderOrder::Left_Up:
			return "Left-Up";
		default:
			return "Unknown";
	}
}

const char *staggerAxisToString(MapModel::StaggerAxis staggerAxis)
{
	switch (staggerAxis)
	{
		case MapModel::StaggerAxis::X:
			return "X";
		case MapModel::StaggerAxis::Y:
			return "Y";
		default:
			return "Unknown";
	}
}

const char *staggerIndexToString(MapModel::StaggerIndex staggerIndex)
{
	switch (staggerIndex)
	{
		case MapModel::StaggerIndex::Even:
			return "Even";
		case MapModel::StaggerIndex::Odd:
			return "Odd";
		default:
			return "Unknown";
	}
}

const char *objectAlignmentToString(MapModel::ObjectAlignment objectAlignment)
{
	switch (objectAlignment)
	{
		case MapModel::ObjectAlignment::Unspecified:
			return "Unspecified";
		case MapModel::ObjectAlignment::TopLeft:
			return "Top Left";
		case MapModel::ObjectAlignment::Top:
			return "Top";
		case MapModel::ObjectAlignment::TopRight:
			return "Top Right";
		case MapModel::ObjectAlignment::Left:
			return "Left";
		case MapModel::ObjectAlignment::Center:
			return "Center";
		case MapModel::ObjectAlignment::Right:
			return "Right";
		case MapModel::ObjectAlignment::BottomLeft:
			return "Bottom Left";
		case MapModel::ObjectAlignment::Bottom:
			return "Bottom";
		case MapModel::ObjectAlignment::BottomRight:
			return "Bottom Right";
		default:
			return "Unknown";
	}
}

const char *encodingToString(MapModel::Encoding encoding)
{
	switch (encoding)
	{
		case MapModel::Encoding::Base64:
			return "Base64";
		case MapModel::Encoding::CSV:
			return "CSV";
		default:
			return "Unknown";
	}
}

const char *compressionToString(MapModel::Compression compression)
{
	switch (compression)
	{
		case MapModel::Compression::Uncompressed:
			return "Uncompressed";
		case MapModel::Compression::gzip:
			return "gzip";
		case MapModel::Compression::zlib:
			return "zlib";
		case MapModel::Compression::zstd:
			return "zstd";
		default:
			return "Unknown";
	}
}

const char *drawOrderToString(MapModel::DrawOrder drawOrder)
{
	switch (drawOrder)
	{
		case MapModel::DrawOrder::Index:
			return "Index";
		case MapModel::DrawOrder::TopDown:
			return "Top Down";
		default:
			return "Unknown";
	}
}

const char *objectTypeToString(MapModel::ObjectType objectType)
{
	switch (objectType)
	{
		case MapModel::ObjectType::Tile:
			return "Tile";
		case MapModel::ObjectType::Rectangle:
			return "Rectangle";
		case MapModel::ObjectType::Ellipse:
			return "Ellipse";
		case MapModel::ObjectType::Point:
			return "Point";
		case MapModel::ObjectType::Polygon:
			return "Polygon";
		case MapModel::ObjectType::Polyline:
			return "Polyline";
		case MapModel::ObjectType::Text:
			return "Text";
		default:
			return "Unknown";
	}
}

const char *HorizontalAlignToString(MapModel::HorizontalAlign hAlign)
{
	switch (hAlign)
	{
		case MapModel::HorizontalAlign::Left:
			return "Left";
		case MapModel::HorizontalAlign::Center:
			return "Center";
		case MapModel::HorizontalAlign::Right:
			return "Right";
		case MapModel::HorizontalAlign::Justify:
			return "Justify";
		default:
			return "Unknown";
	}
}

const char *VerticalAlignToString(MapModel::VerticalAlign vAlign)
{
	switch (vAlign)
	{
		case MapModel::VerticalAlign::Top:
			return "Top";
		case MapModel::VerticalAlign::Center:
			return "Center";
		case MapModel::VerticalAlign::Bottom:
			return "Bottom";
		default:
			return "Unknown";
	}
}

void treeProperties(const nctl::Array<MapModel::Property> &properties)
{
	static nctl::String auxString(256);

	if (properties.isEmpty() == false && ImGui::TreeNode(&properties, "Properties"))
	{
		for (unsigned int propertyIDx = 0; propertyIDx < properties.size(); propertyIDx++)
		{
			const MapModel::Property &property = properties[propertyIDx];
			auxString.format("#%u %s \"%s\":", propertyIDx, propertyTypeToString(property.type), property.name);
			if (property.type != MapModel::PropertyType::ColorType)
				auxString.append(" ");

			switch (property.type)
			{
				case MapModel::PropertyType::StringType:
					auxString.formatAppend("\"%s\"", property.string);
					break;
				case MapModel::PropertyType::IntType:
					auxString.formatAppend("%d", property.intValue());
					break;
				case MapModel::PropertyType::FloatType:
					auxString.formatAppend("%f", property.floatValue());
					break;
				case MapModel::PropertyType::BoolType:
					auxString.formatAppend("%s", property.boolValue() ? "yes" : "no");
					break;
				case MapModel::PropertyType::ColorType:
					break;
				case MapModel::PropertyType::FileType:
					auxString.formatAppend("\"%s\"", property.string);
					break;
				case MapModel::PropertyType::ObjectType:
					auxString.formatAppend("%d", property.object());
					break;
			}

			ImGui::Text("%s", auxString.data());
			if (property.type == MapModel::PropertyType::ColorType)
			{
				ImGui::SameLine();
				nc::Colorf objectColor(property.color());
				auxString.format("###%s", property.name);
				ImGui::ColorEdit4(auxString.data(), objectColor.data(), ImGuiColorEditFlags_NoPicker |
				                  ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop);
			}
		}

		ImGui::TreePop();
	}
}

void unload(MapFactory::Configuration &config)
{
	if (config.animSprites)
		config.animSprites->clear();

	if (config.sprites)
		config.sprites->clear();

	if (config.meshSprites)
		config.meshSprites->clear();

	if (config.textures)
		config.textures->clear();
}

bool loadMap(MapFactory::Configuration &mapConfig, MapModel &mapModel, const char *filename)
{
	if (filename[0] == '\0' || nc::fs::isReadableFile(filename) == false)
		return false;

	LOGI_X("Loading map \"%s\"", filename);
	mapModel = MapModel();
	nc::TimeStamp timestamp = nc::TimeStamp::now();
	const bool hasParsed = TmxParser::loadFromFile(mapModel, filename);
	LOGI_X("Map parsed in %f ms", timestamp.millisecondsSince());
	if (hasParsed)
	{
		unload(mapConfig);
		timestamp = nc::TimeStamp::now();
		MapFactory::instantiate(mapModel, mapConfig);
		LOGI_X("Map instantiated in %f ms", timestamp.millisecondsSince());
		return true;
	}
	return false;
}

MapModel mapModel;
MapFactory::Configuration mapConfig;
bool showInterface = true;
bool withVSync = true;
bool drawOverlay = true;
}

nctl::UniquePtr<nc::IAppEventHandler> createAppEventHandler()
{
	return nctl::makeUnique<MyEventHandler>();
}

void MyEventHandler::onPreInit(nc::AppConfiguration &config)
{
#if defined(__ANDROID__)
	config.dataPath() = "asset::";
#elif defined(__EMSCRIPTEN__)
	config.dataPath() = "/";
#else
	#ifdef NCPROJECT_DEFAULT_DATA_DIR
	config.dataPath() = NCPROJECT_DEFAULT_DATA_DIR;
	#else
	config.dataPath() = "data/";
	#endif
#endif

	config.windowTitle = "ncTiledViewer";
	config.windowIconFilename = "icon48.png";
	config.consoleLogLevel = nc::ILogger::LogLevel::INFO;
	withVSync = config.withVSync;
}

void MyEventHandler::onInit()
{
#ifdef __ANDROID__
	ImGuiIO &io = ImGui::GetIO();
	io.FontGlobalScale = 2.0f;
#endif

	cameraCtrl_ = nctl::makeUnique<CameraController>();
	nc::theApplication().rootViewport().setCamera(&cameraCtrl_->camera());
	parent_ = nctl::makeUnique<nc::SceneNode>(&nc::theApplication().rootNode());
	nc::theApplication().inputManager().setHandler(this);
	mapConfig.textures = &textures_;
	mapConfig.sprites = &sprites_;
	mapConfig.meshSprites = &meshSprites_;
	mapConfig.animSprites = &animSprites_;
	mapConfig.parent = parent_.get();

	const nctl::String MapsPath = nc::fs::joinPath(nc::fs::dataPath(), "maps");
	const nctl::String StartupFile = nc::fs::joinPath(MapsPath, "orthogonal-outside.tmx");

	FileDialog::config.directory.assign(MapsPath);
	if (nc::fs::isReadableFile(StartupFile.data()))
		loadMap(mapConfig, mapModel, StartupFile.data());
}

void MyEventHandler::onFrameStart()
{
	const float interval = nc::theApplication().interval();
	const MapModel::Map &map = mapModel.map();

	static nctl::String fileSelection(nc::fs::MaxPathLength);
	if (FileDialog::create(FileDialog::config, fileSelection))
		loadMap(mapConfig, mapModel, fileSelection.data());

	ImGui::SetNextWindowPos(ImVec2(nc::theApplication().width() * 0.75f, 0.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(nc::theApplication().width() * 0.25f, nc::theApplication().height()), ImGuiCond_FirstUseEver);
	if (showInterface && ImGui::Begin("ncTiledViewer", &showInterface))
	{
		ImGui::Checkbox("Use Mesh Sprites", &mapConfig.useMeshSprites);
		if (ImGui::Button("Load Map..."))
		{
			FileDialog::config.windowTitle = "Open TMX map";
			FileDialog::config.selectionType = FileDialog::SelectionType::FILE;
			FileDialog::config.extensions = "tmx\0\0";
			FileDialog::config.modalPopup = true;
			FileDialog::config.windowOpen = true;
		}
		ImGui::Separator();

		nc::Application::RenderingSettings &renderingSettings = nc::theApplication().renderingSettings();
		ImGui::Checkbox("Batching", &renderingSettings.batchingEnabled);
		ImGui::SameLine();
		ImGui::Checkbox("Culling", &renderingSettings.cullingEnabled);
		ImGui::SameLine();
#ifdef __ANDROID__
		ImGui::Text("VSync: true");
#else
		ImGui::Checkbox("VSync", &withVSync);
		nc::theApplication().gfxDevice().setSwapInterval(withVSync ? 1 : 0);
#endif
		ImGui::Text("FPS: %.2f (%.2f ms)", 1.0f / interval, interval * 1000.0f);
		ImGui::Separator();

		nc::Camera::ViewValues &viewValues = cameraCtrl_->viewValues();
		ImGui::InputFloat2("Position", viewValues.position.data());
		ImGui::SameLine();
		if (ImGui::Button("Reset##Position"))
			viewValues.position.set(nc::theApplication().width() * 0.5f, nc::theApplication().height() * 0.5f);
		ImGui::SliderFloat("Scale", &viewValues.scale, cameraCtrl_->minCameraScale(), cameraCtrl_->maxCameraScale());
		ImGui::SameLine();
		if (ImGui::Button("Reset##Scale"))
			viewValues.scale = 1.0f;
		ImGui::SliderFloat("Rotation", &viewValues.rotation, 0.0f, 360.0f);
		ImGui::SameLine();
		if (ImGui::Button("Reset##Rotation"))
			viewValues.rotation = 0.0f;

		if (ImGui::Button("Reset All##Camera"))
			cameraCtrl_->reset();

		ImGui::SameLine();
		bool ignoreEvents = cameraCtrl_->isIgnoringEvents();
		ImGui::Checkbox("Ignore Events", &ignoreEvents);
		cameraCtrl_->setIgnoreEvents(ignoreEvents);

		ImGui::SameLine();
		bool snapMovement = cameraCtrl_->isSnappingMovement();
		ImGui::Checkbox("Snap Movement", &snapMovement);
		cameraCtrl_->setSnapMovement(snapMovement);
		ImGui::Separator();

		for (unsigned int i = 0; i < mapConfig.parent->children().size(); i++)
		{
			nc::SceneNode *child = mapConfig.parent->children()[i];
			if (child->type() == nc::Object::ObjectType::MESH_SPRITE)
			{
				const nc::MeshSprite *meshSprite = static_cast<const nc::MeshSprite *>(child);
				ImGui::Text("#%d - %s (%u vertices)", i, child->name().data(), meshSprite->numVertices());
			}
			else
				ImGui::Text("#%d - %s (%u children)", i, child->name().data(), child->children().size());
			ImGui::Indent();
			ImGui::PushID(child);
			bool isEnabled = child->isEnabled();
			ImGui::Checkbox("Visible", &isEnabled);
			child->setEnabled(isEnabled);
			ImGui::PopID();
			ImGui::Unindent();
		}
		if (mapConfig.animSprites && mapConfig.animSprites->isEmpty() == false && ImGui::TreeNode("Animated Sprites"))
		{
			for (unsigned int i = 0; i < mapConfig.animSprites->size(); i++)
			{
				nc::AnimatedSprite *animSprite = (*mapConfig.animSprites)[i].get();
				ImGui::Text("#%d at frame %u", i, animSprite->frame());
				ImGui::SameLine();
				if (ImGui::Button("Reset"))
					animSprite->setFrame(0);
				ImGui::SameLine();
				bool isPaused = animSprite->isPaused();
				ImGui::Checkbox("Paused", &isPaused);
				animSprite->setPaused(isPaused);
			}

			ImGui::TreePop();
		}
		ImGui::Checkbox("Draw Overlay", &drawOverlay);
		ImGui::Separator();

		if (ImGui::CollapsingHeader("Map Model"))
		{
			ImGui::Text("Format Version: %s", map.version);
			ImGui::Text("Tiled Version: %s", map.tiledVersion);
			ImGui::Text("Orientation: %s", orientationToString(map.orientation));
			ImGui::Text("Render Order: %s", renderOrderToString(map.renderOrder));
			ImGui::Text("Compression Level: %d", map.compressionLevel);
			ImGui::Text("Width: %d", map.width);
			ImGui::Text("Height: %d", map.height);
			ImGui::Text("Tile Width: %d", map.tileWidth);
			ImGui::Text("Tile Height: %d", map.tileHeight);
			ImGui::Text("Hex Side Length: %d", map.hexSideLength);
			ImGui::Text("Stagger Axis: %s", staggerAxisToString(map.staggerAxis));
			ImGui::Text("Stagger Index: %s", staggerIndexToString(map.staggerIndex));
			nc::Colorf backgroundColor(map.backgroundColor);
			ImGui::ColorEdit4("Background Color", backgroundColor.data(), ImGuiColorEditFlags_NoPicker |
			                  ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop);
			ImGui::Text("Next Layer Id: %d", map.nextLayerId);
			ImGui::Text("Next Object Id: %d", map.nextObjectId);
			ImGui::Text("Infinite: %s", map.infinite ? "yes" : "no");

			treeProperties(map.properties);

			if (map.tileSets.isEmpty() == false && ImGui::TreeNode("Tilesets"))
			{
				for (unsigned int tileSetIdx = 0; tileSetIdx < map.tileSets.size(); tileSetIdx++)
				{
					const MapModel::TileSet &tileSet = map.tileSets[tileSetIdx];
					if (ImGui::TreeNode(&tileSet, "Tileset #%u", tileSetIdx))
					{
						ImGui::Text("First GID: %d", tileSet.firstGid);
						ImGui::Text("Source: %s", tileSet.source);
						ImGui::Text("Name: %s", tileSet.name);
						ImGui::Text("Tile Width: %d", tileSet.tileWidth);
						ImGui::Text("Tile Height: %d", tileSet.tileHeight);
						ImGui::Text("Spacing: %d", tileSet.spacing);
						ImGui::Text("Margin: %d", tileSet.margin);
						ImGui::Text("Tile Count: %d", tileSet.tileCount);
						ImGui::Text("Columns: %d", tileSet.columns);
						ImGui::Text("Object Alignment: %s", objectAlignmentToString(tileSet.objectAlignment));

						if (nctl::strnlen(tileSet.image.source, MapModel::MaxSourceLength) && ImGui::TreeNode("Image"))
						{
							const MapModel::Image &image = tileSet.image;
							ImGui::Text("Format: %s", image.format);
							ImGui::Text("Source: %s", image.source);
							if (image.hasTransparency)
							{
								nc::Colorf transColor(image.trans);
								ImGui::ColorEdit3("Transparent", transColor.data(), ImGuiColorEditFlags_NoPicker |
								                  ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop);
							}
							ImGui::Text("Width: %d", image.width);
							ImGui::Text("Height: %d", image.height);

							ImGui::TreePop();
						}

						if (ImGui::TreeNode("Tile Offset"))
						{
							ImGui::Text("X: %f", tileSet.tileOffset.x);
							ImGui::Text("Y: %f", tileSet.tileOffset.y);

							ImGui::TreePop();
						}

						if (ImGui::TreeNode("Grid"))
						{
							ImGui::Text("Orientation: %s", gridOrientationToString(tileSet.grid.orientation));
							ImGui::Text("Width: %d", tileSet.grid.width);
							ImGui::Text("Height: %d", tileSet.grid.height);

							ImGui::TreePop();
						}

						if (tileSet.terrainTypes.isEmpty() == false && ImGui::TreeNode("Terrain Types"))
						{
							for (unsigned int terrainTypeIdx = 0; terrainTypeIdx < tileSet.terrainTypes.size(); terrainTypeIdx++)
							{
								const MapModel::Terrain &terrain = tileSet.terrainTypes[terrainTypeIdx];
								if (ImGui::TreeNode(&terrain, "Terrain #%u", terrainTypeIdx))
								{
									ImGui::Text("Name: %s", terrain.name);
									ImGui::Text("Tile: %d", terrain.tile);

									treeProperties(terrain.properties);

									ImGui::TreePop();
								}
							}

							ImGui::TreePop();
						}

						if (tileSet.tiles.isEmpty() == false && ImGui::TreeNode("Tiles"))
						{
							for (unsigned int tileIdx = 0; tileIdx < tileSet.tiles.size(); tileIdx++)
							{
								const MapModel::Tile &tile = tileSet.tiles[tileIdx];
								if (ImGui::TreeNode(&tile, "Tile #%u", tileIdx))
								{
									ImGui::Text("Id: %d", tile.id);
									if (tile.type > -1)
										ImGui::Text("Type: %d", tile.type);
									ImGui::Text("Terrain: %d, %d, %d, %d", tile.terrain[0], tile.terrain[1], tile.terrain[2], tile.terrain[3]);
									ImGui::Text("Probability: %f", tile.probability);

									if (tile.frames.isEmpty() == false && ImGui::TreeNode("Frames"))
									{
										for (unsigned int frameIdx = 0; frameIdx < tile.frames.size(); frameIdx++)
										{
											const MapModel::Frame &frame = tile.frames[frameIdx];
											if (ImGui::TreeNode(&frame, "Frame #%u", frameIdx))
											{
												ImGui::Text("Tile Id: %d", frame.tileId);
												ImGui::Text("Duration: %d ms", frame.duration);

												ImGui::TreePop();
											}
										}

										ImGui::TreePop();
									}

									treeProperties(tile.properties);

									ImGui::TreePop();
								}
							}

							ImGui::TreePop();
						}

						treeProperties(tileSet.properties);

						ImGui::TreePop();
					}
				}

				ImGui::TreePop();
			}

			if (map.layers.isEmpty() == false && ImGui::TreeNode("Layers"))
			{
				for (unsigned int layerIdx = 0; layerIdx < map.layers.size(); layerIdx++)
				{
					const MapModel::Layer &layer = map.layers[layerIdx];
					if (ImGui::TreeNode(&layer, "Layer #%u", layerIdx))
					{
						ImGui::Text("Id: %d", layer.id);
						ImGui::Text("Name: %s", layer.name);
						ImGui::Text("X: %d", layer.x);
						ImGui::Text("Y: %d", layer.y);
						ImGui::Text("Width: %d", layer.width);
						ImGui::Text("Height: %d", layer.height);
						ImGui::Text("Opacity: %f", layer.opacity);
						ImGui::Text("Visible: %s", layer.visible ? "yes" : "no");
						nc::Colorf tintColor(layer.tintColor);
						ImGui::ColorEdit4("Tint Color", tintColor.data(), ImGuiColorEditFlags_NoPicker |
						                  ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop);
						ImGui::Text("Offset X: %f", layer.offsetX);
						ImGui::Text("Offset Y: %f", layer.offsetY);

						if (layer.data.string && ImGui::TreeNode("Data"))
						{
							const MapModel::Data &data = layer.data;
							ImGui::Text("Encoding: %s", encodingToString(data.encoding));
							ImGui::Text("Compression: %s", compressionToString(data.compression));

							if (data.tileGids.isEmpty() == false)
								ImGui::Text("Tile GIDs: %u", data.tileGids.size());

							ImGui::TreePop();
						}

						treeProperties(layer.properties);

						ImGui::TreePop();
					}
				}

				ImGui::TreePop();
			}

			if (map.objectGroups.isEmpty() == false && ImGui::TreeNode("Object Groups"))
			{
				for (unsigned int groupIdx = 0; groupIdx < map.objectGroups.size(); groupIdx++)
				{
					const MapModel::ObjectGroup &objectGroup = map.objectGroups[groupIdx];
					if (ImGui::TreeNode(&objectGroup, "Object Group #%u", groupIdx))
					{
						ImGui::Text("Id: %d", objectGroup.id);
						ImGui::Text("Name: %s", objectGroup.name);
						nc::Colorf color(objectGroup.color);
						ImGui::ColorEdit4("Color", color.data(), ImGuiColorEditFlags_NoPicker |
						                  ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop);
						ImGui::Text("X: %d", objectGroup.x);
						ImGui::Text("Y: %d", objectGroup.y);
						ImGui::Text("Width: %d", objectGroup.width);
						ImGui::Text("Height: %d", objectGroup.height);
						ImGui::Text("Opacity: %f", objectGroup.opacity);
						ImGui::Text("Visible: %s", objectGroup.visible ? "yes" : "no");
						nc::Colorf tintColor(objectGroup.tintColor);
						ImGui::ColorEdit4("Tint Color", tintColor.data(), ImGuiColorEditFlags_NoPicker |
						                  ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop);
						ImGui::Text("Offset X: %f", objectGroup.offsetX);
						ImGui::Text("Offset Y: %f", objectGroup.offsetY);
						ImGui::Text("Draw Order: %s", drawOrderToString(objectGroup.drawOrder));

						treeProperties(objectGroup.properties);

						if (map.objectGroups.isEmpty() == false && ImGui::TreeNode("Objects"))
						{
							for (unsigned int objectIdx = 0; objectIdx < objectGroup.objects.size(); objectIdx++)
							{
								const MapModel::Object &object = objectGroup.objects[objectIdx];
								if (ImGui::TreeNode(&object, "Object #%u (%s)", objectIdx, objectTypeToString(object.objectType)))
								{
									ImGui::Text("Id: %d", object.id);
									ImGui::Text("Name: %s", object.name);
									ImGui::Text("X: %f", object.x);
									ImGui::Text("Y: %f", object.y);
									ImGui::Text("Width: %f", object.width);
									ImGui::Text("Height: %f", object.height);
									ImGui::Text("Rotation: %f", object.rotation);
									if (object.objectType == MapModel::ObjectType::Tile)
										ImGui::Text("GID: %d", object.gid);
									ImGui::Text("Visible: %s", object.visible ? "yes" : "no");
									if (object.templateFile[0] != '\0')
										ImGui::Text("Template: %s", object.templateFile);
									ImGui::Text("Object Type: %s", objectTypeToString(object.objectType));

									if (object.objectType == MapModel::ObjectType::Polygon ||
									    object.objectType == MapModel::ObjectType::Polyline)
									{
										const nctl::Array<nc::Vector2i> &points = object.points;

										if (points.isEmpty() == false && ImGui::TreeNode(&points, "%u points", points.size()))
										{
											ImGui::Columns(2, "points", true);
											ImGui::Text("X");
											ImGui::NextColumn();
											ImGui::Text("Y");
											ImGui::NextColumn();
											ImGui::Separator();

											for (unsigned int i = 0; i < points.size(); i++)
											{
												ImGui::Text("%d", points[i].x);
												ImGui::NextColumn();
												ImGui::Text("%d", points[i].y);
												ImGui::NextColumn();
											}

											ImGui::Columns(1);
											ImGui::TreePop();
										}
									}
									else if (object.objectType == MapModel::ObjectType::Text)
									{
										const MapModel::Text &text = object.text;
										if (ImGui::TreeNode(&text, "Text (\"%10s\")", text.data))
										{
											ImGui::Text("Data: %s", text.data);
											ImGui::Text("Font Family: %s", text.fontFamily);
											ImGui::Text("Pixel Size: %d", text.pixelSize);
											ImGui::Text("Wrap: %s", text.wrap ? "yes" : "no");
											nc::Colorf textColor(text.color);
											ImGui::ColorEdit4("Color", textColor.data(), ImGuiColorEditFlags_NoPicker |
											                  ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop);
											ImGui::Text("Bold: %s", text.bold ? "yes" : "no");
											ImGui::Text("Italic: %s", text.italic ? "yes" : "no");
											ImGui::Text("Underline: %s", text.underline ? "yes" : "no");
											ImGui::Text("Strikeout: %s", text.strikeout ? "yes" : "no");
											ImGui::Text("Kerning: %s", text.kerning ? "yes" : "no");
											ImGui::Text("Horizontal Align: %s", HorizontalAlignToString(text.hAlign));
											ImGui::Text("Vertical Align: %s", VerticalAlignToString(text.vAlign));

											ImGui::TreePop();
										}
									}

									treeProperties(object.properties);

									ImGui::TreePop();
								}
							}

							ImGui::TreePop();
						}

						ImGui::TreePop();
					}
				}

				ImGui::TreePop();
			}

			if (map.imageLayers.isEmpty() == false && ImGui::TreeNode("Image Layers"))
			{
				for (unsigned int layerIdx = 0; layerIdx < map.imageLayers.size(); layerIdx++)
				{
					const MapModel::ImageLayer &imageLayer = map.imageLayers[layerIdx];
					if (ImGui::TreeNode(&imageLayer, "Image Layer #%u", layerIdx))
					{
						ImGui::Text("Id: %d", imageLayer.id);
						ImGui::Text("Name: %s", imageLayer.name);
						ImGui::Text("Offset X: %f", imageLayer.offsetX);
						ImGui::Text("Offset Y: %f", imageLayer.offsetY);
						ImGui::Text("X: %d", imageLayer.x);
						ImGui::Text("Y: %d", imageLayer.y);

						ImGui::Text("Opacity: %f", imageLayer.opacity);
						ImGui::Text("Visible: %s", imageLayer.visible ? "yes" : "no");
						nc::Colorf tintColor(imageLayer.tintColor);
						ImGui::ColorEdit4("Tint Color", tintColor.data(), ImGuiColorEditFlags_NoPicker |
						                  ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop);

						ImGui::TreePop();
					}
				}

				ImGui::TreePop();
			}
		}
		ImGui::End();
	}

	cameraCtrl_->update(interval);
}

void MyEventHandler::onPostUpdate()
{
	if (drawOverlay)
	{
		for (unsigned int i = 0; i < mapModel.map().objectGroups.size(); i++)
			MapFactory::drawObjectsWithImGui(cameraCtrl_->camera(), mapModel, i);
	}
}

void MyEventHandler::onKeyReleased(const nc::KeyboardEvent &event)
{
	nc::Application::RenderingSettings &renderingSettings = nc::theApplication().renderingSettings();

	if (event.mod & nc::KeyMod::CTRL && event.sym == nc::KeySym::Q)
		nc::theApplication().quit();

	if (event.sym == nc::KeySym::B)
		renderingSettings.batchingEnabled = !renderingSettings.batchingEnabled;
	else if (event.sym == nc::KeySym::C)
		renderingSettings.cullingEnabled = !renderingSettings.cullingEnabled;
	else if (event.sym == nc::KeySym::H)
		showInterface = !showInterface;
	else if (event.sym == nc::KeySym::ESCAPE)
		FileDialog::config.windowOpen = false;
}

void MyEventHandler::onTouchDown(const nc::TouchEvent &event)
{
	cameraCtrl_->onTouchDown(event);
}

void MyEventHandler::onTouchMove(const nc::TouchEvent &event)
{
	cameraCtrl_->onTouchMove(event);
}

void MyEventHandler::onPointerDown(const nc::TouchEvent &event)
{
	cameraCtrl_->onPointerDown(event);
}

void MyEventHandler::onMouseButtonPressed(const nc::MouseEvent &event)
{
	cameraCtrl_->onMouseButtonPressed(event);
}

void MyEventHandler::onMouseMoved(const nc::MouseState &state)
{
	cameraCtrl_->onMouseMoved(state);
}

void MyEventHandler::onScrollInput(const nc::ScrollEvent &event)
{
	cameraCtrl_->onScrollInput(event);
}

void MyEventHandler::onJoyMappedAxisMoved(const nc::JoyMappedAxisEvent &event)
{
	cameraCtrl_->onJoyMappedAxisMoved(event);
}

void MyEventHandler::onJoyMappedButtonReleased(const nc::JoyMappedButtonEvent &event)
{
	nc::Application::RenderingSettings &renderingSettings = nc::theApplication().renderingSettings();
	nc::IDebugOverlay::DisplaySettings &overlaySettings = nc::theApplication().debugOverlaySettings();

	if (event.buttonName == nc::ButtonName::A)
		renderingSettings.batchingEnabled = !renderingSettings.batchingEnabled;
	else if (event.buttonName == nc::ButtonName::Y)
		renderingSettings.cullingEnabled = !renderingSettings.cullingEnabled;
	else if (event.buttonName == nc::ButtonName::BACK)
	{
		overlaySettings.showProfilerGraphs = !overlaySettings.showProfilerGraphs;
		overlaySettings.showInfoText = !overlaySettings.showInfoText;
	}
	else if (event.buttonName == nc::ButtonName::START)
		showInterface = !showInterface;
	else if (event.buttonName == nc::ButtonName::GUIDE)
		nc::theApplication().quit();

	cameraCtrl_->onJoyMappedButtonReleased(event);
}

void MyEventHandler::onJoyDisconnected(const nc::JoyConnectionEvent &event)
{
	cameraCtrl_->onJoyDisconnected(event);
}
