#include "BoidScene.hpp"

#include <atlas/gl/GL.hpp>
#include <atlas/utils/GUI.hpp>
#include <atlas/core/Log.hpp>
#include <atlas/math/Math.hpp>

namespace bns
{
    BoidScene::BoidScene() :
        mCameraMode(0),
        mPlay(false),
        mFPS(60.0f),
        mAnimLength(10.0f),
        mSpline(int(mAnimLength * mFPS)),
        mCounter(mFPS)
    { }

    void BoidScene::mousePressEvent(int button, int action, int modifiers,
        double xPos, double yPos)
    {
        using atlas::tools::MayaMovements;
        atlas::utils::Gui::getInstance().mousePressed(button, action, modifiers);

        if (action == GLFW_PRESS)
        {
            atlas::math::Point2 point(xPos, yPos);

            if (button == GLFW_MOUSE_BUTTON_LEFT &&
                modifiers == GLFW_MOD_SHIFT)
            {
                if (mCameraMode == 0)
                {
                    mCamera.setMovement(MayaMovements::Tumble);
                    mCamera.mouseDown(point);
                }
            }
            else if (button == GLFW_MOUSE_BUTTON_MIDDLE &&
                modifiers == GLFW_MOD_SHIFT)
            {
                if (mCameraMode == 0)
                {
                    mCamera.setMovement(MayaMovements::Track);
                    mCamera.mouseDown(point);
                }
            }
            else if (button == GLFW_MOUSE_BUTTON_RIGHT &&
                modifiers == GLFW_MOD_SHIFT)
            {
                if (mCameraMode == 0)
                {
                    mCamera.setMovement(MayaMovements::Dolly);
                    mCamera.mouseDown(point);
                }
            }
        }
        else
        {
            if (mCameraMode == 0)
            {
                mCamera.mouseUp();
            }
        }
    }

    void BoidScene::mouseMoveEvent(double xPos, double yPos)
    {
        atlas::utils::Gui::getInstance().mouseMoved(xPos, yPos);
        if (mCameraMode == 0)
        {
            mCamera.mouseMove(atlas::math::Point2(xPos, yPos));
        }
    }

    void BoidScene::mouseScrollEvent(double xOffset, double yOffset)
    {
        atlas::utils::Gui::getInstance().mouseScroll(xOffset, yOffset);

        if (mCameraMode == 0)
        {
            mCamera.mouseScroll(atlas::math::Point2(xOffset, yOffset));
        }
    }

    void BoidScene::updateScene(double time)
    {
        using atlas::core::Time;

        ModellingScene::updateScene(time);
        if (mPlay && mCounter.isFPS(mTime))
        {
            const float delta = 1.0f / mFPS;
            mAnimTime.currentTime += delta;
            mAnimTime.deltaTime = delta;
            mAnimTime.totalTime = mAnimTime.currentTime;

            mSpline.updateGeometry(mAnimTime);
            mBoidFlock.updateGeometry(mAnimTime);
        }

        if(mCameraMode == 0)
        {
            mCamera.setCameraPosition({20,20,20});
        }
        else if(mCameraMode == 1)
        {
            auto point = mSpline.getPosition();
            mCamera.setCameraPosition(point);
        }
        else if(mCameraMode ==2)
        {
            atlas::math::Vector offset = {0,0.2f,0};
            mCamera.setCameraPosition(mBoidFlock.getBoidPosition() + offset);
            //mCamera.target = mBoidFlock.getBoidLook();
        }
    }

    void BoidScene::renderScene()
    {
        using atlas::utils::Gui;

        Gui::getInstance().newFrame();
        const float grey = 92.0f / 255.0f;
        glClearColor(grey, grey, grey, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mProjection = glm::perspective(
            glm::radians(mCamera.getCameraFOV()),
            (float)mWidth / mHeight, 1.0f, 100000000.0f);

        mView = mCamera.getCameraMatrix();

        mGrid.renderGeometry(mProjection, mView);
        mBoidFlock.renderGeometry(mProjection, mView);
        mSpline.renderGeometry(mProjection, mView);

        // Global HUD
        ImGui::SetNextWindowSize(ImVec2(350, 150), ImGuiSetCond_FirstUseEver);
        ImGui::Begin("Global HUD");

        if (mPlay)
        {
            if (ImGui::Button("Pause"))
            {
                mPlay = !mPlay;
            }
        }
        else
        {
            if (ImGui::Button("Play"))
            {
                mPlay = !mPlay;
            }
        }

        if (ImGui::Button("Reset Camera"))
        {
            mCamera.resetCamera();
            mSpline.resetGeometry();
            mAnimTime.currentTime = 0.0f;
            mAnimTime.totalTime = 0.0f;
            mPlay = false;
        }

        if (ImGui::Button("Reset Boids"))
        {
            mBoidFlock.resetGeometry();
            mAnimTime.currentTime = 0.0f;
            mAnimTime.totalTime = 0.0f;
            mPlay = false;
        }

        std::vector<const char*> options = { "Stage", "Spline Track", "Boid POV" };
        ImGui::Combo("Camera mode: ", &mCameraMode, options.data(),
            ((int)options.size()));

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
            1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();

        mSpline.drawGui();
        ImGui::Render();
    }
}
