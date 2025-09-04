// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2014-2019, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/dma-mapping.h>
#include <linux/of_address.h>
#include <linux/slab.h>

#include "cam_compat.h"
#include "cam_debug_util.h"
#include "cam_cpas_api.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
int cam_reserve_icp_fw(struct cam_fw_alloc_info *icp_fw, size_t fw_length)
{
	int rc = 0;
	struct device_node *of_node;
	struct device_node *mem_node;
	struct resource     res;

	of_node = (icp_fw->fw_dev)->of_node;
	mem_node = of_parse_phandle(of_node, "memory-region", 0);
	if (!mem_node) {
		rc = -ENOMEM;
		CAM_ERR(CAM_SMMU, "FW memory carveout not found");
		goto end;
	}
	rc = of_address_to_resource(mem_node, 0, &res);
	of_node_put(mem_node);
	if (rc < 0) {
		CAM_ERR(CAM_SMMU, "Unable to get start of FW mem carveout");
		goto end;
	}
	icp_fw->fw_hdl = res.start;
	icp_fw->fw_kva = ioremap_wc(icp_fw->fw_hdl, fw_length);
	if (!icp_fw->fw_kva) {
		CAM_ERR(CAM_SMMU, "Failed to map the FW.");
		rc = -ENOMEM;
		goto end;
	}
	memset_io(icp_fw->fw_kva, 0, fw_length);

end:
	return rc;
}

void cam_unreserve_icp_fw(struct cam_fw_alloc_info *icp_fw, size_t fw_length)
{
	iounmap(icp_fw->fw_kva);
}

int cam_ife_notify_safe_lut_scm(bool safe_trigger)
{
	const uint32_t smmu_se_ife = 0;
	uint32_t camera_hw_version, rc = 0;

	rc = cam_cpas_get_cpas_hw_version(&camera_hw_version);
	if (!rc && qcom_scm_smmu_notify_secure_lut(smmu_se_ife, safe_trigger)) {
		switch (camera_hw_version) {
		case CAM_CPAS_TITAN_170_V100:
		case CAM_CPAS_TITAN_170_V110:
		case CAM_CPAS_TITAN_175_V100:
			CAM_ERR(CAM_ISP, "scm call to enable safe failed");
			rc = -EINVAL;
			break;
		default:
			break;
		}
	}

	return rc;
}

int cam_csiphy_notify_secure_mode(struct csiphy_device *csiphy_dev,
	bool protect, int32_t offset)
{
	int rc = 0;

	if (offset >= CSIPHY_MAX_INSTANCES) {
		CAM_ERR(CAM_CSIPHY, "Invalid CSIPHY offset");
		rc = -EINVAL;
	} else if (qcom_scm_camera_protect_phy_lanes(protect,
			csiphy_dev->csiphy_cpas_cp_reg_mask[offset])) {
		CAM_ERR(CAM_CSIPHY, "SCM call to hypervisor failed");
		rc = -EINVAL;
	}

	return 0;
}

void cam_cpastop_scm_write(struct cam_cpas_hw_errata_wa *errata_wa)
{
	int reg_val;

	qcom_scm_io_readl(errata_wa->data.reg_info.offset, &reg_val);
	reg_val |= errata_wa->data.reg_info.value;
	qcom_scm_io_writel(errata_wa->data.reg_info.offset, reg_val);
}
#else
int cam_reserve_icp_fw(struct cam_fw_alloc_info *icp_fw, size_t fw_length)
{
	int rc = 0;

	icp_fw->fw_kva = dma_alloc_coherent(icp_fw->fw_dev, fw_length,
		&icp_fw->fw_hdl, GFP_KERNEL);

	if (!icp_fw->fw_kva) {
		CAM_ERR(CAM_SMMU, "FW memory alloc failed");
		rc = -ENOMEM;
	}

	return rc;
}

void cam_unreserve_icp_fw(struct cam_fw_alloc_info *icp_fw, size_t fw_length)
{
	dma_free_coherent(icp_fw->fw_dev, fw_length, icp_fw->fw_kva,
		icp_fw->fw_hdl);
}

int cam_ife_notify_safe_lut_scm(bool safe_trigger)
{
	const uint32_t smmu_se_ife = 0;
	uint32_t camera_hw_version, rc = 0;
	struct scm_desc description = {
		.arginfo = SCM_ARGS(2, SCM_VAL, SCM_VAL),
		.args[0] = smmu_se_ife,
		.args[1] = safe_trigger,
	};

	rc = cam_cpas_get_cpas_hw_version(&camera_hw_version);
	if (!rc && scm_call2(SCM_SIP_FNID(0x15, 0x3), &description)) {
		switch (camera_hw_version) {
		case CAM_CPAS_TITAN_170_V100:
		case CAM_CPAS_TITAN_170_V110:
		case CAM_CPAS_TITAN_175_V100:
			CAM_ERR(CAM_ISP, "scm call to enable safe failed");
			rc = -EINVAL;
			break;
		default:
			break;
		}
	}

	return rc;
}

