#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/syscalls.h>
#include <linux/string.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kamal Marhubi");
MODULE_DESCRIPTION("Rickroll module");

static char *rickroll_filename = "/home/bork/media/music/Rick Astley - Never Gonna Give You Up.mp3";

/*
 * Set up a module parameter for the filename. The arguments are variable name,
 * type, and permissions The thid argument is the permissions for the parameter
 * file in sysfs, something like
 *
 * /sys/module/<module_name>/parameters/<parameter_name>
 *
 * We're setting it writeable by root so it can be modified without reloading
 * the module.
 */
module_param(rickroll_filename, charp, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(rickroll_filename, "The location of the rick roll file");


/*
 * cr0 is an x86 control register, and the bit we're twiddling here controls
 * the write protection. We need to do this because the area of memory
 * containing the system call table is write protected, and modifying it would
 * case a protection fault.
 */
#define DISABLE_WRITE_PROTECTION (write_cr0(read_cr0() & (~ 0x10000)))
#define ENABLE_WRITE_PROTECTION (write_cr0(read_cr0() | 0x10000))


static unsigned long **find_sys_call_table(void);
asmlinkage long rickroll_open(const char __user *filename, int flags, umode_t mode);

asmlinkage long (*original_sys_open)(const char __user *, int, umode_t);
asmlinkage unsigned long **sys_call_table;


static int __init rickroll_init(void)
{
    if(!rickroll_filename) {
	printk(KERN_ERR "No rick roll filename given.");
	return -EINVAL;  /* invalid argument */
    }

    sys_call_table = find_sys_call_table();

    if(!sys_call_table) {
	printk(KERN_ERR "Couldn't find sys_call_table.\n");
	return -EPERM;  /* operation not permitted; couldn't find general error */
    }

    /*
     * Replace the entry for open with our own function. We save the location
     * of the real sys_open so we can put it back when we're unloaded.
     */
    DISABLE_WRITE_PROTECTION;
    original_sys_open = (void *) sys_call_table[__NR_open];
    sys_call_table[__NR_open] = (unsigned long *) rickroll_open;
    ENABLE_WRITE_PROTECTION;

    printk(KERN_INFO "Never gonna give you up!\n");
    return 0;  /* zero indicates success */
}


/*
 * Our replacement for sys_open, which forwards to the real sys_open unless the
 * file name ends with .mp3, in which case it opens the rick roll file instead.
 */
asmlinkage long rickroll_open(const char __user *filename, int flags, umode_t mode)
{
    int len = strlen(filename);

    /* See if we should hijack the open */
    if(strcmp(filename + len - 4, ".mp3")) {
	/* Just pass through to the real sys_open if the extension isn't .mp3 */
	return (*original_sys_open)(filename, flags, mode);
    } else {
	/* Otherwise we're going to hijack the open */
	mm_segment_t old_fs;
	long fd;

	/*
	 * sys_open checks to see if the filename is a pointer to user space
	 * memory. When we're hijacking, the filename we pass will be in kernel
	 * memory. To get around this, we juggle some segment registers. I
	 * believe fs is the segment used for user space, and we're temporarily
	 * changing it to be the segment the kernel uses.
	 *
	 * An alternative would be to use read_from_user() and copy_to_user()
	 * and place the rickroll filename at the location the user code passed
	 * in, saving and restoring the memory we overwrite.
	 */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	/* Open the rickroll file instead */
	fd = (*original_sys_open)(rickroll_filename, flags, mode);

	/* Restore fs to its original value */
	set_fs(old_fs);

	return fd;
    }
}


static void __exit rickroll_cleanup(void)
{
    printk(KERN_INFO "Ok, now we're gonna give you up. Sorry.\n");

    /* Restore the original sys_open in the table */
    DISABLE_WRITE_PROTECTION;
    sys_call_table[__NR_open] = (unsigned long *) original_sys_open;
    ENABLE_WRITE_PROTECTION;
}


/*
 * Finds the system call table's location in memory.
 *
 * This is necessary because the sys_call_table symbol is not exported. We find
 * it by iterating through kernel space memory, and looking for a known system
 * call's address. We use sys_close because all the examples I saw used
 * sys_close. Since we know the offset of the pointer to sys_close in the table
 * (__NR_close), we can get the table's base address.
 */
static unsigned long **find_sys_call_table() {
    unsigned long offset;
    unsigned long **sct;

    for(offset = PAGE_OFFSET; offset < ULLONG_MAX; offset += sizeof(void *)) {
	sct = (unsigned long **) offset;

	if(sct[__NR_close] == (unsigned long *) sys_close)
	    return sct;
    }

    /*
     * Given the loop limit, it's somewhat unlikely we'll get here. I don't
     * even know if we can attempt to fetch such high addresses from memory,
     * and even if you can, it will take a while!
     */
    return NULL;
}


module_init(rickroll_init);
module_exit(rickroll_cleanup);
