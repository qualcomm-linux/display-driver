/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * Backward-compatibility shims for sde-dlkm across kernel versions.
 *
 * Kernel 7.2 (v7.2-rc1, commit 5164f7e7ff8e):
 *   - struct drm_atomic_state renamed to struct drm_atomic_commit
 *   - drm_atomic_state_init/get/put/default_clear/default_release/clear renamed
 *   - drm_atomic_private_obj_init() state argument removed
 *   - drm_private_state_funcs gained atomic_create_state callback
 *   - thermal_of_cooling_device_register() gained u32 cdev_id as 2nd arg
 *   - drm_panel_init() made static / unexported (commit fe417df954a9)
 *   - qcom_clk_set_flags() / CLKFLAG_RETAIN_MEM removed from <linux/clk/qcom.h>
 */

#ifndef __MSM_DRM_COMPAT_H__
#define __MSM_DRM_COMPAT_H__

#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(7, 2, 0))

/*
 * struct drm_atomic_state was renamed to struct drm_atomic_commit in 7.2.
 * We cannot use a simple #define because it would also mangle
 * "drm_atomic_state_helper" in include paths and identifiers.
 */
#define drm_atomic_state drm_atomic_commit

/* Function renames: drm_atomic_state_* -> drm_atomic_commit_* */
#define drm_atomic_state_init		drm_atomic_commit_init
#define drm_atomic_state_default_clear	drm_atomic_commit_default_clear
#define drm_atomic_state_default_release drm_atomic_commit_default_release
#define drm_atomic_state_get		drm_atomic_commit_get
#define drm_atomic_state_put		drm_atomic_commit_put
#define drm_atomic_state_alloc		drm_atomic_commit_alloc
#define drm_atomic_state_clear		drm_atomic_commit_clear

/*
 * drm_atomic_private_obj_init() dropped its initial state argument in 7.2
 * (commit 3590a52f0d09). Wrap the 4-arg call form used in sde-dlkm so it
 * compiles on both kernels without touching the call sites.
 */
#include <drm/drm_atomic.h>
static inline int
__msm_atomic_private_obj_init_compat(struct drm_device *dev,
				     struct drm_private_obj *obj,
				     struct drm_private_state *state,
				     const struct drm_private_state_funcs *funcs)
{
	return drm_atomic_private_obj_init(dev, obj, funcs);
}
#define drm_atomic_private_obj_init(dev, obj, state, funcs) \
	__msm_atomic_private_obj_init_compat(dev, obj, state, funcs)

/*
 * thermal_of_cooling_device_register() gained a new u32 cdev_id argument
 * as its 2nd parameter in kernel 7.2. The inline helper is defined before
 * the #define so its body sees the real 5-arg prototype, not the macro.
 */
#include <linux/thermal.h>
static inline struct thermal_cooling_device *
__msm_thermal_of_cdev_register_compat(struct device_node *np,
				      const char *type, void *data,
				      const struct thermal_cooling_device_ops *ops)
{
	/* cdev_id=0 preserves the legacy single-device-per-node behaviour */
	return thermal_of_cooling_device_register(np, 0, type, data, ops);
}
/* Now shadow the 4-arg call sites — the inline above is already resolved */
#define thermal_of_cooling_device_register(np, type, data, ops) \
	__msm_thermal_of_cdev_register_compat(np, type, data, ops)

/*
 * drm_panel_init() was made static / unexported in 7.2 (commit fe417df954a9).
 * Provide an equivalent open-coded initialiser so sde-dlkm call sites that
 * kzalloc a bare drm_panel and call drm_panel_init() continue to work.
 * The body mirrors the last exported implementation exactly.
 */
#include <drm/drm_panel.h>
static inline void
__msm_drm_panel_init_compat(struct drm_panel *panel, struct device *dev,
			    const struct drm_panel_funcs *funcs,
			    int connector_type)
{
	INIT_LIST_HEAD(&panel->list);
	INIT_LIST_HEAD(&panel->followers);
	mutex_init(&panel->follower_lock);
	panel->dev = dev;
	panel->funcs = funcs;
	panel->connector_type = connector_type;
}
#define drm_panel_init(panel, dev, funcs, connector_type) \
	__msm_drm_panel_init_compat(panel, dev, funcs, connector_type)

/*
 * qcom_clk_set_flags() and CLKFLAG_RETAIN_MEM/CLKFLAG_NORETAIN_MEM were
 * removed from <linux/clk/qcom.h> in 7.2. The call site in sde_power_handle.c
 * is already guarded by __has_include(<clk/qcom.h>) which now resolves to the
 * new header that no longer declares these symbols. Provide no-op stubs so the
 * guarded code compiles and links cleanly; the retain-mem hint is advisory only.
 */
#include <linux/clk.h>
#define CLKFLAG_RETAIN_MEM	0
#define CLKFLAG_NORETAIN_MEM	0
static inline int qcom_clk_set_flags(struct clk *clk, unsigned long flags)
{
	return 0;
}

#endif /* LINUX_VERSION_CODE >= 7.2.0 */

#endif /* __MSM_DRM_COMPAT_H__ */
