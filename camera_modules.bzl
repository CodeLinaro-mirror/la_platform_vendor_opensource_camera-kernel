load("//build/kernel/kleaf:kernel.bzl", "ddk_module")
load("//build/bazel_common_rules/dist:dist.bzl", "copy_to_dist_dir")
load(":target_variants.bzl", "get_all_variants")
load(":project_defconfig.bzl", "get_project_defconfig")

def _get_common_srcs():
    """Common source files used across all targets"""
    return [
        "common/cam_common_util.c",
        "common/cam_compat.c",
        "common/cam_debug_util.c",
        "common/cam_io_util.c",
        "common/cam_req_mgr_timer.c",
        "common/cam_req_mgr_util.c",
        "common/cam_req_mgr_workq.c",
        "common/cam_soc_icc.c",
    ]

def _get_camera_base_srcs():
    """Base source files for camera variant shikra"""
    return [
        "camera/drivers/cam_req_mgr/cam_req_mgr_core.c",
        "camera/drivers/cam_req_mgr/cam_req_mgr_dev.c",
        "camera/drivers/cam_req_mgr/cam_mem_mgr.c",
        "camera/drivers/cam_req_mgr/cam_req_mgr_debug.c",
        "camera/drivers/cam_utils/cam_soc_util.c",
        "camera/drivers/cam_utils/cam_packet_util.c",
        "camera/drivers/cam_utils/cam_trace.c",
        "camera/drivers/cam_core/cam_context.c",
        "camera/drivers/cam_core/cam_context_utils.c",
        "camera/drivers/cam_core/cam_node.c",
        "camera/drivers/cam_core/cam_subdev.c",
        "camera/drivers/cam_sync/cam_sync.c",
        "camera/drivers/cam_sync/cam_sync_util.c",
        "camera/drivers/cam_sync/cam_sync_dma_fence.c",
        "camera/drivers/cam_cpas/cpas_top/cam_cpastop_hw.c",
        "camera/drivers/cam_cpas/camss_top/cam_camsstop_hw.c",
        "camera/drivers/cam_cpas/cam_cpas_soc.c",
        "camera/drivers/cam_cpas/cam_cpas_intf.c",
        "camera/drivers/cam_cpas/cam_cpas_hw.c",
        "camera/drivers/cam_cdm/cam_cdm_soc.c",
        "camera/drivers/cam_cdm/cam_cdm_util.c",
        "camera/drivers/cam_cdm/cam_cdm_intf.c",
        "camera/drivers/cam_cdm/cam_cdm_core_common.c",
        "camera/drivers/cam_cdm/cam_cdm_virtual_core.c",
        "camera/drivers/cam_cdm/cam_cdm_hw_core.c",
        "camera/drivers/cam_smmu/cam_smmu_api.c",
        "camera/drivers/cam_presil/stub/cam_presil_hw_access_stub.c",
        "camera/drivers/camera_main.c",
    ]

