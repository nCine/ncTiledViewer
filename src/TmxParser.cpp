#include <cstring> // for `strncmp()`
#include <cstdio> // for `sscanf()`
#include "pugixml.hpp"
#include <nctl/CString.h>
#include <ncine/IFile.h>
#include <ncine/FileSystem.h>

#include "TmxParser.h"
#include "MapModel.h"

namespace {

bool loadFile(const char *filename, nctl::UniquePtr<unsigned char[]> &fileBuffer, long int &fileSize)
{
	nctl::UniquePtr<nc::IFile> file = nc::IFile::createFileHandle(filename);
	file->setExitOnFailToOpen(false);
	file->open(nc::IFile::OpenMode::READ);
	if (file->isOpened() == false)
	{
		LOGE_X("Cannot open file: %s", filename);
		return false;
	}

	fileSize = file->size();
	fileBuffer = nctl::makeUnique<unsigned char[]>(fileSize);
	file->read(fileBuffer.get(), fileSize);
	file->close();
	return true;
}

bool loadXmlFile(const char *filename, pugi::xml_document &document)
{
	long int xmlFileSize = 0;
	nctl::UniquePtr<unsigned char[]> xmlFileBuffer;
	const bool hasLoaded = loadFile(filename, xmlFileBuffer, xmlFileSize);
	if (hasLoaded == false)
		return false;

	pugi::xml_parse_result result = document.load_buffer_inplace(xmlFileBuffer.get(), xmlFileSize);

	if (result == false)
	{
		LOGE_X("Error description: %s", result.description());
		LOGE_X("Error offset: %s (error at [...%ul])", result.offset, xmlFileBuffer.get() + result.offset);
		return false;
	}

	return true;
}

unsigned int parseIntColor(pugi::xml_attribute colorAttr)
{
	unsigned int hexColor = 0;
	if (colorAttr.empty() == false)
	{
		// In the #AARRGGBB or #RRGGBB forms
		const char *value = colorAttr.value();
		if (value[0] == '#')
			sscanf(value, "#%8x", &hexColor);
	}
	return hexColor;
}

bool parseColor(nc::Color &color, pugi::xml_attribute colorAttr)
{
	if (colorAttr.empty() == false)
	{
		// In the #AARRGGBB or #RRGGBB forms
		const char *value = colorAttr.value();
		if (value[0] == '#')
		{
			unsigned int hexColor = 0;
			sscanf(value, "#%8x", &hexColor);
			color.set(hexColor);
		}
		return true;
	}
	return false;
}

bool parseProperties(nctl::Array<MapModel::Property> &properties, pugi::xml_node propertiesNode)
{
	if (propertiesNode.empty())
		return false;

	pugi::xml_node firstPropertyNode = propertiesNode.child("property");

	unsigned int numProperties = 0;
	for (pugi::xml_node propertyNode = firstPropertyNode; propertyNode; propertyNode = propertyNode.next_sibling("property"))
		numProperties++;
	if (numProperties == 0)
		return false;
	properties.setCapacity(numProperties);

	for (pugi::xml_node propertyNode = firstPropertyNode; propertyNode; propertyNode = propertyNode.next_sibling("property"))
	{
		properties.emplaceBack();
		MapModel::Property &property = properties.back();

		pugi::xml_attribute nameAttr = propertyNode.attribute("name");
		if (nameAttr.empty() == false)
			nctl::strncpy(property.name, nameAttr.value(), MapModel::MaxNameLength - 1);

		pugi::xml_attribute typeAttr = propertyNode.attribute("type");
		if (typeAttr.empty() == false)
		{
			const char *value = typeAttr.value();
			if (strncmp(value, "string", strlen("string")) == 0)
				property.type = MapModel::PropertyType::StringType;
			else if (strncmp(value, "int", strlen("int")) == 0)
				property.type = MapModel::PropertyType::IntType;
			else if (strncmp(value, "float", strlen("float")) == 0)
				property.type = MapModel::PropertyType::FloatType;
			else if (strncmp(value, "bool", strlen("bool")) == 0)
				property.type = MapModel::PropertyType::BoolType;
			else if (strncmp(value, "color", strlen("color")) == 0)
				property.type = MapModel::PropertyType::ColorType;
			else if (strncmp(value, "file", strlen("file")) == 0)
				property.type = MapModel::PropertyType::FileType;
			else if (strncmp(value, "object", strlen("object")) == 0)
				property.type = MapModel::PropertyType::ObjectType;
		}

		pugi::xml_attribute valueAttr = propertyNode.attribute("value");
		if (valueAttr.empty() == false)
		{
			switch (property.type)
			{
				case MapModel::PropertyType::StringType:
					nctl::strncpy(property.string, valueAttr.value(), MapModel::Property::MaxStringLength - 1);
					break;
				case MapModel::PropertyType::IntType:
					property.value.intValue = valueAttr.as_int();
					break;
				case MapModel::PropertyType::FloatType:
					property.value.floatValue = valueAttr.as_float();
					break;
				case MapModel::PropertyType::BoolType:
					property.value.boolValue = valueAttr.as_bool();
					break;
				case MapModel::PropertyType::ColorType:
					property.value.color = parseIntColor(valueAttr);
					break;
				case MapModel::PropertyType::FileType:
					nctl::strncpy(property.string, valueAttr.value(), MapModel::Property::MaxStringLength - 1);
					break;
				case MapModel::PropertyType::ObjectType:
					property.value.object = valueAttr.as_int();
					break;
			}
		}
	}

	return true;
}

bool parseTextObject(MapModel::Text &text, pugi::xml_node textNode)
{
	text.data[0] = '\0';
	nctl::strncpy(text.data, textNode.child_value(), MapModel::Text::MaxDataLength - 1);

	pugi::xml_attribute fontFamilyAttr = textNode.attribute("fontfamily");
	if (fontFamilyAttr.empty() == false)
		nctl::strncpy(text.fontFamily, fontFamilyAttr.value(), MapModel::MaxNameLength - 1);

	pugi::xml_attribute pixelSizeAttr = textNode.attribute("pixelsize");
	if (pixelSizeAttr.empty() == false)
		text.pixelSize = pixelSizeAttr.as_int();

	pugi::xml_attribute wrapAttr = textNode.attribute("wrap");
	if (wrapAttr.empty() == false)
		text.wrap = wrapAttr.as_bool();

	parseColor(text.color, textNode.attribute("color"));

	pugi::xml_attribute boldAttr = textNode.attribute("bold");
	if (boldAttr.empty() == false)
		text.bold = boldAttr.as_bool();

	pugi::xml_attribute italicAttr = textNode.attribute("italic");
	if (italicAttr.empty() == false)
		text.italic = italicAttr.as_bool();

	pugi::xml_attribute underlineAttr = textNode.attribute("underline");
	if (underlineAttr.empty() == false)
		text.underline = underlineAttr.as_bool();

	pugi::xml_attribute strikeoutAttr = textNode.attribute("strikeout");
	if (strikeoutAttr.empty() == false)
		text.strikeout = strikeoutAttr.as_bool();

	pugi::xml_attribute kerningAttr = textNode.attribute("kerning");
	if (kerningAttr.empty() == false)
		text.kerning = kerningAttr.as_bool();

	pugi::xml_attribute hAlignAttr = textNode.attribute("halign");
	if (hAlignAttr.empty() == false)
	{
		const char *value = hAlignAttr.value();
		if (strncmp(value, "left", strlen("left")) == 0)
			text.hAlign = MapModel::HorizontalAlign::Left;
		else if (strncmp(value, "center", strlen("center")) == 0)
			text.hAlign = MapModel::HorizontalAlign::Center;
		else if (strncmp(value, "right", strlen("right")) == 0)
			text.hAlign = MapModel::HorizontalAlign::Right;
		else if (strncmp(value, "justify", strlen("justify")) == 0)
			text.hAlign = MapModel::HorizontalAlign::Justify;
	}

	pugi::xml_attribute vAlignAttr = textNode.attribute("valign");
	if (vAlignAttr.empty() == false)
	{
		const char *value = vAlignAttr.value();
		if (strncmp(value, "top", strlen("top")) == 0)
			text.vAlign = MapModel::VerticalAlign::Top;
		else if (strncmp(value, "center", strlen("center")) == 0)
			text.vAlign = MapModel::VerticalAlign::Center;
		else if (strncmp(value, "bottom", strlen("bottom")) == 0)
			text.vAlign = MapModel::VerticalAlign::Bottom;
	}

	return true;
}

bool parsePolyPoints(const char *string, nctl::Array<nc::Vector2i> &points)
{
	// Count elements
	const char *buffer = string;
	unsigned int numElements = 0;
	while (*buffer != '\0')
	{
		while (*buffer != ',' && *buffer != '\0')
			buffer++;
		if (*buffer == ',')
		{
			numElements++;
			buffer++;
		}
	}

	if (numElements > 0)
		LOGI_X("There are %u elements in the list of points", numElements);
	else
	{
		LOGE_X("There are no elements in the list of points");
		return false;
	}

	points.setCapacity(numElements);

	// Parse elements
	buffer = string;
	while (*buffer != '\0')
	{
		const char *begin = buffer;
		while (*begin == ' ' || *begin == '\t' || *begin == '\n')
			begin++;
		const char *comma = begin;
		while (*comma != ' ' && *comma != '\t' && *comma != '\n' && *comma != '\0' && *comma != ',')
			comma++;
		const char *end = comma + 1;
		while (*end != ' ' && *end != '\t' && *end != '\n' && *end != '\0' && *end != ',')
			end++;

		int x = 0;
		const int xMatched = sscanf(begin, "%d", &x);
		if (xMatched != 1)
		{
			LOGE_X("Parsing list of points failed at byte %u", begin - string);
			return false;
		}
		int y = 0;
		const int yMatched = sscanf(comma + 1, "%d", &y);
		if (yMatched != 1)
		{
			LOGE_X("Parsing list of points failed at byte %u", comma + 1 - string);
			return false;
		}

		points.emplaceBack(x, y);
		buffer = end;
	}

	return true;
}

bool parseObjectNodes(MapModel::ObjectGroup &objectGroup, pugi::xml_node firstObjectNode)
{
	unsigned int numObjects = 0;
	for (pugi::xml_node objectNode = firstObjectNode; objectNode; objectNode = objectNode.next_sibling("object"))
		numObjects++;
	if (numObjects == 0)
		return false;
	objectGroup.objects.setCapacity(numObjects);

	for (pugi::xml_node objectNode = firstObjectNode; objectNode; objectNode = objectNode.next_sibling("object"))
	{
		objectGroup.objects.emplaceBack();
		MapModel::Object &object = objectGroup.objects.back();

		pugi::xml_attribute idAttr = objectNode.attribute("id");
		if (idAttr.empty() == false)
			object.id = idAttr.as_int();

		pugi::xml_attribute nameAttr = objectNode.attribute("name");
		if (nameAttr.empty() == false)
			nctl::strncpy(object.name, nameAttr.value(), MapModel::MaxNameLength - 1);

		pugi::xml_attribute typeAttr = objectNode.attribute("type");
		if (typeAttr.empty() == false)
			nctl::strncpy(object.type, typeAttr.value(), MapModel::Object::MaxTypeLength - 1);

		pugi::xml_attribute xAttr = objectNode.attribute("x");
		if (xAttr.empty() == false)
			object.x = xAttr.as_float();

		pugi::xml_attribute yAttr = objectNode.attribute("y");
		if (yAttr.empty() == false)
			object.y = yAttr.as_float();

		pugi::xml_attribute widthAttr = objectNode.attribute("width");
		if (widthAttr.empty() == false)
			object.width = widthAttr.as_float();

		pugi::xml_attribute heightAttr = objectNode.attribute("height");
		if (heightAttr.empty() == false)
			object.height = heightAttr.as_float();

		pugi::xml_attribute rotationAttr = objectNode.attribute("rotation");
		if (rotationAttr.empty() == false)
			object.rotation = rotationAttr.as_float();

		pugi::xml_attribute gidAttr = objectNode.attribute("gid");
		if (gidAttr.empty() == false)
		{
			object.gid = gidAttr.as_uint(); // unsigned to make flipping work
			object.objectType = MapModel::ObjectType::Tile;
		}
		else
			object.objectType = MapModel::ObjectType::Rectangle;

		pugi::xml_attribute visibleAttr = objectNode.attribute("visible");
		if (visibleAttr.empty() == false)
			object.visible = visibleAttr.as_bool();

		pugi::xml_attribute templateAttr = objectNode.attribute("template");
		object.templateFile[0] = '\0';
		if (templateAttr.empty() == false)
			nctl::strncpy(object.templateFile, templateAttr.value(), MapModel::MaxSourceLength - 1);

		pugi::xml_node ellipseNode = objectNode.child("ellipse");
		if (ellipseNode.empty() == false)
			object.objectType = MapModel::ObjectType::Ellipse;

		pugi::xml_node pointNode = objectNode.child("point");
		if (pointNode.empty() == false)
			object.objectType = MapModel::ObjectType::Point;

		pugi::xml_node polygonNode = objectNode.child("polygon");
		if (polygonNode.empty() == false)
		{
			object.objectType = MapModel::ObjectType::Polygon;
			pugi::xml_attribute pointsAttr = polygonNode.attribute("points");
			parsePolyPoints(pointsAttr.value(), object.points);
		}

		pugi::xml_node polylineNode = objectNode.child("polyline");
		if (polylineNode.empty() == false)
		{
			object.objectType = MapModel::ObjectType::Polyline;
			pugi::xml_attribute pointsAttr = polylineNode.attribute("points");
			parsePolyPoints(pointsAttr.value(), object.points);
		}

		pugi::xml_node textNode = objectNode.child("text");
		if (textNode.empty() == false)
		{
			object.objectType = MapModel::ObjectType::Text;
			parseTextObject(object.text, textNode);
		}

		parseProperties(object.properties, objectNode.child("properties"));
	}

	return true;
}

bool parseObjectGroupNodes(nctl::Array<MapModel::ObjectGroup> &objectGroups, pugi::xml_node firstObjectGroupNode)
{
	unsigned int numObjectGroups = 0;
	for (pugi::xml_node objectGroupNode = firstObjectGroupNode; objectGroupNode; objectGroupNode = objectGroupNode.next_sibling("objectgroup"))
		numObjectGroups++;
	if (numObjectGroups == 0)
		return false;
	objectGroups.setCapacity(numObjectGroups);

	for (pugi::xml_node objectGroupNode = firstObjectGroupNode; objectGroupNode; objectGroupNode = objectGroupNode.next_sibling("objectgroup"))
	{
		objectGroups.emplaceBack();
		MapModel::ObjectGroup &objectGroup = objectGroups.back();

		pugi::xml_attribute idAttr = objectGroupNode.attribute("id");
		if (idAttr.empty() == false)
			objectGroup.id = idAttr.as_int();

		pugi::xml_attribute nameAttr = objectGroupNode.attribute("name");
		if (nameAttr.empty() == false)
			nctl::strncpy(objectGroup.name, nameAttr.value(), MapModel::MaxNameLength - 1);

		parseColor(objectGroup.color, objectGroupNode.attribute("color"));

		pugi::xml_attribute xAttr = objectGroupNode.attribute("x");
		if (xAttr.empty() == false)
			objectGroup.x = xAttr.as_int();

		pugi::xml_attribute yAttr = objectGroupNode.attribute("y");
		if (yAttr.empty() == false)
			objectGroup.y = yAttr.as_int();

		pugi::xml_attribute widthAttr = objectGroupNode.attribute("width");
		if (widthAttr.empty() == false)
			objectGroup.width = widthAttr.as_int();

		pugi::xml_attribute heightAttr = objectGroupNode.attribute("height");
		if (heightAttr.empty() == false)
			objectGroup.height = heightAttr.as_int();

		pugi::xml_attribute opacityAttr = objectGroupNode.attribute("opacity");
		if (opacityAttr.empty() == false)
			objectGroup.opacity = opacityAttr.as_float();

		pugi::xml_attribute visibleAttr = objectGroupNode.attribute("visible");
		if (visibleAttr.empty() == false)
			objectGroup.visible = visibleAttr.as_bool();

		parseColor(objectGroup.tintColor, objectGroupNode.attribute("tintcolor"));

		pugi::xml_attribute offsetXAttr = objectGroupNode.attribute("offsetx");
		if (offsetXAttr.empty() == false)
			objectGroup.offsetX = offsetXAttr.as_float();

		pugi::xml_attribute offsetYAttr = objectGroupNode.attribute("offsety");
		if (offsetYAttr.empty() == false)
			objectGroup.offsetY = offsetYAttr.as_float();

		pugi::xml_attribute drawOrderAttr = objectGroupNode.attribute("draworder");
		if (drawOrderAttr.empty() == false)
		{
			const char *value = drawOrderAttr.value();
			if (strncmp(value, "index", strlen("index")) == 0)
				objectGroup.drawOrder = MapModel::DrawOrder::Index;
			else if (strncmp(value, "topdown", strlen("topdown")) == 0)
				objectGroup.drawOrder = MapModel::DrawOrder::TopDown;
		}

		pugi::xml_node firstObjectNode = objectGroupNode.child("object");
		if (firstObjectNode.empty() == false)
			parseObjectNodes(objectGroup, firstObjectNode);

		parseProperties(objectGroup.properties, objectGroupNode.child("properties"));
	}

	return true;
}

bool parseImageNode(MapModel::Image &image, pugi::xml_node imageNode)
{
	if (imageNode.empty())
		return false;

	pugi::xml_attribute formatAttr = imageNode.attribute("format");
	if (formatAttr.empty() == false)
		nctl::strncpy(image.format, formatAttr.value(), MapModel::Image::MaxFormatLength - 1);

	pugi::xml_attribute sourceAttr = imageNode.attribute("source");
	if (sourceAttr.empty() == false)
		nctl::strncpy(image.source, sourceAttr.value(), MapModel::MaxSourceLength - 1);

	// Can't use the `parseColor` function in this case
	pugi::xml_attribute transAttr = imageNode.attribute("trans");
	if (transAttr.empty() == false)
	{
		// In the #RRGGBB or RRGGBB forms
		const char *value = transAttr.value();
		unsigned int hexColor = 0;
		if (value[0] == '#')
			sscanf(value, "#%6x", &hexColor);
		else
			sscanf(value, "%6x", &hexColor);
		image.trans.set(hexColor);
		image.hasTransparency = true;
	}

	pugi::xml_attribute widthAttr = imageNode.attribute("width");
	if (widthAttr.empty() == false)
		image.width = widthAttr.as_int();

	pugi::xml_attribute heightAttr = imageNode.attribute("height");
	if (heightAttr.empty() == false)
		image.height = heightAttr.as_int();

	return true;
}

bool parseImageLayerNodes(nctl::Array<MapModel::ImageLayer> &imageLayers, pugi::xml_node firstImageLayerNode)
{
	unsigned int numImageLayers = 0;
	for (pugi::xml_node imageLayerNode = firstImageLayerNode; imageLayerNode; imageLayerNode = imageLayerNode.next_sibling("imagelayer"))
		numImageLayers++;
	if (numImageLayers == 0)
		return false;
	imageLayers.setCapacity(numImageLayers);

	for (pugi::xml_node imageLayerNode = firstImageLayerNode; imageLayerNode; imageLayerNode = imageLayerNode.next_sibling("imagelayer"))
	{
		imageLayers.emplaceBack();
		MapModel::ImageLayer &imageLayer = imageLayers.back();

		pugi::xml_attribute idAttr = imageLayerNode.attribute("id");
		if (idAttr.empty() == false)
			imageLayer.id = idAttr.as_int();

		pugi::xml_attribute nameAttr = imageLayerNode.attribute("name");
		if (nameAttr.empty() == false)
			nctl::strncpy(imageLayer.name, nameAttr.value(), MapModel::MaxNameLength - 1);

		pugi::xml_attribute offsetXAttr = imageLayerNode.attribute("offsetx");
		if (offsetXAttr.empty() == false)
			imageLayer.offsetX = offsetXAttr.as_float();

		pugi::xml_attribute offsetYAttr = imageLayerNode.attribute("offsety");
		if (offsetYAttr.empty() == false)
			imageLayer.offsetY = offsetYAttr.as_float();

		pugi::xml_attribute xAttr = imageLayerNode.attribute("x");
		if (xAttr.empty() == false)
			imageLayer.x = xAttr.as_int();

		pugi::xml_attribute yAttr = imageLayerNode.attribute("y");
		if (yAttr.empty() == false)
			imageLayer.y = yAttr.as_int();

		pugi::xml_attribute opacityAttr = imageLayerNode.attribute("opacity");
		if (opacityAttr.empty() == false)
			imageLayer.opacity = opacityAttr.as_float();

		pugi::xml_attribute visibleAttr = imageLayerNode.attribute("visible");
		if (visibleAttr.empty() == false)
			imageLayer.visible = visibleAttr.as_bool();

		parseColor(imageLayer.tintColor, imageLayerNode.attribute("tintcolor"));

		parseImageNode(imageLayer.image, imageLayerNode.child("image"));
		parseProperties(imageLayer.properties, imageLayerNode.child("properties"));
	}

	return true;
}

bool parseCSVLayerData(const char *string, nctl::Array<unsigned int> &array)
{
	// Count elements
	const char *buffer = string;
	unsigned int numElements = 0;
	while (*buffer != '\0')
	{
		while (*buffer != ',' && *buffer != '\0')
			buffer++;
		if (*buffer == ',')
		{
			numElements++;
			buffer++;
		}
	}

	if (numElements > 0)
		LOGI_X("There are %u elements in the CSV layer data", numElements);
	else
	{
		LOGE_X("There are no elements in the CSV layer data");
		return false;
	}

	array.setCapacity(numElements);

	// Parse elements
	buffer = string;
	while (*buffer != '\0')
	{
		const char *begin = buffer;
		while (*begin == ' ' || *begin == '\t' || *begin == '\n')
			begin++;
		const char *end = begin;
		while (*end != ' ' && *end != '\t' && *end != '\n' && *end != '\0' && *end != ',')
			end++;

		unsigned int value = 0;
		const int matched = sscanf(begin, "%u", &value);
		if (matched != 1)
		{
			LOGE_X("CSV layer data parsing failed at byte %u", begin - string);
			return false;
		}
		array.pushBack(value);

		buffer = end;
		while (*buffer != ',' && *buffer != '\0')
			buffer++;
		if (*buffer == ',')
			buffer++;
	}

	return true;
}

bool parseDataNode(MapModel::Data &data, pugi::xml_node dataNode)
{
	if (dataNode.empty())
		return false;

	pugi::xml_attribute encodingAttr = dataNode.attribute("encoding");
	if (encodingAttr.empty() == false)
	{
		const char *value = encodingAttr.value();
		if (strncmp(value, "base64", strlen("base64")) == 0)
			data.encoding = MapModel::Encoding::Base64;
		else if (strncmp(value, "csv", strlen("csv")) == 0)
			data.encoding = MapModel::Encoding::CSV;
	}

	pugi::xml_attribute compressionAttr = dataNode.attribute("compression");
	if (compressionAttr.empty() == false)
	{
		const char *value = compressionAttr.value();
		if (strncmp(value, "gzip", strlen("gzip")) == 0)
			data.compression = MapModel::Compression::gzip;
		else if (strncmp(value, "zlib", strlen("zlib")) == 0)
			data.compression = MapModel::Compression::zlib;
		else if (strncmp(value, "zstd", strlen("zstd")) == 0)
			data.compression = MapModel::Compression::zstd;
	}

	if (data.encoding != MapModel::Encoding::CSV || data.compression != MapModel::Compression::Uncompressed)
	{
		const unsigned int stringLength = nctl::strnlen(dataNode.child_value(), MapModel::Data::MaxDataLength);
		data.string = nctl::makeUnique<char[]>(stringLength + 1);
		memcpy(data.string.get(), dataNode.child_value(), stringLength);
		data.string[stringLength] = '\0';
	}
	else
	{
		const bool hasParsed = parseCSVLayerData(dataNode.child_value(), data.tileGids);
		return hasParsed;
	}

	return true;
}

bool parseLayerNodes(nctl::Array<MapModel::Layer> &layers, pugi::xml_node firstLayerNode)
{
	unsigned int numLayers = 0;
	for (pugi::xml_node layerNode = firstLayerNode; layerNode; layerNode = layerNode.next_sibling("layer"))
		numLayers++;
	if (numLayers == 0)
		return false;
	layers.setCapacity(numLayers);

	for (pugi::xml_node layerNode = firstLayerNode; layerNode; layerNode = layerNode.next_sibling("layer"))
	{
		layers.emplaceBack();
		MapModel::Layer &layer = layers.back();

		pugi::xml_attribute idAttr = layerNode.attribute("id");
		if (idAttr.empty() == false)
			layer.id = idAttr.as_int();

		pugi::xml_attribute nameAttr = layerNode.attribute("name");
		if (nameAttr.empty() == false)
			nctl::strncpy(layer.name, nameAttr.value(), MapModel::MaxNameLength - 1);

		pugi::xml_attribute xAttr = layerNode.attribute("x");
		if (xAttr.empty() == false)
			layer.x = xAttr.as_int();

		pugi::xml_attribute yAttr = layerNode.attribute("y");
		if (yAttr.empty() == false)
			layer.y = yAttr.as_int();

		pugi::xml_attribute widthAttr = layerNode.attribute("width");
		if (widthAttr.empty() == false)
			layer.width = widthAttr.as_int();

		pugi::xml_attribute heightAttr = layerNode.attribute("height");
		if (heightAttr.empty() == false)
			layer.height = heightAttr.as_int();

		pugi::xml_attribute opacityAttr = layerNode.attribute("opacity");
		if (opacityAttr.empty() == false)
			layer.opacity = opacityAttr.as_float();

		pugi::xml_attribute visibleAttr = layerNode.attribute("visible");
		if (visibleAttr.empty() == false)
			layer.visible = visibleAttr.as_bool();

		parseColor(layer.tintColor, layerNode.attribute("tintcolor"));

		pugi::xml_attribute offsetXAttr = layerNode.attribute("offsetx");
		if (offsetXAttr.empty() == false)
			layer.offsetX = offsetXAttr.as_float();

		pugi::xml_attribute offsetYAttr = layerNode.attribute("offsety");
		if (offsetYAttr.empty() == false)
			layer.offsetY = offsetYAttr.as_float();

		parseDataNode(layer.data, layerNode.child("data"));

		parseProperties(layer.properties, layerNode.child("properties"));
	}

	return true;
}

bool parseTileOffsetNode(MapModel::TileOffset &tileOffset, pugi::xml_node tileOffsetNode)
{
	if (tileOffsetNode.empty())
		return false;

	pugi::xml_attribute xAttr = tileOffsetNode.attribute("x");
	if (xAttr.empty() == false)
		tileOffset.x = xAttr.as_float();

	pugi::xml_attribute yAttr = tileOffsetNode.attribute("y");
	if (yAttr.empty() == false)
		tileOffset.y = yAttr.as_float();

	return true;
}

bool parseGridNode(MapModel::Grid &grid, pugi::xml_node gridNode)
{
	if (gridNode.empty())
		return false;

	pugi::xml_attribute orientationAttr = gridNode.attribute("orientation");
	if (orientationAttr.empty() == false)
	{
		const char *value = orientationAttr.value();
		if (strncmp(value, "orthogonal", strlen("orthogonal")) == 0)
			grid.orientation = MapModel::GridOrientation::Orthogonal;
		else if (strncmp(value, "isometric", strlen("isometric")) == 0)
			grid.orientation = MapModel::GridOrientation::Isometric;
	}

	pugi::xml_attribute widthAttr = gridNode.attribute("width");
	if (widthAttr.empty() == false)
		grid.width = widthAttr.as_int();

	pugi::xml_attribute heightAttr = gridNode.attribute("height");
	if (heightAttr.empty() == false)
		grid.height = heightAttr.as_int();

	return true;
}

bool parseTerrainTypesNode(nctl::Array<MapModel::Terrain> &terrainTypes, pugi::xml_node terrainTypesNode)
{
	if (terrainTypesNode.empty())
		return false;

	pugi::xml_node firstTerrainNode = terrainTypesNode.child("terrain");

	unsigned int numTerrains = 0;
	for (pugi::xml_node terrainNode = firstTerrainNode; terrainNode; terrainNode = terrainNode.next_sibling("terrain"))
		numTerrains++;
	if (numTerrains == 0)
		return false;
	terrainTypes.setCapacity(numTerrains);

	for (pugi::xml_node terrainNode = firstTerrainNode; terrainNode; terrainNode = terrainNode.next_sibling("terrain"))
	{
		terrainTypes.emplaceBack();
		MapModel::Terrain &terrain = terrainTypes.back();

		pugi::xml_attribute nameAttr = terrainNode.attribute("name");
		if (nameAttr.empty() == false)
			nctl::strncpy(terrain.name, nameAttr.value(), MapModel::MaxNameLength - 1);

		pugi::xml_attribute tileAttr = terrainNode.attribute("tile");
		if (tileAttr.empty() == false)
			terrain.tile = tileAttr.as_int();

		parseProperties(terrain.properties, terrainNode.child("properties"));
	}

	return true;
}

bool parseTileTerrain(int terrain[4], const char *string)
{
	const char *buffer = string;

	int terrainIdx = 0;
	while (*buffer != '\0' && terrainIdx < 4)
	{
		while (*buffer == ',' || *buffer == ' ' || *buffer == '\t' || *buffer == '\n')
			buffer++;

		int value = 0;
		const int matched = sscanf(buffer, "%d", &value);
		if (matched == 1)
			terrain[terrainIdx] = value;
		terrainIdx++;

		while (*buffer != ',' && *buffer != '\0')
			buffer++;
	}

	return true;
}

bool parseFrameNodes(nctl::Array<MapModel::Frame> &frames, pugi::xml_node firstFrameNode)
{
	unsigned int numFrames = 0;
	for (pugi::xml_node frameNode = firstFrameNode; frameNode; frameNode = frameNode.next_sibling("frame"))
		numFrames++;
	if (numFrames == 0)
		return false;
	frames.setCapacity(numFrames);

	for (pugi::xml_node frameNode = firstFrameNode; frameNode; frameNode = frameNode.next_sibling("frame"))
	{
		frames.emplaceBack();
		MapModel::Frame &frame = frames.back();

		pugi::xml_attribute tileIdAttr = frameNode.attribute("tileid");
		if (tileIdAttr.empty() == false)
			frame.tileId = tileIdAttr.as_int();

		pugi::xml_attribute durationAttr = frameNode.attribute("duration");
		if (durationAttr.empty() == false)
			frame.duration = durationAttr.as_int();
	}

	return true;
}

bool parseTileNodes(nctl::Array<MapModel::Tile> &tiles, pugi::xml_node firstTileNode)
{
	unsigned int numTiles = 0;
	for (pugi::xml_node tileNode = firstTileNode; tileNode; tileNode = tileNode.next_sibling("tile"))
		numTiles++;
	if (numTiles == 0)
		return false;
	tiles.setCapacity(numTiles);

	for (pugi::xml_node tileNode = firstTileNode; tileNode; tileNode = tileNode.next_sibling("tile"))
	{
		tiles.emplaceBack();
		MapModel::Tile &tile = tiles.back();

		pugi::xml_attribute idAttr = tileNode.attribute("id");
		if (idAttr.empty() == false)
			tile.id = idAttr.as_int();

		pugi::xml_attribute typeAttr = tileNode.attribute("type");
		if (typeAttr.empty() == false)
			tile.type = typeAttr.as_int();

		pugi::xml_attribute terrainAttr = tileNode.attribute("terrain");
		if (terrainAttr.empty() == false)
			parseTileTerrain(tile.terrain, terrainAttr.value());

		pugi::xml_attribute probabilityAttr = tileNode.attribute("probability");
		if (probabilityAttr.empty() == false)
			tile.probability = probabilityAttr.as_float();

		pugi::xml_node animationNode = tileNode.child("animation");
		if (animationNode.empty() == false)
			parseFrameNodes(tile.frames, animationNode.child("frame"));

		parseProperties(tile.properties, tileNode.child("properties"));

		// Not parsing <image>, <objectgroup>
	}

	return true;
}

bool parseTileSetNodes(nctl::Array<MapModel::TileSet> &tileSets, pugi::xml_node firstTileSetNode, const nctl::String &tmxDirName, nctl::String &tsxDirName)
{
	unsigned int numTileSets = 0;
	for (pugi::xml_node tileSetNode = firstTileSetNode; tileSetNode; tileSetNode = tileSetNode.next_sibling("tileset"))
		numTileSets++;
	if (numTileSets == 0)
		return false;
	tileSets.setCapacity(numTileSets);

	for (pugi::xml_node tileSetNode = firstTileSetNode; tileSetNode; tileSetNode = tileSetNode.next_sibling("tileset"))
	{
		tileSets.emplaceBack();
		MapModel::TileSet &tileSet = tileSets.back();

		pugi::xml_attribute firstGidAttr = tileSetNode.attribute("firstgid");
		if (firstGidAttr.empty() == false)
			tileSet.firstGid = firstGidAttr.as_uint();

		bool hasSource = false;
		pugi::xml_attribute sourceAttr = tileSetNode.attribute("source");
		if (sourceAttr.empty() == false)
		{
			nctl::strncpy(tileSet.source, sourceAttr.value(), MapModel::MaxSourceLength - 1);
			if (tileSet.source[0] != '\0')
				hasSource = true;
		}

		pugi::xml_node tileSetExtNode = tileSetNode;
		if (hasSource)
		{
			const nctl::String tsxFilePath = nc::fs::joinPath(tmxDirName, nctl::String(tileSet.source));

			pugi::xml_document tsxFile;
			const bool result = loadXmlFile(tsxFilePath.data(), tsxFile);
			if (result == false)
				return false;

			tsxDirName = nc::fs::dirName(tsxFilePath.data());
			tileSetExtNode = tsxFile.child("tileset");
		}

		pugi::xml_attribute nameAttr = tileSetExtNode.attribute("name");
		if (nameAttr.empty() == false)
			nctl::strncpy(tileSet.name, nameAttr.value(), MapModel::MaxNameLength - 1);

		pugi::xml_attribute tileWidthAttr = tileSetExtNode.attribute("tilewidth");
		if (tileWidthAttr.empty() == false)
			tileSet.tileWidth = tileWidthAttr.as_int();

		pugi::xml_attribute tileHeightAttr = tileSetExtNode.attribute("tileheight");
		if (tileHeightAttr.empty() == false)
			tileSet.tileHeight = tileHeightAttr.as_int();

		pugi::xml_attribute spacingAttr = tileSetExtNode.attribute("spacing");
		if (spacingAttr.empty() == false)
			tileSet.spacing = spacingAttr.as_int();

		pugi::xml_attribute marginAttr = tileSetExtNode.attribute("margin");
		if (marginAttr.empty() == false)
			tileSet.margin = marginAttr.as_int();

		pugi::xml_attribute tileCountAttr = tileSetExtNode.attribute("tilecount");
		if (tileCountAttr.empty() == false)
			tileSet.tileCount = tileCountAttr.as_int();

		pugi::xml_attribute columnsAttr = tileSetExtNode.attribute("columns");
		if (columnsAttr.empty() == false)
			tileSet.columns = columnsAttr.as_int();

		pugi::xml_attribute objectAlignmentAttr = tileSetExtNode.attribute("objectalignment");
		if (objectAlignmentAttr.empty() == false)
		{
			const char *value = objectAlignmentAttr.value();
			if (strncmp(value, "unspecified", strlen("unspecified")) == 0)
				tileSet.objectAlignment = MapModel::ObjectAlignment::Unspecified;
			else if (strncmp(value, "topleft", strlen("topleft")) == 0)
				tileSet.objectAlignment = MapModel::ObjectAlignment::TopLeft;
			else if (strncmp(value, "top", strlen("top")) == 0)
				tileSet.objectAlignment = MapModel::ObjectAlignment::Top;
			else if (strncmp(value, "topright", strlen("topright")) == 0)
				tileSet.objectAlignment = MapModel::ObjectAlignment::TopRight;
			else if (strncmp(value, "left", strlen("left")) == 0)
				tileSet.objectAlignment = MapModel::ObjectAlignment::Left;
			else if (strncmp(value, "center", strlen("center")) == 0)
				tileSet.objectAlignment = MapModel::ObjectAlignment::Center;
			else if (strncmp(value, "right", strlen("right")) == 0)
				tileSet.objectAlignment = MapModel::ObjectAlignment::Right;
			else if (strncmp(value, "bottomleft", strlen("bottomleft")) == 0)
				tileSet.objectAlignment = MapModel::ObjectAlignment::BottomLeft;
			else if (strncmp(value, "bottom", strlen("bottom")) == 0)
				tileSet.objectAlignment = MapModel::ObjectAlignment::Bottom;
			else if (strncmp(value, "bottomright", strlen("bottomright")) == 0)
				tileSet.objectAlignment = MapModel::ObjectAlignment::BottomRight;
		}

		parseImageNode(tileSet.image, tileSetExtNode.child("image"));
		parseTileOffsetNode(tileSet.tileOffset, tileSetExtNode.child("tileoffset"));
		parseGridNode(tileSet.grid, tileSetExtNode.child("grid"));
		parseTerrainTypesNode(tileSet.terrainTypes, tileSetExtNode.child("terrainTypes"));
		parseTileNodes(tileSet.tiles, tileSetExtNode.child("tile"));
		parseProperties(tileSet.properties, tileSetExtNode.child("properties"));

		// Not parsing <wangsets>
	}

	return true;
}

bool parseMapNode(MapModel &mapModel, pugi::xml_node mapNode)
{
	if (mapNode.empty())
		return false;

	MapModel::Map &map = mapModel.map();

	pugi::xml_attribute versionAttr = mapNode.attribute("version");
	if (versionAttr.empty() == false)
		nctl::strncpy(map.version, versionAttr.value(), MapModel::Map::MaxVersionLength - 1);

	pugi::xml_attribute tiledversionAttr = mapNode.attribute("tiledversion");
	if (tiledversionAttr.empty() == false)
		nctl::strncpy(map.tiledVersion, tiledversionAttr.value(), MapModel::Map::MaxVersionLength - 1);

	pugi::xml_attribute orientationAttr = mapNode.attribute("orientation");
	if (orientationAttr.empty() == false)
	{
		const char *value = orientationAttr.value();
		if (strncmp(value, "orthogonal", strlen("orthogonal")) == 0)
			map.orientation = MapModel::Orientation::Orthogonal;
		else if (strncmp(value, "isometric", strlen("isometric")) == 0)
			map.orientation = MapModel::Orientation::Isometric;
		else if (strncmp(value, "staggered", strlen("staggered")) == 0)
			map.orientation = MapModel::Orientation::Staggered;
		else if (strncmp(value, "hexagonal", strlen("hexagonal")) == 0)
			map.orientation = MapModel::Orientation::Hexagonal;
	}

	pugi::xml_attribute renderOrderAttr = mapNode.attribute("renderorder");
	if (renderOrderAttr.empty() == false)
	{
		const char *value = renderOrderAttr.value();
		if (strncmp(value, "right-down", strlen("right-down")) == 0)
			map.renderOrder = MapModel::RenderOrder::Right_Down;
		else if (strncmp(value, "right-up", strlen("right-up")) == 0)
			map.renderOrder = MapModel::RenderOrder::Right_Up;
		else if (strncmp(value, "left-down", strlen("left-down")) == 0)
			map.renderOrder = MapModel::RenderOrder::Left_Down;
		else if (strncmp(value, "left-up", strlen("left-up")) == 0)
			map.renderOrder = MapModel::RenderOrder::Left_Up;
	}

	pugi::xml_attribute compressionLevelAttr = mapNode.attribute("compressionlevel");
	if (compressionLevelAttr.empty() == false)
		map.compressionLevel = compressionLevelAttr.as_int();

	pugi::xml_attribute widthAttr = mapNode.attribute("width");
	if (widthAttr.empty() == false)
		map.width = widthAttr.as_int();

	pugi::xml_attribute heightAttr = mapNode.attribute("height");
	if (heightAttr.empty() == false)
		map.height = heightAttr.as_int();

	pugi::xml_attribute tileWidthAttr = mapNode.attribute("tilewidth");
	if (tileWidthAttr.empty() == false)
		map.tileWidth = tileWidthAttr.as_int();

	pugi::xml_attribute tileHeightAttr = mapNode.attribute("tileheight");
	if (tileHeightAttr.empty() == false)
		map.tileHeight = tileHeightAttr.as_int();

	pugi::xml_attribute hexSideLengthAttr = mapNode.attribute("hexsidelength");
	if (hexSideLengthAttr.empty() == false)
		map.hexSideLength = hexSideLengthAttr.as_int();

	pugi::xml_attribute staggerAxisAttr = mapNode.attribute("staggeraxis");
	if (staggerAxisAttr.empty() == false)
	{
		const char *value = staggerAxisAttr.value();
		if (strncmp(value, "x", strlen("x")) == 0)
			map.staggerAxis = MapModel::StaggerAxis::X;
		else if (strncmp(value, "y", strlen("y")) == 0)
			map.staggerAxis = MapModel::StaggerAxis::Y;
	}

	pugi::xml_attribute staggerIndexAttr = mapNode.attribute("staggerindex");
	if (staggerIndexAttr.empty() == false)
	{
		const char *value = staggerIndexAttr.value();
		if (strncmp(value, "even", strlen("even")) == 0)
			map.staggerIndex = MapModel::StaggerIndex::Even;
		else if (strncmp(value, "odd", strlen("odd")) == 0)
			map.staggerIndex = MapModel::StaggerIndex::Odd;
	}

	parseColor(map.backgroundColor, mapNode.attribute("backgroundcolor"));

	pugi::xml_attribute nextLayerIdAttr = mapNode.attribute("nextlayerid");
	if (nextLayerIdAttr.empty() == false)
		map.nextLayerId = nextLayerIdAttr.as_int();

	pugi::xml_attribute nextObjectIdAttr = mapNode.attribute("nextobjectid");
	if (nextObjectIdAttr.empty() == false)
		map.nextObjectId = nextObjectIdAttr.as_int();

	pugi::xml_attribute infiniteAttr = mapNode.attribute("infinite");
	if (infiniteAttr.empty() == false)
		map.infinite = infiniteAttr.as_bool();

	parseTileSetNodes(map.tileSets, mapNode.child("tileset"), mapModel.tmxDirName(), mapModel.tsxDirName());
	parseLayerNodes(map.layers, mapNode.child("layer"));
	parseObjectGroupNodes(map.objectGroups, mapNode.child("objectgroup"));
	parseImageLayerNodes(map.imageLayers, mapNode.child("imagelayer"));
	parseProperties(map.properties, mapNode.child("properties"));

	// Not parsing <group>, <editorsettings>

	return true;
}

}

bool TmxParser::loadFromMemory(MapModel &mapModel, unsigned char *bufferPtr, unsigned long int bufferSize)
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_buffer_inplace(bufferPtr, bufferSize);

	if (result == false)
	{
		LOGE_X("Error description: %s", result.description());
		LOGE_X("Error offset: %s (error at [...%ul])", result.offset, bufferPtr + result.offset);
		return false;
	}

	return parseMapNode(mapModel, doc.child("map"));
}

bool TmxParser::loadFromFile(MapModel &mapModel, const char *filename)
{
	long int xmlFileSize = 0;
	nctl::UniquePtr<unsigned char[]> xmlFileBuffer;
	loadFile(filename, xmlFileBuffer, xmlFileSize);

	mapModel.tmxDirName() = nc::fs::dirName(filename);

	return loadFromMemory(mapModel, xmlFileBuffer.get(), xmlFileSize);
}
