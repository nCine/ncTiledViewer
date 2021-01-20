#ifndef TMXPARSER_H
#define TMXPARSER_H

namespace nc = ncine;

class MapModel;

/// The class that parses an XML Tiled map
class TmxParser
{
  public:
	static bool loadFromMemory(MapModel &mapModel, unsigned char *bufferPtr, unsigned long int bufferSize);
	static bool loadFromFile(MapModel &mapModel, const char *filename);
};

#endif
