/* hello.c
 *
 * "Hello, world" - the loadable kernel module version.
 *
 * Compile this with
 *
 *          gcc -c hello.c -Wall
 */

/* Declare what kind of code we want from the header files */
#define __KERNEL__         /* We're part of the kernel */
#define MODULE             /* Not a permanent part, though. */

/* Standard headers for LKMs */
//#include <linux/modversions.h>
#include <linux/module.h>

#include <linux/tty.h>      /* console_print() interface */

/* Initialize the LKM */
int init_module()
{
  console_print("Hello, world - this is the kernel speaking\n");
  /* More normal is printk(), but there's less that can go wrong with
     console_print(), so let's start simple.
  */

  /* If we return a non zero value, it means that
   * init_module failed and the LKM can't be loaded
   */
  return 0;
}


/* Cleanup - undo whatever init_module did */
void cleanup_module()
{
  console_print("Short is the life of an LKM\n");
}
