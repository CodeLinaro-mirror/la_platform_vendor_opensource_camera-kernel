load("//build/kernel/kleaf:kernel.bzl", "ddk_module")

def define_shikra():
    ddk_module(
        name = "camera_shikra",
        out = "camera.ko",

        # Add the generated header to sources so the compiler finds it
        srcs = native.glob([
            "config/**",
            "camera/**",
            "common/**",
            "Kbuild",
        ]) + [
            # INCLUDE THE GENERATED FILE HERE
            "//vendor/qcom/opensource/camera-kernel:cam_generated_header" 
        ],

        hdrs = ["//vendor/qcom/opensource/camera-kernel:camera_headers"],

        # Pass the ARCH flag here
        kbuild_options = [
            "MODNAME=camera",
            "CAMERA_ARCH=shikra",
            "BOARD_PLATFORM=shikra",
        ],

        kernel_build = "//msm-kernel:kernel_aarch64", 
        deps = ["//msm-kernel:all_headers"],
    )

load("//build/kernel/kleaf:kernel.bzl", "ddk_module")
