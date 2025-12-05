add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"}) 

set_languages("c++17")
set_warnings("all")

add_requires("raylib")

target("buffered-raylib")
    set_kind("static")

    add_files(
        "src/BufferedRaylib.cpp"
    )

    add_includedirs(
        "src", 
        {public = true}
    )

    add_packages(
        "raylib", 
        {public=true}
    )
target_end()

target("example")
    set_kind("binary")

    add_files(
        "examples/test.cpp"
    )

    add_deps("buffered-raylib")
target_end()