int cam_csiphy_notify_secure_mode(struct csiphy_device *csiphy_dev,
	bool protect, int32_t offset)
{
	int rc = 0;
	struct scm_desc description = {
		.arginfo = SCM_ARGS(2, SCM_VAL, SCM_VAL),
		.args[0] = protect,
		.args[1] = csiphy_dev->csiphy_cpas_cp_reg_mask[offset],
	};

	if (offset >= CSIPHY_MAX_INSTANCES) {
		CAM_ERR(CAM_CSIPHY, "Invalid CSIPHY offset");
		rc = -EINVAL;
	} else if (scm_call2(SCM_SIP_FNID(0x18, 0x7), &description)) {
		CAM_ERR(CAM_CSIPHY, "SCM call to hypervisor failed");
		rc = -EINVAL;
	}

	return 0;
}

void cam_cpastop_scm_write(struct cam_cpas_hw_errata_wa *errata_wa)
{
	int reg_val;

	reg_val = scm_io_read(errata_wa->data.reg_info.offset);
	reg_val |= errata_wa->data.reg_info.value;
	scm_io_write(errata_wa->data.reg_info.offset, reg_val);
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
void cam_free_clear(const void * ptr)
{
	kfree_sensitive(ptr);
}
#else
void cam_free_clear(const void * ptr)
{
	kzfree(ptr);
}
#endif

#if (KERNEL_VERSION(5, 18, 0) <= LINUX_VERSION_CODE)
int cam_compat_util_get_dmabuf_va(struct dma_buf *dmabuf, uintptr_t *vaddr)
{
	struct iosys_map mapping;
#if (KERNEL_VERSION(6, 2, 0) <= LINUX_VERSION_CODE)
	int error_code = dma_buf_vmap_unlocked(dmabuf, &mapping);
#else
	int error_code = dma_buf_vmap(dmabuf, &mapping);
#endif
	if (error_code) {
		*vaddr = 0;
	} else {
		*vaddr = (mapping.is_iomem) ?
			(uintptr_t)mapping.vaddr_iomem : (uintptr_t)mapping.vaddr;
		CAM_DBG(CAM_MEM,
			"dmabuf=%p, *vaddr=%p, is_iomem=%d, vaddr_iomem=%p, vaddr=%p",
			dmabuf, *vaddr, mapping.is_iomem, mapping.vaddr_iomem, mapping.vaddr);
	}

	return error_code;
}

void cam_compat_util_put_dmabuf_va(struct dma_buf *dmabuf, void *vaddr)
{
	struct iosys_map mapping = IOSYS_MAP_INIT_VADDR(vaddr);
#if (KERNEL_VERSION(6, 2, 0) <= LINUX_VERSION_CODE)
	dma_buf_vunmap_unlocked(dmabuf, &mapping);
#else
	dma_buf_vunmap(dmabuf, &mapping);
#endif
}
#elif (KERNEL_VERSION(5, 15, 0) <= LINUX_VERSION_CODE)
int cam_compat_util_get_dmabuf_va(struct dma_buf *dmabuf, uintptr_t *vaddr)
{
	struct dma_buf_map mapping;
	int error_code = dma_buf_vmap(dmabuf, &mapping);

	if (error_code)
		*vaddr = 0;
	else
		*vaddr = (mapping.is_iomem) ?
			(uintptr_t)mapping.vaddr_iomem : (uintptr_t)mapping.vaddr;

	return error_code;
}

void cam_compat_util_put_dmabuf_va(struct dma_buf *dmabuf, void *vaddr)
{
	struct dma_buf_map mapping = DMA_BUF_MAP_INIT_VADDR(vaddr);

	dma_buf_vunmap(dmabuf, &mapping);
}
#else
int cam_compat_util_get_dmabuf_va(struct dma_buf *dmabuf, uintptr_t *vaddr)
{
	int error_code = 0;
	void *addr = dma_buf_vmap(dmabuf);

	if (!addr) {
		*vaddr = 0;
		error_code = -ENOSPC;
	} else {
		*vaddr = (uintptr_t)addr;
	}

	return error_code;
}

void cam_compat_util_put_dmabuf_va(struct dma_buf *dmabuf, void *vaddr)
{
	dma_buf_vunmap(dmabuf, vaddr);
}
#endif


struct sg_table *cam_compat_dmabuf_map_attach(struct dma_buf_attachment *attach,
	enum dma_data_direction dma_dir)
{
#if (KERNEL_VERSION(6, 2, 0) <= LINUX_VERSION_CODE)
	return dma_buf_map_attachment_unlocked(attach, dma_dir);
#else
	return dma_buf_map_attachment(attach, dma_dir);
#endif
}

void cam_compat_dmabuf_unmap_attach(struct dma_buf_attachment *attach,
	struct sg_table *table, enum dma_data_direction dma_dir)
{
#if (KERNEL_VERSION(6, 2, 0) <= LINUX_VERSION_CODE)
	dma_buf_unmap_attachment_unlocked(attach, table, dma_dir);
#else
	dma_buf_unmap_attachment(attach, table, dma_dir);
#endif
}

#if (KERNEL_VERSION(5, 15, 0) <= LINUX_VERSION_CODE)
void cam_smmu_util_iommu_custom(struct device *dev,
	dma_addr_t discard_start, size_t discard_length)
{

}

#else
void cam_smmu_util_iommu_custom(struct device *dev,
	dma_addr_t discard_start, size_t discard_length)
{
	iommu_dma_enable_best_fit_algo(dev);

	if (discard_start)
		iommu_dma_reserve_iova(dev, discard_start, discard_length);

	return;
}

int cam_compat_util_get_dmabuf_va(struct dma_buf *dmabuf, uintptr_t *vaddr)
{
	int error_code = 0;
	void *addr = dma_buf_vmap(dmabuf);

	if (!addr) {
		*vaddr = 0;
		error_code = -ENOSPC;
	} else {
		*vaddr = (uintptr_t)addr;
	}

	return error_code;
}

void cam_compat_util_put_dmabuf_va(struct dma_buf *dmabuf, void *vaddr)
{
	dma_buf_vunmap(dmabuf, vaddr);
}

#endif

#if KERNEL_VERSION(6, 2, 0) <= LINUX_VERSION_CODE
int cam_iommu_map(struct iommu_domain *domain,
	size_t firmware_start, phys_addr_t fw_hdl,
	size_t firmware_len, int prot)
{
	int rc = 0;

	rc = iommu_map(domain, firmware_start,
			fw_hdl,	firmware_len,
			prot, GFP_ATOMIC);
	return rc;
}
#else
int cam_iommu_map(struct iommu_domain *domain,
	size_t firmware_start, phys_addr_t fw_hdl,
	size_t firmware_len, int prot)
{
	int rc = 0;

	rc = iommu_map(domain, firmware_start,
			fw_hdl,	firmware_len,
			prot);
	return rc;
}
#endif

#if KERNEL_VERSION(6, 2, 0) <= LINUX_VERSION_CODE
size_t cam_iommu_map_sg(struct iommu_domain *domain,
	dma_addr_t iova_start, struct scatterlist *sgl,
	uint64_t orig_nents, int prot)
{
	size_t size = 0;

	size = iommu_map_sg(domain,
			iova_start,
			sgl, orig_nents,
			prot, GFP_ATOMIC);
	return size;
}
#else
size_t cam_iommu_map_sg(struct iommu_domain *domain,
	dma_addr_t iova_start, struct scatterlist *sgl,
	uint64_t orig_nents, int prot)
{
	size_t size = 0;

	size = iommu_map_sg(domain, iova_start,
			sgl, orig_nents,
			prot);
	return size;
}
#endif

int16_t cam_get_gpio_counts(struct cam_hw_soc_info *soc_info)
{
	struct device_node *of_node = NULL;
	int16_t gpio_array_size = 0;

	of_node = soc_info->dev->of_node;
#if KERNEL_VERSION(6, 2, 0) <= LINUX_VERSION_CODE
	gpio_array_size = of_count_phandle_with_args(
		of_node, soc_info->compatible, NULL);
#else
	gpio_array_size = of_gpio_count(of_node);
#endif

	return gpio_array_size;
}

uint16_t cam_get_named_gpio(struct cam_hw_soc_info *soc_info,
	int index)
{
	struct device_node *of_node = NULL;
	uint16_t gpio_pin = 0;

	of_node = soc_info->dev->of_node;
#if KERNEL_VERSION(6, 2, 0) <= LINUX_VERSION_CODE
	gpio_pin = of_get_named_gpio(of_node, soc_info->compatible, index);
#else
	gpio_pin = of_get_gpio(of_node, index);
#endif

	return gpio_pin;
}
