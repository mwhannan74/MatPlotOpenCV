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

    if (failures != 0)
        return 1;

    std::cout << "All Figure tests passed\n";
    return 0;
}
