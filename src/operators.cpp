#include "operators.hpp"
#include <cmath>
#include <iostream>
#include <map>
#include <numeric>
#include <sstream>
#include <string>
/* explicit instantiation declaration */
template float operators::sum<float> (std::vector<float> values);
template double operators::sum<double> (std::vector<double> values);

operators::operators () {
}

operators::~operators () {
}

float operators::classify_scale (const points_t& reference_points, const points_t& data_points) {
    points_t points = data_points;
    float scale     = 1;
    match_points (reference_points, points);
    if (points.size () <= _minimum_ref_points || points.size () > _maximum_ref_points) {
        return scale;
    }
    // centroid
    point_t centroid_r = centroid (reference_points);
    point_t centroid_p = centroid (points);

    // two keypoints to use
    keypoint_t ref_A = { reference_points.begin ()->first,
        reference_points.begin ()->second };
    // keypoint_t ref_B = {};
    std::map<unsigned int, float> optimal_angle_ratios; // <point ID, ratio>

    // 1.0 means a perfect 45 degrees through A
    point_t diff = {};
    for (auto point : reference_points) {
        if (ref_A.id != point.first) {
            diff.x = (point.second.x - ref_A.p.x);
            diff.y = (point.second.y - ref_A.p.y);
            if (diff.y != 0) {
                optimal_angle_ratios.insert (
                { point.first, std::fabs (diff.x / diff.y) });
            } else {
                optimal_angle_ratios.insert ({ point.first, 0 });
            }
        }
    }
    // find closest to optimal_ratio
    const float optimal_ratio  = 1.0;
    unsigned int optimal_point = optimal_angle_ratios.begin ()->first;
    for (auto ratio : optimal_angle_ratios) {
        optimal_point = std::fabs (ratio.second - optimal_ratio) <
        std::fabs (optimal_angle_ratios.find (optimal_point)->second - optimal_ratio) ?
        ratio.first :
        optimal_point;
    }
    keypoint_t ref_B = { optimal_point, reference_points.find (optimal_point)->second };

    // find intersections
    point_t intersection_ref   = intersections (ref_A.p, ref_B.p, centroid_r);
    point_t intersection_ref_x = { intersection_ref.x, 0 };
    point_t intersection_ref_y = { 0, intersection_ref.y };
    // ratio of intersection
    // <------->
    // <--->
    // A   X   B
    // X
    // calculate vector
    // Y
    float ratio_ref_a_x_b;
    float ratio_ref_a_y_b;

    // [from reference]
    // find line(set of 2 points) that crosses x and y axis at some point
    // make sure that this line is as close to 45 degrees to the centroid
    // so that an overflow isn't bound to happen due to very large values
    //
    // with these points, solve Y=AX+B
    // (intersection points with X and Y axis)
    // where the centroid is the origin
    //
    // note the ratio where the intersection lays
    // on the line between the two points
    //
    // [from data]
    // using the two points and the ratio, look for the x/y intersection
    // and compare the vectors from origin to intersections
    // The one wit the difference can be used to calculate the scale
    // (new size/old size)
    //
    // for reference
    // pick always first point (let's call it A)
    //  for all points
    //  calculate the mean x, y difference from point A
    //  pick point with highest mean difference
    //  solve Y=AX+B
    //  solve Y=0
    //   note ratio of vectors between points
    //  and X=0
    //   note ratio of vectors between points
    //
    // calculate x, y location of x axis
    // calculate x, y location of y axis
    //
    // calculate vector x axis
    // calculate vector y axis
    //
    // line with smallest difference can be used for scale

    return scale;
}

translation_t operators::classify_translation (const points_t& reference_points,
const points_t& data_points) {
    translation_t translation = { 0, 0 };
    points_t points           = data_points;
    match_points (reference_points, points);
    if (points.size () <= _minimum_ref_points) {
        return translation;
    }
    point_t centroid_reference = centroid (reference_points);
    point_t centroid_points    = centroid (points);
    translation.x              = centroid_points.x - centroid_reference.x;
    translation.y              = centroid_points.y - centroid_reference.y;
    return translation;
}

float operators::classify_yaw (const points_t& reference_points, const points_t& data_points) {
    return 0;
}

float operators::classify_pitch (const points_t& reference_points, const points_t& data_points) {
    return 0;
}

float operators::classify_roll (const points_t& reference_points, const points_t& data_points) {
    return 0;
}

point_t operators::centroid (const points_t& points) {
    point_t centroid = { 0, 0 };
    if (points.size () && points.size () <= _maximum_ref_points) {
        centroid = sum (points);
        centroid.x /= points.size ();
        centroid.y /= points.size ();
    }
    return centroid;
}

template <typename T> T operators::sum (std::vector<T> values) {
    kahan_accumulation<T> init;
    kahan_accumulation<T> result = std::accumulate (values.begin (),
    values.end (), init, [](kahan_accumulation<T> accumulation, T value) {
        kahan_accumulation<T> result;
        T y               = value - accumulation.correction;
        T t               = accumulation.sum + y;
        result.correction = (t - accumulation.sum) - y;
        result.sum        = t;
        return result;
    });
    return result.sum;
}

point_t operators::sum (const points_t& points) {
    point_t keypoint_sum     = {};
    std::vector<float> vec_x = {};
    std::vector<float> vec_y = {};
    for (auto point : points) {
        vec_x.push_back (point.second.x);
        vec_y.push_back (point.second.y);
    }
    keypoint_sum.x = sum<float> (vec_x);
    keypoint_sum.y = sum<float> (vec_y);
    return keypoint_sum;
}

point_t operators::intersections (point_t p1, point_t p2, point_t origin) {
    point_t I = { 0, 0 }; // intersection
    // normalize using the origin
    // p1 = { p1.x - origin.x, p1.y - origin.y };
    // p2 = { p2.x - origin.x, p2.y - origin.y };
    // Y = AX + B
    float At = p2.y - p1.y;
    float Ab = p2.x - p1.x;
    // A
    float A;
    try {
        if (Ab == 0) {
            throw std::overflow_error ("Divide by zero exception");
        }
        A = At / Ab;
        if (A == 0) {
            throw std::overflow_error ("Never crosses X axes");
        }
    } catch (std::overflow_error) {
        return I;
    }
    // B
    float B = p1.y - A * p1.x;
    // intersection X axis; Y = 0
    // x = (y - B) / A
    I.x = (0 - B) / A;
    // intersection Y axis; X = 0
    // y = Ax+b = b
    I.y = B;
    // de-normalize using the origin
    I = { I.x - origin.x, I.y - origin.y };
    return I;
}

void operators::match_points (const points_t& reference_points, points_t& data_points) {
    for (auto point = data_points.begin (); point != data_points.end ();) {
        if (reference_points.find (point->first) == reference_points.end ()) {
            point = data_points.erase (point);
        } else {
            ++point;
        }
    }
}

template <typename T> std::string operators::to_string (T value) {
    std::ostringstream os;
    os << value;
    return os.str ();
}
