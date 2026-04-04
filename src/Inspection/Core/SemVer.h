#ifndef SEMVER_H
#define SEMVER_H

#include <regex>
#include <sstream>
#include <string>

// SemVer struct to represent Semantic Versioning 2.0.0
// For more information, visit: https://semver.org/
struct SemVer
{
    int major;
    int minor;
    int patch;
    std::string preRelease;
    std::string buildMetadata;

    inline SemVer(int maj = 0, int min = 0, int pat = 0, const std::string &pre = "", const std::string &build = "") :
        major(maj),
        minor(min),
        patch(pat),
        preRelease(pre),
        buildMetadata(build)
    {
    }

    // Convert SemVer object to string representation
    inline std::string toString() const
    {
        std::ostringstream oss;
        oss << major << "." << minor << "." << patch;
        if (!preRelease.empty())
            oss << "-" << preRelease;

        if (!buildMetadata.empty())
            oss << "+" << buildMetadata;

        return oss.str();
    }

    // Parse a SemVer string into a SemVer object
    // Returns a SemVer object and sets success flag if provided
    static SemVer parse(const std::string &version, bool *success = nullptr)
    {
        static const std::regex semverRegex(R"(^(\d+)\.(\d+)\.(\d+)(?:-([\w\.-]+))?(?:\+([\w\.-]+))?$)");

        std::smatch match;
        if (!std::regex_match(version, match, semverRegex))
        {
            if (success)
                *success = false;

            return SemVer();
        }

        if (success)
            *success = true;

        return SemVer(std::stoi(match[1].str()), std::stoi(match[2].str()), std::stoi(match[3].str()), match[4].str(), match[5].str());
    }
};

// #include <Core/SemVer.h>

// #include <iostream>

// int main()
// {
//     std::string versionStr = "1.2.3-alpha+001";

//     bool success;
//     SemVer version = SemVer::parse(versionStr, &success);
//     if (success)
//         std::cout << "Parsed version: " << version.toString() << std::endl;
//     else
//         std::cerr << "Error: Invalid SemVer string" << std::endl;

//     SemVer newVersion(2, 0, 0, "beta", "exp.sha.5114f85");
//     std::cout << "New version: " << newVersion.toString() << std::endl;

//     return 0;
// }

#endif // SEMVER_H
