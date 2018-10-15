#pragma once

#include "BoidFlock.hpp"
#include "Spline.hpp"

#include <atlas/tools/ModellingScene.hpp>
#include <atlas/tools/MayaCamera.hpp>
#include <atlas/tools/Grid.hpp>
#include <atlas/utils/FPSCounter.hpp>

namespace bns
{
    class BoidScene : public atlas::tools::ModellingScene
    {
    public:
        BoidScene();

        void mousePressEvent(int button, int action, int modifiers,
            double xPos, double yPos) override;
        void mouseMoveEvent(double xPos, double yPos) override;
        void mouseScrollEvent(double xOffset, double yOffset) override;

        void updateScene(double time) override;
        void renderScene() override;

    private:
        int mCameraMode;
        bool mPlay;
        float mFPS;
        float mAnimLength;

        atlas::core::Time<float> mAnimTime;
        atlas::utils::FPSCounter mCounter;

        BoidFlock mBoidFlock;
        Spline mSpline;
    };
}
