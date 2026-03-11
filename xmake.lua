set_xmakever("2.9.8")

set_project("uwvm2_vulkan_wrapper")
set_version("0.1.0")

add_defines("UWVM2_VULKAN_VERSION_X=0")
add_defines("UWVM2_VULKAN_VERSION_Y=1")
add_defines("UWVM2_VULKAN_VERSION_Z=0")

set_allowedplats("windows", "mingw", "linux", "unix", "bsd", "freebsd", "dragonflybsd", "netbsd", "openbsd", "macosx")
set_defaultmode("release")
set_allowedmodes("debug", "release", "minsizerel", "releasedbg")

includes("xmake/impl.lua")

def_build()
