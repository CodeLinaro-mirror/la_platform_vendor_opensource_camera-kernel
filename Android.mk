ifneq ($(TARGET_IS_HEADLESS),true)
ifneq ($(TARGET_USES_QMAA_OVERRIDE_CAMERA),false)
# Make target to specify building the camera.ko from within Android build system.
LOCAL_PATH := $(call my-dir)
# Path to DLKM make scripts
DLKM_DIR := $(TOP)/device/qcom/common/dlkm

CAMERA_KERNEL_ROOT := $(LOCAL_PATH)
# Kbuild options
KBUILD_OPTIONS := CAMERA_KERNEL_ROOT=$(TOP)/vendor/qcom/opensource/ais-kernel/$(LOCAL_PATH)
KBUILD_OPTIONS += KERNEL_ROOT=$(TOP)/kernel/msm-$(TARGET_KERNEL_VERSION)/
KBUILD_OPTIONS += MODNAME=ais
KBUILD_OPTIONS += BOARD_PLATFORM=$(TARGET_BOARD_PLATFORM)


# Clear shell environment variables from previous android module during build
include $(CLEAR_VARS)
# For incremental compilation support.
LOCAL_SRC_FILES             :=                                                                                  				  \
                                $(CAMERA_KERNEL_ROOT)/drivers/ais_isp/ais_ife_dev.c                                                 \
                                $(CAMERA_KERNEL_ROOT)/drivers/ais_isp/csid_hw/ais_ife_csid17x.c                                     \
                                $(CAMERA_KERNEL_ROOT)/drivers/ais_isp/csid_hw/ais_ife_csid_core.c                                   \
                                $(CAMERA_KERNEL_ROOT)/drivers/ais_isp/csid_hw/ais_ife_csid_dev.c                                    \
                                $(CAMERA_KERNEL_ROOT)/drivers/ais_isp/csid_hw/ais_ife_csid_lite17x.c                                \
                                $(CAMERA_KERNEL_ROOT)/drivers/ais_isp/csid_hw/ais_ife_csid_soc.c                                    \
                                $(CAMERA_KERNEL_ROOT)/drivers/ais_isp/utils/ais_isp_trace.c                                         \
                                $(CAMERA_KERNEL_ROOT)/drivers/ais_isp/vfe_hw/ais_vfe_core.c                                         \
                                $(CAMERA_KERNEL_ROOT)/drivers/ais_isp/vfe_hw/ais_vfe_dev.c                                          \
                                $(CAMERA_KERNEL_ROOT)/drivers/ais_isp/vfe_hw/ais_vfe_soc.c                                          \
                                $(CAMERA_KERNEL_ROOT)/drivers/ais_isp/vfe_hw/vfe17x/ais_vfe17x.c                                    \
                                $(CAMERA_KERNEL_ROOT)/drivers/ais_main.c                                                            \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_cdm/cam_cdm_core_common.c                                         \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_cdm/cam_cdm_hw_core.c                                             \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_cdm/cam_cdm_intf.c                                                \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_cdm/cam_cdm_soc.c                                                 \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_cdm/cam_cdm_util.c                                                \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_cdm/cam_cdm_virtual_core.c                                        \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_core/cam_context.c                                                \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_core/cam_context_utils.c                                          \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_core/cam_node.c                                                   \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_core/cam_subdev.c                                                 \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_cpas/cam_cpas_hw.c                                                \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_cpas/cam_cpas_intf.c                                              \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_cpas/cam_cpas_soc.c                                               \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_cpas/camss_top/cam_camsstop_hw.c                                  \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_cpas/cpas_top/cam_cpastop_hw.c                                    \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_fd/cam_fd_context.c                                               \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_fd/cam_fd_dev.c                                                   \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_fd/fd_hw_mgr/cam_fd_hw_mgr.c                                      \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_fd/fd_hw_mgr/fd_hw/cam_fd_hw_core.c                               \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_fd/fd_hw_mgr/fd_hw/cam_fd_hw_dev.c                                \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_fd/fd_hw_mgr/fd_hw/cam_fd_hw_soc.c                                \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_hyp_intf/cam_hyp_intf.c                                           \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_icp/cam_icp_context.c                                             \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_icp/cam_icp_subdev.c                                              \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_icp/hfi.c                                                         \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_icp/icp_hw/a5_hw/a5_core.c                                        \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_icp/icp_hw/a5_hw/a5_dev.c                                         \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_icp/icp_hw/a5_hw/a5_soc.c                                         \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_icp/icp_hw/bps_hw/bps_core.c                                      \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_icp/icp_hw/bps_hw/bps_dev.c                                       \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_icp/icp_hw/bps_hw/bps_soc.c                                       \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_icp/icp_hw/icp_hw_mgr/cam_icp_hw_mgr.c                            \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_icp/icp_hw/ipe_hw/ipe_core.c                                      \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_icp/icp_hw/ipe_hw/ipe_dev.c                                       \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_icp/icp_hw/ipe_hw/ipe_soc.c                                       \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/cam_isp_context.c                                             \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/cam_isp_dev.c                                                 \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/cam_ife_hw_mgr.c                                   \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/cam_isp_hw_mgr.c                                   \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/hw_utils/cam_isp_packet_parser.c                   \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/hw_utils/cam_tasklet_util.c                        \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/hw_utils/irq_controller/cam_irq_controller.c       \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/isp_hw/ife_csid_hw/cam_csid_ppi170.c               \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/isp_hw/ife_csid_hw/cam_csid_ppi_core.c             \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/isp_hw/ife_csid_hw/cam_csid_ppi_dev.c              \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/isp_hw/ife_csid_hw/cam_ife_csid17x.c               \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/isp_hw/ife_csid_hw/cam_ife_csid_core.c             \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/isp_hw/ife_csid_hw/cam_ife_csid_dev.c              \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/isp_hw/ife_csid_hw/cam_ife_csid_lite17x.c          \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/isp_hw/ife_csid_hw/cam_ife_csid_soc.c              \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/cam_vfe_core.c                       \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/cam_vfe_dev.c                        \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/cam_vfe_soc.c                        \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe17x/cam_vfe17x.c                  \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_bus/cam_vfe_bus.c                \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_bus/cam_vfe_bus_rd_ver1.c        \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_bus/cam_vfe_bus_ver2.c           \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_camif_lite_ver2.c    \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_camif_ver2.c         \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_fe_ver1.c            \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_rdi.c                \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_top.c                \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_top_ver2.c           \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_jpeg/cam_jpeg_context.c                                           \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_jpeg/cam_jpeg_dev.c                                               \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_jpeg/jpeg_hw/cam_jpeg_hw_mgr.c                                    \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_jpeg/jpeg_hw/jpeg_dma_hw/jpeg_dma_core.c                          \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_jpeg/jpeg_hw/jpeg_dma_hw/jpeg_dma_dev.c                           \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_jpeg/jpeg_hw/jpeg_dma_hw/jpeg_dma_soc.c                           \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_jpeg/jpeg_hw/jpeg_enc_hw/jpeg_enc_core.c                          \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_jpeg/jpeg_hw/jpeg_enc_hw/jpeg_enc_dev.c                           \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_jpeg/jpeg_hw/jpeg_enc_hw/jpeg_enc_soc.c                           \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_lrme/cam_lrme_context.c                                           \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_lrme/cam_lrme_dev.c                                               \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_lrme/lrme_hw_mgr/cam_lrme_hw_mgr.c                                \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_lrme/lrme_hw_mgr/lrme_hw/cam_lrme_hw_core.c                       \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_lrme/lrme_hw_mgr/lrme_hw/cam_lrme_hw_dev.c                        \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_lrme/lrme_hw_mgr/lrme_hw/cam_lrme_hw_soc.c                        \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_req_mgr/cam_mem_mgr.c                                             \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_req_mgr/cam_req_mgr_core.c                                        \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_req_mgr/cam_req_mgr_debug.c                                       \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_req_mgr/cam_req_mgr_dev.c                                         \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_req_mgr/cam_req_mgr_timer.c                                       \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_req_mgr/cam_req_mgr_util.c                                        \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_req_mgr/cam_req_mgr_workq.c                                       \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_actuator/cam_actuator_core.c                    \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_actuator/cam_actuator_dev.c                     \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_actuator/cam_actuator_soc.c                     \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_cci/cam_cci_core.c                              \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_cci/cam_cci_dev.c                               \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_cci/cam_cci_soc.c                               \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_csiphy/cam_csiphy_core.c                        \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_csiphy/cam_csiphy_dev.c                         \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_csiphy/cam_csiphy_soc.c                         \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_eeprom/cam_eeprom_core.c                        \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_eeprom/cam_eeprom_dev.c                         \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_eeprom/cam_eeprom_soc.c                         \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_flash/cam_flash_core.c                          \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_flash/cam_flash_dev.c                           \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_flash/cam_flash_soc.c                           \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_ir_led/cam_ir_led_core.c                        \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_ir_led/cam_ir_led_dev.c                         \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_ir_led/cam_ir_led_soc.c                         \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_ois/cam_ois_core.c                              \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_ois/cam_ois_dev.c                               \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_ois/cam_ois_soc.c                               \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_res_mgr/cam_res_mgr.c                           \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_sensor/cam_sensor_core.c                        \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_sensor/cam_sensor_dev.c                         \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_sensor/cam_sensor_soc.c                         \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_sensor_io/cam_sensor_cci_i2c.c                  \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_sensor_io/cam_sensor_io.c                       \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_sensor_io/cam_sensor_qup_i2c.c                  \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_sensor_io/cam_sensor_spi.c                      \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sensor_module/cam_sensor_utils/cam_sensor_util.c                  \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_smmu/cam_smmu_api.c                                               \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sync/cam_sync.c                                                   \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_sync/cam_sync_util.c                                              \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_utils/cam_common_util.c                                           \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_utils/cam_compat.c                                                \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_utils/cam_cx_ipeak.c                                              \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_utils/cam_debug_util.c                                            \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_utils/cam_io_util.c                                               \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_utils/cam_packet_util.c                                           \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_utils/cam_soc_bus.c                                               \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_utils/cam_soc_icc.c                                               \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_utils/cam_soc_util.c                                              \
                                $(CAMERA_KERNEL_ROOT)/drivers/cam_utils/cam_trace.c                                                 \
                                $(CAMERA_KERNEL_ROOT)/drivers/v4l2loopback-master_v2/v4l2loopback.c                                 \
                                $(LOCAL_PATH)/ais-board.mk   \
                                $(LOCAL_PATH)/ais-product.mk \
                                $(LOCAL_PATH)/Kbuild
LOCAL_MODULE_PATH           := $(KERNEL_MODULES_OUT)
LOCAL_MODULE                := ais.ko
LOCAL_MODULE_TAGS           := optional

include $(DLKM_DIR)/Build_external_kernelmodule.mk

endif #TARGET_USES_QMAA_OVERRIDE_CAMERA
endif #TARGET_IS_HEADLESS
