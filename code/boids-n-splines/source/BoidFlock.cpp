#include "BoidFlock.hpp"
#include "Paths.hpp"
#include "LayoutLocations.glsl"

#include <atlas/utils/Mesh.hpp>
#include <atlas/core/STB.hpp>
#include <atlas/core/Float.hpp>
#include <atlas/utils/GUI.hpp>
#include <stdlib.h>
#include <stdio.h>

namespace bns
{
    BoidFlock::BoidFlock() :
        mVertexBuffer(GL_ARRAY_BUFFER),
        mIndexBuffer(GL_ELEMENT_ARRAY_BUFFER)
    {
        using atlas::utils::Mesh;
        namespace gl = atlas::gl;
        namespace math = atlas::math;

        Mesh sphere;
        std::string path{ DataDirectory };
        path = path + "sphere.obj";
        Mesh::fromFile(path, sphere);

        mIndexCount = static_cast<GLsizei>(sphere.indices().size());

        mFlockRadius = 5.0;
        mNumBoids = 50;

        for (int i = 0; i < mNumBoids; i++)
        {
            float rx = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
            //float ry= static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
            float rz = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
            float rr = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * mFlockRadius;
            atlas::math::Vector rp = {2*rx-1,0,2*rz-1};
            rp = normalize(rp);

            float rvx = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
            //float rvy= static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
            float rvz = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
            atlas::math::Vector rv = {2*rvx-1,0,2*rvz-1};

            Boid mBoid = Boid(rp * rr, rv * 0.001f);
            mBoids.push_back(mBoid);
        }

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

        std::vector<gl::ShaderUnit> shaders
        {
            {std::string(ShaderDirectory) + "Ball.vs.glsl", GL_VERTEX_SHADER},
            {std::string(ShaderDirectory) + "Ball.fs.glsl", GL_FRAGMENT_SHADER}
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
        var = mShaders[0].getUniformVariable("materialColour");
        mUniforms.insert(UniformKey("materialColour", var));

        mShaders[0].disableShaders();
    }

    void BoidFlock::updateGeometry(atlas::core::Time<> const& t)
    {
        //for each boid:
        for(std::size_t i = 0; i < mBoids.size(); i++)
        {
            //determine forces at current state
            atlas::math::Vector separation = computeSeparation(mBoids[i]);
            atlas::math::Vector alignment = computeAlignment(mBoids[i]);
            atlas::math::Vector cohesion = computeCohesion(mBoids[i]);

            //sum forces & move boids
            atlas::math::Vector forces = separation*10.0f + alignment + cohesion;
            mBoids[i].mVelocity += forces * 0.00001f;
            mBoids[i].mPosition += mBoids[i].mVelocity;
            mBoids[i].mForward = normalize(mBoids[i].mVelocity);
        }
    }

    void BoidFlock::renderGeometry(atlas::math::Matrix4 const& projection,
        atlas::math::Matrix4 const& view)
    {
        namespace math = atlas::math;

        mShaders[0].hotReloadShaders();
        if (!mShaders[0].shaderProgramValid())
        {
            return;
        }

        mShaders[0].enableShaders();

        mVao.bindVertexArray();
        mIndexBuffer.bindBuffer();

        glUniformMatrix4fv(mUniforms["projection"], 1, GL_FALSE,
            &projection[0][0]);
        glUniformMatrix4fv(mUniforms["view"], 1, GL_FALSE, &view[0][0]);

        for (std::size_t i = 1; i < mBoids.size(); i++)
        {
            //draw boid "body"
            const math::Vector black{ 0.0f, 0.0f, 0.0f };
            auto bodyModel = glm::translate(math::Matrix4(1.0f), mBoids[i].mPosition) * glm::scale(math::Matrix4(1.0f), math::Vector(0.1f));
            glUniformMatrix4fv(mUniforms["model"], 1, GL_FALSE, &bodyModel[0][0]);
            glUniform3fv(mUniforms["materialColour"], 1, &black[0]);
            glDrawElements(GL_TRIANGLES, mIndexCount, GL_UNSIGNED_INT, 0);

            //draw boid "head"
            const math::Vector white{ 1.0f, 1.0f, 1.0f };
            auto headModel = glm::translate(math::Matrix4(1.0f), mBoids[i].mPosition + mBoids[i].mForward/5.0f) * glm::scale(math::Matrix4(1.0f), math::Vector(0.05f));
            glUniformMatrix4fv(mUniforms["model"], 1, GL_FALSE, &headModel[0][0]);
            glUniform3fv(mUniforms["materialColour"], 1, &white[0]);
            glDrawElements(GL_TRIANGLES, mIndexCount, GL_UNSIGNED_INT, 0);
        }

        mIndexBuffer.unBindBuffer();
        mVao.unBindVertexArray();
        mShaders[0].disableShaders();
    }

    void BoidFlock::transformGeometry(atlas::math::Matrix4 const& t)
    {
        mModel = t;
    }

    atlas::math::Vector BoidFlock::getBoidPosition()
    {
        return mBoids[0].mPosition;
    }

    atlas::math::Vector BoidFlock::getBoidLook()
    {
        return mBoids[0].mForward;
    }

    atlas::math::Vector BoidFlock::computeSeparation(Boid &self)
    {
        atlas::math::Vector sForce = {0,0,0};
        for (std::size_t i = 0; i < mBoids.size(); i++)
        {
            Boid other = mBoids[i];
            float distance = getDistance(self,other);

            if(distance > 0 && distance <= mFlockRadius*0.1f){

                atlas::math::Vector direction = self.mPosition - other.mPosition;
                if (distance < 0.1f) distance = 0.1f;
                float weight = 1.0 / distance*distance;
                sForce += direction * weight;

                //sForce -= (other.mPosition - self.mPosition);
            }
        }
        return sForce;
    }

    atlas::math::Vector BoidFlock::computeAlignment(Boid &self)
    {
        atlas::math::Vector avgAlignment = self.mVelocity;
        float neighbours = 0;
        for (std::size_t i = 0; i < mBoids.size(); i++)
        {
            Boid other = mBoids[i];
            float distance = getDistance(self,other);

            if(distance > 0 && distance <= mFlockRadius){
                avgAlignment += other.mVelocity;
                neighbours++;
            }
        }
        if (neighbours > 0) avgAlignment = avgAlignment / neighbours;
        return (avgAlignment - self.mVelocity);
    }

    atlas::math::Vector BoidFlock::computeCohesion(Boid &self)
    {
        atlas::math::Vector avgPosition = self.mPosition;
        float neighbours = 0;
        for (std::size_t i = 0; i < mBoids.size(); i++)
        {
            Boid other = mBoids[i];
            float distance = getDistance(self,other);

            if(distance > 0 && distance <= mFlockRadius){
                avgPosition += other.mPosition;
                neighbours++;
            }
        }
        if (neighbours > 0) avgPosition = avgPosition / neighbours;

        return (avgPosition - self.mPosition);
    }

    void BoidFlock::resetGeometry()
    {
        for (std::size_t i = 0; i < mBoids.size(); i++)
        {
            float rx = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
            //float ry= static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
            float rz = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
            float rr = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * mFlockRadius;
            atlas::math::Vector rp = {2*rx-1,0,2*rz-1};
            rp = normalize(rp);

            float rvx = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
            //float rvy= static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
            float rvz = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
            atlas::math::Vector rv = {2*rvx-1,0,2*rvz-1};

            mBoids[i] = Boid(rp * rr, rv * 0.001f);
        }
    }

    float BoidFlock::getDistance(Boid &boid1, Boid &boid2)
    {
        atlas::math::Vector line = boid1.mPosition - boid2.mPosition;
        return sqrt(line.x*line.x + line.y*line.y + line.z*line.z);
    }

}
