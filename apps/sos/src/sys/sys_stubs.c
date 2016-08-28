/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

/* Dummy implementations of any syscalls we do not implement */

#ifdef ARCH_IA32

long sys_restart_syscall()
{
    assert(!"sys_restart_syscall not implemented");
    __builtin_unreachable();
}
/*long sys_exit() {
	assert(!"sys_exit not implemented");
	__builtin_unreachable();
}*/
long sys_fork()
{
    assert(!"sys_fork not implemented");
    __builtin_unreachable();
}
/*long sys_read()
{
    assert(!"sys_read not implemented");
    __builtin_unreachable();
}*/
long sys_write()
{
    assert(!"sys_write not implemented");
    __builtin_unreachable();
}
long sys_waitpid()
{
    assert(!"sys_waitpid not implemented");
    __builtin_unreachable();
}
long sys_creat()
{
    assert(!"sys_creat not implemented");
    __builtin_unreachable();
}
long sys_link()
{
    assert(!"sys_link not implemented");
    __builtin_unreachable();
}
long sys_unlink()
{
    assert(!"sys_unlink not implemented");
    __builtin_unreachable();
}
long sys_execve()
{
    assert(!"sys_execve not implemented");
    __builtin_unreachable();
}
long sys_chdir()
{
    assert(!"sys_chdir not implemented");
    __builtin_unreachable();
}
long sys_time()
{
    assert(!"sys_time not implemented");
    __builtin_unreachable();
}
long sys_mknod()
{
    assert(!"sys_mknod not implemented");
    __builtin_unreachable();
}
long sys_chmod()
{
    assert(!"sys_chmod not implemented");
    __builtin_unreachable();
}
long sys_lchown()
{
    assert(!"sys_lchown not implemented");
    __builtin_unreachable();
}
long sys_break()
{
    assert(!"sys_break not implemented");
    __builtin_unreachable();
}
long sys_oldstat()
{
    assert(!"sys_oldstat not implemented");
    __builtin_unreachable();
}
/*long sys_getpid()
{
    assert(!"sys_getpid not implemented");
    __builtin_unreachable();
}*/
long sys_mount()
{
    assert(!"sys_mount not implemented");
    __builtin_unreachable();
}
long sys_umount()
{
    assert(!"sys_umount not implemented");
    __builtin_unreachable();
}
long sys_setuid()
{
    assert(!"sys_setuid not implemented");
    __builtin_unreachable();
}
long sys_getuid()
{
    assert(!"sys_getuid not implemented");
    __builtin_unreachable();
}
long sys_stime()
{
    assert(!"sys_stime not implemented");
    __builtin_unreachable();
}
long sys_ptrace()
{
    assert(!"sys_ptrace not implemented");
    __builtin_unreachable();
}
long sys_alarm()
{
    assert(!"sys_alarm not implemented");
    __builtin_unreachable();
}
long sys_oldfstat()
{
    assert(!"sys_oldfstat not implemented");
    __builtin_unreachable();
}
long sys_pause()
{
    assert(!"sys_pause not implemented");
    __builtin_unreachable();
}
long sys_utime()
{
    assert(!"sys_utime not implemented");
    __builtin_unreachable();
}
long sys_stty()
{
    assert(!"sys_stty not implemented");
    __builtin_unreachable();
}
long sys_gtty()
{
    assert(!"sys_gtty not implemented");
    __builtin_unreachable();
}
long sys_access()
{
    assert(!"sys_access not implemented");
    __builtin_unreachable();
}
long sys_nice()
{
    assert(!"sys_nice not implemented");
    __builtin_unreachable();
}
long sys_ftime()
{
    assert(!"sys_ftime not implemented");
    __builtin_unreachable();
}
long sys_sync()
{
    assert(!"sys_sync not implemented");
    __builtin_unreachable();
}
long sys_kill()
{
    assert(!"sys_kill not implemented");
    __builtin_unreachable();
}
long sys_rename()
{
    assert(!"sys_rename not implemented");
    __builtin_unreachable();
}
long sys_mkdir()
{
    assert(!"sys_mkdir not implemented");
    __builtin_unreachable();
}
long sys_rmdir()
{
    assert(!"sys_rmdir not implemented");
    __builtin_unreachable();
}
long sys_dup()
{
    assert(!"sys_dup not implemented");
    __builtin_unreachable();
}
long sys_pipe()
{
    assert(!"sys_pipe not implemented");
    __builtin_unreachable();
}
long sys_times()
{
    assert(!"sys_times not implemented");
    __builtin_unreachable();
}
long sys_prof()
{
    assert(!"sys_prof not implemented");
    __builtin_unreachable();
}
/*long sys_brk() {
	assert(!"sys_brk not implemented");
	__builtin_unreachable();
}*/
long sys_setgid()
{
    assert(!"sys_setgid not implemented");
    __builtin_unreachable();
}
long sys_getgid()
{
    assert(!"sys_getgid not implemented");
    __builtin_unreachable();
}
long sys_signal()
{
    assert(!"sys_signal not implemented");
    __builtin_unreachable();
}
long sys_geteuid()
{
    assert(!"sys_geteuid not implemented");
    __builtin_unreachable();
}
long sys_getegid()
{
    assert(!"sys_getegid not implemented");
    __builtin_unreachable();
}
long sys_acct()
{
    assert(!"sys_acct not implemented");
    __builtin_unreachable();
}
long sys_umount2()
{
    assert(!"sys_umount2 not implemented");
    __builtin_unreachable();
}
long sys_lock()
{
    assert(!"sys_lock not implemented");
    __builtin_unreachable();
}
/*long sys_ioctl() {
	assert(!"sys_ioctl not implemented");
	__builtin_unreachable();
}*/
long sys_fcntl()
{
    assert(!"sys_fcntl not implemented");
    __builtin_unreachable();
}
long sys_mpx()
{
    assert(!"sys_mpx not implemented");
    __builtin_unreachable();
}
long sys_setpgid()
{
    assert(!"sys_setpgid not implemented");
    __builtin_unreachable();
}
long sys_ulimit()
{
    assert(!"sys_ulimit not implemented");
    __builtin_unreachable();
}
long sys_oldolduname()
{
    assert(!"sys_oldolduname not implemented");
    __builtin_unreachable();
}
long sys_umask()
{
    assert(!"sys_umask not implemented");
    __builtin_unreachable();
}
long sys_chroot()
{
    assert(!"sys_chroot not implemented");
    __builtin_unreachable();
}
long sys_ustat()
{
    assert(!"sys_ustat not implemented");
    __builtin_unreachable();
}
long sys_dup2()
{
    assert(!"sys_dup2 not implemented");
    __builtin_unreachable();
}
long sys_getppid()
{
    assert(!"sys_getppid not implemented");
    __builtin_unreachable();
}
long sys_getpgrp()
{
    assert(!"sys_getpgrp not implemented");
    __builtin_unreachable();
}
long sys_setsid()
{
    assert(!"sys_setsid not implemented");
    __builtin_unreachable();
}
long sys_sigaction()
{
    assert(!"sys_sigaction not implemented");
    __builtin_unreachable();
}
long sys_sgetmask()
{
    assert(!"sys_sgetmask not implemented");
    __builtin_unreachable();
}
long sys_ssetmask()
{
    assert(!"sys_ssetmask not implemented");
    __builtin_unreachable();
}
long sys_setreuid()
{
    assert(!"sys_setreuid not implemented");
    __builtin_unreachable();
}
long sys_setregid()
{
    assert(!"sys_setregid not implemented");
    __builtin_unreachable();
}
long sys_sigsuspend()
{
    assert(!"sys_sigsuspend not implemented");
    __builtin_unreachable();
}
long sys_sigpending()
{
    assert(!"sys_sigpending not implemented");
    __builtin_unreachable();
}
long sys_sethostname()
{
    assert(!"sys_sethostname not implemented");
    __builtin_unreachable();
}
long sys_setrlimit()
{
    assert(!"sys_setrlimit not implemented");
    __builtin_unreachable();
}
long sys_getrlimit()
{
    assert(!"sys_getrlimit not implemented");
    __builtin_unreachable();
}
long sys_getrusage()
{
    assert(!"sys_getrusage not implemented");
    __builtin_unreachable();
}
long sys_gettimeofday()
{
    assert(!"sys_gettimeofday not implemented");
    __builtin_unreachable();
}
long sys_settimeofday()
{
    assert(!"sys_settimeofday not implemented");
    __builtin_unreachable();
}
long sys_getgroups()
{
    assert(!"sys_getgroups not implemented");
    __builtin_unreachable();
}
long sys_setgroups()
{
    assert(!"sys_setgroups not implemented");
    __builtin_unreachable();
}
long sys_select()
{
    assert(!"sys_select not implemented");
    __builtin_unreachable();
}
long sys_symlink()
{
    assert(!"sys_symlink not implemented");
    __builtin_unreachable();
}
long sys_oldlstat()
{
    assert(!"sys_oldlstat not implemented");
    __builtin_unreachable();
}
long sys_readlink()
{
    assert(!"sys_readlink not implemented");
    __builtin_unreachable();
}
long sys_uselib()
{
    assert(!"sys_uselib not implemented");
    __builtin_unreachable();
}
long sys_swapon()
{
    assert(!"sys_swapon not implemented");
    __builtin_unreachable();
}
long sys_reboot()
{
    assert(!"sys_reboot not implemented");
    __builtin_unreachable();
}
long sys_readdir()
{
    assert(!"sys_readdir not implemented");
    __builtin_unreachable();
}
long sys_mmap()
{
    assert(!"sys_mmap not implemented");
    __builtin_unreachable();
}
/*long sys_munmap()
{
    assert(!"sys_munmap not implemented");
    __builtin_unreachable();
}*/
long sys_truncate()
{
    assert(!"sys_truncate not implemented");
    __builtin_unreachable();
}
long sys_ftruncate()
{
    assert(!"sys_ftruncate not implemented");
    __builtin_unreachable();
}
long sys_fchmod()
{
    assert(!"sys_fchmod not implemented");
    __builtin_unreachable();
}
long sys_fchown()
{
    assert(!"sys_fchown not implemented");
    __builtin_unreachable();
}
long sys_getpriority()
{
    assert(!"sys_getpriority not implemented");
    __builtin_unreachable();
}
long sys_setpriority()
{
    assert(!"sys_setpriority not implemented");
    __builtin_unreachable();
}
long sys_profil()
{
    assert(!"sys_profil not implemented");
    __builtin_unreachable();
}
long sys_statfs()
{
    assert(!"sys_statfs not implemented");
    __builtin_unreachable();
}
long sys_fstatfs()
{
    assert(!"sys_fstatfs not implemented");
    __builtin_unreachable();
}
long sys_ioperm()
{
    assert(!"sys_ioperm not implemented");
    __builtin_unreachable();
}
long sys_socketcall()
{
    assert(!"sys_socketcall not implemented");
    __builtin_unreachable();
}
long sys_syslog()
{
    assert(!"sys_syslog not implemented");
    __builtin_unreachable();
}
long sys_setitimer()
{
    assert(!"sys_setitimer not implemented");
    __builtin_unreachable();
}
long sys_getitimer()
{
    assert(!"sys_getitimer not implemented");
    __builtin_unreachable();
}
long sys_stat()
{
    assert(!"sys_stat not implemented");
    __builtin_unreachable();
}
long sys_lstat()
{
    assert(!"sys_lstat not implemented");
    __builtin_unreachable();
}
long sys_fstat()
{
    assert(!"sys_fstat not implemented");
    __builtin_unreachable();
}
long sys_olduname()
{
    assert(!"sys_olduname not implemented");
    __builtin_unreachable();
}
long sys_iopl()
{
    assert(!"sys_iopl not implemented");
    __builtin_unreachable();
}
long sys_vhangup()
{
    assert(!"sys_vhangup not implemented");
    __builtin_unreachable();
}
long sys_idle()
{
    assert(!"sys_idle not implemented");
    __builtin_unreachable();
}
long sys_vm86old()
{
    assert(!"sys_vm86old not implemented");
    __builtin_unreachable();
}
long sys_wait4()
{
    assert(!"sys_wait4 not implemented");
    __builtin_unreachable();
}
long sys_swapoff()
{
    assert(!"sys_swapoff not implemented");
    __builtin_unreachable();
}
long sys_sysinfo()
{
    assert(!"sys_sysinfo not implemented");
    __builtin_unreachable();
}
long sys_ipc()
{
    assert(!"sys_ipc not implemented");
    __builtin_unreachable();
}
long sys_fsync()
{
    assert(!"sys_fsync not implemented");
    __builtin_unreachable();
}
long sys_sigreturn()
{
    assert(!"sys_sigreturn not implemented");
    __builtin_unreachable();
}
long sys_clone()
{
    assert(!"sys_clone not implemented");
    __builtin_unreachable();
}
long sys_setdomainname()
{
    assert(!"sys_setdomainname not implemented");
    __builtin_unreachable();
}
long sys_uname()
{
    assert(!"sys_uname not implemented");
    __builtin_unreachable();
}
long sys_modify_ldt()
{
    assert(!"sys_modify_ldt not implemented");
    __builtin_unreachable();
}
long sys_adjtimex()
{
    assert(!"sys_adjtimex not implemented");
    __builtin_unreachable();
}
long sys_mprotect()
{
    assert(!"sys_mprotect not implemented");
    __builtin_unreachable();
}
long sys_sigprocmask()
{
    assert(!"sys_sigprocmask not implemented");
    __builtin_unreachable();
}
long sys_create_module()
{
    assert(!"sys_create_module not implemented");
    __builtin_unreachable();
}
long sys_init_module()
{
    assert(!"sys_init_module not implemented");
    __builtin_unreachable();
}
long sys_delete_module()
{
    assert(!"sys_delete_module not implemented");
    __builtin_unreachable();
}
long sys_get_kernel_syms()
{
    assert(!"sys_get_kernel_syms not implemented");
    __builtin_unreachable();
}
long sys_quotactl()
{
    assert(!"sys_quotactl not implemented");
    __builtin_unreachable();
}
long sys_getpgid()
{
    assert(!"sys_getpgid not implemented");
    __builtin_unreachable();
}
long sys_fchdir()
{
    assert(!"sys_fchdir not implemented");
    __builtin_unreachable();
}
long sys_bdflush()
{
    assert(!"sys_bdflush not implemented");
    __builtin_unreachable();
}
long sys_sysfs()
{
    assert(!"sys_sysfs not implemented");
    __builtin_unreachable();
}
long sys_personality()
{
    assert(!"sys_personality not implemented");
    __builtin_unreachable();
}
long sys_afs_syscall()
{
    assert(!"sys_afs_syscall not implemented");
    __builtin_unreachable();
}
long sys_setfsuid()
{
    assert(!"sys_setfsuid not implemented");
    __builtin_unreachable();
}
long sys_setfsgid()
{
    assert(!"sys_setfsgid not implemented");
    __builtin_unreachable();
}
/*long sys__llseek()
{
    assert(!"sys__llseek not implemented");
    __builtin_unreachable();
}*/
long sys_getdents()
{
    assert(!"sys_getdents not implemented");
    __builtin_unreachable();
}
long sys__newselect()
{
    assert(!"sys__newselect not implemented");
    __builtin_unreachable();
}
long sys_flock()
{
    assert(!"sys_flock not implemented");
    __builtin_unreachable();
}
long sys_msync()
{
    assert(!"sys_msync not implemented");
    __builtin_unreachable();
}
/*long sys_readv() {
	assert(!"sys_readv not implemented");
	__builtin_unreachable();
}*/
/*long sys_writev() {
	assert(!"sys_writev not implemented");
	__builtin_unreachable();
}*/
long sys_getsid()
{
    assert(!"sys_getsid not implemented");
    __builtin_unreachable();
}
long sys_fdatasync()
{
    assert(!"sys_fdatasync not implemented");
    __builtin_unreachable();
}
long sys__sysctl()
{
    assert(!"sys__sysctl not implemented");
    __builtin_unreachable();
}
long sys_mlock()
{
    assert(!"sys_mlock not implemented");
    __builtin_unreachable();
}
long sys_munlock()
{
    assert(!"sys_munlock not implemented");
    __builtin_unreachable();
}
long sys_mlockall()
{
    assert(!"sys_mlockall not implemented");
    __builtin_unreachable();
}
long sys_munlockall()
{
    assert(!"sys_munlockall not implemented");
    __builtin_unreachable();
}
long sys_sched_setparam()
{
    assert(!"sys_sched_setparam not implemented");
    __builtin_unreachable();
}
long sys_sched_getparam()
{
    assert(!"sys_sched_getparam not implemented");
    __builtin_unreachable();
}
long sys_sched_setscheduler()
{
    assert(!"sys_sched_setscheduler not implemented");
    __builtin_unreachable();
}
long sys_sched_getscheduler()
{
    assert(!"sys_sched_getscheduler not implemented");
    __builtin_unreachable();
}
long sys_sched_get_priority_max()
{
    assert(!"sys_sched_get_priority_max not implemented");
    __builtin_unreachable();
}
long sys_sched_get_priority_min()
{
    assert(!"sys_sched_get_priority_min not implemented");
    __builtin_unreachable();
}
long sys_sched_rr_get_interval()
{
    assert(!"sys_sched_rr_get_interval not implemented");
    __builtin_unreachable();
}
long sys_nanosleep()
{
    assert(!"sys_nanosleep not implemented");
    __builtin_unreachable();
}
long sys_setresuid()
{
    assert(!"sys_setresuid not implemented");
    __builtin_unreachable();
}
long sys_getresuid()
{
    assert(!"sys_getresuid not implemented");
    __builtin_unreachable();
}
long sys_vm86()
{
    assert(!"sys_vm86 not implemented");
    __builtin_unreachable();
}
long sys_query_module()
{
    assert(!"sys_query_module not implemented");
    __builtin_unreachable();
}
long sys_poll()
{
    assert(!"sys_poll not implemented");
    __builtin_unreachable();
}
long sys_nfsservctl()
{
    assert(!"sys_nfsservctl not implemented");
    __builtin_unreachable();
}
long sys_setresgid()
{
    assert(!"sys_setresgid not implemented");
    __builtin_unreachable();
}
long sys_getresgid()
{
    assert(!"sys_getresgid not implemented");
    __builtin_unreachable();
}
long sys_prctl()
{
    assert(!"sys_prctl not implemented");
    __builtin_unreachable();
}
long sys_rt_sigreturn()
{
    assert(!"sys_rt_sigreturn not implemented");
    __builtin_unreachable();
}
long sys_rt_sigaction()
{
    assert(!"sys_rt_sigaction not implemented");
    __builtin_unreachable();
}
/*long sys_rt_sigprocmask()
{
    assert(!"sys_rt_sigprocmask not implemented");
    __builtin_unreachable();
}*/
long sys_rt_sigpending()
{
    assert(!"sys_rt_sigpending not implemented");
    __builtin_unreachable();
}
long sys_rt_sigtimedwait()
{
    assert(!"sys_rt_sigtimedwait not implemented");
    __builtin_unreachable();
}
long sys_rt_sigqueueinfo()
{
    assert(!"sys_rt_sigqueueinfo not implemented");
    __builtin_unreachable();
}
long sys_rt_sigsuspend()
{
    assert(!"sys_rt_sigsuspend not implemented");
    __builtin_unreachable();
}
long sys_pread64()
{
    assert(!"sys_pread64 not implemented");
    __builtin_unreachable();
}
long sys_pwrite64()
{
    assert(!"sys_pwrite64 not implemented");
    __builtin_unreachable();
}
long sys_chown()
{
    assert(!"sys_chown not implemented");
    __builtin_unreachable();
}
long sys_getcwd()
{
    assert(!"sys_getcwd not implemented");
    __builtin_unreachable();
}
long sys_capget()
{
    assert(!"sys_capget not implemented");
    __builtin_unreachable();
}
long sys_capset()
{
    assert(!"sys_capset not implemented");
    __builtin_unreachable();
}
long sys_sigaltstack()
{
    assert(!"sys_sigaltstack not implemented");
    __builtin_unreachable();
}
long sys_sendfile()
{
    assert(!"sys_sendfile not implemented");
    __builtin_unreachable();
}
long sys_getpmsg()
{
    assert(!"sys_getpmsg not implemented");
    __builtin_unreachable();
}
long sys_putpmsg()
{
    assert(!"sys_putpmsg not implemented");
    __builtin_unreachable();
}
long sys_vfork()
{
    assert(!"sys_vfork not implemented");
    __builtin_unreachable();
}
long sys_ugetrlimit()
{
    assert(!"sys_ugetrlimit not implemented");
    __builtin_unreachable();
}
/*long sys_mmap2() {
	assert(!"sys_mmap2 not implemented");
	__builtin_unreachable();
}*/
long sys_truncate64()
{
    assert(!"sys_truncate64 not implemented");
    __builtin_unreachable();
}
long sys_ftruncate64()
{
    assert(!"sys_ftruncate64 not implemented");
    __builtin_unreachable();
}
long sys_stat64()
{
    assert(!"sys_stat64 not implemented");
    __builtin_unreachable();
}
long sys_lstat64()
{
    assert(!"sys_lstat64 not implemented");
    __builtin_unreachable();
}
long sys_fstat64()
{
    assert(!"sys_fstat64 not implemented");
    __builtin_unreachable();
}
long sys_lchown32()
{
    assert(!"sys_lchown32 not implemented");
    __builtin_unreachable();
}
long sys_getuid32()
{
    assert(!"sys_getuid32 not implemented");
    __builtin_unreachable();
}
long sys_getgid32()
{
    assert(!"sys_getgid32 not implemented");
    __builtin_unreachable();
}
long sys_geteuid32()
{
    assert(!"sys_geteuid32 not implemented");
    __builtin_unreachable();
}
long sys_getegid32()
{
    assert(!"sys_getegid32 not implemented");
    __builtin_unreachable();
}
long sys_setreuid32()
{
    assert(!"sys_setreuid32 not implemented");
    __builtin_unreachable();
}
long sys_setregid32()
{
    assert(!"sys_setregid32 not implemented");
    __builtin_unreachable();
}
long sys_getgroups32()
{
    assert(!"sys_getgroups32 not implemented");
    __builtin_unreachable();
}
long sys_setgroups32()
{
    assert(!"sys_setgroups32 not implemented");
    __builtin_unreachable();
}
long sys_fchown32()
{
    assert(!"sys_fchown32 not implemented");
    __builtin_unreachable();
}
long sys_setresuid32()
{
    assert(!"sys_setresuid32 not implemented");
    __builtin_unreachable();
}
long sys_getresuid32()
{
    assert(!"sys_getresuid32 not implemented");
    __builtin_unreachable();
}
long sys_setresgid32()
{
    assert(!"sys_setresgid32 not implemented");
    __builtin_unreachable();
}
long sys_getresgid32()
{
    assert(!"sys_getresgid32 not implemented");
    __builtin_unreachable();
}
long sys_chown32()
{
    assert(!"sys_chown32 not implemented");
    __builtin_unreachable();
}
long sys_setuid32()
{
    assert(!"sys_setuid32 not implemented");
    __builtin_unreachable();
}
long sys_setgid32()
{
    assert(!"sys_setgid32 not implemented");
    __builtin_unreachable();
}
long sys_setfsuid32()
{
    assert(!"sys_setfsuid32 not implemented");
    __builtin_unreachable();
}
long sys_setfsgid32()
{
    assert(!"sys_setfsgid32 not implemented");
    __builtin_unreachable();
}
long sys_pivot_root()
{
    assert(!"sys_pivot_root not implemented");
    __builtin_unreachable();
}
long sys_mincore()
{
    assert(!"sys_mincore not implemented");
    __builtin_unreachable();
}
long sys_madvise()
{
    assert(!"sys_madvise not implemented");
    __builtin_unreachable();
}
long sys_madvise1()
{
    assert(!"sys_madvise1 not implemented");
    __builtin_unreachable();
}
long sys_getdents64()
{
    assert(!"sys_getdents64 not implemented");
    __builtin_unreachable();
}
long sys_fcntl64()
{
    assert(!"sys_fcntl64 not implemented");
    __builtin_unreachable();
}
/*long sys_gettid()
{
    assert(!"sys_gettid not implemented");
    __builtin_unreachable();
}*/
long sys_readahead()
{
    assert(!"sys_readahead not implemented");
    __builtin_unreachable();
}
long sys_setxattr()
{
    assert(!"sys_setxattr not implemented");
    __builtin_unreachable();
}
long sys_lsetxattr()
{
    assert(!"sys_lsetxattr not implemented");
    __builtin_unreachable();
}
long sys_fsetxattr()
{
    assert(!"sys_fsetxattr not implemented");
    __builtin_unreachable();
}
long sys_getxattr()
{
    assert(!"sys_getxattr not implemented");
    __builtin_unreachable();
}
long sys_lgetxattr()
{
    assert(!"sys_lgetxattr not implemented");
    __builtin_unreachable();
}
long sys_fgetxattr()
{
    assert(!"sys_fgetxattr not implemented");
    __builtin_unreachable();
}
long sys_listxattr()
{
    assert(!"sys_listxattr not implemented");
    __builtin_unreachable();
}
long sys_llistxattr()
{
    assert(!"sys_llistxattr not implemented");
    __builtin_unreachable();
}
long sys_flistxattr()
{
    assert(!"sys_flistxattr not implemented");
    __builtin_unreachable();
}
long sys_removexattr()
{
    assert(!"sys_removexattr not implemented");
    __builtin_unreachable();
}
long sys_lremovexattr()
{
    assert(!"sys_lremovexattr not implemented");
    __builtin_unreachable();
}
long sys_fremovexattr()
{
    assert(!"sys_fremovexattr not implemented");
    __builtin_unreachable();
}
/*long sys_tkill()
{
    assert(!"sys_tkill not implemented");
    __builtin_unreachable();
}*/
long sys_sendfile64()
{
    assert(!"sys_sendfile64 not implemented");
    __builtin_unreachable();
}
long sys_futex()
{
    assert(!"sys_futex not implemented");
    __builtin_unreachable();
}
long sys_sched_setaffinity()
{
    assert(!"sys_sched_setaffinity not implemented");
    __builtin_unreachable();
}
long sys_sched_getaffinity()
{
    assert(!"sys_sched_getaffinity not implemented");
    __builtin_unreachable();
}
long sys_set_thread_area()
{
    assert(!"sys_set_thread_area not implemented");
    __builtin_unreachable();
}
long sys_get_thread_area()
{
    assert(!"sys_get_thread_area not implemented");
    __builtin_unreachable();
}
long sys_io_setup()
{
    assert(!"sys_io_setup not implemented");
    __builtin_unreachable();
}
long sys_io_destroy()
{
    assert(!"sys_io_destroy not implemented");
    __builtin_unreachable();
}
long sys_io_getevents()
{
    assert(!"sys_io_getevents not implemented");
    __builtin_unreachable();
}
long sys_io_submit()
{
    assert(!"sys_io_submit not implemented");
    __builtin_unreachable();
}
long sys_io_cancel()
{
    assert(!"sys_io_cancel not implemented");
    __builtin_unreachable();
}
long sys_fadvise64()
{
    assert(!"sys_fadvise64 not implemented");
    __builtin_unreachable();
}
long sys_exit_group()
{
    assert(!"sys_exit_group not implemented");
    __builtin_unreachable();
}
long sys_lookup_dcookie()
{
    assert(!"sys_lookup_dcookie not implemented");
    __builtin_unreachable();
}
long sys_epoll_create()
{
    assert(!"sys_epoll_create not implemented");
    __builtin_unreachable();
}
long sys_epoll_ctl()
{
    assert(!"sys_epoll_ctl not implemented");
    __builtin_unreachable();
}
long sys_epoll_wait()
{
    assert(!"sys_epoll_wait not implemented");
    __builtin_unreachable();
}
long sys_remap_file_pages()
{
    assert(!"sys_remap_file_pages not implemented");
    __builtin_unreachable();
}
/*long sys_set_tid_address()
{
    assert(!"sys_set_tid_address not implemented");
    __builtin_unreachable();
}*/
long sys_timer_create()
{
    assert(!"sys_timer_create not implemented");
    __builtin_unreachable();
}
long sys_timer_settime()
{
    assert(!"sys_timer_settime not implemented");
    __builtin_unreachable();
}
long sys_timer_gettime()
{
    assert(!"sys_timer_gettime not implemented");
    __builtin_unreachable();
}
long sys_timer_getoverrun()
{
    assert(!"sys_timer_getoverrun not implemented");
    __builtin_unreachable();
}
long sys_timer_delete()
{
    assert(!"sys_timer_delete not implemented");
    __builtin_unreachable();
}
long sys_clock_settime()
{
    assert(!"sys_clock_settime not implemented");
    __builtin_unreachable();
}
long sys_clock_gettime()
{
    assert(!"sys_clock_gettime not implemented");
    __builtin_unreachable();
}
long sys_clock_getres()
{
    assert(!"sys_clock_getres not implemented");
    __builtin_unreachable();
}
long sys_clock_nanosleep()
{
    assert(!"sys_clock_nanosleep not implemented");
    __builtin_unreachable();
}
long sys_statfs64()
{
    assert(!"sys_statfs64 not implemented");
    __builtin_unreachable();
}
long sys_fstatfs64()
{
    assert(!"sys_fstatfs64 not implemented");
    __builtin_unreachable();
}
/*long sys_tgkill()
{
    assert(!"sys_tgkill not implemented");
    __builtin_unreachable();
}*/
long sys_utimes()
{
    assert(!"sys_utimes not implemented");
    __builtin_unreachable();
}
long sys_fadvise64_64()
{
    assert(!"sys_fadvise64_64 not implemented");
    __builtin_unreachable();
}
long sys_vserver()
{
    assert(!"sys_vserver not implemented");
    __builtin_unreachable();
}
long sys_mbind()
{
    assert(!"sys_mbind not implemented");
    __builtin_unreachable();
}
long sys_get_mempolicy()
{
    assert(!"sys_get_mempolicy not implemented");
    __builtin_unreachable();
}
long sys_set_mempolicy()
{
    assert(!"sys_set_mempolicy not implemented");
    __builtin_unreachable();
}
long sys_mq_open()
{
    assert(!"sys_mq_open not implemented");
    __builtin_unreachable();
}
long sys_mq_unlink()
{
    assert(!"sys_mq_unlink not implemented");
    __builtin_unreachable();
}
long sys_mq_timedsend()
{
    assert(!"sys_mq_timedsend not implemented");
    __builtin_unreachable();
}
long sys_mq_timedreceive()
{
    assert(!"sys_mq_timedreceive not implemented");
    __builtin_unreachable();
}
long sys_mq_notify()
{
    assert(!"sys_mq_notify not implemented");
    __builtin_unreachable();
}
long sys_mq_getsetattr()
{
    assert(!"sys_mq_getsetattr not implemented");
    __builtin_unreachable();
}
long sys_kexec_load()
{
    assert(!"sys_kexec_load not implemented");
    __builtin_unreachable();
}
long sys_waitid()
{
    assert(!"sys_waitid not implemented");
    __builtin_unreachable();
}
long sys_add_key()
{
    assert(!"sys_add_key not implemented");
    __builtin_unreachable();
}
long sys_request_key()
{
    assert(!"sys_request_key not implemented");
    __builtin_unreachable();
}
long sys_keyctl()
{
    assert(!"sys_keyctl not implemented");
    __builtin_unreachable();
}
long sys_ioprio_set()
{
    assert(!"sys_ioprio_set not implemented");
    __builtin_unreachable();
}
long sys_ioprio_get()
{
    assert(!"sys_ioprio_get not implemented");
    __builtin_unreachable();
}
long sys_inotify_init()
{
    assert(!"sys_inotify_init not implemented");
    __builtin_unreachable();
}
long sys_inotify_add_watch()
{
    assert(!"sys_inotify_add_watch not implemented");
    __builtin_unreachable();
}
long sys_inotify_rm_watch()
{
    assert(!"sys_inotify_rm_watch not implemented");
    __builtin_unreachable();
}
long sys_migrate_pages()
{
    assert(!"sys_migrate_pages not implemented");
    __builtin_unreachable();
}
long sys_openat()
{
    assert(!"sys_openat not implemented");
    __builtin_unreachable();
}
long sys_mkdirat()
{
    assert(!"sys_mkdirat not implemented");
    __builtin_unreachable();
}
long sys_mknodat()
{
    assert(!"sys_mknodat not implemented");
    __builtin_unreachable();
}
long sys_fchownat()
{
    assert(!"sys_fchownat not implemented");
    __builtin_unreachable();
}
long sys_futimesat()
{
    assert(!"sys_futimesat not implemented");
    __builtin_unreachable();
}
long sys_fstatat64()
{
    assert(!"sys_fstatat64 not implemented");
    __builtin_unreachable();
}
long sys_unlinkat()
{
    assert(!"sys_unlinkat not implemented");
    __builtin_unreachable();
}
long sys_renameat()
{
    assert(!"sys_renameat not implemented");
    __builtin_unreachable();
}
long sys_linkat()
{
    assert(!"sys_linkat not implemented");
    __builtin_unreachable();
}
long sys_symlinkat()
{
    assert(!"sys_symlinkat not implemented");
    __builtin_unreachable();
}
long sys_readlinkat()
{
    assert(!"sys_readlinkat not implemented");
    __builtin_unreachable();
}
long sys_fchmodat()
{
    assert(!"sys_fchmodat not implemented");
    __builtin_unreachable();
}
long sys_faccessat()
{
    assert(!"sys_faccessat not implemented");
    __builtin_unreachable();
}
long sys_pselect6()
{
    assert(!"sys_pselect6 not implemented");
    __builtin_unreachable();
}
long sys_ppoll()
{
    assert(!"sys_ppoll not implemented");
    __builtin_unreachable();
}
long sys_unshare()
{
    assert(!"sys_unshare not implemented");
    __builtin_unreachable();
}
long sys_set_robust_list()
{
    assert(!"sys_set_robust_list not implemented");
    __builtin_unreachable();
}
long sys_get_robust_list()
{
    assert(!"sys_get_robust_list not implemented");
    __builtin_unreachable();
}
long sys_splice()
{
    assert(!"sys_splice not implemented");
    __builtin_unreachable();
}
long sys_sync_file_range()
{
    assert(!"sys_sync_file_range not implemented");
    __builtin_unreachable();
}
long sys_tee()
{
    assert(!"sys_tee not implemented");
    __builtin_unreachable();
}
long sys_vmsplice()
{
    assert(!"sys_vmsplice not implemented");
    __builtin_unreachable();
}
long sys_move_pages()
{
    assert(!"sys_move_pages not implemented");
    __builtin_unreachable();
}
long sys_getcpu()
{
    assert(!"sys_getcpu not implemented");
    __builtin_unreachable();
}
long sys_epoll_pwait()
{
    assert(!"sys_epoll_pwait not implemented");
    __builtin_unreachable();
}
long sys_utimensat()
{
    assert(!"sys_utimensat not implemented");
    __builtin_unreachable();
}
long sys_signalfd()
{
    assert(!"sys_signalfd not implemented");
    __builtin_unreachable();
}
long sys_timerfd_create()
{
    assert(!"sys_timerfd_create not implemented");
    __builtin_unreachable();
}
long sys_eventfd()
{
    assert(!"sys_eventfd not implemented");
    __builtin_unreachable();
}
long sys_fallocate()
{
    assert(!"sys_fallocate not implemented");
    __builtin_unreachable();
}
long sys_timerfd_settime()
{
    assert(!"sys_timerfd_settime not implemented");
    __builtin_unreachable();
}
long sys_timerfd_gettime()
{
    assert(!"sys_timerfd_gettime not implemented");
    __builtin_unreachable();
}
long sys_signalfd4()
{
    assert(!"sys_signalfd4 not implemented");
    __builtin_unreachable();
}
long sys_eventfd2()
{
    assert(!"sys_eventfd2 not implemented");
    __builtin_unreachable();
}
long sys_epoll_create1()
{
    assert(!"sys_epoll_create1 not implemented");
    __builtin_unreachable();
}
long sys_dup3()
{
    assert(!"sys_dup3 not implemented");
    __builtin_unreachable();
}
long sys_pipe2()
{
    assert(!"sys_pipe2 not implemented");
    __builtin_unreachable();
}
long sys_inotify_init1()
{
    assert(!"sys_inotify_init1 not implemented");
    __builtin_unreachable();
}
long sys_preadv()
{
    assert(!"sys_preadv not implemented");
    __builtin_unreachable();
}
long sys_pwritev()
{
    assert(!"sys_pwritev not implemented");
    __builtin_unreachable();
}
long sys_name_to_handle_at()
{
    assert(!"sys_name_to_handle_at not implemented");
    __builtin_unreachable();
}
long sys_open_by_handle_at()
{
    assert(!"sys_open_by_handle_at not implemented");
    __builtin_unreachable();
}
long sys_clock_adjtime()
{
    assert(!"sys_clock_adjtime not implemented");
    __builtin_unreachable();
}
long sys_syncfs()
{
    assert(!"sys_syncfs not implemented");
    __builtin_unreachable();
}
long sys_sendmmsg()
{
    assert(!"sys_sendmmsg not implemented");
    __builtin_unreachable();
}
long sys_setns()
{
    assert(!"sys_setns not implemented");
    __builtin_unreachable();
}
long sys_process_vm_readv()
{
    assert(!"sys_process_vm_readv not implemented");
    __builtin_unreachable();
}
long sys_process_vm_writev()
{
    assert(!"sys_process_vm_writev not implemented");
    __builtin_unreachable();
}
long sys_fstatat()
{
    assert(!"sys_fstatat not implemented");
    __builtin_unreachable();
}
long sys_pread()
{
    assert(!"sys_pread not implemented");
    __builtin_unreachable();
}
long sys_pwrite()
{
    assert(!"sys_pwrite not implemented");
    __builtin_unreachable();
}
long sys_fadvise()
{
    assert(!"sys_fadvise not implemented");
    __builtin_unreachable();
}

