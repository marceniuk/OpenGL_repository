#ifndef WIN32
	typedef char byte;
#endif

#define	LUMP_ENTITIES		0
#define	LUMP_SHADERS		1
#define	LUMP_PLANES			2
#define	LUMP_NODES			3
#define	LUMP_LEAFS			4
#define	LUMP_LEAFSURFACES	5
#define	LUMP_LEAFBRUSHES	6
#define	LUMP_MODELS			7
#define	LUMP_BRUSHES		8
#define	LUMP_BRUSHSIDES		9
#define	LUMP_DRAWVERTS		10
#define	LUMP_DRAWINDEXES	11
#define	LUMP_FOGS			12
#define	LUMP_SURFACES		13
#define	LUMP_LIGHTMAPS		14
#define	LUMP_LIGHTGRID		15
#define	LUMP_VISIBILITY		16
#define	HEADER_LUMPS		17

// 'Unknown' surface flags pulled from q3map source
#define	SURF_NODAMAGE			0x1		// never give falling damage
#define	SURF_SLICK				0x2		// effects game physics
#define	SURF_SKY				0x4		// lighting from environment map
#define	SURF_LADDER				0x8
#define	SURF_NOIMPACT			0x10	// don't make missile explosions
#define	SURF_NOMARKS			0x20	// don't leave missile marks
#define	SURF_FLESH				0x40	// make flesh sounds and effects
#define	SURF_NODRAW				0x80	// don't generate a drawsurface at all
#define	SURF_HINT				0x100	// make a primary bsp splitter
#define	SURF_SKIP				0x200	// completely ignore, allowing non-closed brushes
#define	SURF_NOLIGHTMAP			0x400	// surface doesn't need a lightmap
#define	SURF_POINTLIGHT			0x800	// generate lighting info at vertexes
#define	SURF_METALSTEPS			0x1000	// clanking footsteps
#define	SURF_NOSTEPS			0x2000	// no footstep sounds
#define	SURF_NONSOLID			0x4000	// don't collide against curves with this set
#define	SURF_LIGHTFILTER		0x8000	// act as a light filter during q3map -light
#define	SURF_ALPHASHADOW		0x10000	// do per-pixel light shadow casting in q3map
#define	SURF_NODLIGHT			0x20000	// don't dlight even if solid (solid lava, skies)
#define SURF_DUST				0x40000 // leave a dust trail when walking on this surface

// 'Unknown' content flags pulled from q3map source
#define	CONTENTS_SOLID			1		// an eye is never valid in a solid
#define	CONTENTS_LAVA			8
#define	CONTENTS_SLIME			16
#define	CONTENTS_WATER			32
#define	CONTENTS_FOG			64

typedef enum
{
	Entities = 0,	// Stores player/object positions, etc...
	Materials,		// Stores texture information
	Planes,			// Stores the splitting planes
	Nodes,			// Stores the BSP nodes
	Leafs,			// Stores the leafs of the nodes
	LeafFaces,		// Stores the leaf's indices into the faces
	LeafBrushes,	// Stores the leaf's indices into the brushes
	Models,			// Stores the info of world models
	Brushes,		// Stores the brushes info (brushes are a set of planes defining solid volume)
	BrushSides,		// Stores the brush surfaces info
	VertexArray,	// Stores the level vertices
	IndexArray,		// Stores the model vertices offsets - this is just wrong, these are the indicies for the face verticies. Try rendering q3map2 bsps.
	Fog,			// Stores the shader files (blending, anims..)
	Faces,			// Stores the faces for the level
	LightMaps,		// Stores the lightmaps for the level
	LightGrids,		// Stores extra world lighting information
	VisData,		// Stores PVS cluster info (visibility)
	MaxLumps		// A constant to store the number of lumps
} lumps;

typedef struct
{
	vec3	position;	// (x, y, z) position. 
	vec2	texCoord0;	// (u, v) texture coordinate
	vec2	texCoord1;	// (u, v) lightmap coordinate
	vec3	normal;		// (x, y, z) normal vector
	int		color;		// RGBA color for the vertex 
} bspvertex_t;

