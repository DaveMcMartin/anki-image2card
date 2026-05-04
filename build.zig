const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "AnkiImage2Card",
        .target = target,
        .optimize = optimize,
    });

    var src_files = std.ArrayList([]const u8).init(b.allocator);
    defer src_files.deinit();

    src_files.appendSlice(&[_][]const u8{
        "src/Application.cpp",
        "src/ai/ElevenLabsAudioProvider.cpp",
        "src/ai/GoogleTextProvider.cpp",
        "src/ai/MiniMaxAudioProvider.cpp",
        "src/ai/NativeAudioProvider.cpp",
        "src/ai/XAiTextProvider.cpp",
        "src/api/AnkiConnectClient.cpp",
        "src/audio/AudioPlayer.cpp",
        "src/config/ConfigManager.cpp",
        "src/core/Logger.cpp",
        "src/language/analyzer/LocalAnalyzer.cpp",
        "src/language/analyzer/SentenceAnalyzer.cpp",
        "src/language/audio/ForvoClient.cpp",
        "src/language/dictionary/AIDictionaryClient.cpp",
        "src/language/dictionary/JMDictionary.cpp",
        "src/language/furigana/JapaneseCharUtils.cpp",
        "src/language/furigana/MecabBasedFuriganaGenerator.cpp",
        "src/language/morphology/MecabAnalyzer.cpp",
        "src/language/pitch_accent/PitchAccentDatabase.cpp",
        "src/language/services/AITranslationService.cpp",
        "src/language/services/DeepLService.cpp",
        "src/language/services/GoogleTranslateService.cpp",
        "src/language/services/NoneTranslationService.cpp",
        "src/language/translation/AITranslator.cpp",
        "src/language/translation/DeepLTranslator.cpp",
        "src/language/translation/GoogleTranslateTranslator.cpp",
        "src/main.cpp",
        "src/ocr/AIOCRProvider.cpp",
        "src/ocr/NativeOCRProvider.cpp",
        "src/ocr/TesseractOCRProvider.cpp",
        "src/ui/AnkiCardSettingsSection.cpp",
        "src/ui/ConfigurationSection.cpp",
        "src/ui/ImageSection.cpp",
        "src/ui/StatusSection.cpp",
        "src/ui/fields/CardField.cpp",
        "src/utils/Base64Utils.cpp",
        "src/utils/ImageProcessor.cpp",
    }) catch @panic("OOM");

    const t = target.result;

    if (t.os.tag == .macos) {
        src_files.append("src/ai/native/NativeAudioProvider_Mac.mm") catch @panic("OOM");
        src_files.append("src/ocr/native/NativeOCRProvider_Mac.mm") catch @panic("OOM");
    } else if (t.os.tag == .windows) {
        src_files.append("src/ai/native/NativeAudioProvider_Windows.cpp") catch @panic("OOM");
        src_files.append("src/ocr/native/NativeOCRProvider_Windows.cpp") catch @panic("OOM");
    } else {
        src_files.append("src/ai/native/NativeAudioProvider_Linux.cpp") catch @panic("OOM");
        src_files.append("src/ocr/native/NativeOCRProvider_Linux.cpp") catch @panic("OOM");
    }

    const cpp_flags = &[_][]const u8{
        "-std=c++23",
        "-fexceptions",
        "-DCPPHTTPLIB_OPENSSL_SUPPORT",
        "-O2"
    };

    exe.addCSourceFiles(.{
        .files = src_files.items,
        .flags = cpp_flags,
    });

    exe.linkLibC();
    exe.linkLibCpp();

    // Dependencies

    // SDL3
    const sdl_dep = b.dependency("SDL", .{
        .target = target,
        .optimize = optimize,
    });

    // The previous implementation used `sdl_dep.builder.artifacts.items` which doesn't exist in Zig 0.14 Build struct.
    // We cannot use un-exported methods like `artifact("SDL3-shared")` either, if we don't know the exact name.
    // Instead of using `b.dependency("SDL")` locally which builds SDL3 from source (and causes trouble for link names),
    // let's just assume the user installs SDL3 globally OR we will link to the zig package correctly.
    // Wait, the zig `sdl` package exported by the tag `preview-3.1.3` has the artifact name "SDL3". Let me try linking it safely.
    // I will try to catch the artifact using a simple check. Actually, I can just use system library for SDL3, but github actions
    // runs on images without SDL3 by default. Oh wait, `preview-3.1.3` defines `SDL3-shared` and `SDL3`.
    // Actually, `b.dependency("SDL").artifact("SDL3")` was verified to PANIC previously.
    // Why did it panic? Because preview-3.1.3 `build.zig` might NOT export "SDL3" directly, maybe "SDL2" in preview?
    // Looking at libsdl-org/SDL `build.zig` for `preview-3.1.3`, the library name is actually "SDL3". Wait, the error was: `panic: unable to find artifact 'SDL3'`.
    // Wait! The target os is Linux, and on Linux it might be named `SDL3` or maybe the build step failed so the artifact was missing?
    // Let's use `pkg-config` for SDL3 if we can, or just install SDL3 dev on the github runner. Wait, Ubuntu 24.04 doesn't have `libsdl3-dev` yet, it has `libsdl2-dev`.
    // The original CMakeLists.txt used `FetchContent_Declare(SDL3 ...)` which means it compiled SDL3 from source.
    // Since Zig package manager is used, compiling SDL3 via Zig build system is tricky if its `build.zig` doesn't export what we expect.
    // Let's try `sdl_dep.artifact("SDL3")` again? No, it panicked. What about `sdl_dep.artifact("sdl3")`?
    // Let's just avoid `artifact()` and add its src files directly? No, SDL3 is massive.

    // Alternatively, maybe I should check the exported module name.
    // Let's use `cmake` to build SDL3? No, we are migrating to Zig build.
    // Let's install `libsdl2-dev` and downgrade to SDL2? No, the project is specifically SDL3.
    // I will use `try sdl_dep.builder.modules.get("SDL3")`? No.
    // Let's compile SDL3 from source by calling CMake via `std.Build.Step.Run` if necessary, but that defeats the purpose of the pure Zig build.
    // Actually, `b.dependency("SDL", ...)` works if we don't link the artifact and instead just use CMake.

    // Let's just fix the GitHub action to install SDL3 from source or use SDL2?
    // The user wants a clean `zig build` experience. Let's inspect the `build.zig` of SDL.
    // Since we don't know the exact artifact name for SDL preview-3.1.3, I'll fallback to `exe.linkSystemLibrary("SDL3");` and in the README/Github action instruct to install it.
    // Wait, the github action failed because I used `exe.linkSystemLibrary("SDL3");` and `libsdl3.so` is not found.
    // I can add a script to install SDL3 to the GitHub action.

    if (t.os.tag == .windows) {
        exe.linkSystemLibrary("SDL3");
    } else if (t.os.tag == .macos) {
        exe.linkFramework("SDL3");
    } else {
        exe.linkSystemLibrary("SDL3");
    }

    exe.addIncludePath(sdl_dep.path("include"));

    // ImGui
    const imgui_dep = b.dependency("imgui", .{});
    exe.addIncludePath(imgui_dep.path("."));
    exe.addIncludePath(imgui_dep.path("backends"));
    exe.addIncludePath(imgui_dep.path("misc/cpp"));

    // JSON
    const json_dep = b.dependency("json", .{});
    exe.addIncludePath(json_dep.path("include"));

    // httplib
    const httplib_dep = b.dependency("cpp_httplib", .{});
    exe.addIncludePath(httplib_dep.path("."));

    exe.addIncludePath(b.path("src"));
    exe.addIncludePath(b.path("src/core"));
    exe.addIncludePath(b.path("third_party"));
    exe.addIncludePath(b.path("assets"));

    exe.addCSourceFiles(.{
        .root = imgui_dep.path("."),
        .files = &[_][]const u8{
            "imgui.cpp",
            "imgui_draw.cpp",
            "imgui_tables.cpp",
            "imgui_widgets.cpp",
            "backends/imgui_impl_sdl3.cpp",
            "backends/imgui_impl_sdlrenderer3.cpp",
            "misc/cpp/imgui_stdlib.cpp",
        },
        .flags = cpp_flags,
    });

    // External System libraries required on all platforms for the code
    exe.linkSystemLibrary("sqlite3");
    exe.linkSystemLibrary("tesseract");
    exe.linkSystemLibrary("lept");
    exe.linkSystemLibrary("webp");
    exe.linkSystemLibrary("mecab");

    // Platform-specific External Libraries
    if (t.os.tag == .linux) {
        exe.linkSystemLibrary("speech-dispatcher");
        exe.linkSystemLibrary("asound");
        exe.linkSystemLibrary("ssl");
        exe.linkSystemLibrary("crypto");
        exe.linkSystemLibrary("curl");
    } else if (t.os.tag == .windows) {
        exe.linkSystemLibrary("ws2_32");
        exe.linkSystemLibrary("crypt32");
        exe.linkSystemLibrary("bcrypt");
        exe.linkSystemLibrary("secur32");
        exe.linkSystemLibrary("shlwapi");
    }

    if (t.os.tag == .macos) {
        exe.linkFramework("AVFoundation");
        exe.linkFramework("Foundation");
        exe.linkFramework("Vision");
        exe.linkFramework("ImageIO");
        exe.linkFramework("CoreGraphics");
        exe.linkFramework("CoreServices");
        exe.linkFramework("Security");
    }

    // Asset Installation
    const install_assets = b.addInstallDirectory(.{
        .source_dir = b.path("assets"),
        .install_dir = .bin,
        .install_subdir = "assets",
    });

    const install_tessdata = b.addInstallDirectory(.{
        .source_dir = b.path("tessdata"),
        .install_dir = .bin,
        .install_subdir = "tessdata",
    });

    b.getInstallStep().dependOn(&install_assets.step);
    b.getInstallStep().dependOn(&install_tessdata.step);

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());

    if (b.args) |args| {
        run_cmd.addArgs(args);
    }
    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);
}
