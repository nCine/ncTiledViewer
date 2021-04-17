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

nc::Recti calculateTileRect(const MapModel::TileSet &tileSet, unsigned int column, unsigned int row)
{
	return nc::Recti(tileSet.spacing / 2 + tileSet.margin + column * (tileSet.tileWidth + tileSet.spacing),
	                 tileSet.spacing / 2 + tileSet.margin + row * (tileSet.tileHeight + tileSet.spacing),
	                 tileSet.tileWidth, tileSet.tileHeight);
}

nc::Vector2f calculateTilePosition(const MapModel::Map &map, const MapModel::Layer &layer, const MapModel::TileSet &tileSet, unsigned int column, unsigned int row)
{
	return nc::Vector2f(layer.offsetX + tileSet.tileOffset.x + (layer.x + column) * tileSet.tileWidth + tileSet.tileWidth * 0.5f - (tileSet.tileWidth * map.width * 0.5f),
	                    layer.offsetY + tileSet.tileOffset.y - ((layer.y + row) * tileSet.tileHeight + tileSet.tileHeight * 0.5f - (tileSet.tileHeight * map.height * 0.5f)));
}

ImVec2 transform(const ImVec2 &v, const nc::Matrix4x4f &m)
{
	return ImVec2(m[0][0] * v[0] + m[0][1] * v[1] + m[3][0],
	              m[1][0] * v[0] + m[1][1] * v[1] - m[3][1]);
}

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

		bool skippedPreviousGid = false;
		bool skipNextGid = false;
		int vertexIdx = 0;
		for (unsigned int gidIdx = 0; gidIdx < tileGids.size(); gidIdx++)
		{
			// Check if this or next GID needs to be skipped
			if (gidIdx + 1 < tileGids.size())
				skipNextGid = (tileGids[gidIdx + 1] == 0);

			const unsigned int preFlippingGid = tileGids[gidIdx];
			if (preFlippingGid == 0)
			{
				skippedPreviousGid = true;
				continue;
			}

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
				const nc::Vector2f position = calculateTilePosition(mapModel.map(), layer, tileSet, column, row);
				animSprite->setPosition(position);
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

					const nc::Recti frameTexRect = calculateTileRect(tileSet, frameTileSetColumn, frameTileSetRow);
					anim.addRect(frameTexRect, frame.duration * 0.001f);
				}
				animSprite->addAnimation(anim);
				animSprite->setPaused(false);
				config.animSprites->pushBack(nctl::move(animSprite));
			}
			else if (canUseMeshSprites)
			{
				nc::Vector2f pos = calculateTilePosition(mapModel.map(), layer, tileSet, column, row);
				pos.x = (pos.x - tileSet.tileWidth * 0.5f) / float(layer.width * tileSet.tileWidth);
				pos.y = (pos.y + tileSet.tileHeight * 0.5f) / float(layer.height * tileSet.tileHeight);
				const float tileWidth = tileSet.tileWidth / float(layer.width * tileSet.tileWidth);
				const float tileHeight = tileSet.tileHeight / float(layer.height * tileSet.tileHeight);

				const nc::Recti texRect = calculateTileRect(tileSet, tileSetColumn, tileSetRow);
				float u = texRect.x / float(tileSet.image.width);
				float v = texRect.y / float(tileSet.image.height);
				float du = texRect.w / float(tileSet.image.width);
				float dv = texRect.h / float(tileSet.image.height);

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

				// Insert a degenerate vertex if previous tile was on the previous column or completely skipped
				if ((gidIdx > 0 && column == 0) || skippedPreviousGid)
				{
					vertices.emplaceBack(pos.x, pos.y, u, v + dv);
					indices.pushBack(vertexIdx++);
					skippedPreviousGid = false;
				}

				vertices.emplaceBack(pos.x, pos.y, u, v + dv);
				vertices.emplaceBack(pos.x, pos.y + tileHeight, u, v);
				vertices.emplaceBack(pos.x + tileWidth, pos.y, u + du, v + dv);
				vertices.emplaceBack(pos.x + tileWidth, pos.y + tileHeight, u + du, v);

				indices.pushBack(vertexIdx++);
				indices.pushBack(vertexIdx++);
				indices.pushBack(vertexIdx++);
				indices.pushBack(vertexIdx++);

				// Insert a degenerate vertex if next tile is on the next column or going to be completely skipped
				if ((gidIdx < tileGids.size() - 1 && column == static_cast<unsigned int>(layer.width - 1)) || skipNextGid)
				{
					vertices.emplaceBack(pos.x + tileWidth, pos.y + tileHeight, u + du, v);
					indices.pushBack(vertexIdx++);
				}
			}
			else if (config.sprites)
			{
				const nc::Recti texRect = calculateTileRect(tileSet, tileSetColumn, tileSetRow);
				nc::Texture *texture = (*config.textures)[tileSetTextureIndices[tileSetIdx]].get();
				nctl::UniquePtr<nc::Sprite> sprite = nctl::makeUnique<nc::Sprite>(layerParent, texture);
				sprite->setTexRect(texRect);
				const nc::Vector2f position = calculateTilePosition(mapModel.map(), layer, tileSet, column, row);
				sprite->setPosition(position);
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

					const nc::Recti texRect = calculateTileRect(tileSet, tileSetColumn, tileSetRow);
					nc::Texture *texture = (*config.textures)[tileSetTextureIndices[tileSetIdx]].get();
					nctl::UniquePtr<nc::Sprite> sprite = nctl::makeUnique<nc::Sprite>(objectsParent, texture);
					sprite->setTexRect(texRect);
					const nc::Vector2f objectPos(objectGroup.offsetX + object.x + tileSet.tileWidth * 0.5f - (tileSet.tileWidth * mapModel.map().width * 0.5f),
					                             -objectGroup.offsetY - object.y + tileSet.tileHeight * 0.5f + (tileSet.tileHeight * mapModel.map().height * 0.5f));
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

bool MapFactory::drawObjectsWithImGui(const nc::SceneNode &node, const MapModel &mapModel, unsigned int objectGroupIdx)
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

	const bool onlyTranslation = (node.absScale().x == 1.0f && node.absScale().y == 1.0f && node.absRotation() == 0.0f);

	const float diffX = mapModel.map().width * mapModel.map().tileWidth * 0.5f;
	const float diffY = mapModel.map().height * mapModel.map().tileHeight * 0.5f;

	nc::Matrix4x4f matrix = nc::Matrix4x4f::translation(0.0f, -nc::theApplication().height(), 0.0f);
	matrix *= node.worldMatrix();
	matrix.translate(-diffX, diffY, 0.0f);

	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y), ImGuiCond_Always);
	ImGui::Begin("screen", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);
	ImDrawList *drawList = ImGui::GetWindowDrawList();

	const ImU32 color = nc::Color(255, 255, 255, 200).abgr();
	const float thickness = 2.0f;
	for (unsigned int objectIdx = 0; objectIdx < objectGroup.objects.size(); objectIdx++)
	{
		const MapModel::Object &object = objectGroup.objects[objectIdx];
		if (object.visible == false)
			continue;

		const ImVec2 origin(objectGroup.offsetX + object.x, objectGroup.offsetY + object.y);

		if (object.objectType == MapModel::ObjectType::Rectangle)
		{
			if (onlyTranslation)
			{
				const ImVec2 min = transform(ImVec2(origin.x, origin.y), matrix);
				const ImVec2 max = transform(ImVec2(origin.x + object.width, origin.y + object.height), matrix);
				drawList->AddRect(min, max, color, 0.0f, ImDrawFlags_RoundCornersNone, thickness);
			}
			else
			{
				// Cannot use `ImDrawList::AddRect()` for a rotated rectangle
				points[0] = transform(ImVec2(origin.x, origin.y), matrix);
				points[1] = transform(ImVec2(origin.x + object.width, origin.y), matrix);
				points[2] = transform(ImVec2(origin.x + object.width, origin.y + object.height), matrix);
				points[3] = transform(ImVec2(origin.x, origin.y + object.height), matrix);

				drawList->AddPolyline(points, 4, color, true, thickness);
			}
		}
		else if (object.objectType == MapModel::ObjectType::Ellipse)
		{
			const ImVec2 transformed = transform(ImVec2(origin.x + object.width / 2, origin.y + object.height / 2), matrix);
			const float radius = object.height * 0.5f * node.absScale().y;
			drawList->AddCircle(transformed, radius, color, 32, thickness);
		}
		else if (object.objectType == MapModel::ObjectType::Point)
		{
			const ImVec2 transformed = transform(origin, matrix);
			drawList->AddCircleFilled(transformed, thickness * 2.0f, color);
		}
		else if (object.objectType == MapModel::ObjectType::Polygon ||
		         object.objectType == MapModel::ObjectType::Polyline)
		{
			for (unsigned int i = 0; i < object.points.size() && i < MaxOverlayPoints; i++)
			{
				points[i] = transform(ImVec2(origin.x + object.points[i].x,
				                             origin.y + object.points[i].y), matrix);
			}

			const bool closed = object.objectType == MapModel::ObjectType::Polygon;
			drawList->AddCircleFilled(points[0], thickness * 2.0f, color);
			drawList->AddPolyline(points, object.points.size(), color, closed, thickness);
		}
		else if (object.objectType == MapModel::ObjectType::Text && onlyTranslation)
			drawList->AddText(transform(origin, matrix), object.text.color.abgr(), object.text.data);

		if (object.name[0] != '\0')
		{
			if (object.objectType != MapModel::ObjectType::Tile && object.objectType != MapModel::ObjectType::Text && onlyTranslation)
				drawList->AddText(transform(ImVec2(origin.x, origin.y - ImGui::GetFontSize()), matrix), color, object.name);
		}
	}

	ImGui::End();

	return true;
}
