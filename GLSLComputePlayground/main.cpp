#include <windows.h>
#include <cstdint>
#include <cassert>
#include <cstdio>
#include <GL/GL.h>
#include <GL/glext.h>
#include <vector>
#include <iostream>

int main(int argc, char* argv[])
{
	HDC ourWindowHandleToDeviceContext = GetDC(0);

	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
		PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
		32,                   // Colordepth of the framebuffer.
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,                   // Number of bits for the depthbuffer
		8,                    // Number of bits for the stencilbuffer
		0,                    // Number of Aux buffers in the framebuffer.
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	int letWindowsChooseThisPixelFormat = ChoosePixelFormat(ourWindowHandleToDeviceContext, &pfd);
	SetPixelFormat(ourWindowHandleToDeviceContext, letWindowsChooseThisPixelFormat, &pfd);

	HGLRC ourOpenGLRenderingContext = wglCreateContext(ourWindowHandleToDeviceContext);
	auto result = wglMakeCurrent(ourWindowHandleToDeviceContext, ourOpenGLRenderingContext);
	assert(result);
	PFNGLGETSTRINGIPROC glGetStringi = (PFNGLGETSTRINGIPROC)wglGetProcAddress("glGetStringi");
	PFNGLGENBUFFERSPROC glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
	PFNGLDELETEBUFFERSPROC glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffers");
	PFNGLBINDBUFFERPROC glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
	PFNGLBUFFERSTORAGEPROC glBufferStorage = (PFNGLBUFFERSTORAGEPROC)wglGetProcAddress("glBufferStorage");
	PFNGLBUFFERDATAPROC glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
	PFNGLBINDBUFFERBASEPROC glBindBufferBase = (PFNGLBINDBUFFERBASEPROC)wglGetProcAddress("glBindBufferBase");
	PFNGLCREATEPROGRAMPROC glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
	PFNGLDELETEPROGRAMPROC glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
	PFNGLCREATESHADERPROC glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
	PFNGLDELETESHADERPROC glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
	PFNGLSHADERSOURCEPROC glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
	PFNGLCOMPILESHADERPROC glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
	PFNGLGETSHADERIVPROC glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
	PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
	PFNGLATTACHSHADERPROC glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
	PFNGLLINKPROGRAMPROC glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
	PFNGLGETPROGRAMIVPROC glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
	PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
	PFNGLUSEPROGRAMPROC glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
	PFNGLDISPATCHCOMPUTEPROC glDispatchCompute = (PFNGLDISPATCHCOMPUTEPROC)wglGetProcAddress("glDispatchCompute");
	PFNGLGETBUFFERSUBDATAPROC glGetBufferSubData = (PFNGLGETBUFFERSUBDATAPROC)wglGetProcAddress("glGetBufferSubData");

	FILE* fin = fopen("Shader.glsl", "r");
	fseek(fin, 0, SEEK_END);
	long int shaderSize = ftell(fin);
	fseek(fin, 0, SEEK_SET);
	std::vector<char> shaderSource(shaderSize);
	fread(shaderSource.data(), 1, shaderSize, fin);
	fclose(fin);

	float inputData[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 };
	constexpr int outputFloats = 16;

	GLint majorVersion;
	glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
	auto error = glGetError();
	assert(error == GL_NO_ERROR);
	GLint minorVersion;
	glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
	error = glGetError();
	assert(error == GL_NO_ERROR);
	assert(majorVersion > 4 || (majorVersion == 4 && minorVersion >= 3));

	GLuint inputBuffer;
	glGenBuffers(1, &inputBuffer);
	error = glGetError();
	assert(error == GL_NO_ERROR);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, inputBuffer);
	error = glGetError();
	assert(error == GL_NO_ERROR);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(inputData), inputData, GL_DYNAMIC_DRAW);
	error = glGetError();
	assert(error == GL_NO_ERROR);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, inputBuffer);
	error = glGetError();
	assert(error == GL_NO_ERROR);

	GLuint outputBuffer;
	glGenBuffers(1, &outputBuffer);
	error = glGetError();
	assert(error == GL_NO_ERROR);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputBuffer);
	error = glGetError();
	assert(error == GL_NO_ERROR);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, outputFloats * sizeof(float), nullptr, GL_DYNAMIC_STORAGE_BIT);
	error = glGetError();
	assert(error == GL_NO_ERROR);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, outputBuffer);
	error = glGetError();
	assert(error == GL_NO_ERROR);

	GLuint program = glCreateProgram();
	error = glGetError();
	assert(error == GL_NO_ERROR);
	GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
	error = glGetError();
	assert(error == GL_NO_ERROR);
	GLchar* shaderSources[] = { shaderSource.data() };
	GLint shaderSizes[] = { shaderSize };
	glShaderSource(shader, 1, shaderSources, shaderSizes);
	error = glGetError();
	assert(error == GL_NO_ERROR);
	glCompileShader(shader);
	error = glGetError();
	assert(error == GL_NO_ERROR);
	GLint compileStatus;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
	error = glGetError();
	assert(error == GL_NO_ERROR);
	assert(compileStatus == GL_TRUE);
	/*GLint logLength;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
	error = glGetError();
	assert(error == GL_NO_ERROR);
	std::vector<char> log(logLength);
	glGetShaderInfoLog(shader, logLength, &logLength, log.data());
	error = glGetError();
	assert(error == GL_NO_ERROR);*/
	glAttachShader(program, shader);
	error = glGetError();
	assert(error == GL_NO_ERROR);
	glLinkProgram(program);
	error = glGetError();
	assert(error == GL_NO_ERROR);
	GLint linkStatus;
	glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
	error = glGetError();
	assert(error == GL_NO_ERROR);
	assert(linkStatus == GL_TRUE);
	glUseProgram(program);
	error = glGetError();
	assert(error == GL_NO_ERROR);

	glDispatchCompute(1, 1, 1);
	error = glGetError();
	assert(error == GL_NO_ERROR);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputBuffer);
	error = glGetError();
	assert(error == GL_NO_ERROR);
	float outputData[outputFloats];
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, outputFloats * sizeof(float), outputData);
	error = glGetError();
	assert(error == GL_NO_ERROR);
	for (size_t i = 0; i < outputFloats; ++i)
		std::cout << i << ": " << outputData[i] << std::endl;

	glDeleteShader(shader);
	error = glGetError();
	assert(error == GL_NO_ERROR);
	glDeleteProgram(program);
	error = glGetError();
	assert(error == GL_NO_ERROR);
	glDeleteBuffers(1, &outputBuffer);
	error = glGetError();
	assert(error == GL_NO_ERROR);
	glDeleteBuffers(1, &inputBuffer);
	error = glGetError();
	assert(error == GL_NO_ERROR);
	result = wglDeleteContext(ourOpenGLRenderingContext);
	assert(result);

	return 0;
}