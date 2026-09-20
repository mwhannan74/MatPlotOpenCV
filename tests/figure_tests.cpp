#include "figure.h"

#include <opencv2/core/utils/logger.hpp>

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

    if (failures != 0)
        return 1;

    std::cout << "All Figure tests passed\n";
    return 0;
}