#endif

#ifdef ARCH_ARM

long sys_restart_syscall()
{
    assert(!"sys_restart_syscall not implemented");
    __builtin_unreachable();
}
/*long sys_exit() {
    assert(!"sys_exit not implemented");
    __builtin_unreachable();
}*/
long sys_fork()
{
    assert(!"sys_fork not implemented");
    __builtin_unreachable();
}
/*long sys_read()
{
    assert(!"sys_read not implemented");
    __builtin_unreachable();
}*/
long sys_write()
{
    assert(!"sys_write not implemented");
    __builtin_unreachable();
}
long sys_creat()
{
    assert(!"sys_creat not implemented");
    __builtin_unreachable();
}
long sys_link()
{
    assert(!"sys_link not implemented");
    __builtin_unreachable();
}
long sys_unlink()
{
    assert(!"sys_unlink not implemented");
    __builtin_unreachable();
}
long sys_execve()
{
    assert(!"sys_execve not implemented");
    __builtin_unreachable();
}
long sys_chdir()
{
    assert(!"sys_chdir not implemented");
    __builtin_unreachable();
}
long sys_mknod()
{
    assert(!"sys_mknod not implemented");
    __builtin_unreachable();
}
long sys_chmod()
{
    assert(!"sys_chmod not implemented");
    __builtin_unreachable();
}
long sys_lchown()
{
    assert(!"sys_lchown not implemented");
    __builtin_unreachable();
}
/*long sys_getpid()
{
    assert(!"sys_getpid not implemented");
    __builtin_unreachable();
}*/
long sys_mount()
{
    assert(!"sys_mount not implemented");
    __builtin_unreachable();
}
long sys_setuid()
{
    assert(!"sys_setuid not implemented");
    __builtin_unreachable();
}
long sys_getuid()
{
    assert(!"sys_getuid not implemented");
    __builtin_unreachable();
}
long sys_ptrace()
{
    assert(!"sys_ptrace not implemented");
    __builtin_unreachable();
}
long sys_pause()
{
    assert(!"sys_pause not implemented");
    __builtin_unreachable();
}
/*long sys_access()
{
    assert(!"sys_access not implemented");
    __builtin_unreachable();
}*/
long sys_nice()
{
    assert(!"sys_nice not implemented");
    __builtin_unreachable();
}
long sys_sync()
{
    assert(!"sys_sync not implemented");
    __builtin_unreachable();
}
long sys_kill()
{
    assert(!"sys_kill not implemented");
    __builtin_unreachable();
}
long sys_rename()
{
    assert(!"sys_rename not implemented");
    __builtin_unreachable();
}
long sys_mkdir()
{
    assert(!"sys_mkdir not implemented");
    __builtin_unreachable();
}
long sys_rmdir()
{
    assert(!"sys_rmdir not implemented");
    __builtin_unreachable();
}
long sys_dup()
{
    assert(!"sys_dup not implemented");
    __builtin_unreachable();
}
long sys_pipe()
{
    assert(!"sys_pipe not implemented");
    __builtin_unreachable();
}
long sys_times()
{
    assert(!"sys_times not implemented");
    __builtin_unreachable();
}
/*long sys_brk() {
    assert(!"sys_brk not implemented");
    __builtin_unreachable();
}*/
long sys_setgid()
{
    assert(!"sys_setgid not implemented");
    __builtin_unreachable();
}
long sys_getgid()
{
    assert(!"sys_getgid not implemented");
    __builtin_unreachable();
}
long sys_geteuid()
{
    assert(!"sys_geteuid not implemented");
    __builtin_unreachable();
}
long sys_getegid()
{
    assert(!"sys_getegid not implemented");
    __builtin_unreachable();
}
long sys_acct()
{
    assert(!"sys_acct not implemented");
    __builtin_unreachable();
}
long sys_umount2()
{
    assert(!"sys_umount2 not implemented");
    __builtin_unreachable();
}
/*long sys_ioctl() {
    assert(!"sys_ioctl not implemented");
    __builtin_unreachable();
}*/
long sys_fcntl()
{
    assert(!"sys_fcntl not implemented");
    __builtin_unreachable();
}
long sys_setpgid()
{
    assert(!"sys_setpgid not implemented");
    __builtin_unreachable();
}
long sys_umask()
{
    assert(!"sys_umask not implemented");
    __builtin_unreachable();
}
long sys_chroot()
{
    assert(!"sys_chroot not implemented");
    __builtin_unreachable();
}
long sys_ustat()
{
    assert(!"sys_ustat not implemented");
    __builtin_unreachable();
}
long sys_dup2()
{
    assert(!"sys_dup2 not implemented");
    __builtin_unreachable();
}
long sys_getppid()
{
    assert(!"sys_getppid not implemented");
    __builtin_unreachable();
}
long sys_getpgrp()
{
    assert(!"sys_getpgrp not implemented");
    __builtin_unreachable();
}
long sys_setsid()
{
    assert(!"sys_setsid not implemented");
    __builtin_unreachable();
}
long sys_sigaction()
{
    assert(!"sys_sigaction not implemented");
    __builtin_unreachable();
}
long sys_setreuid()
{
    assert(!"sys_setreuid not implemented");
    __builtin_unreachable();
}
long sys_setregid()
{
    assert(!"sys_setregid not implemented");
    __builtin_unreachable();
}
long sys_sigsuspend()
{
    assert(!"sys_sigsuspend not implemented");
    __builtin_unreachable();
}
long sys_sigpending()
{
    assert(!"sys_sigpending not implemented");
    __builtin_unreachable();
}
long sys_sethostname()
{
    assert(!"sys_sethostname not implemented");
    __builtin_unreachable();
}
long sys_setrlimit()
{
    assert(!"sys_setrlimit not implemented");
    __builtin_unreachable();
}
long sys_getrusage()
{
    assert(!"sys_getrusage not implemented");
    __builtin_unreachable();
}
long sys_gettimeofday()
{
    assert(!"sys_gettimeofday not implemented");
    __builtin_unreachable();
}
long sys_settimeofday()
{
    assert(!"sys_settimeofday not implemented");
    __builtin_unreachable();
}
long sys_getgroups()
{
    assert(!"sys_getgroups not implemented");
    __builtin_unreachable();
}
long sys_setgroups()
{
    assert(!"sys_setgroups not implemented");
    __builtin_unreachable();
}
long sys_symlink()
{
    assert(!"sys_symlink not implemented");
    __builtin_unreachable();
}
long sys_readlink()
{
    assert(!"sys_readlink not implemented");
    __builtin_unreachable();
}
long sys_uselib()
{
    assert(!"sys_uselib not implemented");
    __builtin_unreachable();
}
long sys_swapon()
{
    assert(!"sys_swapon not implemented");
    __builtin_unreachable();
}
long sys_reboot()
{
    assert(!"sys_reboot not implemented");
    __builtin_unreachable();
}
/*long sys_munmap()
{
    assert(!"sys_munmap not implemented");
    __builtin_unreachable();
}*/
long sys_truncate()
{
    assert(!"sys_truncate not implemented");
    __builtin_unreachable();
}
long sys_ftruncate()
{
    assert(!"sys_ftruncate not implemented");
    __builtin_unreachable();
}
long sys_fchmod()
{
    assert(!"sys_fchmod not implemented");
    __builtin_unreachable();
}
long sys_fchown()
{
    assert(!"sys_fchown not implemented");
    __builtin_unreachable();
}
long sys_getpriority()
{
    assert(!"sys_getpriority not implemented");
    __builtin_unreachable();
}
long sys_setpriority()
{
    assert(!"sys_setpriority not implemented");
    __builtin_unreachable();
}
long sys_statfs()
{
    assert(!"sys_statfs not implemented");
    __builtin_unreachable();
}
long sys_fstatfs()
{
    assert(!"sys_fstatfs not implemented");
    __builtin_unreachable();
}
long sys_syslog()
{
    assert(!"sys_syslog not implemented");
    __builtin_unreachable();
}
long sys_setitimer()
{
    assert(!"sys_setitimer not implemented");
    __builtin_unreachable();
}
long sys_getitimer()
{
    assert(!"sys_getitimer not implemented");
    __builtin_unreachable();
}
long sys_stat()
{
    assert(!"sys_stat not implemented");
    __builtin_unreachable();
}
long sys_lstat()
{
    assert(!"sys_lstat not implemented");
    __builtin_unreachable();
}
long sys_fstat()
{
    assert(!"sys_fstat not implemented");
    __builtin_unreachable();
}
long sys_vhangup()
{
    assert(!"sys_vhangup not implemented");
    __builtin_unreachable();
}
long sys_wait4()
{
    assert(!"sys_wait4 not implemented");
    __builtin_unreachable();
}
long sys_swapoff()
{
    assert(!"sys_swapoff not implemented");
    __builtin_unreachable();
}
long sys_sysinfo()
{
    assert(!"sys_sysinfo not implemented");
    __builtin_unreachable();
}
long sys_fsync()
{
    assert(!"sys_fsync not implemented");
    __builtin_unreachable();
}
long sys_sigreturn()
{
    assert(!"sys_sigreturn not implemented");
    __builtin_unreachable();
}
long sys_clone()
{
    assert(!"sys_clone not implemented");
    __builtin_unreachable();
}
long sys_setdomainname()
{
    assert(!"sys_setdomainname not implemented");
    __builtin_unreachable();
}
long sys_uname()
{
    assert(!"sys_uname not implemented");
    __builtin_unreachable();
}
long sys_adjtimex()
{
    assert(!"sys_adjtimex not implemented");
    __builtin_unreachable();
}
long sys_mprotect()
{
    assert(!"sys_mprotect not implemented");
    __builtin_unreachable();
}
long sys_sigprocmask()
{
    assert(!"sys_sigprocmask not implemented");
    __builtin_unreachable();
}
long sys_init_module()
{
    assert(!"sys_init_module not implemented");
    __builtin_unreachable();
}
long sys_delete_module()
{
    assert(!"sys_delete_module not implemented");
    __builtin_unreachable();
}
long sys_quotactl()
{
    assert(!"sys_quotactl not implemented");
    __builtin_unreachable();
}
long sys_getpgid()
{
    assert(!"sys_getpgid not implemented");
    __builtin_unreachable();
}
long sys_fchdir()
{
    assert(!"sys_fchdir not implemented");
    __builtin_unreachable();
}
long sys_bdflush()
{
    assert(!"sys_bdflush not implemented");
    __builtin_unreachable();
}
long sys_sysfs()
{
    assert(!"sys_sysfs not implemented");
    __builtin_unreachable();
}
long sys_personality()
{
    assert(!"sys_personality not implemented");
    __builtin_unreachable();
}
long sys_setfsuid()
{
    assert(!"sys_setfsuid not implemented");
    __builtin_unreachable();
}
long sys_setfsgid()
{
    assert(!"sys_setfsgid not implemented");
    __builtin_unreachable();
}
long sys_getdents()
{
    assert(!"sys_getdents not implemented");
    __builtin_unreachable();
}
long sys__newselect()
{
    assert(!"sys__newselect not implemented");
    __builtin_unreachable();
}
long sys_flock()
{
    assert(!"sys_flock not implemented");
    __builtin_unreachable();
}
long sys_msync()
{
    assert(!"sys_msync not implemented");
    __builtin_unreachable();
}
/*long sys_readv() {
    assert(!"sys_readv not implemented");
    __builtin_unreachable();
}*/
/*long sys_writev() {
    assert(!"sys_writev not implemented");
    __builtin_unreachable();
}*/
long sys_getsid()
{
    assert(!"sys_getsid not implemented");
    __builtin_unreachable();
}
long sys_fdatasync()
{
    assert(!"sys_fdatasync not implemented");
    __builtin_unreachable();
}
long sys__sysctl()
{
    assert(!"sys__sysctl not implemented");
    __builtin_unreachable();
}
long sys_mlock()
{
    assert(!"sys_mlock not implemented");
    __builtin_unreachable();
}
long sys_munlock()
{
    assert(!"sys_munlock not implemented");
    __builtin_unreachable();
}
long sys_mlockall()
{
    assert(!"sys_mlockall not implemented");
    __builtin_unreachable();
}
long sys_munlockall()
{
    assert(!"sys_munlockall not implemented");
    __builtin_unreachable();
}
long sys_sched_setparam()
{
    assert(!"sys_sched_setparam not implemented");
    __builtin_unreachable();
}
long sys_sched_getparam()
{
    assert(!"sys_sched_getparam not implemented");
    __builtin_unreachable();
}
long sys_sched_setscheduler()
{
    assert(!"sys_sched_setscheduler not implemented");
    __builtin_unreachable();
}
long sys_sched_getscheduler()
{
    assert(!"sys_sched_getscheduler not implemented");
    __builtin_unreachable();
}
long sys_sched_get_priority_max()
{
    assert(!"sys_sched_get_priority_max not implemented");
    __builtin_unreachable();
}
long sys_sched_get_priority_min()
{
    assert(!"sys_sched_get_priority_min not implemented");
    __builtin_unreachable();
}
long sys_sched_rr_get_interval()
{
    assert(!"sys_sched_rr_get_interval not implemented");
    __builtin_unreachable();
}
long sys_nanosleep()
{
    assert(!"sys_nanosleep not implemented");
    __builtin_unreachable();
}

