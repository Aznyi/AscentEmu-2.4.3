#ifndef ASCENT_MMAP_MAP_FILE_FORMAT_H
#define ASCENT_MMAP_MAP_FILE_FORMAT_H

static const uint32 ASCENT_TERRAIN_HEADER_SIZE = 1048576;
static const uint32 ASCENT_TERRAIN_GRID_SIZE = 512;
static const uint32 ASCENT_TERRAIN_CELLS_PER_TILE = 8;
static const uint32 ASCENT_TERRAIN_SAMPLES_PER_CELL = 32;
static const uint32 ASCENT_TERRAIN_SAMPLES_PER_TILE = ASCENT_TERRAIN_CELLS_PER_TILE * ASCENT_TERRAIN_SAMPLES_PER_CELL;

struct AscentTerrainCell
{
	uint16 AreaID[2][2];
	uint8 LiquidType[2][2];
	float LiquidLevel[2][2];
	float Z[ASCENT_TERRAIN_SAMPLES_PER_CELL][ASCENT_TERRAIN_SAMPLES_PER_CELL];
};

struct GridMapFileHeader
{
	uint32 mapMagic;
	uint32 versionMagic;
	uint32 areaMapOffset;
	uint32 areaMapSize;
	uint32 heightMapOffset;
	uint32 heightMapSize;
	uint32 liquidMapOffset;
	uint32 liquidMapSize;
	uint32 holesOffset;
	uint32 holesSize;
};

#define MAP_AREA_NO_AREA      0x0001

struct GridMapAreaHeader
{
	uint32 fourcc;
	uint16 flags;
	uint16 gridArea;
};

#define MAP_HEIGHT_NO_HEIGHT  0x0001
#define MAP_HEIGHT_AS_INT16   0x0002
#define MAP_HEIGHT_AS_INT8    0x0004

struct GridMapHeightHeader
{
	uint32 fourcc;
	uint32 flags;
	float gridHeight;
	float gridMaxHeight;
};

#define MAP_LIQUID_NO_TYPE    0x01
#define MAP_LIQUID_NO_HEIGHT  0x02

struct GridMapLiquidHeader
{
	uint32 fourcc;
	uint8 flags;
	uint8 liquidFlags;
	uint16 liquidType;
	uint8 offsetX;
	uint8 offsetY;
	uint8 width;
	uint8 height;
	float liquidLevel;
};

#define MAP_LIQUID_TYPE_NO_WATER    0x00
#define MAP_LIQUID_TYPE_MAGMA       0x01
#define MAP_LIQUID_TYPE_OCEAN       0x02
#define MAP_LIQUID_TYPE_SLIME       0x04
#define MAP_LIQUID_TYPE_WATER       0x08
#define MAP_ALL_LIQUIDS   (MAP_LIQUID_TYPE_WATER | MAP_LIQUID_TYPE_MAGMA | MAP_LIQUID_TYPE_OCEAN | MAP_LIQUID_TYPE_SLIME)
#define MAP_LIQUID_TYPE_DEEP_WATER  0x10
#define MAP_LIQUID_TYPE_WMO_WATER   0x20

#endif
