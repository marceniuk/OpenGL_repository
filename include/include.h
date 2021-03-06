#ifndef INCLUDE_H
#define INCLUDE_H

#define LINE_SIZE 512

#ifdef _WIN32
#define _CRTDBG_MAP_ALLOC
	#define _USE_MATH_DEFINES
	#define WIN32_EXTRA_LEAN
	#define _CRT_SECURE_NO_DEPRECATE
	#define NOMINMAX
	#include <windows.h>
	#include <winsock.h>
#include <omp.h>

	#pragma comment(lib, "wsock32.lib")

	#define snprintf sprintf_s

	typedef int socklen_t;
	int inet_pton(int af, const char *src, void *dst);

#ifdef DIRECTX
	#include <d3d9.h>
	#include <d3dx9.h>
	#include <d3dx9math.h>
#else
	#define GLEW_STATIC
	#include <GL/glew.h>
	#include <GL/wglew.h>
#endif

#endif

#ifdef LINUX
	#include <GL/gl.h>
	#include <GL/glu.h>
	#include <GL/glx.h>
	#include <X11/Xlib.h>
	#include <X11/Xatom.h>
	#include <X11/keysym.h>
	#include <unistd.h>
	#include <sys/select.h>
	#include <sys/types.h>
	#include <sys/time.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>

	#define closesocket close

	typedef	int SOCKET;
	#define SOCKET_ERROR	-1
	#define INVALID_SOCKET	-1
#endif

#ifdef MAC
	#include <Carbon/Carbon.h>
	#include <AGL/agl.h>
#endif

#define MAX(x,y) (x) > (y) ? (x) : (y)
#define MIN(x,y) (x) < (y) ? (x) : (y)

//audio
#include <al.h>
#include <alc.h>
#include <efx.h>
#include <EFX-Util.h>
#include <efx-creative.h>

//std
#include <cstdio>
#include <cstdlib>
#include <crtdbg.h>
#include <cstring>
#include <vector>

using namespace std;

#include "net.h"
#include "vector.h"
#include "matrix.h"
#include "quaternion.h"
#include "plane.h"
#include "types.h"
#include "md5_types.h"

#include "md5.h"
#include "graphics.h"
#include "bspTypes.h"
#include "sound.h"
#include "frame.h"

#include "edge.h"
#include "bsp.h"
#include "trigger.h"
#include "light.h"
#include "speaker.h"
#include "model.h"
#include "rigidbody.h"
#include "vehicle.h"
#include "player.h"

#include "entity.h"
#include "shader.h"
#include "parse.h"
#include "menu.h"
#include "engine.h"

#define DEBUG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)


byte *gltLoadTGA(const char *file, int *iWidth, int *iHeight, int *iComponents, int *eFormat);
byte *tga_24to32(int width, int height, byte *pBits);
int load_texture(Graphics &gfx, char *file_name);
float abs32(float val);
int abs32(int val);

char *get_file(char *filename);
double fsin(double rad);
double fcos(double rad);

#define MY_PI 3.14159265359f
#define MY_HALF_PI 1.5707963268f
#define MAXLINE 4096

//quake3 game units, 8 units = 1 foot, ~3.3ft per meter (each unit is 8 values large)
#define UNITS_TO_METERS (8.0f * 8.0f * 3.3f)



#endif
