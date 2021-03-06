#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h> 
#include <asm/uaccess.h>
#include <linux/io.h>　
#include <linux/delay.h>

MODULE_AUTHOR("Koshiro Tanimoto");
MODULE_DESCRIPTION("driver for LED control");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.5");

static volatile u32 *gpio_base = NULL;

static dev_t dev;
static struct cdev cdv;
static struct class *cls = NULL; 

static ssize_t led_write(struct file* filp, const char* buf, size_t count, loff_t* pos)
{
	char c;   //読み込んだ字を入れる変数
        if(copy_from_user(&c,buf,sizeof(char)))
            return -EFAULT;

        if(c == '0'){
	        gpio_base[7] = 1 << 23;
		gpio_base[10] = 1 << 24;
		gpio_base[10] = 1 << 25;
	}

	else if(c == '1'){
		gpio_base[10] = 1 << 23;
		gpio_base[7] = 1 << 24;
                gpio_base[10] = 1 << 25;

	}

	else if(c == '2'){
		gpio_base[10] = 1 << 23;
		gpio_base[10] = 1 << 24;
		gpio_base[7] = 1 << 25;
	}

	else if(c == '3'){
		gpio_base[7] = 1 << 23;
		gpio_base[7] = 1 << 24;
		gpio_base[7] = 1 << 25;
	}

	else if(c == '4'){
		gpio_base[10] = 1 << 23;
		gpio_base[10] = 1 << 24;
		gpio_base[10] = 1 << 25;
	}

                 
        return 1; //読み込んだ文字数を返す（この場合はダミーの1）
}

static struct file_operations led_fops = {
        .owner = THIS_MODULE,
        .write = led_write
};

static int __init init_mod(void)
{
	int retval;

        gpio_base = ioremap_nocache(0x3f200000, 0xA0);

	const u32 led = 23; //GPFSEL2
	const u32 led2 = 24; 
        const u32 led3 = 25;
	  
        const u32 index = led/10; //GPFSEL2

        const u32 shift = (led%10)*3; //9bit~11bit
        const u32 shift2 = (led2%10)*3; //12bit~14bit
        const u32 shift3 = (led3%10)*3; //15bit~17bit
	

        const u32 mask = ~(0x7 << shift); //11111111111111111111100011111111
        const u32 mask2 = ~(0x7 << shift2); 
        const u32 mask3 = ~(0x7 << shift3); 


        gpio_base[index] = (gpio_base[index] & mask) | (0x1 << shift);  //001: output flag
        gpio_base[index] = (gpio_base[index] & mask2) | (0x1 << shift2);
        gpio_base[index] = (gpio_base[index] & mask3) | (0x1 << shift3);

	retval =  alloc_chrdev_region(&dev, 0, 1, "myled");
	if(retval < 0){
		printk(KERN_ERR "alloc_chrdev_region failed.\n");
		return retval;
	}
	printk(KERN_INFO "%s is loaded. major:%d\n",__FILE__,MAJOR(dev));

	cdev_init(&cdv, &led_fops);
        retval = cdev_add(&cdv, dev, 1);
        if(retval < 0){
                printk(KERN_ERR "cdev_add failed. major:%d, minor:%d",MAJOR(dev),MINOR(dev));
                return retval;
        }

	cls = class_create(THIS_MODULE,"myled");  
        if(IS_ERR(cls)){
                printk(KERN_ERR "class_create failed.");
                return PTR_ERR(cls);
        }
        
        device_create(cls, NULL, dev, NULL, "myled%d",MINOR(dev));

        return 0;
}

static void __exit cleanup_mod(void)
{
	cdev_del(&cdv);
	device_destroy(cls, dev);
	class_destroy(cls);
	unregister_chrdev_region(dev, 1);
	printk(KERN_INFO "%s is unloaded. major:%d\n",__FILE__,MAJOR(dev));
}

module_init(init_mod);
module_exit(cleanup_mod);
