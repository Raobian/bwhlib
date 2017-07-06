
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <pthread.h>
#include <time.h>


#include "liblog.h"

int __d_bug__ = 0;
int __d_info__ = D_BUG | D_INFO;
int __d_goto__ = 1;

uint64_t ylog_max_bytes = 104857600; //100 * 1024 * 1024 = 104857600 100M

ylog_t *ylog = NULL;
ylog_t __ylog__;


int ylog_init(ylogmode_t logmode, const char *file) 
{
        int ret, fd;
        struct stat stbuf;

        if (ylog != NULL) 
                return 0;
        fd = -1;

        if (logmode == YLOG_FILE && file) {
                ret = pthread_rwlock_init(&__ylog__.lock, NULL);
                if (ret) {
                        fprintf(stderr, "ylog init rwlock %u", ret);
                        goto err_ret;
                }

                strcpy(__ylog__.file, file);
                __ylog__.time = 0;

                fd = open(file, O_APPEND | O_CREAT | O_WRONLY, 0644);
                if (fd == -1) {
                        ret = errno;
                        fprintf(stderr, "open(%s, ...) ret (%d) %s\n", file, ret,
                                        strerror(ret));
                        goto err_ret;
                }
                __ylog__.logfd = fd;

                ret = fstat(__ylog__.logfd, &stbuf);
                if (ret) {
                        goto  err_ret;
                }
                __ylog__.size = stbuf.st_size;

        } else if (logmode == YLOG_STDERR) {
                __ylog__.logfd = 2;
                __ylog__.size = 0;
        }

        __ylog__.logmode = logmode;
        ylog = &__ylog__;

        return 0;
err_ret:
        return ret;
}

int ylog_reset()
{
        int ret, fd;
        struct stat stbuf;

        if (ylog->logmode == YLOG_STDERR) {
                return 0;
        }

        ret = pthread_rwlock_wrlock(&ylog->lock);
        if (ret) {
                goto err_ret;
        }

        close(ylog->logfd);

retry:
        fd = open(ylog->file, O_APPEND | O_CREAT | O_WRONLY, 0644);
        if (fd == -1) {
                goto retry;
        }
        ylog->logfd = fd;

        ret = fstat(ylog->logfd, &stbuf);
        if (ret) {
                goto err_unlock;
        }
        ylog->size = stbuf.st_size;

        pthread_rwlock_unlock(&ylog->lock);

        return 0;
err_unlock:
        pthread_rwlock_unlock(&ylog->lock);
err_ret:
        return ret;
}

int ylog_destory()
{
        if (ylog == NULL) 
                return 0;
        if (ylog->logmode == YLOG_FILE && ylog->logfd != -1) 
                close(ylog->logfd);
        return 0;
}


static int __ylog_write_rdlock(const char *_msg)
{
        int ret, len;

        ret = pthread_rwlock_rdlock(&ylog->lock);
        if (ret) {
                goto err_ret;
        }

        len = strlen(_msg);
        ret = write(ylog->logfd, _msg, len);
        if (ret < 0) {
                goto err_ret;
        }
        ylog->size += len;

        pthread_rwlock_unlock(&ylog->lock);

        return 0;
err_ret:
        return ret;
}

static int __ylog_rollover_ifneed(const char *_msg)
{
        int ret;
        char target_file[4096], t[128];
        time_t now;
        struct tm *tm_now;

        if (ylog->size + strlen(_msg) < ylog_max_bytes)
                goto out;

        ret = pthread_rwlock_wrlock(&ylog->lock);
        if (ret) {
                goto err_ret;
        }

        close(ylog->logfd);

        time(&now);
        tm_now = localtime(&now);
        strftime(t, 128, "%Y%m%d-%H%M%S", tm_now);
        snprintf(target_file, 4096, "%s-%s", ylog->file, t);

        ret = rename(ylog->file, target_file);
        if (ret)  {
                ret = errno;
        }

        pthread_rwlock_unlock(&ylog->lock);

        ylog_reset();

out:
        return 0;
err_ret:
        return ret;
}



int ylog_write(const char *_msg)
{
        int ret;

        if (ylog && ylog->logmode == YLOG_FILE && ylog->logfd != -1) {
                ret = __ylog_rollover_ifneed(_msg);
                if (ret) {
                        goto err_ret;
                }

                ret = __ylog_write_rdlock(_msg);
                if (ret) {
                        goto err_ret;
                }
        } else {
                fprintf(stderr, "%s", _msg);
        }

        return 0;
err_ret:
        return ret;
}


int set_ylog_max_bytes(uint64_t max)
{
        ylog_max_bytes = max;
        return 0;
}




