const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const replacement = `    exe.addIncludePath(sdl_dep.path("include"));
    const imgui_dep = b.dependency("imgui", .{ .target = target, .optimize = optimize });
    const json_dep = b.dependency("json", .{});
    const httplib_dep = b.dependency("cpp_httplib", .{});

    exe.addIncludePath(imgui_dep.path(""));
    exe.addCSourceFile(.{
        .file = b.path("third_party/sqlite/sqlite3.c"),
        .flags = &[_][]const u8{"-O2"},
    });

    const imgui_files = &[_][]const u8{
        "misc/cpp/imgui_stdlib.cpp",
        "imgui.cpp",
        "imgui_draw.cpp",
        "imgui_tables.cpp",
        "imgui_widgets.cpp",
        "backends/imgui_impl_sdl3.cpp",
        "backends/imgui_impl_sdlrenderer3.cpp",
    };

    for (imgui_files) |file| {
        exe.addCSourceFile(.{
            .file = imgui_dep.path(file),
            .flags = cpp_flags,
        });
    }

    exe.addIncludePath(json_dep.path("include"));
    exe.addIncludePath(httplib_dep.path(""));
    exe.addIncludePath(b.path("src"));
    exe.addIncludePath(imgui_dep.path("misc/cpp"));
    exe.addIncludePath(b.path("third_party"));
    exe.addIncludePath(b.path("third_party/sqlite"));`;

code = code.replace(/    exe\.addIncludePath\(sdl_dep\.path\("include"\)\);([\s\S]*?)exe\.addIncludePath\(b\.path\("third_party\/sqlite"\)\);/, replacement);

fs.writeFileSync('build.zig', code);
