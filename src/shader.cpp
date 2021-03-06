#include "shader.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

void Font::init(Graphics *gfx)
{
#ifdef DIRECTX
	Shader::init(gfx, "media/hlsl/font.vsh", NULL, NULL);
#else
	if (Shader::init(gfx, "media/glsl/font.vs", NULL, "media/glsl/font.fs"))
	{
		program_handle = -1;
		return;
	}
	u_scale = glGetUniformLocation(program_handle, "u_scale");
	u_row = glGetUniformLocation(program_handle, "u_row");
	u_col = glGetUniformLocation(program_handle, "u_col");
	u_xpos = glGetUniformLocation(program_handle, "u_xpos");
	u_ypos = glGetUniformLocation(program_handle, "u_ypos");
	u_color = glGetUniformLocation(program_handle, "u_color");
	texture0 = glGetUniformLocation(program_handle, "texture0");
#endif
}

void Font::prelink()
{
#ifndef DIRECTX
	glBindAttribLocation(program_handle, 0, "attr_position");
	glBindAttribLocation(program_handle, 1, "attr_TexCoord");
	glBindAttribLocation(program_handle, 4, "attr_color");
#endif
}

void Font::Params(char c, float x, float y, float scale, vec3 &color)
{
	int col = c % 16;
	int row = 15 - c / 16;
	float offset = 16.0f / 256.0f;

#ifdef DIRECTX
	uniform->SetFloat(gfx->device, "u_scale", scale);
	uniform->SetFloat(gfx->device, "u_col", col * offset);
	uniform->SetFloat(gfx->device, "u_row", row * offset);
	uniform->SetFloat(gfx->device, "u_xpos", (2.0f * x));
	uniform->SetFloat(gfx->device, "u_ypos", (2.0f * y));
	uniform->SetFloatArray(gfx->device, "u_color", (float *)&color, 3);
#else
	glUniform1f(u_scale, scale);
	glUniform1f(u_col, col * offset);
	glUniform1f(u_row, row * offset);
	glUniform1f(u_xpos, (2.0f * x - 1.0f));
	glUniform1f(u_ypos, (2.0f * (1.0f - y) - 1.0f));
	glUniform3fv(u_color, 3, (float *)&color);
	glUniform1i(texture0, 0);
#endif
}

void Global::init(Graphics *gfx)
{
#ifdef DIRECTX
	Shader::init(gfx, "media/hlsl/basic.vsh", NULL, "media/hlsl/basic.psh");
#else
	if (Shader::init(gfx, "media/glsl/global.vs", NULL, "media/glsl/global.fs"))
	{
		program_handle = -1;
		return;
	}
	matrix = glGetUniformLocation(program_handle, "mvp");
	texture0 = glGetUniformLocation(program_handle, "texture0");
#endif
}

void Global::prelink()
{
#ifndef DIRECTX
	glBindAttribLocation(program_handle, 0, "attr_position");
	glBindAttribLocation(program_handle, 1, "attr_TexCoord");
	glBindAttribLocation(program_handle, 4, "attr_color");
#endif
}

void Global::Params(matrix4 &mvp, int tex0)
{
#ifdef DIRECTX
	uniform->SetMatrix(gfx->device, "mvp", (D3DXMATRIX *)mvp.m);
#else
	glUniformMatrix4fv(matrix, 1, GL_FALSE, mvp.m);
	glUniform1i(texture0, tex0);
#endif
}

void mLight2::init(Graphics *gfx)
{
	//"media/glsl/mlighting3.gs"
#ifdef DIRECTX
	Shader::init(gfx, "media/glsl/mlighting3.vsh", NULL, "media/glsl/mlighting3.psh");
#else
	if (Shader::init(gfx, "media/glsl/mlighting3.vs", NULL, "media/glsl/mlighting3.fs"))
	{
		program_handle = -1;
		return;
	}

	matrix = glGetUniformLocation(program_handle, "mvp");
	texture0 = glGetUniformLocation(program_handle, "texture0");
	texture1 = glGetUniformLocation(program_handle, "texture1");
	texture2 = glGetUniformLocation(program_handle, "texture2");
	u_num_lights = glGetUniformLocation(program_handle, "u_num_lights");
	u_position = glGetUniformLocation(program_handle, "u_position");
	u_color = glGetUniformLocation(program_handle, "u_color");
#endif
}

