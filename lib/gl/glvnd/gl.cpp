#include "gl.hpp"

PFNGLSPECIALIZESHADERPROC _glSpecializeShader;
PFNGLMULTIDRAWARRAYSINDIRECTCOUNTPROC _glMultiDrawArraysIndirectCount;
PFNGLMULTIDRAWELEMENTSINDIRECTCOUNTPROC _glMultiDrawElementsIndirectCount;
PFNGLPOLYGONOFFSETCLAMPPROC _glPolygonOffsetClamp;

GLenum loadGL(void (*glGetProcAddress(const char*))()) {
	_glSpecializeShader = reinterpret_cast<PFNGLSPECIALIZESHADERPROC>(glGetProcAddress("glSpecializeShader"));
	_glMultiDrawArraysIndirectCount =
		reinterpret_cast<PFNGLMULTIDRAWARRAYSINDIRECTCOUNTPROC>(glGetProcAddress("glMultiDrawArraysIndirectCount"));
	_glMultiDrawElementsIndirectCount =
		reinterpret_cast<PFNGLMULTIDRAWELEMENTSINDIRECTCOUNTPROC>(glGetProcAddress("glMultiDrawElementsIndirectCount"));
	_glPolygonOffsetClamp = reinterpret_cast<PFNGLPOLYGONOFFSETCLAMPPROC>(glGetProcAddress("glPolygonOffsetClamp"));
	return 0;
}

GLAPI void APIENTRY glSpecializeShader(
	GLuint shader, const GLchar* pEntryPoint, GLuint numSpecializationConstants, const GLuint* pConstantIndex,
	const GLuint* pConstantValue) {
	return _glSpecializeShader(shader, pEntryPoint, numSpecializationConstants, pConstantIndex, pConstantValue);
}
GLAPI void APIENTRY glMultiDrawArraysIndirectCount(
	GLenum mode, const void* indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride) {
	return _glMultiDrawArraysIndirectCount(mode, indirect, drawcount, maxdrawcount, stride);
}
GLAPI void APIENTRY glMultiDrawElementsIndirectCount(
	GLenum mode, GLenum type, const void* indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride) {
	return _glMultiDrawElementsIndirectCount(mode, type, indirect, drawcount, maxdrawcount, stride);
}
GLAPI void APIENTRY glPolygonOffsetClamp(GLfloat factor, GLfloat units, GLfloat clamp) {
	return _glPolygonOffsetClamp(factor, units, clamp);
}