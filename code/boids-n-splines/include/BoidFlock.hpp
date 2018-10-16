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
        mRadius = 0.5f;
    }

    Boid(atlas::math::Vector position, atlas::math::Vector velocity, float radius)
    {
        mPosition = position;
        mVelocity = velocity;
        mForward = normalize(mVelocity);
        mRadius = radius;
    }

    atlas::math::Vector mPosition;
    atlas::math::Vector mForward;
    atlas::math::Vector mVelocity;
    float mRadius;
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

        atlas::math::Vector computeAvoidance(Boid &boid);

        atlas::math::Vector random2DVector(float max);

        atlas::math::Vector random3DVector(float max);

        float dot(atlas::math::Vector v1, atlas::math::Vector v2);

        float mag(atlas::math::Vector v);

        float angle(atlas::math::Vector v1, atlas::math::Vector v2);

        atlas::gl::Buffer mVertexBuffer;
        atlas::gl::Buffer mIndexBuffer;
        atlas::gl::VertexArrayObject mVao;

        GLsizei mIndexCount;

        float mMass;
        float mFlockRadius;
        float mViewRadius;
        float mViewAngle;
        int mNumBoids;
        std::vector<Boid> mBoids;
    };
}
