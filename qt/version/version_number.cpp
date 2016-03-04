#include "version_number.h"


const VersionNumber& productVersion()
{
    const static VersionNumber version(VERSION_PROJECT_MAJOR,
                                       VERSION_PROJECT_MINOR,
                                       VERSION_PROJECT_PATCH);
    return version;
}

const VersionNumber& minCompatibleVersion()
{
    const static VersionNumber version(VERSION_MIN_COMPATIBLE_MAJOR,
                                       VERSION_MIN_COMPATIBLE_MINOR,
                                       VERSION_MIN_COMPATIBLE_PATCH);
    return version;
}

VersionNumber::VersionNumber(quint8 major, quint8 minor, quint8 patch)

{
    this->major = major;
    this->minor = minor;
    this->patch = patch;
    this->build = 0;
}

QString VersionNumber::toString() const
{
    return QString("%1.%2.%3").arg(major).arg(minor).arg(patch);
}

bool operator== (const VersionNumber v1, const VersionNumber v2)
{
    return (v1.vers == v2.vers);
}

bool operator!= (const VersionNumber v1, const VersionNumber v2)
{
    return (v1.vers != v2.vers);
}

static int compare(const VersionNumber v1, const VersionNumber v2)
{
    #define COMPARE(f1, f2) if (f1 != f2) return (f1 < f2) ? -1 : 1;
    COMPARE(v1.major, v2.major)
    COMPARE(v1.minor, v2.minor)
    COMPARE(v1.patch, v2.patch)
    #undef COMPARE
    return 0;
}

bool operator> (const VersionNumber v1, const VersionNumber v2)
{
    return (compare(v1, v2) == 1);
}

bool operator< (const VersionNumber v1, const VersionNumber v2)
{
    return (compare(v1, v2) == -1);
}

bool operator>= (const VersionNumber v1, const VersionNumber v2)
{
    return (v1 > v2) || (v1 == v2);
}

bool operator<= (const VersionNumber v1, const VersionNumber v2)
{
    return (v1 < v2) || (v1 == v2);
}
