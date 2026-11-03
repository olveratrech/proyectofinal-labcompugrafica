#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>



#include "Shader.h"

using namespace std;

struct Vertex
{
	// Position
	glm::vec3 Position;
	// Normal
	glm::vec3 Normal;
	// TexCoords
	glm::vec2 TexCoords;
};

struct Texture
{
	GLuint id;
	string type;
	aiString path;
};

// Propiedades del .mtl, incluso cuando el modelo no usa imagenes.
struct SurfaceMaterial
{
    glm::vec3 diffuse = glm::vec3(1.0f);
    glm::vec3 specular = glm::vec3(0.5f);
    float shininess = 64.0f;
    float opacity = 1.0f;
};

class Mesh
{
public:
	/*  Mesh Data  */
	vector<Vertex> vertices;
	vector<GLuint> indices;
	vector<Texture> textures;
    SurfaceMaterial surface;

	/*  Functions  */
	// Constructor
	Mesh(vector<Vertex> vertices, vector<GLuint> indices, vector<Texture> textures,
         SurfaceMaterial material = SurfaceMaterial())
	{
		this->vertices = vertices;
		this->indices = indices;
		this->textures = textures;
        this->surface = material;

		// Now that we have all the required data, set the vertex buffers and its attribute pointers.
		this->setupMesh();
	}

	// Render the mesh
	void Draw(Shader shader)
	{
        bool hasDiffuse = false;
        bool hasSpecular = false;
		// Bind appropriate textures
		GLuint diffuseNr = 1;
		GLuint specularNr = 1;

		for (GLuint i = 0; i < this->textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i); // Active proper texture unit before binding
											  // Retrieve texture number (the N in diffuse_textureN)
			stringstream ss;
			string number;
			string name = this->textures[i].type;

			if (name == "texture_diffuse")
			{
                if (!hasDiffuse && this->textures[i].id != 0)
                {
                    glUniform1i(glGetUniformLocation(shader.Program, "material.diffuse"), i);
                    hasDiffuse = true;
                }
				ss << diffuseNr++; // Transfer GLuint to stream
			}
			else if (name == "texture_specular")
			{
                if (!hasSpecular && this->textures[i].id != 0)
                {
                    glUniform1i(glGetUniformLocation(shader.Program, "material.specular"), i);
                    hasSpecular = true;
                }
				ss << specularNr++; // Transfer GLuint to stream
			}

			number = ss.str();
			// Now set the sampler to the correct texture unit
			glUniform1i(glGetUniformLocation(shader.Program, (name + number).c_str()), i);
			// And finally bind the texture
			glBindTexture(GL_TEXTURE_2D, this->textures[i].id);
		}

        // Los samplers siempre apuntan a una textura valida, aun sin mapas.
        static GLuint whiteTexture = 0;
        GLuint fallbackUnit = static_cast<GLuint>(textures.size());
        glActiveTexture(GL_TEXTURE0 + fallbackUnit);
        if (whiteTexture == 0)
        {
            const unsigned char white[] = {255, 255, 255, 255};
            glGenTextures(1, &whiteTexture);
            glBindTexture(GL_TEXTURE_2D, whiteTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        glBindTexture(GL_TEXTURE_2D, whiteTexture);
        if (!hasDiffuse)
            glUniform1i(glGetUniformLocation(shader.Program, "material.diffuse"), fallbackUnit);
        if (!hasSpecular)
            glUniform1i(glGetUniformLocation(shader.Program, "material.specular"), fallbackUnit);

        // Respetar los colores, brillo y opacidad de cada material.
        glUniform1i(glGetUniformLocation(shader.Program, "hasDiffuseMap"), hasDiffuse);
        glUniform1i(glGetUniformLocation(shader.Program, "hasSpecularMap"), hasSpecular);
        glUniform3fv(glGetUniformLocation(shader.Program, "material.diffuseColor"), 1, &surface.diffuse[0]);
        glUniform3fv(glGetUniformLocation(shader.Program, "material.specularColor"), 1, &surface.specular[0]);
        glUniform1f(glGetUniformLocation(shader.Program, "material.shininess"), surface.shininess);
        glUniform1f(glGetUniformLocation(shader.Program, "material.opacity"), surface.opacity);

		// Draw mesh
		glBindVertexArray(this->VAO);
		glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// Always good practice to set everything back to defaults once configured.
        glActiveTexture(GL_TEXTURE0 + fallbackUnit);
        glBindTexture(GL_TEXTURE_2D, 0);
		for (GLuint i = 0; i < this->textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
        glActiveTexture(GL_TEXTURE0);
	}

private:
	/*  Render data  */
	GLuint VAO, VBO, EBO;

	/*  Functions    */
	// Initializes all the buffer objects/arrays
	void setupMesh()
	{
		// Create buffers/arrays
		glGenVertexArrays(1, &this->VAO);
		glGenBuffers(1, &this->VBO);
		glGenBuffers(1, &this->EBO);

		glBindVertexArray(this->VAO);
		// Load data into vertex buffers
		glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
		// A great thing about structs is that their memory layout is sequential for all its items.
		// The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
		// again translates to 3/2 floats which translates to a byte array.
		glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(Vertex), &this->vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(GLuint), &this->indices[0], GL_STATIC_DRAW);

		// Set the vertex attribute pointers
		// Vertex Positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)0);
		// Vertex Normals
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, Normal));
		// Vertex Texture Coords
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, TexCoords));

		glBindVertexArray(0);
	}
};
