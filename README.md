[![Linux](https://github.com/nCine/ncTiledViewer/workflows/Linux/badge.svg)](https://github.com/nCine/ncTiledViewer/actions?workflow=Linux)
[![macOS](https://github.com/nCine/ncTiledViewer/workflows/macOS/badge.svg)](https://github.com/nCine/ncTiledViewer/actions?workflow=macOS)
[![Windows](https://github.com/nCine/ncTiledViewer/workflows/Windows/badge.svg)](https://github.com/nCine/ncTiledViewer/actions?workflow=Windows)
[![MinGW](https://github.com/nCine/ncTiledViewer/workflows/MinGW/badge.svg)](https://github.com/nCine/ncTiledViewer/actions?workflow=MinGW)
[![Emscripten](https://github.com/nCine/ncTiledViewer/workflows/Emscripten/badge.svg)](https://github.com/nCine/ncTiledViewer/actions?workflow=Emscripten)
[![Android](https://github.com/nCine/ncTiledViewer/workflows/Android/badge.svg)](https://github.com/nCine/ncTiledViewer/actions?workflow=Android)
[![CodeQL](https://github.com/nCine/ncTiledViewer/workflows/CodeQL/badge.svg)](https://github.com/nCine/ncTiledViewer/actions?workflow=CodeQL)

# ncTiledViewer
A viewer for [Tiled](https://www.mapeditor.org/) maps made with the nCine.

The parser is pretty complete, lacking only elements related to layer groups, Wang sets, templates, editor settings, and chunks.

Layer data should be uncompressed and in CSV format for the loader to work. Embedded images are not supported but external TSX files are.

The viewer can only show orthogonal maps but it can load multiple tilesets, layers and animation frames.

At the moment image layers are not supported by the viewer.
