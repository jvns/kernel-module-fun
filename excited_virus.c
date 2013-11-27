#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros
#include <linux/fcntl.h> // included for O_RDONLY
#include <linux/fs.h> // included for filp_open

#define MODULE_NAME "rootkit"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Julia Evans");
MODULE_DESCRIPTION("Makes your keyboard WAY MORE EXCITED");

static struct file_operations handler_fops;
const struct file_operations *handler_original = 0;
static ssize_t (*old_tty_read)(struct file * file, char * buf, size_t count,
                    loff_t *ppos) = NULL;


static ssize_t excited_tty_read(struct file * file, char * buf, size_t count,
                    loff_t *ppos) {

  printk("hacking ur readz\n");
  return old_tty_read(file, buf, count, ppos);
}

static int __init excited_init(void)
{
    struct file* file = filp_open("/dev/tty0", O_RDONLY, 0);
    if (file != NULL) {
      old_tty_read = file->f_op->read;
      handler_original = file->f_op;
      handler_fops = *handler_original;
      handler_fops.read = excited_tty_read;
      file->f_op = &handler_fops;
      printk(KERN_INFO "Starting excited time!\n");
      filp_close(file, NULL);
    } else {
      printk("OH NO FAILED!\n");
    };
    return 0;    // Non-zero return means that the module couldn't be loaded.
}

static void __exit excited_cleanup(void)
{
    struct file* file = filp_open("/dev/tty0", O_RDONLY, 0);
    file->f_op = handler_original;
    printk(KERN_INFO "Excited time is over =(\n");
}



module_init(excited_init);
module_exit(excited_cleanup);