void mLight2::prelink()
{
#ifndef DIRECTX
	glBindAttribLocation(program_handle, 0, "attr_position");
	glBindAttribLocation(program_handle, 1, "attr_TexCoord");
	glBindAttribLocation(program_handle, 2, "attr_LightCoord");
	glBindAttribLocation(program_handle, 3, "attr_normal");
	glBindAttribLocation(program_handle, 4, "attr_color");
	glBindAttribLocation(program_handle, 5, "attr_tangent");
	glProgramParameteriEXT(program_handle, GL_GEOMETRY_INPUT_TYPE_EXT, GL_TRIANGLES);
	glProgramParameteriEXT(program_handle, GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLE_STRIP);
	glProgramParameteriEXT(program_handle, GL_GEOMETRY_VERTICES_OUT_EXT, 1024);
//	int n;
//	glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT, &n);
#endif
}


void mLight2::Params(matrix4 &mvp, int tex0, int tex1, int tex2, vector<Light *> &light_list, size_t num_lights)
{
	vec3 position[MAX_LIGHTS];
	vec4 color[MAX_LIGHTS];

	if (num_lights > MAX_LIGHTS)
		num_lights = MAX_LIGHTS;

	for(int i = 0; i < num_lights; i++)
	{
		position[i] = light_list[i]->entity->position;
		color[i] = vec4(light_list[i]->color.x, light_list[i]->color.y,
			light_list[i]->color.z, (float)light_list[i]->intensity);
	}
#ifdef DIRECTX
	uniform->SetMatrix(gfx->device, "mvp", (D3DXMATRIX *)mvp.m);
	uniform->SetFloatArray(gfx->device, "u_position", (float *)position, num_lights);
	uniform->SetFloatArray(gfx->device, "u_color", (float *)color, num_lights);
	uniform->SetInt(gfx->device, "u_num_lights", num_lights);
#else
	glUniformMatrix4fv(matrix, 1, GL_FALSE, mvp.m);
	glUniform1i(texture0, tex0);
	glUniform1i(texture1, tex1);
	glUniform1i(texture2, tex2);
	glUniform1i(u_num_lights, (int)num_lights);
	glUniform3fv(u_position, MAX_LIGHTS, (float *)&position);
	glUniform4fv(u_color, MAX_LIGHTS, (float *)&color);
#endif
}

void Post::init(Graphics *gfx)
{
#ifndef DIRECTX
	if (Shader::init(gfx, "media/glsl/post.vs", NULL, "media/glsl/post.fs"))
	{
		program_handle = -1;
		return;
	}
#else
	Shader::init(gfx, "media/hlsl/post.vsh", NULL, "media/hlsl/post.psh");
#endif

#ifndef DIRECTX
	texture0 = glGetUniformLocation(program_handle, "texture0");
	texture1 = glGetUniformLocation(program_handle, "texture1");
	tc_offset = glGetUniformLocation(program_handle, "tc_offset");

	glGenTextures(1, &image);
	glBindTexture(GL_TEXTURE_2D, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);   
	glGenTextures(1, &swap);
	glBindTexture(GL_TEXTURE_2D, swap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
#endif
}


void Post::prelink(void)
{
#ifndef DIRECTX
	glBindAttribLocation(program_handle, 0, "attr_position");
	glBindAttribLocation(program_handle, 1, "attr_TexCoord");
#endif
}

/*
	Could be done in fragment shader, but this should be faster
*/
void Post::resize(int width, int height)
{
	float xInc = 1.0f / width;
    float yInc = 1.0f / height;

    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            texCoordOffsets[((i * 3 + j) * 2)] = (-1.0f * xInc) + ((float)i * xInc);
            texCoordOffsets[((i * 3 + j) * 2) + 1] = (-1.0f * yInc) + ((float)j * yInc);
        }
    }
}

void Post::Params(int tex0, int tex1)
{
#ifdef DIRECTX
	uniform->SetFloatArray(gfx->device, "tc_offset", texCoordOffsets, 9);
#else
	glUniform1i(texture0, tex0);
	glUniform1i(texture1, tex1);
	glUniform2fv(tc_offset, 9, texCoordOffsets);
#endif
}
