#include "standard_mesh.hpp"

#include <glm/geometric.hpp>
#include <mikktspace.h>
#include <weldmesh.h>

namespace Render {

void StandardMesh::resize(size_t p_vertex_count, Format p_format) {

	const std::vector<float> old_vertex_data = std::move(vertex_data);
	size_t next_offset = 0;
#define STANDARD_MESH_VERTEX_FEILD(name, size)                                                                         \
	size_t old_offset_##name;                                                                                          \
	if (p_format.has_##name) {                                                                                         \
		old_offset_##name = offset_##name;                                                                             \
		offset_##name = next_offset;                                                                                   \
		next_offset += size;                                                                                           \
	}
	STANDARD_MESH_VERTEX_FORMAT
#undef STANDARD_MESH_VERTEX_FEILD

	size_t old_stride = stride;
	stride = next_offset;

	vertex_data.clear();
	vertex_data.resize(stride * p_vertex_count);

#define STANDARD_MESH_VERTEX_FEILD(name, size)                                                                         \
	if (p_format.has_##name && format.has_##name) {                                                                    \
		for (size_t i = 0; i < min(vertex_count, p_vertex_count); i++)                                                 \
			*reinterpret_cast<vec##size*>(vertex_data.data() + stride * i + offset_##name) =                           \
				*reinterpret_cast<const vec##size*>(old_vertex_data.data() + old_stride * i + old_offset_##name);      \
	}
	STANDARD_MESH_VERTEX_FORMAT
#undef STANDARD_MESH_VERTEX_FEILD

	vertex_count = p_vertex_count;
	format = p_format;
}

void StandardMesh::deindex() {
	if (indices.empty())
		return;
	resize(indices.size());
	const std::vector<float> old_vertex_data = vertex_data;

	for (size_t i = 0; i < indices.size(); i++) {
		uint32_t index = indices[i];
		std::copy(
			old_vertex_data.begin() + index * stride, old_vertex_data.begin() + (index + 1) * stride,
			vertex_data.begin() + i * stride);
	}

	indices.clear();
}

void StandardMesh::reindex() {
	deindex();
	indices.resize(vertex_count);
	std::vector<float> vertex_out;
	vertex_out.resize(vertex_count * stride);
	vertex_count =
		WeldMesh(reinterpret_cast<int*>(indices.data()), vertex_out.data(), vertex_data.data(), vertex_count, stride);
	vertex_data = vertex_out;
	vertex_data.resize(vertex_count * stride);
}

void StandardMesh::gen_normals() {
	deindex();
	if (!format.has_normal) {
		Format f = format;
		f.has_normal = true;
		resize(f);
	}
	size_t triangle_count = vertex_count / 3;
	for (size_t triangle = 0; triangle < triangle_count; triangle++) {
		vec3& v1 = position(triangle * 3);
		vec3& v2 = position(triangle * 3 + 1);
		vec3& v3 = position(triangle * 3 + 2);
		vec3 norm = normalize(cross(v2 - v1, v3 - v1));
		normal(triangle * 3) = norm;
		normal(triangle * 3 + 1) = norm;
		normal(triangle * 3 + 2) = norm;
	}
}

int mikkGetNumFaces(const SMikkTSpaceContext* pContext) {
	auto& mesh = *reinterpret_cast<StandardMesh*>(pContext->m_pUserData);
	return mesh.get_vertex_count() / 3;
}
int mikkGetNumVerticesOfFace(const SMikkTSpaceContext*, const int) { return 3; }
void mikkGetPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert) {
	auto& mesh = *reinterpret_cast<StandardMesh*>(pContext->m_pUserData);
	vec3& pos = mesh.position(iFace * 3 + iVert);
	fvPosOut[0] = pos.x;
	fvPosOut[1] = pos.y;
	fvPosOut[2] = pos.z;
}
void mikkGetNormal(const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert) {
	auto& mesh = *reinterpret_cast<StandardMesh*>(pContext->m_pUserData);
	vec3& normal = mesh.normal(iFace * 3 + iVert);
	fvNormOut[0] = normal.x;
	fvNormOut[1] = normal.y;
	fvNormOut[2] = normal.z;
}
void mikkGetTexCoord(const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert) {
	auto& mesh = *reinterpret_cast<StandardMesh*>(pContext->m_pUserData);
	vec2& uv = mesh.tex_coord_0(iFace * 3 + iVert);
	fvTexcOut[0] = uv.x;
	fvTexcOut[1] = uv.y;
}
// void mikkSetTSpaceBasic(
// 	const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert) {
// 	auto& mesh = *reinterpret_cast<StandardMesh*>(pContext->m_pUserData);
// 	mesh.tangent(iFace * 3 + iVert) = vec4{fvTangent[0], fvTangent[1], fvTangent[2], fSign};
// }
void mikkSetTSpace(
	const SMikkTSpaceContext* pContext, const float fvTangent[], const float fvBiTangent[], const float, const float,
	const tbool, const int iFace, const int iVert) {
	auto& mesh = *reinterpret_cast<StandardMesh*>(pContext->m_pUserData);
	mesh.tangent(iFace * 3 + iVert) = vec3{fvTangent[0], fvTangent[1], fvTangent[2]};
	mesh.bitangent(iFace * 3 + iVert) = vec3{fvBiTangent[0], fvBiTangent[1], fvBiTangent[2]};
}
SMikkTSpaceInterface mikk_inter{
	.m_getNumFaces = mikkGetNumFaces,
	.m_getNumVerticesOfFace = mikkGetNumVerticesOfFace,
	.m_getPosition = mikkGetPosition,
	.m_getNormal = mikkGetNormal,
	.m_getTexCoord = mikkGetTexCoord,
	.m_setTSpaceBasic = nullptr,
	.m_setTSpace = mikkSetTSpace};
void StandardMesh::gen_tangents() {
	deindex();
	if (!format.has_tangent || !format.has_bitangent) {
		Format f = format;
		f.has_tangent = true;
		f.has_bitangent = true;
		resize(f);
	}
	SMikkTSpaceContext mikk_ctx{.m_pInterface = &mikk_inter, .m_pUserData = this};
	genTangSpaceDefault(&mikk_ctx);
}

} // namespace Render