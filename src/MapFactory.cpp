#include <cstdio>
#include <ncine/imgui.h>
#include <ncine/Sprite.h>
#include <ncine/MeshSprite.h>
#include <ncine/AnimatedSprite.h>
#include <ncine/Texture.h>
#include <ncine/FileSystem.h>
#include <ncine/Application.h>

#include "MapFactory.h"
#include "MapModel.h"

namespace {

ImVec2 points[MapFactory::MaxOverlayPoints];
nctl::Array<nctl::String> tileSetTextureFiles;
nctl::Array<unsigned int> tileSetTextureIndices;

}

bool MapFactory::Configuration::check() const
{
	if (textures == nullptr)
	{
		LOGE_X("No texture array specified");
		return false;
	}
	if (sprites == nullptr && meshSprites == nullptr)
	{
		LOGE_X("Neither sprite or meshsprite arrays specified");
		return false;
	}

	return true;
}

bool MapFactory::instantiate(const MapModel &mapModel, const Configuration &config)
{
	if (config.check() == false)
		return false;

	const int screenHeight = nc::theApplication().heightInt();

	// Create textures for tile sets
	tileSetTextureFiles.clear();
	tileSetTextureIndices.clear();
	const unsigned int firstTextureIndex = config.textures->size();
	for (unsigned int tileSetIdx = 0; tileSetIdx < mapModel.map().tileSets.size(); tileSetIdx++)
	{
		const MapModel::TileSet &tileSet = mapModel.map().tileSets[tileSetIdx];
		nctl::String tileSetImagePath = nc::fs::joinPath(mapModel.tsxDirName(), tileSet.image.source);

		bool textureFound = false;
		for (unsigned int i = 0; i < tileSetTextureFiles.size(); i++)
		{
			if (tileSetTextureFiles[i] == tileSetImagePath)
			{
				textureFound = true;
				tileSetTextureIndices.pushBack(firstTextureIndex + i);
				break;
			}
		}

		if (textureFound == false)
		{
			nctl::UniquePtr<nc::Texture> texture = nctl::makeUnique<nc::Texture>();
			texture->setChromaKeyEnabled(tileSet.image.hasTransparency);
			texture->setChromaKeyColor(tileSet.image.trans);
			if (config.nearestFilter)
			{
				texture->setMinFiltering(nc::Texture::Filtering::NEAREST);
				texture->setMagFiltering(nc::Texture::Filtering::NEAREST);
			}

			const bool hasLoaded = texture->loadFromFile(tileSetImagePath.data());
			if (hasLoaded == false)
			{
				LOGE_X("Cannot load image \"%s\" for tileset #%u (\"%s\")", tileSetImagePath.data(), tileSetIdx, tileSet.name);
				return false;
			}
			config.textures->pushBack(nctl::move(texture));
			tileSetTextureFiles.pushBack(tileSetImagePath);
			tileSetTextureIndices.pushBack(config.textures->size() - 1);
		}
	}
	if (config.textures->isEmpty())
	{
		LOGE("No textures have been loaded");
		return false;
	}

	// Mesh sprites can't have more than one texture
	const bool canUseMeshSprites = (config.useMeshSprites && config.meshSprites && config.textures->size() <= firstTextureIndex + 1);
	if (config.useMeshSprites && canUseMeshSprites == false)
		LOGW("Mesh sprites have been disabled");

	// Create sprites from layers
	for (unsigned int layerIdx = 0; layerIdx < mapModel.map().layers.size(); layerIdx++)
	{
		const MapModel::Layer &layer = mapModel.map().layers[layerIdx];
		if (layer.visible == false)
			continue;

		if (layer.data.encoding != MapModel::Encoding::CSV || layer.data.compression != MapModel::Compression::Uncompressed)
		{
			LOGE_X("Unsupported layer data encoding or compression for layer %u (\"%s\")", layerIdx, layer.name);
			return false;
		}

		const nctl::Array<unsigned int> &tileGids = layer.data.tileGids;
		if (tileGids.isEmpty())
		{
			LOGE_X("No tile GIDs for layer %u (\"%s\")", layerIdx, layer.name);
			return false;
		}

		nc::SceneNode *layerParent = nullptr;
		if (canUseMeshSprites == false && config.sprites)
		{
			config.sprites->setCapacity(config.sprites->capacity() + tileGids.size() + 1);
			config.sprites->pushBack(nctl::makeUnique<nc::SceneNode>(config.parent));
			layerParent = config.sprites->back().get();
			layerParent->setPosition(layer.offsetX, layer.offsetY);
			layerParent->setName(layer.name);
			layerParent->setAlphaF(layer.opacity);
		}

		nctl::Array<nc::MeshSprite::Vertex> vertices;
		nctl::Array<unsigned short int> indices;
		if (canUseMeshSprites)
		{
			vertices.setCapacity(tileGids.size() * 4 + layer.width * 2);
			indices.setCapacity(tileGids.size() * 4 + layer.width * 2);
		}

		int vertexIdx = 0;
		for (unsigned int gidIdx = 0; gidIdx < tileGids.size(); gidIdx++)
		{
			const unsigned int preFlippingGid = tileGids[gidIdx];
			if (preFlippingGid == 0 && canUseMeshSprites == false)
				continue;

			TileFlip tileFlip(preFlippingGid);
			const unsigned int gid = tileFlip.gid;

			unsigned int tileSetIdx = 0;
			for (; tileSetIdx < mapModel.map().tileSets.size(); tileSetIdx++)
			{
				const MapModel::TileSet &tileSet = mapModel.map().tileSets[tileSetIdx];
				if (tileSet.firstGid > gid)
					break;
			}
			if (tileSetIdx > 0)
				tileSetIdx--;

			const MapModel::TileSet &tileSet = mapModel.map().tileSets[tileSetIdx];
			const unsigned int tileSetRow = (gid - tileSet.firstGid) / tileSet.columns;
			const unsigned int tileSetColumn = (gid - tileSet.firstGid) % tileSet.columns;
			const unsigned int row = gidIdx / layer.width;
			const unsigned int column = gidIdx % layer.width;

			const MapModel::Tile *tile = nullptr;
			for (unsigned int i = 0; i < tileSet.tiles.size(); i++)
			{
				if (gid - tileSet.firstGid == static_cast<unsigned int>(tileSet.tiles[i].id))
				{
					tile = &tileSet.tiles[i];
					break;
				}
			}

			if (tile && tile->frames.isEmpty() == false && config.animSprites)
			{
				nc::Texture *texture = (*config.textures)[tileSetTextureIndices[tileSetIdx]].get();
				nctl::UniquePtr<nc::AnimatedSprite> animSprite = nctl::makeUnique<nc::AnimatedSprite>(layerParent, texture);
				const float x = layer.offsetX + tileSet.tileOffset.x + (layer.x + column) * tileSet.tileWidth + tileSet.tileWidth * 0.5f;
				const float y = layer.offsetY + tileSet.tileOffset.y + screenHeight - ((layer.y + row) * tileSet.tileHeight) - tileSet.tileHeight * 0.5f;
				animSprite->setPosition(x, y);
				animSprite->setAlphaF(layer.opacity);
				//animSprite->setBlendingEnabled(layer.opacity < 1.0f ? true : false);
				animSprite->setLayer(config.firstLayerDepth + layerIdx);
				animSprite->setFlippedX(tileFlip.isDiagonallyFlipped || tileFlip.isHorizontallyFlipped);
				animSprite->setFlippedY(tileFlip.isDiagonallyFlipped || tileFlip.isVerticallyFlipped);

				nc::RectAnimation anim(1.0f / 60.0f, nc::RectAnimation::LoopMode::ENABLED, nc::RectAnimation::RewindMode::FROM_START);
				for (unsigned int frameIdx = 0; frameIdx < tile->frames.size(); frameIdx++)
				{
					const MapModel::Frame &frame = tile->frames[frameIdx];

					const unsigned int frameTileSetRow = (gid - tileSet.firstGid + frame.tileId) / tileSet.columns;
					const unsigned int frameTileSetColumn = (gid - tileSet.firstGid + frame.tileId) % tileSet.columns;

					const nc::Recti frameTexRect(tileSet.spacing / 2 + frameTileSetColumn * (tileSet.tileWidth + tileSet.spacing),
					                             tileSet.spacing / 2 + frameTileSetRow * (tileSet.tileHeight + tileSet.spacing),
					                             tileSet.tileWidth, tileSet.tileHeight);
					anim.addRect(frameTexRect, frame.duration * 0.001f);
				}
				animSprite->addAnimation(anim);
				animSprite->setPaused(false);
				config.animSprites->pushBack(nctl::move(animSprite));
			}
			else if (canUseMeshSprites)
			{
				const float x = (layer.offsetX + tileSet.tileOffset.x + (layer.x + column) * tileSet.tileWidth) / float(layer.width * tileSet.tileWidth);
				const float y = (layer.offsetY + tileSet.tileOffset.y + screenHeight - ((layer.y + row) * tileSet.tileHeight)) / float(layer.height * tileSet.tileHeight);
				const float tileWidth = tileSet.tileWidth / float(layer.width * tileSet.tileWidth);
				const float tileHeight = tileSet.tileHeight / float(layer.height * tileSet.tileHeight);

				float u = (tileSet.spacing / 2 + tileSetColumn * (tileSet.tileWidth + tileSet.spacing)) / float(tileSet.image.width);
				float v = (tileSet.spacing / 2 + tileSetRow * (tileSet.tileHeight + tileSet.spacing)) / float(tileSet.image.height);
				float du = tileSet.tileWidth / float(tileSet.image.width);
				float dv = tileSet.tileHeight / float(tileSet.image.height);

				if (tileFlip.isDiagonallyFlipped || tileFlip.isHorizontallyFlipped)
				{
					u += du;
					du *= -1;
				}
				if (tileFlip.isDiagonallyFlipped || tileFlip.isVerticallyFlipped)
				{
					v += dv;
					dv *= -1;
				}

				// Insert a degenerate vertex if previous tile was on the previous column
				if (gidIdx > 0 && column == 0)
				{
					vertices.emplaceBack(x, y, u, v + dv);
					indices.pushBack(vertexIdx++);
				}

				vertices.emplaceBack(x, y, u, v + dv);
				vertices.emplaceBack(x, y + tileHeight, u, v);
				vertices.emplaceBack(x + tileWidth, y, u + du, v + dv);
				vertices.emplaceBack(x + tileWidth, y + tileHeight, u + du, v);

				indices.pushBack(vertexIdx++);
				indices.pushBack(vertexIdx++);
				indices.pushBack(vertexIdx++);
				indices.pushBack(vertexIdx++);

				// Insert a degenerate vertex if next tile is on the next column
				if (gidIdx < tileGids.size() - 1 && column == static_cast<unsigned int>(layer.width - 1))
				{
					vertices.emplaceBack(x + tileWidth, y + tileHeight, u + du, v);
					indices.pushBack(vertexIdx++);
				}
			}
			else if (config.sprites)
			{
				const nc::Recti texRect(tileSet.spacing / 2 + tileSetColumn * (tileSet.tileWidth + tileSet.spacing),
				                        tileSet.spacing / 2 + tileSetRow * (tileSet.tileHeight + tileSet.spacing),
				                        tileSet.tileWidth, tileSet.tileHeight);

				nc::Texture *texture = (*config.textures)[tileSetTextureIndices[tileSetIdx]].get();
				nctl::UniquePtr<nc::Sprite> sprite = nctl::makeUnique<nc::Sprite>(layerParent, texture);
				sprite->setTexRect(texRect);
				const float x = layer.offsetX + tileSet.tileOffset.x + (layer.x + column) * tileSet.tileWidth + tileSet.tileWidth * 0.5f;
				const float y = layer.offsetY + tileSet.tileOffset.y + screenHeight - ((layer.y + row) * tileSet.tileHeight) - tileSet.tileHeight * 0.5f;
				sprite->setPosition(x, y);
				sprite->setAlphaF(layer.opacity);
				//sprite->setBlendingEnabled(layer.opacity < 1.0f ? true : false);
				sprite->setLayer(config.firstLayerDepth + layerIdx);
				sprite->setFlippedX(tileFlip.isDiagonallyFlipped || tileFlip.isHorizontallyFlipped);
				sprite->setFlippedY(tileFlip.isDiagonallyFlipped || tileFlip.isVerticallyFlipped);
				config.sprites->pushBack(nctl::move(sprite));
			}
		}

		if (canUseMeshSprites)
		{
			nctl::UniquePtr<nc::MeshSprite> meshSprite = nctl::makeUnique<nc::MeshSprite>(config.parent, (*config.textures)[0].get());
			meshSprite->setName(layer.name);
			meshSprite->setSize(layer.width * mapModel.map().tileSets[0].tileWidth, layer.height * mapModel.map().tileSets[0].tileHeight);
			meshSprite->setPosition(0.0f, -mapModel.map().tileSets[0].tileHeight);
			meshSprite->setAlphaF(layer.opacity);
			//meshSprite->setBlendingEnabled(layer.opacity < 1.0f ? true : false);
			meshSprite->setLayer(config.firstLayerDepth + layerIdx);
			meshSprite->copyVertices(vertices.size(), vertices.data());
			meshSprite->copyIndices(indices.size(), indices.data());
			config.meshSprites->pushBack(nctl::move(meshSprite));
		}
	}

	if (config.sprites)
	{
		// Create sprites from objects
		for (unsigned int objectGroupIdx = 0; objectGroupIdx < mapModel.map().objectGroups.size(); objectGroupIdx++)
		{
			const MapModel::ObjectGroup &objectGroup = mapModel.map().objectGroups[objectGroupIdx];
			if (objectGroup.visible == false)
				continue;

			config.sprites->setCapacity(config.sprites->capacity() + objectGroup.objects.size() + 1);
			config.sprites->pushBack(nctl::makeUnique<nc::SceneNode>(config.parent));
			nc::SceneNode *objectsParent = config.sprites->back().get();
			objectsParent->setName(objectGroup.name);

			for (unsigned int objectIdx = 0; objectIdx < objectGroup.objects.size(); objectIdx++)
			{
				const MapModel::Object &object = objectGroup.objects[objectIdx];
				if (object.visible == false)
					continue;

				if (object.objectType == MapModel::ObjectType::Tile)
				{
					const unsigned int preFlippingGid = object.gid;
					TileFlip tileFlip(preFlippingGid);
					const unsigned int gid = tileFlip.gid;

					unsigned int tileSetIdx = 0;
					for (; tileSetIdx < mapModel.map().tileSets.size(); tileSetIdx++)
					{
						const MapModel::TileSet &tileSet = mapModel.map().tileSets[tileSetIdx];
						if (tileSet.firstGid > gid)
							break;
					}
					if (tileSetIdx > 0)
						tileSetIdx--;

					const MapModel::TileSet &tileSet = mapModel.map().tileSets[tileSetIdx];
					const unsigned int tileSetRow = (gid - tileSet.firstGid) / tileSet.columns;
					const unsigned int tileSetColumn = (gid - tileSet.firstGid) % tileSet.columns;

					const nc::Recti texRect(tileSet.spacing + tileSetColumn * (tileSet.tileWidth + tileSet.spacing),
					                        tileSet.spacing + tileSetRow * (tileSet.tileHeight + tileSet.spacing),
					                        tileSet.tileWidth, tileSet.tileHeight);

					nc::Texture *texture = (*config.textures)[tileSetTextureIndices[tileSetIdx]].get();
					nctl::UniquePtr<nc::Sprite> sprite = nctl::makeUnique<nc::Sprite>(objectsParent, texture);
					sprite->setTexRect(texRect);
					const nc::Vector2f objectPos(objectGroup.offsetX + object.x + tileSet.tileWidth * 0.5f, screenHeight - objectGroup.offsetY - object.y + tileSet.tileHeight * 0.5f);
					if (config.snapObjectsToPixel)
						sprite->setPosition(roundf(objectPos.x), roundf(objectPos.y));
					else
						sprite->setPosition(objectPos);
					sprite->setRotation(360.0f - object.rotation);
					sprite->setLayer(config.firstLayerDepth + mapModel.map().layers.size() + objectGroupIdx);
					sprite->setFlippedX(tileFlip.isDiagonallyFlipped || tileFlip.isHorizontallyFlipped);
					sprite->setFlippedY(tileFlip.isDiagonallyFlipped || tileFlip.isVerticallyFlipped);
					config.sprites->pushBack(nctl::move(sprite));
				}
			}
		}
	}

	return true;
}

