function ar(toolchainPathPrefix)
{
    return File.exists(toolchainPathPrefix + "/gcc-ar") ? "gcc-ar" : "ar";
}
