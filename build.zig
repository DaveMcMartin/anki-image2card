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

    if (t.os.tag == .windows) {
        exe.linkSystemLibrary("SDL3");
    } else if (t.os.tag == .macos) {
        exe.linkSystemLibrary("SDL3");
    } else {
        exe.linkSystemLibrary("SDL3");
        // On linux, zig cc does not use pkg-config to find /usr/local/lib by default
        exe.addLibraryPath(.{ .cwd_relative = "/usr/local/lib" });
        exe.addIncludePath(.{ .cwd_relative = "/usr/local/include" });
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

    exe.addCSourceFile(.{ .file = b.path("third_party/sqlite/sqlite3.c"), .flags = &[_][]const u8{"-fno-sanitize=undefined", "-O2"} });
    exe.addIncludePath(b.path("third_party/sqlite"));

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
        // curl is also needed by cpp-httplib sometimes
        exe.linkSystemLibrary("curl");
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