long sys_setresuid()
{
    assert(!"sys_setresuid not implemented");
    __builtin_unreachable();
}
long sys_getresuid()
{
    assert(!"sys_getresuid not implemented");
    __builtin_unreachable();
}
long sys_poll()
{
    assert(!"sys_poll not implemented");
    __builtin_unreachable();
}
long sys_nfsservctl()
{
    assert(!"sys_nfsservctl not implemented");
    __builtin_unreachable();
}
long sys_setresgid()
{
    assert(!"sys_setresgid not implemented");
    __builtin_unreachable();
}
long sys_getresgid()
{
    assert(!"sys_getresgid not implemented");
    __builtin_unreachable();
}
long sys_prctl()
{
    assert(!"sys_prctl not implemented");
    __builtin_unreachable();
}
long sys_rt_sigreturn()
{
    assert(!"sys_rt_sigreturn not implemented");
    __builtin_unreachable();
}
long sys_rt_sigaction()
{
    assert(!"sys_rt_sigaction not implemented");
    __builtin_unreachable();
}
/*long sys_rt_sigprocmask()
{
    assert(!"sys_rt_sigprocmask not implemented");
    __builtin_unreachable();
}*/
long sys_rt_sigpending()
{
    assert(!"sys_rt_sigpending not implemented");
    __builtin_unreachable();
}
long sys_rt_sigtimedwait()
{
    assert(!"sys_rt_sigtimedwait not implemented");
    __builtin_unreachable();
}
long sys_rt_sigqueueinfo()
{
    assert(!"sys_rt_sigqueueinfo not implemented");
    __builtin_unreachable();
}
long sys_rt_sigsuspend()
{
    assert(!"sys_rt_sigsuspend not implemented");
    __builtin_unreachable();
}
long sys_pread64()
{
    assert(!"sys_pread64 not implemented");
    __builtin_unreachable();
}
long sys_pwrite64()
{
    assert(!"sys_pwrite64 not implemented");
    __builtin_unreachable();
}
long sys_chown()
{
    assert(!"sys_chown not implemented");
    __builtin_unreachable();
}
long sys_getcwd()
{
    assert(!"sys_getcwd not implemented");
    __builtin_unreachable();
}
long sys_capget()
{
    assert(!"sys_capget not implemented");
    __builtin_unreachable();
}
long sys_capset()
{
    assert(!"sys_capset not implemented");
    __builtin_unreachable();
}
long sys_sigaltstack()
{
    assert(!"sys_sigaltstack not implemented");
    __builtin_unreachable();
}
long sys_sendfile()
{
    assert(!"sys_sendfile not implemented");
    __builtin_unreachable();
}
long sys_vfork()
{
    assert(!"sys_vfork not implemented");
    __builtin_unreachable();
}
long sys_ugetrlimit()
{
    assert(!"sys_ugetrlimit not implemented");
    __builtin_unreachable();
}
/*long sys_mmap2() {
    assert(!"sys_mmap2 not implemented");
    __builtin_unreachable();
}*/
long sys_truncate64()
{
    assert(!"sys_truncate64 not implemented");
    __builtin_unreachable();
}
long sys_ftruncate64()
{
    assert(!"sys_ftruncate64 not implemented");
    __builtin_unreachable();
}
long sys_stat64()
{
    assert(!"sys_stat64 not implemented");
    __builtin_unreachable();
}
long sys_lstat64()
{
    assert(!"sys_lstat64 not implemented");
    __builtin_unreachable();
}
long sys_fstat64()
{
    assert(!"sys_fstat64 not implemented");
    __builtin_unreachable();
}
long sys_lchown32()
{
    assert(!"sys_lchown32 not implemented");
    __builtin_unreachable();
}
long sys_getuid32()
{
    assert(!"sys_getuid32 not implemented");
    __builtin_unreachable();
}
long sys_getgid32()
{
    assert(!"sys_getgid32 not implemented");
    __builtin_unreachable();
}
long sys_geteuid32()
{
    assert(!"sys_geteuid32 not implemented");
    __builtin_unreachable();
}
long sys_getegid32()
{
    assert(!"sys_getegid32 not implemented");
    __builtin_unreachable();
}
long sys_setreuid32()
{
    assert(!"sys_setreuid32 not implemented");
    __builtin_unreachable();
}
long sys_setregid32()
{
    assert(!"sys_setregid32 not implemented");
    __builtin_unreachable();
}
long sys_getgroups32()
{
    assert(!"sys_getgroups32 not implemented");
    __builtin_unreachable();
}
long sys_setgroups32()
{
    assert(!"sys_setgroups32 not implemented");
    __builtin_unreachable();
}
long sys_fchown32()
{
    assert(!"sys_fchown32 not implemented");
    __builtin_unreachable();
}
long sys_setresuid32()
{
    assert(!"sys_setresuid32 not implemented");
    __builtin_unreachable();
}
long sys_getresuid32()
{
    assert(!"sys_getresuid32 not implemented");
    __builtin_unreachable();
}
long sys_setresgid32()
{
    assert(!"sys_setresgid32 not implemented");
    __builtin_unreachable();
}
long sys_getresgid32()
{
    assert(!"sys_getresgid32 not implemented");
    __builtin_unreachable();
}
long sys_chown32()
{
    assert(!"sys_chown32 not implemented");
    __builtin_unreachable();
}
long sys_setuid32()
{
    assert(!"sys_setuid32 not implemented");
    __builtin_unreachable();
}
long sys_setgid32()
{
    assert(!"sys_setgid32 not implemented");
    __builtin_unreachable();
}
long sys_setfsuid32()
{
    assert(!"sys_setfsuid32 not implemented");
    __builtin_unreachable();
}
long sys_setfsgid32()
{
    assert(!"sys_setfsgid32 not implemented");
    __builtin_unreachable();
}
long sys_getdents64()
{
    assert(!"sys_getdents64 not implemented");
    __builtin_unreachable();
}
long sys_pivot_root()
{
    assert(!"sys_pivot_root not implemented");
    __builtin_unreachable();
}
long sys_mincore()
{
    assert(!"sys_mincore not implemented");
    __builtin_unreachable();
}
long sys_madvise()
{
    assert(!"sys_madvise not implemented");
    __builtin_unreachable();
}
long sys_fcntl64()
{
    assert(!"sys_fcntl64 not implemented");
    __builtin_unreachable();
}
/*long sys_gettid()
{
    assert(!"sys_gettid not implemented");
    __builtin_unreachable();
}*/
long sys_readahead()
{
    assert(!"sys_readahead not implemented");
    __builtin_unreachable();
}
long sys_setxattr()
{
    assert(!"sys_setxattr not implemented");
    __builtin_unreachable();
}
long sys_lsetxattr()
{
    assert(!"sys_lsetxattr not implemented");
    __builtin_unreachable();
}
long sys_fsetxattr()
{
    assert(!"sys_fsetxattr not implemented");
    __builtin_unreachable();
}
long sys_getxattr()
{
    assert(!"sys_getxattr not implemented");
    __builtin_unreachable();
}
long sys_lgetxattr()
{
    assert(!"sys_lgetxattr not implemented");
    __builtin_unreachable();
}
long sys_fgetxattr()
{
    assert(!"sys_fgetxattr not implemented");
    __builtin_unreachable();
}
long sys_listxattr()
{
    assert(!"sys_listxattr not implemented");
    __builtin_unreachable();
}
long sys_llistxattr()
{
    assert(!"sys_llistxattr not implemented");
    __builtin_unreachable();
}
long sys_flistxattr()
{
    assert(!"sys_flistxattr not implemented");
    __builtin_unreachable();
}
long sys_removexattr()
{
    assert(!"sys_removexattr not implemented");
    __builtin_unreachable();
}
long sys_lremovexattr()
{
    assert(!"sys_lremovexattr not implemented");
    __builtin_unreachable();
}
long sys_fremovexattr()
{
    assert(!"sys_fremovexattr not implemented");
    __builtin_unreachable();
}
/*long sys_tkill()
{
    assert(!"sys_tkill not implemented");
    __builtin_unreachable();
}*/
long sys_sendfile64()
{
    assert(!"sys_sendfile64 not implemented");
    __builtin_unreachable();
}
long sys_futex()
{
    assert(!"sys_futex not implemented");
    __builtin_unreachable();
}
long sys_sched_setaffinity()
{
    assert(!"sys_sched_setaffinity not implemented");
    __builtin_unreachable();
}
long sys_sched_getaffinity()
{
    assert(!"sys_sched_getaffinity not implemented");
    __builtin_unreachable();
}
long sys_io_setup()
{
    assert(!"sys_io_setup not implemented");
    __builtin_unreachable();
}
long sys_io_destroy()
{
    assert(!"sys_io_destroy not implemented");
    __builtin_unreachable();
}
long sys_io_getevents()
{
    assert(!"sys_io_getevents not implemented");
    __builtin_unreachable();
}
long sys_io_submit()
{
    assert(!"sys_io_submit not implemented");
    __builtin_unreachable();
}
long sys_io_cancel()
{
    assert(!"sys_io_cancel not implemented");
    __builtin_unreachable();
}
long sys_exit_group()
{
    assert(!"sys_exit_group not implemented");
    __builtin_unreachable();
}
long sys_lookup_dcookie()
{
    assert(!"sys_lookup_dcookie not implemented");
    __builtin_unreachable();
}
long sys_epoll_create()
{
    assert(!"sys_epoll_create not implemented");
    __builtin_unreachable();
}
long sys_epoll_ctl()
{
    assert(!"sys_epoll_ctl not implemented");
    __builtin_unreachable();
}
long sys_epoll_wait()
{
    assert(!"sys_epoll_wait not implemented");
    __builtin_unreachable();
}
long sys_remap_file_pages()
{
    assert(!"sys_remap_file_pages not implemented");
    __builtin_unreachable();
}
/*long sys_set_tid_address()
{
    assert(!"sys_set_tid_address not implemented");
    __builtin_unreachable();
}*/
long sys_timer_create()
{
    assert(!"sys_timer_create not implemented");
    __builtin_unreachable();
}
long sys_timer_settime()
{
    assert(!"sys_timer_settime not implemented");
    __builtin_unreachable();
}
long sys_timer_gettime()
{
    assert(!"sys_timer_gettime not implemented");
    __builtin_unreachable();
}
long sys_timer_getoverrun()
{
    assert(!"sys_timer_getoverrun not implemented");
    __builtin_unreachable();
}
long sys_timer_delete()
{
    assert(!"sys_timer_delete not implemented");
    __builtin_unreachable();
}
long sys_clock_settime()
{
    assert(!"sys_clock_settime not implemented");
    __builtin_unreachable();
}
long sys_clock_gettime()
{
    assert(!"sys_clock_gettime not implemented");
    __builtin_unreachable();
}
long sys_clock_getres()
{
    assert(!"sys_clock_getres not implemented");
    __builtin_unreachable();
}
long sys_clock_nanosleep()
{
    assert(!"sys_clock_nanosleep not implemented");
    __builtin_unreachable();
}
long sys_statfs64()
{
    assert(!"sys_statfs64 not implemented");
    __builtin_unreachable();
}
long sys_fstatfs64()
{
    assert(!"sys_fstatfs64 not implemented");
    __builtin_unreachable();
}
/*long sys_tgkill()
{
    assert(!"sys_tgkill not implemented");
    __builtin_unreachable();
}*/
long sys_utimes()
{
    assert(!"sys_utimes not implemented");
    __builtin_unreachable();
}
long sys_fadvise64_64()
{
    assert(!"sys_fadvise64_64 not implemented");
    __builtin_unreachable();
}
long sys_pciconfig_iobase()
{
    assert(!"sys_pciconfig_iobase not implemented");
    __builtin_unreachable();
}
long sys_pciconfig_read()
{
    assert(!"sys_pciconfig_read not implemented");
    __builtin_unreachable();
}
long sys_pciconfig_write()
{
    assert(!"sys_pciconfig_write not implemented");
    __builtin_unreachable();
}
long sys_mq_open()
{
    assert(!"sys_mq_open not implemented");
    __builtin_unreachable();
}
long sys_mq_unlink()
{
    assert(!"sys_mq_unlink not implemented");
    __builtin_unreachable();
}
long sys_mq_timedsend()
{
    assert(!"sys_mq_timedsend not implemented");
    __builtin_unreachable();
}
long sys_mq_timedreceive()
{
    assert(!"sys_mq_timedreceive not implemented");
    __builtin_unreachable();
}
long sys_mq_notify()
{
    assert(!"sys_mq_notify not implemented");
    __builtin_unreachable();
}
long sys_mq_getsetattr()
{
    assert(!"sys_mq_getsetattr not implemented");
    __builtin_unreachable();
}
long sys_waitid()
{
    assert(!"sys_waitid not implemented");
    __builtin_unreachable();
}
long sys_socket()
{
    assert(!"sys_socket not implemented");
    __builtin_unreachable();
}
long sys_bind()
{
    assert(!"sys_bind not implemented");
    __builtin_unreachable();
}
long sys_connect()
{
    assert(!"sys_connect not implemented");
    __builtin_unreachable();
}
long sys_listen()
{
    assert(!"sys_listen not implemented");
    __builtin_unreachable();
}
long sys_accept()
{
    assert(!"sys_accept not implemented");
    __builtin_unreachable();
}
long sys_getsockname()
{
    assert(!"sys_getsockname not implemented");
    __builtin_unreachable();
}
long sys_getpeername()
{
    assert(!"sys_getpeername not implemented");
    __builtin_unreachable();
}
long sys_socketpair()
{
    assert(!"sys_socketpair not implemented");
    __builtin_unreachable();
}
long sys_send()
{
    assert(!"sys_send not implemented");
    __builtin_unreachable();
}
long sys_sendto()
{
    assert(!"sys_sendto not implemented");
    __builtin_unreachable();
}
long sys_recv()
{
    assert(!"sys_recv not implemented");
    __builtin_unreachable();
}
long sys_recvfrom()
{
    assert(!"sys_recvfrom not implemented");
    __builtin_unreachable();
}
long sys_shutdown()
{
    assert(!"sys_shutdown not implemented");
    __builtin_unreachable();
}
long sys_setsockopt()
{
    assert(!"sys_setsockopt not implemented");
    __builtin_unreachable();
}
long sys_getsockopt()
{
    assert(!"sys_getsockopt not implemented");
    __builtin_unreachable();
}
long sys_sendmsg()
{
    assert(!"sys_sendmsg not implemented");
    __builtin_unreachable();
}
long sys_recvmsg()
{
    assert(!"sys_recvmsg not implemented");
    __builtin_unreachable();
}
long sys_semop()
{
    assert(!"sys_semop not implemented");
    __builtin_unreachable();
}
long sys_semget()
{
    assert(!"sys_semget not implemented");
    __builtin_unreachable();
}
long sys_semctl()
{
    assert(!"sys_semctl not implemented");
    __builtin_unreachable();
}
long sys_msgsnd()
{
    assert(!"sys_msgsnd not implemented");
    __builtin_unreachable();
}
long sys_msgrcv()
{
    assert(!"sys_msgrcv not implemented");
    __builtin_unreachable();
}
long sys_msgget()
{
    assert(!"sys_msgget not implemented");
    __builtin_unreachable();
}
long sys_msgctl()
{
    assert(!"sys_msgctl not implemented");
    __builtin_unreachable();
}
long sys_shmat()
{
    assert(!"sys_shmat not implemented");
    __builtin_unreachable();
}
long sys_shmdt()
{
    assert(!"sys_shmdt not implemented");
    __builtin_unreachable();
}
long sys_shmget()
{
    assert(!"sys_shmget not implemented");
    __builtin_unreachable();
}
long sys_shmctl()
{
    assert(!"sys_shmctl not implemented");
    __builtin_unreachable();
}
long sys_add_key()
{
    assert(!"sys_add_key not implemented");
    __builtin_unreachable();
}
long sys_request_key()
{
    assert(!"sys_request_key not implemented");
    __builtin_unreachable();
}
long sys_keyctl()
{
    assert(!"sys_keyctl not implemented");
    __builtin_unreachable();
}
long sys_semtimedop()
{
    assert(!"sys_semtimedop not implemented");
    __builtin_unreachable();
}
long sys_vserver()
{
    assert(!"sys_vserver not implemented");
    __builtin_unreachable();
}
long sys_ioprio_set()
{
    assert(!"sys_ioprio_set not implemented");
    __builtin_unreachable();
}
long sys_ioprio_get()
{
    assert(!"sys_ioprio_get not implemented");
    __builtin_unreachable();
}
long sys_inotify_init()
{
    assert(!"sys_inotify_init not implemented");
    __builtin_unreachable();
}
long sys_inotify_add_watch()
{
    assert(!"sys_inotify_add_watch not implemented");
    __builtin_unreachable();
}
long sys_inotify_rm_watch()
{
    assert(!"sys_inotify_rm_watch not implemented");
    __builtin_unreachable();
}
long sys_mbind()
{
    assert(!"sys_mbind not implemented");
    __builtin_unreachable();
}
long sys_get_mempolicy()
{
    assert(!"sys_get_mempolicy not implemented");
    __builtin_unreachable();
}
long sys_set_mempolicy()
{
    assert(!"sys_set_mempolicy not implemented");
    __builtin_unreachable();
}
long sys_openat()
{
    assert(!"sys_openat not implemented");
    __builtin_unreachable();
}
long sys_mkdirat()
{
    assert(!"sys_mkdirat not implemented");
    __builtin_unreachable();
}
long sys_mknodat()
{
    assert(!"sys_mknodat not implemented");
    __builtin_unreachable();
}
long sys_fchownat()
{
    assert(!"sys_fchownat not implemented");
    __builtin_unreachable();
}
long sys_futimesat()
{
    assert(!"sys_futimesat not implemented");
    __builtin_unreachable();
}
long sys_fstatat64()
{
    assert(!"sys_fstatat64 not implemented");
    __builtin_unreachable();
}
long sys_unlinkat()
{
    assert(!"sys_unlinkat not implemented");
    __builtin_unreachable();
}
long sys_renameat()
{
    assert(!"sys_renameat not implemented");
    __builtin_unreachable();
}
long sys_linkat()
{
    assert(!"sys_linkat not implemented");
    __builtin_unreachable();
}
long sys_symlinkat()
{
    assert(!"sys_symlinkat not implemented");
    __builtin_unreachable();
}
long sys_readlinkat()
{
    assert(!"sys_readlinkat not implemented");
    __builtin_unreachable();
}
long sys_fchmodat()
{
    assert(!"sys_fchmodat not implemented");
    __builtin_unreachable();
}
long sys_faccessat()
{
    assert(!"sys_faccessat not implemented");
    __builtin_unreachable();
}
long sys_pselect6()
{
    assert(!"sys_pselect6 not implemented");
    __builtin_unreachable();
}
long sys_ppoll()
{
    assert(!"sys_ppoll not implemented");
    __builtin_unreachable();
}
long sys_unshare()
{
    assert(!"sys_unshare not implemented");
    __builtin_unreachable();
}
long sys_set_robust_list()
{
    assert(!"sys_set_robust_list not implemented");
    __builtin_unreachable();
}
long sys_get_robust_list()
{
    assert(!"sys_get_robust_list not implemented");
    __builtin_unreachable();
}
long sys_splice()
{
    assert(!"sys_splice not implemented");
    __builtin_unreachable();
}
long sys_sync_file_range2()
{
    assert(!"sys_sync_file_range2 not implemented");
    __builtin_unreachable();
}
long sys_tee()
{
    assert(!"sys_tee not implemented");
    __builtin_unreachable();
}
long sys_vmsplice()
{
    assert(!"sys_vmsplice not implemented");
    __builtin_unreachable();
}
long sys_move_pages()
{
    assert(!"sys_move_pages not implemented");
    __builtin_unreachable();
}
long sys_getcpu()
{
    assert(!"sys_getcpu not implemented");
    __builtin_unreachable();
}
long sys_epoll_pwait()
{
    assert(!"sys_epoll_pwait not implemented");
    __builtin_unreachable();
}
long sys_kexec_load()
{
    assert(!"sys_kexec_load not implemented");
    __builtin_unreachable();
}
long sys_utimensat()
{
    assert(!"sys_utimensat not implemented");
    __builtin_unreachable();
}
long sys_signalfd()
{
    assert(!"sys_signalfd not implemented");
    __builtin_unreachable();
}
long sys_timerfd_create()
{
    assert(!"sys_timerfd_create not implemented");
    __builtin_unreachable();
}
long sys_eventfd()
{
    assert(!"sys_eventfd not implemented");
    __builtin_unreachable();
}
long sys_fallocate()
{
    assert(!"sys_fallocate not implemented");
    __builtin_unreachable();
}
long sys_timerfd_settime()
{
    assert(!"sys_timerfd_settime not implemented");
    __builtin_unreachable();
}
long sys_timerfd_gettime()
{
    assert(!"sys_timerfd_gettime not implemented");
    __builtin_unreachable();
}
long sys_signalfd4()
{
    assert(!"sys_signalfd4 not implemented");
    __builtin_unreachable();
}
long sys_eventfd2()
{
    assert(!"sys_eventfd2 not implemented");
    __builtin_unreachable();
}
long sys_epoll_create1()
{
    assert(!"sys_epoll_create1 not implemented");
    __builtin_unreachable();
}
long sys_dup3()
{
    assert(!"sys_dup3 not implemented");
    __builtin_unreachable();
}
long sys_pipe2()
{
    assert(!"sys_pipe2 not implemented");
    __builtin_unreachable();
}
long sys_inotify_init1()
{
    assert(!"sys_inotify_init1 not implemented");
    __builtin_unreachable();
}
long sys_preadv()
{
    assert(!"sys_preadv not implemented");
    __builtin_unreachable();
}
long sys_pwritev()
{
    assert(!"sys_pwritev not implemented");
    __builtin_unreachable();
}
long sys_rt_tgsigqueueinfo()
{
    assert(!"sys_rt_tgsigqueueinfo not implemented");
    __builtin_unreachable();
}
long sys_perf_event_open()
{
    assert(!"sys_perf_event_open not implemented");
    __builtin_unreachable();
}
long sys_recvmmsg()
{
    assert(!"sys_recvmmsg not implemented");
    __builtin_unreachable();
}
long sys_accept4()
{
    assert(!"sys_accept4 not implemented");
    __builtin_unreachable();
}
long sys_fanotify_init()
{
    assert(!"sys_fanotify_init not implemented");
    __builtin_unreachable();
}
long sys_fanotify_mark()
{
    assert(!"sys_fanotify_mark not implemented");
    __builtin_unreachable();
}
long sys_name_to_handle_at()
{
    assert(!"sys_name_to_handle_at not implemented");
    __builtin_unreachable();
}
long sys_open_by_handle_at()
{
    assert(!"sys_open_by_handle_at not implemented");
    __builtin_unreachable();
}
long sys_clock_adjtime()
{
    assert(!"sys_clock_adjtime not implemented");
    __builtin_unreachable();
}
long sys_syncfs()
{
    assert(!"sys_syncfs not implemented");
    __builtin_unreachable();
}
long sys_sendmmsg()
{
    assert(!"sys_sendmmsg not implemented");
    __builtin_unreachable();
}
long sys_setns()
{
    assert(!"sys_setns not implemented");
    __builtin_unreachable();
}
long sys_process_vm_readv()
{
    assert(!"sys_process_vm_readv not implemented");
    __builtin_unreachable();
}
long sys_process_vm_writev()
{
    assert(!"sys_process_vm_writev not implemented");
    __builtin_unreachable();
}
long sys_lseek()
{
    assert(!"sys_lseek not implemented");
    __builtin_unreachable();
}
long sys_access()
{
    assert(!"sys_access not implemented");
    __builtin_unreachable();
}
long sys_sched_yield()
{
    assert(!"sys_sched_yield not implemented");
    __builtin_unreachable();
}
long sys_prlimit64()
{
    assert(!"sys_prlimit64 not implemented");
    __builtin_unreachable();
}

#endif