def _define_module(target, variant):
    tv = "{}_{}".format(target, variant)

    # For camera variant, use camera/dt-bindings
    base_deps = select({
        "//build/kernel/kleaf:socrepo_true": [
            ":camera_headers",
            ":camera_banner",
            ":camera_dt_bindings",
            "//soc-repo:all_headers",
        ],
        "//build/kernel/kleaf:socrepo_false": [
            ":camera_headers",
            ":camera_banner",
            ":camera_dt_bindings",
            "//msm-kernel:all_headers",
        ],
     })

    deps = []

    # Kernel build target
    # For shikra and holi, use msm_kernel_build for perf variant and kernel_aarch64_consolidate for consolidate variant
    # For other platforms, use the standard naming convention
    if target in ["holi", "shikra"]:
        if variant == "perf":
            kernel_build = select({
                "//build/kernel/kleaf:socrepo_true": "//soc-repo:msm_kernel_build",
                "//build/kernel/kleaf:socrepo_false": "//msm-kernel:{}".format(tv),
            })
        elif variant == "consolidate":
            kernel_build = select({
                "//build/kernel/kleaf:socrepo_true": "//soc-repo:kernel_aarch64_consolidate",
                "//build/kernel/kleaf:socrepo_false": "//msm-kernel:{}".format(tv),
            })
        else:
            # For debug or other variants, use msm_kernel_build as default
            kernel_build = select({
                "//build/kernel/kleaf:socrepo_true": "//soc-repo:msm_kernel_build",
                "//build/kernel/kleaf:socrepo_false": "//msm-kernel:{}".format(tv),
            })
    else:
        kernel_build = select({
            "//build/kernel/kleaf:socrepo_true": "//soc-repo:{}_base_kernel".format(tv),
            "//build/kernel/kleaf:socrepo_false": "//msm-kernel:{}".format(tv),
        })

    # Generate the defconfig file dynamically
    native.genrule(
        name = "{}_defconfig_generated".format(tv),
        srcs = [get_project_defconfig(target, variant)],
        outs = ["{}_defconfig.generated".format(tv)],
        cmd = "cat $(SRCS) > $@",
    )

    base_srcs = _get_camera_base_srcs() + _get_common_srcs()
    src_prefix = "camera/drivers"

    # Conditional sources based on Kconfig
    conditional_srcs = {}

    # ISP sources
    conditional_srcs["CONFIG_SPECTRA_ISP"] = {
        True: [
            "camera/drivers/cam_isp/isp_hw_mgr/hw_utils/cam_tasklet_util.c",
            "camera/drivers/cam_isp/isp_hw_mgr/hw_utils/cam_isp_packet_parser.c",
            "camera/drivers/cam_isp/isp_hw_mgr/hw_utils/irq_controller/cam_irq_controller.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/ife_csid_hw/cam_ife_csid_dev.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/ife_csid_hw/cam_ife_csid_soc.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/ife_csid_hw/cam_ife_csid_common.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/ife_csid_hw/cam_ife_csid_hw_ver1.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/ife_csid_hw/cam_ife_csid_hw_ver2.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/ife_csid_hw/cam_ife_csid_mod.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/ife_csid_hw/cam_ife_csid_lite_mod.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/sfe_hw/cam_sfe_soc.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/sfe_hw/cam_sfe_dev.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/sfe_hw/cam_sfe_core.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/sfe_hw/sfe_top/cam_sfe_top.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/sfe_hw/sfe_bus/cam_sfe_bus.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/sfe_hw/sfe_bus/cam_sfe_bus_rd.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/sfe_hw/sfe_bus/cam_sfe_bus_wr.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/cam_vfe_soc.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/cam_vfe_dev.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/cam_vfe_core.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_bus/cam_vfe_bus.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_bus/cam_vfe_bus_ver2.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_bus/cam_vfe_bus_rd_ver1.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_bus/cam_vfe_bus_ver3.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_camif_lite_ver2.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_top.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_top_common.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_top_ver4.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_top_ver3.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_top_ver2.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_camif_ver2.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_camif_ver3.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_rdi.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_fe_ver1.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_camif_lite_ver3.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe17x/cam_vfe.c",
            "camera/drivers/cam_isp/isp_hw_mgr/cam_isp_hw_mgr.c",
            "camera/drivers/cam_isp/isp_hw_mgr/cam_ife_hw_mgr.c",
            "camera/drivers/cam_isp/isp_hw_mgr/cam_ife_hw_mgr_addons.c",
            "camera/drivers/cam_isp/cam_isp_dev.c",
            "camera/drivers/cam_isp/cam_isp_context.c",
        ],
    }

    # ICP sources
    conditional_srcs["CONFIG_SPECTRA_ICP"] = {
        True: [
            "camera/drivers/cam_icp/icp_hw/icp_hw_mgr/cam_icp_hw_mgr.c",
            "camera/drivers/cam_icp/icp_hw/ipe_hw/ipe_dev.c",
            "camera/drivers/cam_icp/icp_hw/ipe_hw/ipe_core.c",
            "camera/drivers/cam_icp/icp_hw/ipe_hw/ipe_soc.c",
            "camera/drivers/cam_icp/icp_hw/icp_proc/icp_v1_hw/cam_icp_v1_dev.c",
            "camera/drivers/cam_icp/icp_hw/icp_proc/icp_v1_hw/cam_icp_v1_core.c",
            "camera/drivers/cam_icp/icp_hw/icp_proc/icp_v2_hw/cam_icp_v2_dev.c",
            "camera/drivers/cam_icp/icp_hw/icp_proc/icp_v2_hw/cam_icp_v2_core.c",
            "camera/drivers/cam_icp/icp_hw/icp_proc/icp_common/cam_icp_proc_common.c",
            "camera/drivers/cam_icp/icp_hw/icp_proc/icp_common/cam_icp_soc_common.c",
            "camera/drivers/cam_icp/icp_hw/icp_proc/cam_icp_proc.c",
            "camera/drivers/cam_icp/icp_hw/bps_hw/bps_dev.c",
            "camera/drivers/cam_icp/icp_hw/bps_hw/bps_core.c",
            "camera/drivers/cam_icp/icp_hw/bps_hw/bps_soc.c",
            "camera/drivers/cam_icp/icp_hw/ofe_hw/ofe_dev.c",
            "camera/drivers/cam_icp/icp_hw/ofe_hw/ofe_core.c",
            "camera/drivers/cam_icp/icp_hw/ofe_hw/ofe_soc.c",
            "camera/drivers/cam_icp/cam_icp_subdev.c",
            "camera/drivers/cam_icp/cam_icp_context.c",
            "camera/drivers/cam_icp/hfi.c",
        ],
    }

    # JPEG sources
    conditional_srcs["CONFIG_SPECTRA_JPEG"] = {
        True: [
            "camera/drivers/cam_jpeg/jpeg_hw/jpeg_enc_hw/jpeg_enc_dev.c",
            "camera/drivers/cam_jpeg/jpeg_hw/jpeg_enc_hw/jpeg_enc_core.c",
            "camera/drivers/cam_jpeg/jpeg_hw/jpeg_enc_hw/jpeg_enc_soc.c",
            "camera/drivers/cam_jpeg/jpeg_hw/jpeg_dma_hw/jpeg_dma_dev.c",
            "camera/drivers/cam_jpeg/jpeg_hw/jpeg_dma_hw/jpeg_dma_core.c",
            "camera/drivers/cam_jpeg/jpeg_hw/jpeg_dma_hw/jpeg_dma_soc.c",
            "camera/drivers/cam_jpeg/jpeg_hw/cam_jpeg_hw_mgr.c",
            "camera/drivers/cam_jpeg/cam_jpeg_dev.c",
            "camera/drivers/cam_jpeg/cam_jpeg_context.c",
        ],
    }

    # Sensor sources
    conditional_srcs["CONFIG_SPECTRA_SENSOR"] = {
        True: [
            "camera/drivers/cam_sensor_module/cam_actuator/cam_actuator_dev.c",
            "camera/drivers/cam_sensor_module/cam_actuator/cam_actuator_core.c",
            "camera/drivers/cam_sensor_module/cam_actuator/cam_actuator_soc.c",
            "camera/drivers/cam_sensor_module/cam_cci/cam_cci_dev.c",
            "camera/drivers/cam_sensor_module/cam_cci/cam_cci_core.c",
            "camera/drivers/cam_sensor_module/cam_cci/cam_cci_soc.c",
            "camera/drivers/cam_sensor_module/cam_tpg/cam_tpg_dev.c",
            "camera/drivers/cam_sensor_module/cam_tpg/cam_tpg_core.c",
            "camera/drivers/cam_sensor_module/cam_tpg/tpg_hw/tpg_hw.c",
            "camera/drivers/cam_sensor_module/cam_tpg/tpg_hw/tpg_hw_common.c",
            "camera/drivers/cam_sensor_module/cam_tpg/tpg_hw/tpg_hw_v_1_0/tpg_hw_v_1_0.c",
            "camera/drivers/cam_sensor_module/cam_tpg/tpg_hw/tpg_hw_v_1_2/tpg_hw_v_1_2.c",
            "camera/drivers/cam_sensor_module/cam_tpg/tpg_hw/tpg_hw_v_1_3/tpg_hw_v_1_3.c",
            "camera/drivers/cam_sensor_module/cam_tpg/tpg_hw/tpg_hw_v_1_4/tpg_hw_v_1_4.c",
            "camera/drivers/cam_sensor_module/cam_csiphy/cam_csiphy_soc.c",
            "camera/drivers/cam_sensor_module/cam_csiphy/cam_csiphy_dev.c",
            "camera/drivers/cam_sensor_module/cam_csiphy/cam_csiphy_core.c",
            "camera/drivers/cam_sensor_module/cam_eeprom/cam_eeprom_dev.c",
            "camera/drivers/cam_sensor_module/cam_eeprom/cam_eeprom_core.c",
            "camera/drivers/cam_sensor_module/cam_eeprom/cam_eeprom_soc.c",
            "camera/drivers/cam_sensor_module/cam_ois/cam_ois_dev.c",
            "camera/drivers/cam_sensor_module/cam_ois/cam_ois_core.c",
            "camera/drivers/cam_sensor_module/cam_ois/cam_ois_soc.c",
            "camera/drivers/cam_sensor_module/cam_sensor/cam_sensor_dev.c",
            "camera/drivers/cam_sensor_module/cam_sensor/cam_sensor_core.c",
            "camera/drivers/cam_sensor_module/cam_sensor/cam_sensor_soc.c",
            "camera/drivers/cam_sensor_module/cam_sensor_io/cam_sensor_io.c",
            "camera/drivers/cam_sensor_module/cam_sensor_io/cam_sensor_cci_i2c.c",
            "camera/drivers/cam_sensor_module/cam_sensor_io/cam_sensor_qup_i2c.c",
            "camera/drivers/cam_sensor_module/cam_sensor_io/cam_sensor_qup_i3c.c",
            "camera/drivers/cam_sensor_module/cam_sensor_io/cam_sensor_spi.c",
            "camera/drivers/cam_sensor_module/cam_sensor_utils/cam_sensor_util.c",
            "camera/drivers/cam_sensor_module/cam_res_mgr/cam_res_mgr.c",
            "camera/drivers/cam_sensor_module/cam_flash/cam_flash_dev.c",
            "camera/drivers/cam_sensor_module/cam_flash/cam_flash_core.c",
            "camera/drivers/cam_sensor_module/cam_flash/cam_flash_soc.c",
            "camera/drivers/cam_sensor_module/cam_sensor_module_debug.c",
        ],
    }

    # Additional conditional sources for non-camera_kt variants
    conditional_srcs["CONFIG_SPECTRA_FD"] = {
        True: [
            "camera/drivers/cam_fd/fd_hw_mgr/fd_hw/cam_fd_hw_dev.c",
            "camera/drivers/cam_fd/fd_hw_mgr/fd_hw/cam_fd_hw_core.c",
            "camera/drivers/cam_fd/fd_hw_mgr/fd_hw/cam_fd_hw_soc.c",
            "camera/drivers/cam_fd/fd_hw_mgr/cam_fd_hw_mgr.c",
            "camera/drivers/cam_fd/cam_fd_dev.c",
            "camera/drivers/cam_fd/cam_fd_context.c",
        ],
    }

    conditional_srcs["CONFIG_SPECTRA_LRME"] = {
        True: [
            "camera/drivers/cam_lrme/lrme_hw_mgr/lrme_hw/cam_lrme_hw_dev.c",
            "camera/drivers/cam_lrme/lrme_hw_mgr/lrme_hw/cam_lrme_hw_core.c",
            "camera/drivers/cam_lrme/lrme_hw_mgr/lrme_hw/cam_lrme_hw_soc.c",
            "camera/drivers/cam_lrme/lrme_hw_mgr/cam_lrme_hw_mgr.c",
            "camera/drivers/cam_lrme/cam_lrme_dev.c",
            "camera/drivers/cam_lrme/cam_lrme_context.c",
        ],
    }

    conditional_srcs["CONFIG_SPECTRA_CUSTOM"] = {
        True: [
            "camera/drivers/cam_cust/cam_custom_hw_mgr/cam_custom_hw1/cam_custom_sub_mod_soc.c",
            "camera/drivers/cam_cust/cam_custom_hw_mgr/cam_custom_hw1/cam_custom_sub_mod_dev.c",
            "camera/drivers/cam_cust/cam_custom_hw_mgr/cam_custom_hw1/cam_custom_sub_mod_core.c",
            "camera/drivers/cam_cust/cam_custom_hw_mgr/cam_custom_csid/cam_custom_csid_dev.c",
            "camera/drivers/cam_cust/cam_custom_hw_mgr/cam_custom_hw_mgr.c",
            "camera/drivers/cam_cust/cam_custom_dev.c",
            "camera/drivers/cam_cust/cam_custom_context.c",
        ],
    }

    conditional_srcs["CONFIG_SPECTRA_OPE"] = {
        True: [
            "camera/drivers/cam_ope/cam_ope_subdev.c",
            "camera/drivers/cam_ope/cam_ope_context.c",
            "camera/drivers/cam_ope/ope_hw_mgr/cam_ope_hw_mgr.c",
            "camera/drivers/cam_ope/ope_hw_mgr/ope_hw/ope_dev.c",
            "camera/drivers/cam_ope/ope_hw_mgr/ope_hw/ope_soc.c",
            "camera/drivers/cam_ope/ope_hw_mgr/ope_hw/ope_core.c",
            "camera/drivers/cam_ope/ope_hw_mgr/ope_hw/top/ope_top.c",
            "camera/drivers/cam_ope/ope_hw_mgr/ope_hw/bus_rd/ope_bus_rd.c",
            "camera/drivers/cam_ope/ope_hw_mgr/ope_hw/bus_wr/ope_bus_wr.c",
        ],
    }

    conditional_srcs["CONFIG_SPECTRA_CRE"] = {
        True: [
            "camera/drivers/cam_cre/cam_cre_hw_mgr/cre_hw/cre_core.c",
            "camera/drivers/cam_cre/cam_cre_hw_mgr/cre_hw/cre_soc.c",
            "camera/drivers/cam_cre/cam_cre_hw_mgr/cre_hw/cre_dev.c",
            "camera/drivers/cam_cre/cam_cre_hw_mgr/cre_hw/top/cre_top.c",
            "camera/drivers/cam_cre/cam_cre_hw_mgr/cre_hw/bus_rd/cre_bus_rd.c",
            "camera/drivers/cam_cre/cam_cre_hw_mgr/cre_hw/bus_wr/cre_bus_wr.c",
            "camera/drivers/cam_cre/cam_cre_hw_mgr/cam_cre_hw_mgr.c",
            "camera/drivers/cam_cre/cam_cre_dev.c",
            "camera/drivers/cam_cre/cam_cre_context.c",
        ],
    }

    conditional_srcs["CONFIG_SPECTRA_TFE"] = {
        True: [
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/ppi_hw/cam_csid_ppi_core.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/ppi_hw/cam_csid_ppi_dev.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/ppi_hw/cam_csid_ppi100.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/tfe_hw/cam_tfe_soc.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/tfe_hw/cam_tfe_dev.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/tfe_hw/cam_tfe_core.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/tfe_hw/cam_tfe_bus.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/tfe_hw/cam_tfe.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/tfe_csid_hw/cam_tfe_csid_dev.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/tfe_csid_hw/cam_tfe_csid_soc.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/tfe_csid_hw/cam_tfe_csid_core.c",
            "camera/drivers/cam_isp/isp_hw_mgr/isp_hw/tfe_csid_hw/cam_tfe_csid.c",
            "camera/drivers/cam_isp/isp_hw_mgr/cam_tfe_hw_mgr.c",
        ],
    }

    conditional_srcs["CONFIG_ENABLE_US_API"] = {
        True: ["{}/cam_req_mgr/cam_buf_mgr.c".format(src_prefix)],
    }

    conditional_srcs["CONFIG_QCOM_CX_IPEAK"] = {
        True: ["{}/cam_utils/cam_cx_ipeak.c".format(src_prefix)],
    }

    conditional_srcs["CONFIG_QCOM_BUS_SCALING"] = {
        True: ["{}/cam_utils/cam_soc_bus.c".format(src_prefix)],
    }

    conditional_srcs["CONFIG_TARGET_SYNX_ENABLE"] = {
        True: ["camera/drivers/cam_sync/cam_sync_synx.c"],
    }

    # CONFIG_SPECTRA_SENSOR_SYSFS_UTIL - cam_cci_sysfs_util.c doesn't exist, commented out
    conditional_srcs["CONFIG_SPECTRA_SENSOR_SYSFS_UTIL"] = {
        True: ["camera/drivers/cam_sensor_module/cam_cci/cam_cci_sysfs_util.c"],
    }

    # Create the DDK module
    ddk_module(
        name = "{}_camera".format(tv),
        out = "camera.ko",
        srcs = base_srcs,
        conditional_srcs = conditional_srcs,
        copts = ["-include", "$(location :camera_banner)"],
        deps = base_deps + deps,
        kconfig = "Kconfig",
        defconfig = "{}_defconfig_generated".format(tv),
        kernel_build = kernel_build,
    )

    # Copy to distribution directory
    copy_to_dist_dir(
        name = "{}_camera_dist".format(tv),
        data = [":{}_camera".format(tv)],
        dist_dir = "out/target/product/{}/dlkm/lib/modules/".format(target),
        flat = True,
        wipe_dist_dir = False,
        allow_duplicate_filenames = False,
    )

def define_camera_module():
    for (t, v) in get_all_variants():
        _define_module(t, v)
