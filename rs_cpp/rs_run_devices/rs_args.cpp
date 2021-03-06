#include "rs_args.hpp"

rs2args::rs2args() : argparser()
{
}

rs2args::rs2args(int argc, char *argv[]) : argparser(argc, argv)
{
}

rs2args::~rs2args()
{
}

int rs2args::steps()
{
    return std::stoi(getarg("--steps"));
}

int rs2args::fps()
{
    return std::stoi(getarg("--fps"));
}

int rs2args::height()
{
    return std::stoi(getarg("--height"));
}

int rs2args::width()
{
    return std::stoi(getarg("--width"));
}

rs2_format rs2args::color_format()
{
    if (_SUPPORTED_FORMATS.count(getarg("--color-format")))
        return _SUPPORTED_FORMATS[getarg("--color-format")];
    else
        throw std::invalid_argument("rs color format unknown");
}

rs2_format rs2args::depth_format()
{
    if (_SUPPORTED_FORMATS.count(getarg("--depth-format")))
        return _SUPPORTED_FORMATS[getarg("--depth-format")];
    else
        throw std::invalid_argument("rs depth format unknown");
}

std::string rs2args::save_path()
{
    return getarg("--save-path");
}

std::string rs2args::ip()
{
    return getarg("--ip");
}

bool rs2args::network()
{
    if (checkarg("--ip"))
        return true;
    else
        return false;
}

int rs2args::flush_steps()
{
    if (checkarg("--flush-steps"))
        return std::stoi(getarg("--flush-steps"));
    else
        return 30;
}

int rs2args::reset_interval()
{
    if (checkarg("--reset-interval"))
        return std::stoi(getarg("--reset-interval"));
    else
        return 120;
}

void rs2args::print_args()
{
    printout();
}

bool rs2args::check_rs2args()
{
    for (auto &&arg : _REQUIRED_ARGS)
    {
        if (!checkarg(arg))
        {
            print(arg + " is missing", 2);
            return EXIT_FAILURE;
        }
    }
    for (auto &&arg : _OPTIONAL_ARGS)
        if (!checkarg(arg))
            print(arg + " is not used", 1);
    return EXIT_SUCCESS;
}