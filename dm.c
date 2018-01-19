#define ll_entry_declare(_type, _name, _list)				\
	_type _u_boot_list_2_##_list##_2_##_name __aligned(4)		\
			__attribute__((unused,				\
			section(".u_boot_list_2_"#_list"_2_"#_name)))
			
/* Declare a new uclass_driver */
#define UCLASS_DRIVER(__name)						\
	ll_entry_declare(struct uclass_driver, __name, uclass)
	
	
UCLASS_DRIVER(serial) = {
	.id		= UCLASS_SERIAL,
	.name		= "serial",
	.flags		= DM_UC_FLAG_SEQ_ALIAS,
	.post_probe	= serial_post_probe,
	.pre_remove	= serial_pre_remove,
	.per_device_auto_alloc_size = sizeof(struct serial_dev_priv),
};

struct uclass_driver _u_boot_list_2_uclass_2_serial __aligned(4) __attribute__((unused,				\
			section(".u_boot_list_2_uclass_2_serial"))) = {
	.id		= UCLASS_SERIAL,
	.name		= "serial",
	.flags		= DM_UC_FLAG_SEQ_ALIAS,
	.post_probe	= serial_post_probe,
	.pre_remove	= serial_pre_remove,
	.per_device_auto_alloc_size = sizeof(struct serial_dev_priv),
};


/* Declare a new U-Boot driver */
#define U_BOOT_DRIVER(__name)						\
	ll_entry_declare(struct driver, __name, driver)


/* This is the root driver - all drivers are children of this */
U_BOOT_DRIVER(root_driver) = {
	.name	= "root_driver",
	.id	= UCLASS_ROOT,
	.priv_auto_alloc_size = sizeof(struct root_priv),
};

/*展开如下*/
struct driver _u_boot_list_2_driver_2_root_driver __aligned(4)
			__attribute__((unused, section(".u_boot_list_2_driver_2_root_driver"))) = {
	.name	= "root_driver",
	.id	= UCLASS_ROOT,
	.priv_auto_alloc_size = sizeof(struct root_priv),
};


/* This is the root uclass */
UCLASS_DRIVER(root) = {
	.name	= "root",
	.id	= UCLASS_ROOT,
};



/* Cast away any volatile pointer */
#define DM_ROOT_NON_CONST		(((gd_t *)gd)->dm_root)
#define DM_UCLASS_ROOT_NON_CONST	(((gd_t *)gd)->uclass_root)


static const struct driver_info root_info = {
	.name		= "root_driver",
};

int dm_init_and_scan(bool pre_reloc_only)
{
	int ret;

	ret = dm_init();
	if (ret) {
		debug("dm_init() failed: %d\n", ret);
		return ret;
	}
	ret = dm_scan_platdata(pre_reloc_only);
	if (ret) {
		debug("dm_scan_platdata() failed: %d\n", ret);
		return ret;
	}

	if (CONFIG_IS_ENABLED(OF_CONTROL) && !CONFIG_IS_ENABLED(OF_PLATDATA)) {
		ret = dm_scan_fdt(gd->fdt_blob, pre_reloc_only);
		if (ret) {
			debug("dm_scan_fdt() failed: %d\n", ret);
			return ret;
		}
	}

	ret = dm_scan_other(pre_reloc_only);
	if (ret)
		return ret;

	return 0;
}


int dm_init(void)
{
	int ret;

	if (gd->dm_root) {
		dm_warn("Virtual root driver already exists!\n");
		return -EINVAL;
	}
	INIT_LIST_HEAD(&DM_UCLASS_ROOT_NON_CONST);

#if defined(CONFIG_NEEDS_MANUAL_RELOC)
	fix_drivers();
	fix_uclass();
	fix_devices();
#endif

	ret = device_bind_by_name(NULL, false, &root_info, &DM_ROOT_NON_CONST);
	if (ret)
		return ret;
#if CONFIG_IS_ENABLED(OF_CONTROL)
	DM_ROOT_NON_CONST->of_offset = 0;
#endif
	ret = device_probe(DM_ROOT_NON_CONST);
	if (ret)
		return ret;

	return 0;
}

int device_bind_by_name(struct udevice *parent, bool pre_reloc_only,
			const struct driver_info *info, struct udevice **devp)
{
	struct driver *drv;
	uint platdata_size = 0;

	drv = lists_driver_lookup_name(info->name);
	if (!drv)
		return -ENOENT;
	if (pre_reloc_only && !(drv->flags & DM_FLAG_PRE_RELOC))
		return -EPERM;

#if CONFIG_IS_ENABLED(OF_PLATDATA)
	platdata_size = info->platdata_size;
#endif
	return device_bind_common(parent, drv, info->name,
			(void *)info->platdata, 0, -1, platdata_size, devp);
}


#define ll_entry_start(_type, _list)					\
({									\
	static char start[0] __aligned(4) __attribute__((unused,	\
		section(".u_boot_list_2_"#_list"_1")));			\
	(_type *)&start;						\
})

static char start[0] __aligned(4) __attribute__((unused, section(".u_boot_list_2_driver_1")));
	(struct driver *)&start;				


#define ll_entry_end(_type, _list)					\
	({									\
		static char end[0] __aligned(4) __attribute__((unused,		\
			section(".u_boot_list_2_"#_list"_3"))); 		\
		(_type *)&end;							\
	})


#define ll_entry_count(_type, _list)					\
	({								\
		_type *start = ll_entry_start(_type, _list);		\
		_type *end = ll_entry_end(_type, _list);		\
		unsigned int _ll_result = end - start;			\
		_ll_result;						\
	})


struct driver *drv =
{
	static char start[0] __aligned(4) __attribute__((unused, section(".u_boot_list_2_driver_1")));
	(struct driver *)&start;
};

struct driver *lists_driver_lookup_name(const char *name)
{
	struct driver *drv =
		ll_entry_start(struct driver, driver);
	const int n_ents = ll_entry_count(struct driver, driver);
	struct driver *entry;

	for (entry = drv; entry != drv + n_ents; entry++) {
		if (!strcmp(name, entry->name))
			return entry;
	}

	/* Not found */
	return NULL;
}

static int device_bind_common(struct udevice *parent, const struct driver *drv,
			      const char *name, void *platdata,
			      ulong driver_data, int of_offset,
			      uint of_platdata_size, struct udevice **devp)
{
	struct udevice *dev;
	struct uclass *uc;
	int size, ret = 0;

	if (devp)
		*devp = NULL;
	if (!name)
		return -EINVAL;

	ret = uclass_get(drv->id, &uc);
	if (ret) {
		debug("Missing uclass for driver %s\n", drv->name);
		return ret;
	}

	dev = calloc(1, sizeof(struct udevice));
	if (!dev)
		return -ENOMEM;

	INIT_LIST_HEAD(&dev->sibling_node);
	INIT_LIST_HEAD(&dev->child_head);
	INIT_LIST_HEAD(&dev->uclass_node);
#ifdef CONFIG_DEVRES
	INIT_LIST_HEAD(&dev->devres_head);
#endif
	dev->platdata = platdata;
	dev->driver_data = driver_data;
	dev->name = name;
	dev->of_offset = of_offset;
	dev->parent = parent;
	dev->driver = drv;
	dev->uclass = uc;

	dev->seq = -1;
	dev->req_seq = -1;
	if (CONFIG_IS_ENABLED(OF_CONTROL) && CONFIG_IS_ENABLED(DM_SEQ_ALIAS)) {
		/*
		 * Some devices, such as a SPI bus, I2C bus and serial ports
		 * are numbered using aliases.
		 *
		 * This is just a 'requested' sequence, and will be
		 * resolved (and ->seq updated) when the device is probed.
		 */
		if (uc->uc_drv->flags & DM_UC_FLAG_SEQ_ALIAS) {
			if (uc->uc_drv->name && of_offset != -1) {
				fdtdec_get_alias_seq(gd->fdt_blob,
						uc->uc_drv->name, of_offset,
						&dev->req_seq);
			}
		}
	}

	if (drv->platdata_auto_alloc_size) {
		bool alloc = !platdata;

		if (CONFIG_IS_ENABLED(OF_PLATDATA)) {
			if (of_platdata_size) {
				dev->flags |= DM_FLAG_OF_PLATDATA;
				if (of_platdata_size <
						drv->platdata_auto_alloc_size)
					alloc = true;
			}
		}
		if (alloc) {
			dev->flags |= DM_FLAG_ALLOC_PDATA;
			dev->platdata = calloc(1,
					       drv->platdata_auto_alloc_size);
			if (!dev->platdata) {
				ret = -ENOMEM;
				goto fail_alloc1;
			}
			if (CONFIG_IS_ENABLED(OF_PLATDATA) && platdata) {
				memcpy(dev->platdata, platdata,
				       of_platdata_size);
			}
		}
	}

	size = uc->uc_drv->per_device_platdata_auto_alloc_size;
	if (size) {
		dev->flags |= DM_FLAG_ALLOC_UCLASS_PDATA;
		dev->uclass_platdata = calloc(1, size);
		if (!dev->uclass_platdata) {
			ret = -ENOMEM;
			goto fail_alloc2;
		}
	}

	if (parent) {
		size = parent->driver->per_child_platdata_auto_alloc_size;
		if (!size) {
			size = parent->uclass->uc_drv->
					per_child_platdata_auto_alloc_size;
		}
		if (size) {
			dev->flags |= DM_FLAG_ALLOC_PARENT_PDATA;
			dev->parent_platdata = calloc(1, size);
			if (!dev->parent_platdata) {
				ret = -ENOMEM;
				goto fail_alloc3;
			}
		}
	}

	/* put dev into parent's successor list */
	if (parent)
		list_add_tail(&dev->sibling_node, &parent->child_head);

	ret = uclass_bind_device(dev);
	if (ret)
		goto fail_uclass_bind;

	/* if we fail to bind we remove device from successors and free it */
	if (drv->bind) {
		ret = drv->bind(dev);
		if (ret)
			goto fail_bind;
	}
	if (parent && parent->driver->child_post_bind) {
		ret = parent->driver->child_post_bind(dev);
		if (ret)
			goto fail_child_post_bind;
	}
	if (uc->uc_drv->post_bind) {
		ret = uc->uc_drv->post_bind(dev);
		if (ret)
			goto fail_uclass_post_bind;
	}

	if (parent)
		dm_dbg("Bound device %s to %s\n", dev->name, parent->name);
	if (devp)
		*devp = dev;

	dev->flags |= DM_FLAG_BOUND;

	return 0;

fail_uclass_post_bind:
	/* There is no child unbind() method, so no clean-up required */
fail_child_post_bind:
	if (CONFIG_IS_ENABLED(DM_DEVICE_REMOVE)) {
		if (drv->unbind && drv->unbind(dev)) {
			dm_warn("unbind() method failed on dev '%s' on error path\n",
				dev->name);
		}
	}

fail_bind:
	if (CONFIG_IS_ENABLED(DM_DEVICE_REMOVE)) {
		if (uclass_unbind_device(dev)) {
			dm_warn("Failed to unbind dev '%s' on error path\n",
				dev->name);
		}
	}
fail_uclass_bind:
	if (CONFIG_IS_ENABLED(DM_DEVICE_REMOVE)) {
		list_del(&dev->sibling_node);
		if (dev->flags & DM_FLAG_ALLOC_PARENT_PDATA) {
			free(dev->parent_platdata);
			dev->parent_platdata = NULL;
		}
	}
fail_alloc3:
	if (dev->flags & DM_FLAG_ALLOC_UCLASS_PDATA) {
		free(dev->uclass_platdata);
		dev->uclass_platdata = NULL;
	}
fail_alloc2:
	if (dev->flags & DM_FLAG_ALLOC_PDATA) {
		free(dev->platdata);
		dev->platdata = NULL;
	}
fail_alloc1:
	devres_release_all(dev);

	free(dev);

	return ret;
}


