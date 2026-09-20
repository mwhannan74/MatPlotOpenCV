#include "figure.h"

#include <opencv2/core/utils/logger.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

int main()
{
    int failures = 0;

    const auto original_log_level = cv::utils::logging::getLogLevel();
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_DEBUG);
    mpocv::Figure logging_test_figure;
    const auto log_level_after_construction = cv::utils::logging::getLogLevel();
    cv::utils::logging::setLogLevel(original_log_level);

    if (log_level_after_construction != cv::utils::logging::LOG_LEVEL_DEBUG)
    {
        std::cerr << "FAIL: Figure construction changed the OpenCV log level\n";
        ++failures;
    }

    mpocv::Figure figure;

    const auto expect_invalid_argument = [&failures](const char* name, auto&& call)
    {
        try
        {
            call();
            std::cerr << "FAIL: " << name << " did not throw std::invalid_argument\n";
            ++failures;
        }
        catch (const std::invalid_argument&)
        {
        }
        catch (const std::exception& error)
        {
            std::cerr << "FAIL: " << name << " threw " << error.what() << '\n';
            ++failures;
        }
        catch (...)
        {
            std::cerr << "FAIL: " << name << " threw an unexpected exception\n";
            ++failures;
        }
    };

    const std::vector<double> one_value{ 0.0 };
    const std::vector<double> two_values{ 0.0, 1.0 };

    expect_invalid_argument("plot lvalue overload", [&]
        {
            figure.plot(two_values, one_value);
        });
    expect_invalid_argument("plot rvalue overload", [&]
        {
            figure.plot(std::vector<double>{ 0.0 }, std::vector<double>{ 0.0, 1.0 });
        });
    expect_invalid_argument("scatter lvalue overload", [&]
        {
            figure.scatter(one_value, two_values);
        });
    expect_invalid_argument("scatter rvalue overload", [&]
        {
            figure.scatter(std::vector<double>{ 0.0, 1.0 }, std::vector<double>{ 0.0 });
        });

    const auto check_equal_scale = [&failures](int width, int height,
        const char* image_path, const char* case_name)
    {
        mpocv::Figure equal_scale_figure(width, height);
        equal_scale_figure.plot({ 0.0, 1.0 }, { 0.0, 0.0 }, mpocv::Color::Blue(), 2.0f);
        equal_scale_figure.plot({ 0.0, 0.0 }, { 0.0, 1.0 }, mpocv::Color::Red(), 2.0f);
        equal_scale_figure.equal_scale(true);
        equal_scale_figure.save(image_path);

        const cv::Mat equal_scale_image = cv::imread(image_path);
        std::remove(image_path);

        if (equal_scale_image.empty())
        {
            std::cerr << "FAIL: " << case_name << " image was not created\n";
            ++failures;
            return;
        }

        cv::Mat blue_mask;
        cv::Mat red_mask;
        cv::inRange(equal_scale_image, cv::Scalar(201, 0, 0), cv::Scalar(255, 79, 79), blue_mask);
        cv::inRange(equal_scale_image, cv::Scalar(0, 0, 201), cv::Scalar(79, 79, 255), red_mask);

        if (cv::countNonZero(blue_mask) == 0 || cv::countNonZero(red_mask) == 0)
        {
            std::cerr << "FAIL: " << case_name << " did not contain both test lines\n";
            ++failures;
            return;
        }

        const int horizontal_length = cv::boundingRect(blue_mask).width;
        const int vertical_length = cv::boundingRect(red_mask).height;
        if (std::abs(horizontal_length - vertical_length) > 4)
        {
            std::cerr << "FAIL: " << case_name << " rendered unequal unit lengths (x="
                << horizontal_length << ", y=" << vertical_length << ")\n";
            ++failures;
        }
    };

    check_equal_scale(400, 300, "matplotopencv_equal_scale_wide.png", "wide equal-scale figure");
    check_equal_scale(300, 400, "matplotopencv_equal_scale_tall.png", "tall equal-scale figure");

    const auto check_ellipse_bounds = [&failures](double angle_degrees,
        const char* image_path, const char* case_name)
    {
        mpocv::ShapeStyle style;
        style.line_color = mpocv::Color::Blue();
        style.thickness = 2.0f;
        style.fill_alpha = 0.0f;

        mpocv::Figure ellipse_figure(400, 300);
        ellipse_figure.ellipse(0.0, 0.0, 4.0, 1.0, angle_degrees, style);
        ellipse_figure.equal_scale(true);
        ellipse_figure.save(image_path);

        const cv::Mat ellipse_image = cv::imread(image_path);
        std::remove(image_path);

        if (ellipse_image.empty())
        {
            std::cerr << "FAIL: " << case_name << " image was not created\n";
            ++failures;
            return;
        }

        cv::Mat blue_mask;
        cv::inRange(ellipse_image, cv::Scalar(201, 0, 0), cv::Scalar(255, 79, 79), blue_mask);
        if (cv::countNonZero(blue_mask) == 0)
        {
            std::cerr << "FAIL: " << case_name << " did not contain the ellipse\n";
            ++failures;
            return;
        }

        const cv::Rect bounds = cv::boundingRect(blue_mask);
        const cv::Rect plot_area(60, 40, 320, 200);
        if ((bounds & plot_area) != bounds)
        {
            std::cerr << "FAIL: " << case_name << " extended outside the plot area\n";
            ++failures;
        }
    };

    check_ellipse_bounds(0.0, "matplotopencv_ellipse_0.png", "0-degree ellipse");
    check_ellipse_bounds(45.0, "matplotopencv_ellipse_45.png", "45-degree ellipse");
    check_ellipse_bounds(90.0, "matplotopencv_ellipse_90.png", "90-degree ellipse");

    mpocv::Figure manual_limits_figure(400, 300);
    manual_limits_figure.set_xlim(0.0, 10.0);
    manual_limits_figure.set_ylim(0.0, 10.0);
    manual_limits_figure.axis_pad(0.1);
    manual_limits_figure.plot({ 2.0, 8.0 }, { 5.0, 5.0 }, mpocv::Color::Blue(), 2.0f);

    const auto save_blue_bounds = [&failures, &manual_limits_figure](const char* image_path)
    {
        manual_limits_figure.save(image_path);
        const cv::Mat image = cv::imread(image_path);
        std::remove(image_path);

        if (image.empty())
        {
            std::cerr << "FAIL: manual-limits test image was not created\n";
            ++failures;
            return cv::Rect{};
        }

        cv::Mat blue_mask;
        cv::inRange(image, cv::Scalar(201, 0, 0), cv::Scalar(255, 79, 79), blue_mask);
        if (cv::countNonZero(blue_mask) == 0)
        {
            std::cerr << "FAIL: manual-limits test image did not contain the line\n";
            ++failures;
            return cv::Rect{};
        }
        return cv::boundingRect(blue_mask);
    };

    const cv::Rect bounds_before_redraw =
        save_blue_bounds("matplotopencv_manual_limits_before.png");
    manual_limits_figure.title("Redraw");
    const cv::Rect bounds_after_redraw =
        save_blue_bounds("matplotopencv_manual_limits_after.png");

    if (bounds_before_redraw != bounds_after_redraw)
    {
        std::cerr << "FAIL: manual axis limits changed across renders\n";
        ++failures;
    }

    if (failures != 0)
        return 1;

    std::cout << "All Figure tests passed\n";
    return 0;
}
