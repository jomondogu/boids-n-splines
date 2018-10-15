#pragma once

#include <algorithm>

#include <atlas/utils/Geometry.hpp>
#include <atlas/gl/Buffer.hpp>
#include <atlas/gl/VertexArrayObject.hpp>
#include <atlas/gl/Texture.hpp>

namespace bns
{

    class Boid
    {
    public:
    Boid()
    {
        mPosition = atlas::math::Vector(0,0,0);
        mVelocity = atlas::math::Vector(0,0,0);
        mForward = normalize(mVelocity);
    }

    Boid(atlas::math::Vector position, atlas::math::Vector velocity)
    {
        mPosition = position;
        mVelocity = velocity;
        mForward = normalize(mVelocity);
    }

    atlas::math::Vector mPosition;
    atlas::math::Vector mForward;
    atlas::math::Vector mVelocity;
    };

    class BoidFlock : public atlas::utils::Geometry
    {
    public:
        BoidFlock();

        void updateGeometry(atlas::core::Time<> const& t) override;

        void renderGeometry(atlas::math::Matrix4 const& projection,
            atlas::math::Matrix4 const& view) override;

        void transformGeometry(atlas::math::Matrix4 const& t) override;

        void resetGeometry() override;

        atlas::math::Vector getBoidPosition();

        atlas::math::Vector getBoidLook();

    private:

        atlas::math::Vector computeSeparation(Boid &boid);

        atlas::math::Vector computeAlignment(Boid &boid);

        atlas::math::Vector computeCohesion(Boid &boid);
        
        atlas::math::Vector random2DVector(float max);
        
        atlas::math::Vector random3DVector(float max);

        float getDistance(Boid &boid1, Boid &boid2);

        atlas::gl::Buffer mVertexBuffer;
        atlas::gl::Buffer mIndexBuffer;
        atlas::gl::VertexArrayObject mVao;

        GLsizei mIndexCount;

        float mFlockRadius;
        float mViewRadius;
        float mViewAngle;
        int mNumBoids;
        std::vector<Boid> mBoids;
    };
}
