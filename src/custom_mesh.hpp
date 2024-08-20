#ifndef _SPRF_CUSTOM_MESH_HPP_
#define _SPRF_CUSTOM_MESH_HPP_

#include "engine/engine.hpp"

namespace SPRF {

static Vector3 make_vector3(float x, float y, float z) {
    Vector3 out;
    out.x = x;
    out.y = y;
    out.z = z;
    return out;
}

static Vector2 make_vector2(float x, float y) {
    Vector2 out;
    out.x = x;
    out.y = y;
    return out;
}

static MeshUnmanaged WrappedMesh(float width, float length, int resX, int resZ) {
    MeshUnmanaged mesh;// = {0};

    resX++;
    resZ++;

    // Vertices definition
    int vertexCount = resX * resZ; // vertices get reused for the faces

    Vector3* vertices = (Vector3*)RL_MALLOC(vertexCount * sizeof(Vector3));
    for (int z = 0; z < resZ; z++) {
        // [-length/2, length/2]
        float zPos = ((float)z / (resZ - 1) - 0.5f) * length;
        for (int x = 0; x < resX; x++) {
            // [-width/2, width/2]
            float xPos = ((float)x / (resX - 1) - 0.5f) * width;
            vertices[x + z * resX] = make_vector3(xPos, 0.0f, zPos);
        }
    }

    // Normals definition
    Vector3* normals = (Vector3*)RL_MALLOC(vertexCount * sizeof(Vector3));
    for (int n = 0; n < vertexCount; n++)
        normals[n] = make_vector3(0.0f, 1.0f, 0.0f); // Vector3.up;

    // TexCoords definition
    Vector2* texcoords = (Vector2*)RL_MALLOC(vertexCount * sizeof(Vector2));
    for (int v = 0; v < resZ; v++) {
        for (int u = 0; u < resX; u++) {
            texcoords[u + v * resX] =
                make_vector2(((float)u / (resX - 1)) * width * 0.25f,
                             ((float)v / (resZ - 1)) * length * 0.25f);
        }
    }

    // Triangles definition (indices)
    int numFaces = (resX - 1) * (resZ - 1);
    int* triangles = (int*)RL_MALLOC(numFaces * 6 * sizeof(int));
    int t = 0;
    for (int face = 0; face < numFaces; face++) {
        // Retrieve lower left corner from face ind
        int i = face % (resX - 1) + (face / (resZ - 1) * resX);

        triangles[t++] = i + resX;
        triangles[t++] = i + 1;
        triangles[t++] = i;

        triangles[t++] = i + resX;
        triangles[t++] = i + resX + 1;
        triangles[t++] = i + 1;
    }

    mesh.vertexCount = vertexCount;
    mesh.triangleCount = numFaces * 2;
    mesh.vertices = (float*)RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));
    mesh.texcoords = (float*)RL_MALLOC(mesh.vertexCount * 2 * sizeof(float));
    mesh.normals = (float*)RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));
    mesh.indices = (unsigned short*)RL_MALLOC(mesh.triangleCount * 3 *
                                              sizeof(unsigned short));

    // Mesh vertices position array
    for (int i = 0; i < mesh.vertexCount; i++) {
        mesh.vertices[3 * i] = vertices[i].x;
        mesh.vertices[3 * i + 1] = vertices[i].y;
        mesh.vertices[3 * i + 2] = vertices[i].z;
    }

    // Mesh texcoords array
    for (int i = 0; i < mesh.vertexCount; i++) {
        mesh.texcoords[2 * i] = texcoords[i].x;
        mesh.texcoords[2 * i + 1] = texcoords[i].y;
    }

    // Mesh normals array
    for (int i = 0; i < mesh.vertexCount; i++) {
        mesh.normals[3 * i] = normals[i].x;
        mesh.normals[3 * i + 1] = normals[i].y;
        mesh.normals[3 * i + 2] = normals[i].z;
    }

    // Mesh indices array initialization
    for (int i = 0; i < mesh.triangleCount * 3; i++)
        mesh.indices[i] = triangles[i];

    RL_FREE(vertices);
    RL_FREE(normals);
    RL_FREE(texcoords);
    RL_FREE(triangles);

    // Upload vertex data to GPU (static mesh)
    UploadMesh(&mesh, false);

    return mesh;
}

}; // namespace SPRF

#endif // _SPRF_CUSTOM_MESH_HPP_