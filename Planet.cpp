#include "Planet.hpp"
#include "Paths.hpp"
#include "LayoutLocations.glsl"

#include <atlas/utils/Mesh.hpp>
#include <atlas/core/STB.hpp>
#include <atlas/math/Coordinates.hpp>

namespace lab5
{
    Planet::Planet(std::string const& textureFile) :
        mVertexBuffer(GL_ARRAY_BUFFER),
        mIndexBuffer(GL_ELEMENT_ARRAY_BUFFER),
        mTexture(GL_TEXTURE_2D)
    {
        using atlas::utils::Mesh;
        namespace gl = atlas::gl;
        namespace math = atlas::math;

        Mesh sphere;
        std::string path{ DataDirectory };
        path = path + "sphere.obj";
        Mesh::fromFile(path, sphere);

        mIndexCount = static_cast<GLsizei>(sphere.indices().size());

        std::vector<float> data;
        for (std::size_t i = 0; i < sphere.vertices().size(); ++i)
        {
            data.push_back(sphere.vertices()[i].x);
            data.push_back(sphere.vertices()[i].y);
            data.push_back(sphere.vertices()[i].z);

            data.push_back(sphere.normals()[i].x);
            data.push_back(sphere.normals()[i].y);
            data.push_back(sphere.normals()[i].z);

            data.push_back(sphere.texCoords()[i].x);
            data.push_back(sphere.texCoords()[i].y);
        }

        mVao.bindVertexArray();
        mVertexBuffer.bindBuffer();
        mVertexBuffer.bufferData(gl::size<float>(data.size()), data.data(),
            GL_STATIC_DRAW);
        mVertexBuffer.vertexAttribPointer(VERTICES_LAYOUT_LOCATION, 3, GL_FLOAT,
            GL_FALSE, gl::stride<float>(8), gl::bufferOffset<float>(0));
        mVertexBuffer.vertexAttribPointer(NORMALS_LAYOUT_LOCATION, 3, GL_FLOAT,
            GL_FALSE, gl::stride<float>(8), gl::bufferOffset<float>(3));
        mVertexBuffer.vertexAttribPointer(TEXTURES_LAYOUT_LOCATION, 2, GL_FLOAT,
            GL_FALSE, gl::stride<float>(8), gl::bufferOffset<float>(6));

        mVao.enableVertexAttribArray(VERTICES_LAYOUT_LOCATION);
        mVao.enableVertexAttribArray(NORMALS_LAYOUT_LOCATION);
        mVao.enableVertexAttribArray(TEXTURES_LAYOUT_LOCATION);

        mIndexBuffer.bindBuffer();
        mIndexBuffer.bufferData(gl::size<GLuint>(sphere.indices().size()),
            sphere.indices().data(), GL_STATIC_DRAW);

        mIndexBuffer.unBindBuffer();
        mVertexBuffer.unBindBuffer();
        mVao.unBindVertexArray();

        int width, height, nrChannels;
        std::string imagePath = std::string(DataDirectory) + textureFile;
        unsigned char* imageData = stbi_load(imagePath.c_str(), &width, &height,
            &nrChannels, 0);

        mTexture.bindTexture();
        mTexture.texImage2D(0, GL_RGB, width, height, 0,
            GL_RGB, GL_UNSIGNED_BYTE, imageData);
        mTexture.texParameteri(GL_TEXTURE_WRAP_S, GL_REPEAT);
        mTexture.texParameteri(GL_TEXTURE_WRAP_T, GL_REPEAT);
        mTexture.texParameteri(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        mTexture.texParameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(imageData);

        std::vector<gl::ShaderUnit> shaders
        {
            {std::string(ShaderDirectory) + "Planet.vs.glsl", GL_VERTEX_SHADER},
            {std::string(ShaderDirectory) + "Planet.fs.glsl", GL_FRAGMENT_SHADER}
        };

        mShaders.emplace_back(shaders);
        mShaders[0].setShaderIncludeDir(ShaderDirectory);
        mShaders[0].compileShaders();
        mShaders[0].linkShaders();

        auto var = mShaders[0].getUniformVariable("model");
        mUniforms.insert(UniformKey("model", var));
        var = mShaders[0].getUniformVariable("projection");
        mUniforms.insert(UniformKey("projection", var));
        var = mShaders[0].getUniformVariable("view");
        mUniforms.insert(UniformKey("view", var));

        mShaders[0].disableShaders();
        mModel = glm::scale(math::Matrix4(1.0f), math::Vector(4.0f));

        mVelocity = atlas::math::Vector(glm::sqrt(mG * mCentralMass) / mOrbit);
    }

    void Planet::setPosition(atlas::math::Point const& pos)
    {
        using atlas::math::Matrix4;
        using atlas::math::cartesianToPolar;

        mPosition.xz = cartesianToPolar(pos.xz);
        mModel = glm::translate(Matrix4(1.0f), pos);
    }

    void Planet::updateGeometry(atlas::core::Time<> const& t)
    {
        using atlas::math::Matrix4;
        using atlas::math::Vector;
        using atlas::math::polarToCartesian;

        if (glm::abs(mPosition.x) < 6.0f)
        {
            return;
        }

        if (glm::abs(mPosition.x) > 50.0f)
        {
            mVelocity.x = -1.0f * mVelocity.x;
        }
        
        float r = mPosition.x;
        float theta = mPosition.z;
        float rprime = mVelocity.x;
        float thetaprime = mVelocity.z;
        
        atlas::math::Vector accel;
        accel.x = rprime - (mG * mCentralMass)/(r * r);
        accel.z = (-2.0f * r * rprime * thetaprime)/(r * r);
        
        mVelocity += t.deltaTime * accel;
        mPosition += t.deltaTime * mVelocity;
        
        atlas::math::Vector pos;

        pos.xz = polarToCartesian(mPosition.xz);
        
        mModel = glm::translate(Matrix4(1.0f), pos);

        // Todo by students
        //mPosition.x = r
        //mPosition.z = theta
        
        //mVelocity.x = r'
        //mVelocity.z = theta'
        
        /*
         L = T - V
         T = 1/2 * m(r'^2 + r^2 * theta'^2)
         V = -mK/r
         K = gm
         
         DL/Dtheta' = m(2 * r * r' * theta' + m * r * r * theta")
         -> m(2rr'theta' + mr^2theta") = 0
         -> theta" = (-2 * r * r' * theta')/(r^2)
         
         DL/Dr' = m * r"
         -> mr" = r * m * theta'^2 - mK/r^2
         -> r" = r * (theta' ^ 2) - K/r^2
         */
    }
    
    void Planet::renderGeometry(atlas::math::Matrix4 const& projection,
        atlas::math::Matrix4 const& view)
    {
        namespace math = atlas::math;

        mShaders[0].hotReloadShaders();
        if (!mShaders[0].shaderProgramValid())
        {
            return;
        }

        mShaders[0].enableShaders();

        mTexture.bindTexture();
        mVao.bindVertexArray();
        mIndexBuffer.bindBuffer();

        auto model = mModel;

        glUniformMatrix4fv(mUniforms["model"], 1, GL_FALSE, &mModel[0][0]);
        glUniformMatrix4fv(mUniforms["projection"], 1, GL_FALSE,
            &projection[0][0]);
        glUniformMatrix4fv(mUniforms["view"], 1, GL_FALSE, &view[0][0]);

        glDrawElements(GL_TRIANGLES, mIndexCount, GL_UNSIGNED_INT, 0);

        mIndexBuffer.unBindBuffer();
        mVao.unBindVertexArray();
        mTexture.unBindTexture();
        mShaders[0].disableShaders();
    }

    void Planet::resetGeometry()
    {
        mVelocity = atlas::math::Vector(glm::sqrt(mG * mCentralMass) / mOrbit);
    }
}
