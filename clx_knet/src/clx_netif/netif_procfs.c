#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <netif_knl.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
#define PROC_CREATE(_entry, _name, _acc, _path, _fops)                  \
    do {                                                                \
        _entry = proc_create(_name, _acc, _path, _fops);                \
    } while (0)

#define PROC_CREATE_DATA(_entry, _name, _acc, _path, _fops, _data)      \
    do {                                                                \
        _entry = proc_create_data(_name, _acc, _path, _fops, _data);    \
    } while (0)

#define PROC_PDE_DATA(_node) PDE_DATA(_node)
#else
#define PROC_CREATE(_entry, _name, _acc, _path, _fops)                  \
    do {                                                                \
        _entry = create_proc_entry(_name, _acc, _path);                 \
        if (_entry) {                                                   \
            _entry->proc_fops = _fops;                                  \
        }                                                               \
    } while (0)

#define PROC_CREATE_DATA(_entry, _name, _acc, _path, _fops, _data)      \
    do {                                                                \
        _entry = create_proc_entry(_name, _acc, _path);                 \
        if (_entry) {                                                   \
            _entry->proc_fops = _fops;                                  \
            _entry->data=_data;                                         \
        }                                                               \
    } while (0)

#define PROC_PDE_DATA(_node) PROC_I(_node)->pde->data
#endif

static struct proc_dir_entry *netif_procfs_root        = NULL;

static int channel;
static unsigned long long source, dest;
static int debug_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "DMA channel %d, source 0x%llx, dest 0x%llx\n", channel, source, dest);
	return 0;
}

static int debug_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, debug_proc_show, NULL);
}

static ssize_t debug_proc_write(struct file *file, const char __user *buf,
				    size_t count, loff_t *pos)
{
    char debug_str[128];
    char *ptr;

    if (count > sizeof(debug_str)) {
        count = sizeof(debug_str) - 1;
        debug_str[count] = '\0';
    }
    if (copy_from_user(debug_str, buf, count)) {
        return -EFAULT;
    }

    channel = simple_strtol(debug_str, NULL, 0);
    HAL_KNL_DBG(HAL_KNL_DEBUG, "DMA channel: %d\n", channel);

    if ((ptr = strstr(debug_str, "src=")) != NULL) {
        ptr += 4;
        source = simple_strtoull(ptr, NULL, 0);
    } else {
        HAL_KNL_DBG(HAL_KNL_ERR, "Unknown configuration setting\n");
        return -EFAULT;
    }

    if ((ptr = strstr(debug_str, "dest=")) != NULL) {
        ptr += 5;
        dest = simple_strtoull(ptr, NULL, 0);
    } else {
        HAL_KNL_DBG(HAL_KNL_ERR, "Unknown configuration setting\n");
        return -EFAULT;
    }
    HAL_KNL_DBG(HAL_KNL_DEBUG, "DMA channel %d, source 0x%llx, dest 0x%llx\n", 
             channel, source, dest);
	return count;
}

static const struct file_operations netif_procfs_debug_fops = {
	.open		= debug_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.write		= debug_proc_write,
	.release	= single_release,
};

static int help_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "Usage: \n");
	seq_printf(m, "    echo 0 src=0x123456 dest=0x3214 > /proc/clx_netif/debug\n");
	return 0;
}

static int help_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, help_proc_show, NULL);
}

static const struct file_operations netif_procfs_help_fops = {
	.open		= help_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

int char_proc_init(void)
{
    struct proc_dir_entry *entry = NULL;

	netif_procfs_root = proc_mkdir(NETIF_KNL_DRIVER_NAME, NULL);
	if (!netif_procfs_root)
		return -ENOMEM;

    PROC_CREATE(entry, "help", S_IRUGO, netif_procfs_root, &netif_procfs_help_fops);
    if (entry == NULL) {
        return -ENOMEM;
    }

    PROC_CREATE(entry, "debug", S_IWUGO | S_IRUGO, netif_procfs_root, &netif_procfs_debug_fops);
    if (entry == NULL) {
        return -ENOMEM;
    }
    
    return 0;
}

void cleanup_procfs(void)
{
    remove_proc_entry("help", netif_procfs_root);
    remove_proc_entry("debug", netif_procfs_root);
    
    remove_proc_entry(NETIF_KNL_DRIVER_NAME, NULL);
    HAL_KNL_DBG(HAL_KNL_DEBUG, "%s Procfs removed\n",
                NETIF_KNL_DRIVER_NAME);
}
