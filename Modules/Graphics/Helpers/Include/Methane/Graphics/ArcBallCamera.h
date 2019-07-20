/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Graphics/ArcBallCamera.h
Arc-ball camera implementation.

******************************************************************************/

#pragma once

#include "Camera.h"

#include <Methane/Data/Types.h>

namespace Methane
{
namespace Graphics
{

class ArcBallCamera : public Camera
{
public:
    enum class Pivot : uint32_t
    {
        Aim = 0,
        Eye,
    };

    using DistanceRange = std::pair<float /*min_distance*/, float /*max_distance*/>;

    ArcBallCamera(Pivot pivot = Pivot::Aim, cml::AxisOrientation axis_orientation = g_axis_orientation);
    ArcBallCamera(const Camera& view_camera, Pivot pivot = Pivot::Aim, cml::AxisOrientation axis_orientation = g_axis_orientation);

    Pivot GetPivot() const                      { return m_pivot; }

    float GetRadiusRatio() const                { return m_radius_ratio; }
    void  SetRadiusRatio(float radius_ratio)    { m_radius_ratio = radius_ratio; }
    float GetRadiusInPixels() const noexcept    { return GetRadiusInPixels(m_screen_size); }

    float GetZoomStepsCount() const             { return m_zoom_steps_count; }
    void  SetZoomStepsCount(float steps_count)  { m_zoom_steps_count = steps_count;}

    const DistanceRange& GetZoomDistanceRange() const               { return m_zoom_distance_range; }
    void SetZoomDistanceRange(const DistanceRange& distance_range)  { m_zoom_distance_range = distance_range; }

    void OnMousePressed(const Data::Point2i& mouse_screen_pos);
    void OnMouseDragged(const Data::Point2i& mouse_screen_pos);
    void OnMouseScrolled(float scroll_delta);

protected:
    Vector3f GetNormalizedSphereProjection(const Data::Point2i& mouse_screen_pos, bool is_primary) const;

    inline float GetRadiusInPixels(const Data::Point2f& screen_size) const noexcept
    { return std::min(screen_size.x(), screen_size.y()) * m_radius_ratio / 2.f; }

    void ApplyLookDirection(const Vector3f& look_dir, const Orientation& base_orientation);
    void ApplyLookDirection(const Vector3f& look_dir) { return ApplyLookDirection(look_dir, m_current_orientation);  }

    const Camera*   m_p_view_camera;
    const Pivot     m_pivot;
    float           m_radius_ratio              = 0.9f;
    uint32_t        m_zoom_steps_count          = 10;
    DistanceRange   m_zoom_distance_range       = DistanceRange(1.f, 1000.f);
    Vector3f        m_mouse_pressed_on_sphere   = { };
    Orientation     m_mouse_pressed_orientation = { };
};

} // namespace Graphics
} // namespace Methane
