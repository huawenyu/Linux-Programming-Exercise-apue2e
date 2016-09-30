# O_ASYNC/FASYNC

Setting of FASYNC are .fasync call of file operations of character device. Here, I tried to look at the pipe of the read. 

It is why setfl in fcntl system call () is called, but where as the processing of FASYNC .fasync of pipe_read_fasync () will be called.
const struct file_operations read_pipefifo_fops = {
       .llseek         = no_llseek,
       .read           = do_sync_read,
       .aio_read       = pipe_read,
       .write          = bad_pipe_w,
       .poll           = pipe_poll,
       .unlocked_ioctl = pipe_ioctl,
       .open           = pipe_read_open,
       .release        = pipe_read_release,
       .fasync         = pipe_read_fasync,
};
In pipe_read_fasync (), the fasync list consuming inode as the fourth argument, from fasync_helper (), by on of the argument, deletion / addition of the processing of the fasync is done.
static int
pipe_read_fasync(int fd, struct file *filp, int on)
{
       struct inode *inode = filp->f_path.dentry->d_inode;
       int retval;

       mutex_lock(&inode->i_mutex);
       retval = fasync_helper(fd, filp, on, &inode->i_pipe->fasync_readers);
       mutex_unlock(&inode->i_mutex);

       return retval;
}
When the fasync_helper () call in on = 1, fasync_insert_entry in the process of adding fasyn object () is called. The first of the for loop to check whether the same FILE object has already been registered. If you are registered fa-> fa_fd = to update the file ID in fd. (send_sigio () it is to become of the argument, but ...? is a specific meaning not good enough.) 

If you are not registered, set the fasync_struc object, and register to fasync_struct list. This time filp-> f_flags | = You are FASYNC. This is because this process is called by the setting of relase lock. new-> magic = FASYNC_MAGIC in message transmission, new-> If FASYNC_MAGIC is not set to magic, looks like the message is not sent. Maybe the fact that this process of around here is change in the future, like the magic number of LKM, and different this magic number in each kernel, not a module that was created under the same kernel (FASYNC_MAGIC are the same), in FASYNC it's so as not to send a signal to speculate, but ....
struct fasync_struct *fasync_insert_entry(int fd, struct file *filp, struct fasync_struct **fapp, struct  fasync_struct *new)
{
       struct fasync_struct *fa, **fp;

       spin_lock(&filp->f_lock);
       spin_lock(&fasync_lock);
       for (fp = fapp; (fa = *fp) != NULL; fp = &fa->fa_next) {
               if (fa->fa_file != filp)
                       continue;

               spin_lock_irq(&fa->fa_lock);
               factory> fa_fd = fd;
               spin_unlock_irq(&fa->fa_lock);
               goto out;
       }

       spin_lock_init(&new->fa_lock);
       new->magic = FASYNC_MAGIC;
       new->fa_file = filp;
       New-> fa_fd = fd;
       new->fa_next = *fapp;
       rcu_assign_pointer(*fapp, new);
       filp->f_flags |= FASYNC;

out:
       spin_unlock(&fasync_lock);
       spin_unlock(&filp->f_lock);
       return fa;
}
pipe_write () is the process of writing the pipe. After writing the data, if there is a task that has been waiting in the pipe to wake up it, to send a SIGIO in kill_fasync (). kill_fasync () will check whether there is an object in fasync_struct object list, in kill_fasync_rcu (), in the process under the list of fasync_struct that FASYNC_MAGIC has been set, perform the signal transmission. 

in if (! (sig == SIGURG && fown-> signum == 0)), sig is in SIGURG of socket, fown-> signum (F_SETSIG of fcntl) it seems that not be sent has not been set.
static ssize_t
pipe_write(struct kiocb *iocb, const struct iovec *_iov,
           unsigned long nr_segs, loff_t PPOs)
{
       struct pipe_inode_info *pipe;
       pipe = inode->i_pipe;
  :
  :
       mutex_unlock(&inode->i_mutex);
       if (do_wakeup) {
               wake_up_interruptible_sync_poll(&pipe->wait, POLLIN | POLLRDNORM);
               kill_fasync(&pipe->fasync_readers, SIGIO, POLL_IN);
       }
       if (ret > 0)
               file_update_time(filp);
       return ret;
}

static void kill_fasync_rcu(struct fasync_struct *fa, int sig, int band)
{
       while (fa) {
               struct fown_struct *fown;
               unsigned long flags;

               if (fa->magic != FASYNC_MAGIC) {
                       printk(KERN_ERR "kill_fasync: bad magic number in "
                              "fasync_struct!\n");
                       return;
               }
               spin_lock_irqsave(&fa->fa_lock, flags);
               if (fa->fa_file) {
                       fown = &fa->fa_file->f_owner;
                       if (!(sig == SIGURG && fown->signum == 0))
                               send_sigio(fown, fa->fa_fd, band);
               }
               spin_unlock_irqrestore(&fa->fa_lock, flags);
               fa = rcu_dereference(fa->fa_next);
       }
}