typedef struct
{
	char		material[64];
	int			brush_num;
	int			visible_side;	// the brush side that ray tests need to clip against (-1 == none)
} fog_t;

typedef struct
{
	int			material;
	int			fog_num;
	int			type;

	int			vertex;
	int			num_verts;

	int			index;
	int			num_index;

	int			lightmap;
	int			lightmapX, lightmapY;
	int			lightmapWidth, lightmapHeight;

	vec3		lightmapOrigin;
	vec3		lightmapVecs[3];	// for patches, [0] and [1] are lodbounds

	int			patchWidth;
	int			patchHeight;
} face_t;

typedef struct
{
	char name[64];		// The name of the texture w/o the extension 
	int	surface;		// The surface flags (unknown) 
	int	contents;		// The content flags (unknown)
} material_t;

typedef struct
{
	byte image[128][128][3];	// The RGB data in a 128x128 image
} lightmap_t;

typedef struct
{
	int	plane;		// The index into the planes array 
	int	front;		// The child index for the front node 
	int	back;		// The child index for the back node 
	int	min[3];	// The bounding box min position. 
	int	max[3];	// The bounding box max position. 
} node_t;

typedef struct
{
	int	cluster;		// The visibility cluster 
	int	area;			// The area portal 
	int	min[3];		// The bounding box min position 
	int	max[3];		// The bounding box max position 
	int	leaf_face;		// The first index into the leafface array
	int	num_faces;		// The number of faces for this leaf 
	int	leaf_brush;		// The first index for into the leafbrush array
	int	num_brushes;	// The number of brushes for this leaf 
} leaf_t;

typedef struct
{
	vec3	normal;		// Plane normal. 
	float	d;			// The plane distance from origin 
} plane_t;

typedef struct
{
	int	num_vectors;	// This stores the number of bit-vectors
	int	vector_size;		// The size of bit-vectors in bytes
	byte	pVecs;		// This holds all of the cluster bits
} visData_t;

typedef struct 
{
	int	first_side;		// The starting brush side for the brush 
	int	num_sides;		// Number of brush sides for the brush
	int	material;		// the shader that determines the contents flags
} brush_t;

typedef struct  
{
	int	plane;			// The plane index
	int	material;		// The texture index
} brushSide_t;

typedef struct  
{
	float	min[3];		// The min position for the bounding box
	float	max[3];		// The max position for the bounding box. 
	int	faceIndex;	// The first face index in the model 
	int	numOfFaces;	// The number of faces in the model 
	int	brushIndex;	// The first brush index in the model 
	int	numOfBrushes;	// The number brushes for the model
} model_t;

typedef struct
{
	byte	ambient[3];	// This is the ambient color in RGB
	byte	directional[3];	// This is the directional color in RGB
	byte	direction[2];	// The direction of the light: [phi,theta] 
} light_t;

typedef struct
{
	int	offset;		// Offset to start of lump, relative to beginning of file. 
	int	length;		// Length of lump. Always a multiple of 4. 
} lump_t;

typedef struct
{
	byte	strId[4];		// IBSP
	int	version;			// 0x2e
	lump_t	directory[17];
} bsp_t;

typedef struct
{
	byte		*Ent;
	material_t	*Material;
	plane_t		*Plane;
	node_t		*Node;
	leaf_t		*Leaf;
	int			*LeafFace;
	int			*LeafBrush;
	bspvertex_t	*Vert;
	face_t		*Face;
	visData_t	*VisData;
	int			*IndexArray;
	fog_t		*Fog;
	brush_t		*Brushes;
	brushSide_t *BrushSides;
	lightmap_t	*LightMaps;

	int	num_ents;
	int	num_materials;
	int	num_planes;
	int	num_nodes;
	int	num_leafs;
	int	num_LeafFaces;
	int	num_LeafBrushes;
	int	num_brushes;
	int	num_BrushSides;
	int	num_verts;
	int	num_faces;
	int	num_vis;
	int	num_index;
	int num_lightmaps;
	int num_fog;
} bspData_t;
