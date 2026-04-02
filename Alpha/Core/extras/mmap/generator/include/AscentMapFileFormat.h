#ifndef ASCENT_MMAP_MAP_FILE_FORMAT_H
#define ASCENT_MMAP_MAP_FILE_FORMAT_H

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
