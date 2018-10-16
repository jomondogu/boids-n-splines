#include "BoidFlock.hpp"
#include "Paths.hpp"
#include "LayoutLocations.glsl"

#include <atlas/utils/Mesh.hpp>
#include <atlas/core/STB.hpp>
#include <atlas/core/Float.hpp>
#include <atlas/utils/GUI.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

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

        mMass = 1000.0f;
        mFlockRadius = 5.0f;
        mViewRadius = 1.0f;
        mViewAngle = 0.75 * 3.1419f;
        mNumBoids = 100;

        for (int i = 0; i < mNumBoids; i++)
        {
            atlas::math::Vector rp = normalize(random2DVector(1.0f));

            float rr = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * mFlockRadius;
            rp.x *= rr;
            rp.z *= rr;

            atlas::math::Vector rv = random2DVector(1.0f);

            Boid mBoid = Boid(rp, rv * 0.001f, 0.15f);
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
            atlas::math::Vector avoidance = computeAvoidance(mBoids[i]);

            //sum forces & move boids
            atlas::math::Vector forces = separation*3.0f + alignment*100.0f + cohesion*2.0f + avoidance;
            mBoids[i].mVelocity += forces / mMass;
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
            atlas::math::Vector offset = {0,0.2f,0};
            //draw boid "body"
            const math::Vector white{ 1.0f, 1.0f, 1.0f };
            auto bodyModel = glm::translate(math::Matrix4(1.0f), mBoids[i].mPosition + offset) * glm::scale(math::Matrix4(1.0f), math::Vector(0.1f));
            glUniformMatrix4fv(mUniforms["model"], 1, GL_FALSE, &bodyModel[0][0]);
            glUniform3fv(mUniforms["materialColour"], 1, &white[0]);
            glDrawElements(GL_TRIANGLES, mIndexCount, GL_UNSIGNED_INT, 0);

            //draw boid "head"
            const math::Vector black{ 0.0f, 0.0f, 0.0f };
            auto headModel = glm::translate(math::Matrix4(1.0f), mBoids[i].mPosition + mBoids[i].mForward*0.15f + offset) * glm::scale(math::Matrix4(1.0f), math::Vector(0.05f));
            glUniformMatrix4fv(mUniforms["model"], 1, GL_FALSE, &headModel[0][0]);
            glUniform3fv(mUniforms["materialColour"], 1, &black[0]);
            glDrawElements(GL_TRIANGLES, mIndexCount, GL_UNSIGNED_INT, 0);
        }

        mIndexBuffer.unBindBuffer();
        mVao.unBindVertexArray();
        mShaders[0].disableShaders();
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

            float distance = mag(self.mPosition - other.mPosition);

            if(distance > 0 && distance <= mViewRadius * 0.5f &&
                angle(self.mForward, self.mPosition - other.mPosition) <= mViewAngle)
            {
                atlas::math::Vector direction = self.mPosition - other.mPosition;
                float weight = 1.0 / distance*distance;
                sForce += direction * weight;
            }
        }
        return sForce;
    }

    atlas::math::Vector BoidFlock::computeAlignment(Boid &self)
    {
        atlas::math::Vector avgAlignment = {0,0,0};
        float neighbours = 0;

        for (std::size_t i = 0; i < mBoids.size(); i++)
        {
            Boid other = mBoids[i];
            float distance = mag(self.mPosition - other.mPosition);

            if(distance > 0 && distance <= mViewRadius &&
                angle(self.mForward, self.mPosition - other.mPosition) <= mViewAngle)
            {
                avgAlignment += other.mVelocity;
                neighbours++;
            }
        }
        if (neighbours > 0)
        {
            avgAlignment = avgAlignment / neighbours;
            return avgAlignment - self.mVelocity;
        }
        return {0,0,0};
    }

    atlas::math::Vector BoidFlock::computeCohesion(Boid &self)
    {
        atlas::math::Vector avgPosition = {0,0,0};
        float neighbours = 0;

        for (std::size_t i = 0; i < mBoids.size(); i++)
        {
            Boid other = mBoids[i];
            float distance = mag(self.mPosition - other.mPosition);

            if(distance > 0 && distance <= mViewRadius &&
                angle(self.mForward, self.mPosition - other.mPosition) <= mViewAngle)
            {
                avgPosition += other.mPosition;
                neighbours++;
            }
        }

        if (neighbours > 0)
        {
            avgPosition = avgPosition / neighbours;
            return (avgPosition - self.mPosition);
        }

        return {0,0,0};
    }

    atlas::math::Vector BoidFlock::computeAvoidance(Boid &self)
    {
        atlas::math::Vector avoidance = {0,0,0};
        atlas::math::Vector ahead = self.mPosition + self.mForward;

        for (std::size_t i = 0; i < mBoids.size(); i++)
        {
            Boid other = mBoids[i];

            float distance = mag(other.mPosition - self.mPosition);
            float aheadDistance = mag(other.mPosition - ahead);
            float halfDistance = mag(other.mPosition - (ahead * 0.5f));

            if((distance > 0) && (distance <= other.mRadius ||
                aheadDistance <= other.mRadius ||
                halfDistance <= other.mRadius))
            {
                avoidance = normalize(ahead - other.mPosition);
            }
        }

        return avoidance;
    }

    void BoidFlock::resetGeometry()
    {
        for (std::size_t i = 0; i < mBoids.size(); i++)
        {
            atlas::math::Vector rp = normalize(random2DVector(1.0f));

            float rr = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * mFlockRadius;
            rp.x *= rr;
            rp.z *= rr;

            atlas::math::Vector rv = random2DVector(1.0f);

            mBoids[i] = Boid(rp, rv * 0.001f, 0.15f);
        }
    }

    atlas::math::Vector BoidFlock::random2DVector(float max)
    {
        float rx = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * max;
        float rz = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * max;
        atlas::math::Vector rv = {2*rx-1,0,2*rz-1};
        return rv;
    }

    atlas::math::Vector BoidFlock::random3DVector(float max)
    {
        float rx = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * max;
        float ry= static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * max;
        float rz = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * max;
        atlas::math::Vector rv = {2*rx-1,2*ry-1,2*rz-1};
        return rv;
    }

    float BoidFlock::dot(atlas::math::Vector v1, atlas::math::Vector v2)
    {
        return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
    }

    float BoidFlock::mag(atlas::math::Vector v)
    {
        return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    }

    float BoidFlock::angle(atlas::math::Vector v1, atlas::math::Vector v2)
    {
        return acos(dot(v1,v2)/(mag(v1)*mag(v2)));
    }

}