bool MapFactory::drawObjectsWithImGui(const MapModel &mapModel, unsigned int objectGroupIdx)
{
	if (objectGroupIdx > mapModel.map().objectGroups.size() - 1)
		return false;

	const MapModel::ObjectGroup &objectGroup = mapModel.map().objectGroups[objectGroupIdx];
	if (objectGroup.visible == false)
		return false;

	bool hasObjectToDraw = false;
	for (unsigned int objectIdx = 0; objectIdx < objectGroup.objects.size(); objectIdx++)
	{
		const MapModel::Object &object = objectGroup.objects[objectIdx];
		if (object.visible && object.objectType != MapModel::ObjectType::Tile)
		{
			hasObjectToDraw = true;
			break;
		}
	}

	if (hasObjectToDraw == false)
		return false;

	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y), ImGuiCond_Always);
	ImGui::Begin("screen", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);
	ImDrawList *drawList = ImGui::GetWindowDrawList();

	const ImU32 color = IM_COL32(255, 255, 255, 200);
	const float thickness = 2.0f;
	for (unsigned int objectIdx = 0; objectIdx < objectGroup.objects.size(); objectIdx++)
	{
		const MapModel::Object &object = objectGroup.objects[objectIdx];
		if (object.visible == false)
			continue;

		const ImVec2 origin(objectGroup.offsetX + object.x, objectGroup.offsetY + object.y);

		if (object.objectType == MapModel::ObjectType::Rectangle)
		{
			drawList->AddRect(origin, ImVec2(origin.x + object.width, origin.y + object.height),
			                  color, ImDrawCornerFlags_None, thickness);
		}
		else if (object.objectType == MapModel::ObjectType::Ellipse)
		{
			drawList->AddCircle(ImVec2(origin.x + object.width / 2, origin.y + object.height / 2), object.height / 2, color, 32, thickness);
		}
		else if (object.objectType == MapModel::ObjectType::Point)
		{
			drawList->AddCircleFilled(origin, thickness * 2.0f, color);
		}
		else if (object.objectType == MapModel::ObjectType::Polygon)
		{
			for (unsigned int i = 0; i < object.points.size() && i < MaxOverlayPoints; i++)
			{
				points[i].x = origin.x + object.points[i].x;
				points[i].y = origin.y + object.points[i].y;
			}

			drawList->AddCircleFilled(points[0], thickness * 2.0f, color);
			drawList->AddPolyline(points, object.points.size(), color, true, thickness);
		}
		else if (object.objectType == MapModel::ObjectType::Polyline)
		{
			for (unsigned int i = 0; i < object.points.size() && i < MaxOverlayPoints; i++)
			{
				points[i].x = origin.x + object.points[i].x;
				points[i].y = origin.y + object.points[i].y;
			}

			drawList->AddCircleFilled(points[0], thickness * 2.0f, color);
			drawList->AddPolyline(points, object.points.size(), color, false, thickness);
		}
		else if (object.objectType == MapModel::ObjectType::Text)
			drawList->AddText(origin, object.text.color.abgr(), object.text.data);

		if (object.name[0] != '\0')
		{
			if (object.objectType != MapModel::ObjectType::Tile && object.objectType != MapModel::ObjectType::Text)
				drawList->AddText(ImVec2(origin.x, origin.y - ImGui::GetFontSize()), color, object.name);
		}
	}

	ImGui::End();

	return true;
}
