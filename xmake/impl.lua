includes("option.lua")

local weak_symbol_platforms = {
    "linux",
    "bsd",
    "freebsd",
    "dragonflybsd",
    "netbsd",
    "openbsd",
}

local core_sources = {
    "src/runtime/memory_access.cc",
    "src/runtime/plugin_context.cc",
    "src/vulkan/dynamic_backend.cc",
    "src/vulkan/api.cc",
    "src/plugin/module_exports.cc",
}

function _is_weak_symbol_platform()
    return is_plat(table.unpack(weak_symbol_platforms))
end

function _configure_modes()
    add_rules("mode.debug", "mode.release", "mode.minsizerel", "mode.releasedbg")
end

function _configure_compilation()
    set_languages("c17", "cxx20")
    set_encodings("utf-8")
    set_policy("build.warning", true)

    if get_config("warnings-as-errors") then
        set_warnings("all", "extra", "pedantic", "error")
    else
        set_warnings("all", "extra", "pedantic")
    end

    if is_mode("debug") then
        add_defines("UWVM2_VULKAN_DEBUG")
    end

    if get_config("fno-exceptions") then
        set_exceptions("no-cxx")
    end

    local use_stdlib = get_config("stdlib")
    if use_stdlib and use_stdlib ~= "default" then
        local stdlib_flag = "-stdlib=" .. use_stdlib
        add_cxflags(stdlib_flag)
        add_ldflags(stdlib_flag)
    end

    add_defines("UWVM2_VULKAN_SUPPORT_PRELOAD_DL")
    if _is_weak_symbol_platform() then
        add_defines("UWVM2_VULKAN_SUPPORT_WEAK_SYMBOL")
    end

    add_includedirs("include", { public = true })
    add_includedirs("src")
end

function _apply_common_target_settings()
    if is_plat("linux", "bsd", "freebsd", "dragonflybsd", "netbsd", "openbsd") then
        add_syslinks("dl")
    end
end

function _build_core_target()
    target("uwvm2_vulkan_plugin_core")
        set_kind("static")
        add_files(core_sources)
        add_headerfiles("include/(uwvm2_vulkan/**.h)")
        _apply_common_target_settings()
end

function _build_dl_target()
    target("uwvm2_vulkan_plugin_dl")
        set_kind("shared")
        add_deps("uwvm2_vulkan_plugin_core")
        add_files("src/plugin/dl_exports.cc")
        add_defines("UWVM2_VULKAN_BUILD_SHARED")
        if not is_plat("windows", "mingw") then
            add_cxxflags("-fvisibility=hidden")
        end
        _apply_common_target_settings()
end

function _build_weak_target()
    target("uwvm2_vulkan_plugin_weak")
        set_kind("object")
        add_files(core_sources)
        add_files("src/plugin/weak_exports.cc")
        if _is_weak_symbol_platform() and get_config("merge-weak-object") ~= false then
            after_build(function(target)
                local output = path.join(target:targetdir(), "uwvm2_vulkan_plugin_weak.o")
                os.vrunv(target:tool("ld"), {"-r", "-o", output, table.unpack(target:objectfiles())})
            end)
        end
end

function _build_test_targets()
    if not get_config("build-tests") then
        return
    end

    target("guest_header_smoke")
        set_kind("object")
        set_languages("c17")
        add_files("test/guest_header_smoke.c")
        add_headerfiles("include/(uwvm2_vulkan/**.h)")
        add_cflags("-Wno-unused-function")

    target("memory_access_test")
        set_kind("binary")
        add_deps("uwvm2_vulkan_plugin_core")
        add_files("test/memory_access_test.cc")
        _apply_common_target_settings()

    target("module_registry_test")
        set_kind("binary")
        add_deps("uwvm2_vulkan_plugin_core")
        add_files("test/module_registry_test.cc")
        _apply_common_target_settings()

    target("wasi_file_system_test")
        set_kind("binary")
        add_deps("uwvm2_vulkan_plugin_core")
        add_files("test/wasi_file_system_test.cc")
        _apply_common_target_settings()

    target("vulkan_api_test")
        set_kind("binary")
        add_deps("uwvm2_vulkan_plugin_core")
        add_files("test/vulkan_api_test.cc")
        _apply_common_target_settings()
end

function def_build()
    _configure_modes()
    _configure_compilation()
    _build_core_target()
    _build_dl_target()
    _build_weak_target()
    _build_test_targets()
end
