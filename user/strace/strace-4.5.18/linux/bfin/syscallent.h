/*
 * Copyright (c) 1993 Branko Lankester <branko@hacktic.nl>
 * Copyright (c) 1993, 1994, 1995 Rick Sladkey <jrs@world.std.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: syscallent.h 6985 2008-08-09 01:13:40Z vapier $
 */

	{ 0,	0,	sys_restart_syscall,	"restart_syscall"	}, /* 0 */
	{ 1,	TP,	sys_exit,		"_exit", SYS_exit }, /* 1 */
	{ 0,	TP,	sys_fork,		"fork", SYS_fork }, /* 2 */
	{ 3,	TD,	sys_read,		"read", SYS_read }, /* 3 */
	{ 3,	TD,	sys_write,		"write", SYS_write }, /* 4 */
	{ 3,	TD|TF,	sys_open,		"open"		}, /* 5 */
	{ 1,	TD,	sys_close,		"close"		}, /* 6 */
	{ 3,	TP,	sys_waitpid,		"waitpid", SYS_wait4 }, /* 7 */
	{ 2,	TD|TF,	sys_creat,		"creat"		}, /* 8 */
	{ 2,	TF,	sys_link,		"link"		}, /* 9 */
	{ 1,	TF,	sys_unlink,		"unlink"	}, /* 10 */
	{ 3,	TF|TP,	sys_execve,		"execve", SYS_execve }, /* 11 */
	{ 1,	TF,	sys_chdir,		"chdir"		}, /* 12 */
	{ 1,	0,	sys_time,		"time"		}, /* 13 */
	{ 3,	TF,	sys_mknod,		"mknod"		}, /* 14 */
	{ 2,	TF,	sys_chmod,		"chmod"		}, /* 15 */
	{ 3,	TF,	sys_chown,		"chown"		}, /* 16 */
	{ 0,	0,	sys_break,		"break"		}, /* 17 */
	{ 2,	TF,	sys_oldstat,		"oldstat"	}, /* 18 */
	{ 3,	TD,	sys_lseek,		"lseek"		}, /* 19 */
	{ 0,	0,	sys_getpid,		"getpid"	}, /* 20 */
	{ 5,	TF,	sys_mount,		"mount"		}, /* 21 */
	{ 1,	TF,	sys_umount,		"oldumount"	}, /* 22 */
	{ 1,	0,	sys_setuid,		"setuid"	}, /* 23 */
	{ 0,	0,	sys_getuid,		"getuid"	}, /* 24 */
	{ 1,	0,	sys_stime,		"stime"		}, /* 25 */
	{ 4,	0,	sys_ptrace,		"ptrace"	}, /* 26 */
	{ 1,	0,	sys_alarm,		"alarm"		}, /* 27 */
	{ 2,	TD,	sys_oldfstat,		"oldfstat"	}, /* 28 */
	{ 0,	TS,	sys_pause,		"pause"		}, /* 29 */
	{ 2,	TF,	sys_utime,		"utime"		}, /* 30 */
	{ 2,	0,	sys_stty,		"stty"		}, /* 31 */
	{ 2,	0,	sys_gtty,		"gtty"		}, /* 32 */
	{ 2,	TF,	sys_access,		"access"	}, /* 33 */
	{ 1,	0,	sys_nice,		"nice"		}, /* 34 */
	{ 0,	0,	sys_ftime,		"ftime"		}, /* 35 */
	{ 0,	0,	sys_sync,		"sync"		}, /* 36 */
	{ 2,	TS,	sys_kill,		"kill"		}, /* 37 */
	{ 2,	TF,	sys_rename,		"rename"	}, /* 38 */
	{ 2,	TF,	sys_mkdir,		"mkdir"		}, /* 39 */
	{ 1,	TF,	sys_rmdir,		"rmdir"		}, /* 40 */
	{ 1,	TD,	sys_dup,		"dup"		}, /* 41 */
	{ 1,	TD,	sys_pipe,		"pipe"		}, /* 42 */
	{ 1,	0,	sys_times,		"times"		}, /* 43 */
	{ 0,	0,	sys_prof,		"prof"		}, /* 44 */
	{ 1,	0,	sys_brk,		"brk"		}, /* 45 */
	{ 1,	0,	sys_setgid,		"setgid"	}, /* 46 */
	{ 0,	0,	sys_getgid,		"getgid"	}, /* 47 */
	{ 3,	TS,	sys_signal,		"signal"	}, /* 48 */
	{ 0,	0,	sys_geteuid,		"geteuid"	}, /* 49 */
	{ 0,	0,	sys_getegid,		"getegid"	}, /* 50 */
	{ 1,	TF,	sys_acct,		"acct"		}, /* 51 */
	{ 2,	TF,	sys_umount2,		"umount"	}, /* 52 */
	{ 0,	0,	sys_lock,		"lock"		}, /* 53 */
	{ 3,	TD,	sys_ioctl,		"ioctl"		}, /* 54 */
	{ 3,	TD,	sys_fcntl,		"fcntl"		}, /* 55 */
	{ 0,	0,	sys_mpx,		"mpx"		}, /* 56 */
	{ 2,	0,	sys_setpgid,		"setpgid"	}, /* 57 */
	{ 2,	0,	sys_ulimit,		"ulimit"	}, /* 58 */
	{ 1,	0,	sys_oldolduname,	"oldolduname"	}, /* 59 */
	{ 1,	0,	sys_umask,		"umask"		}, /* 60 */
	{ 1,	TF,	sys_chroot,		"chroot"	}, /* 61 */
	{ 2,	0,	sys_ustat,		"ustat"		}, /* 62 */
	{ 2,	TD,	sys_dup2,		"dup2"		}, /* 63 */
	{ 0,	0,	sys_getppid,		"getppid"	}, /* 64 */
	{ 0,	0,	sys_getpgrp,		"getpgrp"	}, /* 65 */
	{ 0,	0,	sys_setsid,		"setsid"	}, /* 66 */
	{ 3,	TS,	sys_sigaction,		"sigaction"	}, /* 67 */
	{ 0,	TS,	sys_siggetmask,		"siggetmask"	}, /* 68 */
	{ 1,	TS,	sys_sigsetmask,		"sigsetmask"	}, /* 69 */
	{ 2,	0,	sys_setreuid,		"setreuid"	}, /* 70 */
	{ 2,	0,	sys_setregid,		"setregid"	}, /* 71 */
	{ 3,	TS,	sys_sigsuspend,		"sigsuspend"	}, /* 72 */
	{ 1,	TS,	sys_sigpending,		"sigpending"	}, /* 73 */
	{ 2,	0,	sys_sethostname,	"sethostname"	}, /* 74 */
	{ 2,	0,	sys_setrlimit,		"setrlimit"	}, /* 75 */
	{ 2,	0,	sys_getrlimit,		"old_getrlimit"	}, /* 76 */
	{ 2,	0,	sys_getrusage,		"getrusage"	}, /* 77 */
	{ 2,	0,	sys_gettimeofday,	"gettimeofday"	}, /* 78 */
	{ 2,	0,	sys_settimeofday,	"settimeofday"	}, /* 79 */
	{ 2,	0,	sys_getgroups,		"getgroups"	}, /* 80 */
	{ 2,	0,	sys_setgroups,		"setgroups"	}, /* 81 */
	{ 1,	TD,	sys_oldselect,		"oldselect"	}, /* 82 */
	{ 2,	TF,	sys_symlink,		"symlink"	}, /* 83 */
	{ 2,	TF,	sys_oldlstat,		"oldlstat"	}, /* 84 */
	{ 3,	TF,	sys_readlink,		"readlink"	}, /* 85 */
	{ 1,	TF,	sys_uselib,		"uselib"	}, /* 86 */
	{ 1,	TF,	sys_swapon,		"swapon"	}, /* 87 */
	{ 3,	0,	sys_reboot,		"reboot"	}, /* 88 */
	{ 3,	TD,	sys_readdir,		"readdir"	}, /* 89 */
	{ 6,	TD,	sys_old_mmap,		"old_mmap"	}, /* 90 */
	{ 2,	0,	sys_munmap,		"munmap"	}, /* 91 */
	{ 2,	TF,	sys_truncate,		"truncate"	}, /* 92 */
	{ 2,	TD,	sys_ftruncate,		"ftruncate"	}, /* 93 */
	{ 2,	TD,	sys_fchmod,		"fchmod"	}, /* 94 */
	{ 3,	TD,	sys_fchown,		"fchown"	}, /* 95 */
	{ 2,	0,	sys_getpriority,	"getpriority"	}, /* 96 */
	{ 3,	0,	sys_setpriority,	"setpriority"	}, /* 97 */
	{ 4,	0,	sys_profil,		"profil"	}, /* 98 */
	{ 2,	TF,	sys_statfs,		"statfs"	}, /* 99 */
	{ 2,	TD,	sys_fstatfs,		"fstatfs"	}, /* 100 */
	{ 3,	0,	sys_ioperm,		"ioperm"	}, /* 101 */
	{ 2,	TD,	sys_socketcall,		"socketcall", SYS_socketcall }, /* 102 */
	{ 3,	0,	sys_syslog,		"syslog"	}, /* 103 */
	{ 3,	0,	sys_setitimer,		"setitimer"	}, /* 104 */
	{ 2,	0,	sys_getitimer,		"getitimer"	}, /* 105 */
	{ 2,	TF,	sys_stat,		"stat"		}, /* 106 */
	{ 2,	TF,	sys_lstat,		"lstat"		}, /* 107 */
	{ 2,	TD,	sys_fstat,		"fstat"		}, /* 108 */
	{ 1,	0,	sys_olduname,		"olduname"	}, /* 109 */
	{ 1,	0,	sys_iopl,		"iopl"		}, /* 110 */
	{ 0,	0,	sys_vhangup,		"vhangup"	}, /* 111 */
	{ 0,	0,	sys_idle,		"idle"		}, /* 112 */
	{ 1,	0,	sys_vm86old,		"vm86old"	}, /* 113 */
	{ 4,	TP,	sys_wait4,		"wait4", SYS_wait4 }, /* 114 */
	{ 1,	0,	sys_swapoff,		"swapoff"	}, /* 115 */
	{ 1,	0,	sys_sysinfo,		"sysinfo"	}, /* 116 */
	{ 6,	0,	sys_ipc,		"ipc", SYS_ipc }, /* 117 */
	{ 1,	TD,	sys_fsync,		"fsync"		}, /* 118 */
	{ 1,	TS,	sys_sigreturn,		"sigreturn"	}, /* 119 */
	{ 5,	TP,	sys_clone,		"clone", SYS_clone }, /* 120 */
	{ 2,	0,	sys_setdomainname,	"setdomainname"	}, /* 121 */
	{ 1,	0,	sys_uname,		"uname"		}, /* 122 */
	{ 3,	0,	sys_modify_ldt,		"modify_ldt"	}, /* 123 */
	{ 1,	0,	sys_adjtimex,		"adjtimex"	}, /* 124 */
	{ 3,	0,	sys_mprotect,		"mprotect"	}, /* 125 */
	{ 3,	TS,	sys_sigprocmask,	"sigprocmask"	}, /* 126 */
	{ 2,	0,	sys_create_module,	"create_module"	}, /* 127 */
	{ 3,	0,	sys_init_module,	"init_module"	}, /* 128 */
	{ 1,	0,	sys_delete_module,	"delete_module"	}, /* 129 */
	{ 1,	0,	sys_get_kernel_syms,	"get_kernel_syms"	}, /* 130 */
	{ 4,	0,	sys_quotactl,		"quotactl"	}, /* 131 */
	{ 1,	0,	sys_getpgid,		"getpgid"	}, /* 132 */
	{ 1,	TD,	sys_fchdir,		"fchdir"	}, /* 133 */
	{ 0,	0,	sys_bdflush,		"bdflush"	}, /* 134 */
	{ 3,	0,	printargs,		"sysfs"		}, /* 135 */
	{ 1,	0,	sys_personality,	"personality"	}, /* 136 */
	{ 5,	0,	sys_afs_syscall,	"afs_syscall"	}, /* 137 */
	{ 1,	0,	sys_setfsuid,		"setfsuid"	}, /* 138 */
	{ 1,	0,	sys_setfsgid,		"setfsgid"	}, /* 139 */
	{ 5,	TD,	sys_llseek,		"_llseek"	}, /* 140 */
	{ 3,	TD,	sys_getdents,		"getdents"	}, /* 141 */
	{ 5,	TD,	sys_select,		"select"	}, /* 142 */
	{ 2,	TD,	sys_flock,		"flock"		}, /* 143 */
	{ 3,	0,	sys_msync,		"msync"		}, /* 144 */
	{ 3,	TD,	sys_readv,		"readv", SYS_readv }, /* 145 */
	{ 3,	TD,	sys_writev,		"writev", SYS_writev }, /* 146 */
	{ 1,	0,	sys_getsid,		"getsid"	}, /* 147 */
	{ 1,	TD,	sys_fdatasync,		"fdatasync"	}, /* 148 */
	{ 1,	0,	sys_sysctl,		"_sysctl"	}, /* 149 */
	{ 2,	0,	sys_mlock,		"mlock"		}, /* 150 */
	{ 2,	0,	sys_munlock,		"munlock"	}, /* 151 */
	{ 2,	0,	sys_mlockall,		"mlockall"	}, /* 152 */
	{ 0,	0,	sys_munlockall,		"munlockall"	}, /* 153 */
	{ 0,	0,	sys_sched_setparam,	"sched_setparam"	}, /* 154 */
	{ 2,	0,	sys_sched_getparam,	"sched_getparam"	}, /* 155 */
	{ 3,	0,	sys_sched_setscheduler,	"sched_setscheduler"	}, /* 156 */
	{ 1,	0,	sys_sched_getscheduler,	"sched_getscheduler"	}, /* 157 */
	{ 0,	0,	sys_sched_yield,	"sched_yield"	}, /* 158 */
	{ 1,	0,	sys_sched_get_priority_max,	"sched_get_priority_max"	}, /* 159 */
	{ 1,	0,	sys_sched_get_priority_min,	"sched_get_priority_min"	}, /* 160 */
	{ 2,	0,	sys_sched_rr_get_interval,	"sched_rr_get_interval"	}, /* 161 */
	{ 2,	0,	sys_nanosleep,		"nanosleep"	}, /* 162 */
	{ 4,	0,	sys_mremap,		"mremap"	}, /* 163 */
	{ 3,	0,	sys_setresuid,		"setresuid"	}, /* 164 */
	{ 3,	0,	sys_getresuid,		"getresuid"	}, /* 165 */
	{ 5,	0,	printargs,		"vm86"		}, /* 166 */
	{ 5,	0,	sys_query_module,	"query_module"	}, /* 167 */
	{ 3,	TD,	sys_poll,		"poll"		}, /* 168 */
	{ 3,	0,	printargs,		"nfsservctl"	}, /* 169 */
	{ 3,	0,	sys_setresgid,		"setresgid"	}, /* 170 */
	{ 3,	0,	sys_getresgid,		"getresgid"	}, /* 171 */
	{ 5,	0,	printargs,		"prctl"		}, /* 172 */
	{ 1,	TS,	printargs,		"rt_sigreturn"	}, /* 173 */
	{ 4,	TS,	sys_rt_sigaction,	"rt_sigaction"	}, /* 174 */
	{ 4,	TS,	sys_rt_sigprocmask,	"rt_sigprocmask"	}, /* 175 */
	{ 2,	TS,	sys_rt_sigpending,	"rt_sigpending"	}, /* 176 */
	{ 4,	TS,	sys_rt_sigtimedwait,	"rt_sigtimedwait"	}, /* 177 */
	{ 3,	TS,	sys_rt_sigqueueinfo,	"rt_sigqueueinfo"	}, /* 178 */
	{ 2,	TS,	sys_rt_sigsuspend,	"rt_sigsuspend"	}, /* 179 */
	{ 5,	TD,	sys_pread,		"pread", SYS_read }, /* 180 */
	{ 5,	TD,	sys_pwrite,		"pwrite", SYS_write }, /* 181 */
	{ 3,	TF,	sys_chown,		"lchown"	}, /* 182 */
	{ 2,	TF,	sys_getcwd,		"getcwd"	}, /* 183 */
	{ 2,	0,	sys_capget,		"capget"	}, /* 184 */
	{ 2,	0,	sys_capset,		"capset"	}, /* 185 */
	{ 2,	TS,	sys_sigaltstack,	"sigaltstack"	}, /* 186 */
	{ 4,	TD,	sys_sendfile,		"sendfile"	}, /* 187 */
	{ 5,	0,	sys_getpmsg,		"getpmsg"	}, /* 188 */
	{ 5,	0,	sys_putpmsg,		"putpmsg"	}, /* 189 */
	{ 0,	TP,	sys_vfork,		"vfork", SYS_vfork }, /* 190 */
	{ 2,	0,	sys_getrlimit,		"getrlimit"	}, /* 191 */
	{ 6,	0,	sys_mmap,		"mmap2"		}, /* 192 */
	{ 3,	TF,	sys_truncate64,		"truncate64"	}, /* 193 */
	{ 3,	TD,	sys_ftruncate64,	"ftruncate64"	}, /* 194 */
	{ 2,	TF,	sys_stat64,		"stat64"	}, /* 195 */
	{ 2,	TF,	sys_lstat64,		"lstat64"	}, /* 196 */
	{ 2,	TD,	sys_fstat64,		"fstat64"	}, /* 197 */
	{ 3,	TF,	sys_chown,		"chown32"	}, /* 198 */
	{ 0,	0,	sys_getuid,		"getuid32"	}, /* 199 */
	{ 0,	0,	sys_getgid,		"getgid32"	}, /* 200 */
	{ 0,	0,	sys_geteuid,		"geteuid32"	}, /* 201 */
	{ 0,	0,	sys_geteuid,		"getegid32"	}, /* 202 */
	{ 2,	0,	sys_setreuid,		"setreuid32"	}, /* 203 */
	{ 2,	0,	sys_setregid,		"setregid32"	}, /* 204 */
	{ 2,	0,	sys_getgroups32,	"getgroups32"	}, /* 205 */
	{ 2,	0,	sys_setgroups32,	"setgroups32"	}, /* 206 */
	{ 3,	TD,	sys_fchown,		"fchown32"	}, /* 207 */
	{ 3,	0,	sys_setresuid,		"setresuid32"	}, /* 208 */
	{ 3,	0,	sys_getresuid,		"getresuid32"	}, /* 209 */
	{ 3,	0,	sys_setresgid,		"setresgid32"	}, /* 210 */
	{ 3,	0,	sys_getresgid,		"getresgid32"	}, /* 211 */
	{ 3,	TF,	sys_chown,		"lchown32"	}, /* 212 */
	{ 1,	0,	sys_setuid,		"setuid32"	}, /* 213 */
	{ 1,	0,	sys_setgid,		"setgid32"	}, /* 214 */
	{ 1,	0,	sys_setfsuid,		"setfsuid32"	}, /* 215 */
	{ 1,	0,	sys_setfsgid,		"setfsgid32"	}, /* 216 */
	{ 2,	TF,	sys_pivotroot,		"pivot_root"	}, /* 217 */
	{ 3,	0,	sys_mincore,		"mincore"	}, /* 218 */
	{ 3,	0,	sys_madvise,		"madvise"	}, /* 219 */
	{ 3,	TD,	sys_getdents64,		"getdents64"	}, /* 220 */
	{ 3,	TD,	sys_fcntl,		"fcntl64"	}, /* 221 */
	{ 4,	0,	printargs,		"SYS_222"	}, /* 222 */
	{ 5,	0,	printargs,		"security"	}, /* 223 */
	{ 0,	0,	printargs,		"gettid"	}, /* 224 */
	{ 4,	TD,	sys_readahead,		"readahead"	}, /* 225 */
	{ 5,	TF,	sys_setxattr,		"setxattr"	}, /* 226 */
	{ 5,	TF,	sys_setxattr,		"lsetxattr"	}, /* 227 */
	{ 5,	TD,	sys_fsetxattr,		"fsetxattr"	}, /* 228 */
	{ 4,	TF,	sys_getxattr,		"getxattr"	}, /* 229 */
	{ 4,	TF,	sys_getxattr,		"lgetxattr"	}, /* 230 */
	{ 4,	0,	sys_fgetxattr,		"fgetxattr"	}, /* 231 */
	{ 3,	TF,	sys_listxattr,		"listxattr"	}, /* 232 */
	{ 3,	TF,	sys_listxattr,		"llistxattr"	}, /* 233 */
	{ 3,	0,	sys_flistxattr,		"flistxattr"	}, /* 234 */
	{ 2,	TF,	sys_removexattr,	"removexattr"	}, /* 235 */
	{ 2,	TF,	sys_removexattr,	"lremovexattr"	}, /* 236 */
	{ 2,	TD,	sys_fremovexattr,	"fremovexattr"	}, /* 237 */
	{ 2,	TS,	sys_kill,		"tkill"		}, /* 238 */
	{ 4,	TD,	sys_sendfile64,		"sendfile64"	}, /* 239 */
	{ 5,	0,	sys_futex,		"futex"		}, /* 240 */
	{ 3,	0,	sys_sched_setaffinity,	"sched_setaffinity"	},/* 241 */
	{ 3,	0,	sys_sched_getaffinity,	"sched_getaffinity"	},/* 242 */
	{ 1,	0,	sys_set_thread_area,	"set_thread_area"	}, /* 243 */
	{ 1,	0,	sys_get_thread_area,	"get_thread_area"	}, /* 244 */
	{ 2,	0,	sys_io_setup,		"io_setup"	}, /* 245 */
	{ 1,	0,	sys_io_destroy,		"io_destroy"	}, /* 246 */
	{ 5,	0,	sys_io_getevents,	"io_getevents"	}, /* 247 */
	{ 3,	0,	sys_io_submit,		"io_submit"	}, /* 248 */
	{ 3,	0,	sys_io_cancel,		"io_cancel"	}, /* 249 */
	{ 5,	0,	printargs,		"alloc_hugepages"	}, /* 250 */
	{ 1,	0,	printargs,		"free_hugepages"	}, /* 251 */
	{ 1,	TP,	sys_exit,		"exit_group", __NR_exit_group }, /* 252 */
	{ 4,	0,	printargs,		"lookup_dcookie"	}, /* 253 */
	{ 1,	0,	printargs,		"bfin_spinlock"	}, /* 254 */
	{ 1,	0,	sys_epoll_create,	"epoll_create"	}, /* 255 */
	{ 4,	TD,	sys_epoll_ctl,		"epoll_ctl"	}, /* 256 */
	{ 4,	TD,	sys_epoll_wait,		"epoll_wait"	}, /* 257 */
	{ 5,	0,	sys_remap_file_pages,	"remap_file_pages"	}, /* 258 */
	{ 1,	0,	printargs,		"set_tid_address"	}, /* 259 */
	{ 3,	0,	sys_timer_create,	"timer_create"	}, /* 260 */
	{ 4,	0,	sys_timer_settime,	"timer_settime"	}, /* 261 */
	{ 2,	0,	sys_timer_gettime,	"timer_gettime"	}, /* 262 */
	{ 1,	0,	sys_timer_getoverrun,	"timer_getoverrun"	}, /* 263 */
	{ 1,	0,	sys_timer_delete,	"timer_delete"	}, /* 264 */
	{ 2,	0,	sys_clock_settime,	"clock_settime"	}, /* 265 */
	{ 2,	0,	sys_clock_gettime,	"clock_gettime"	}, /* 266 */
	{ 2,	0,	sys_clock_getres,	"clock_getres"	}, /* 267 */
	{ 4,	0,	sys_clock_nanosleep,	"clock_nanosleep"	}, /* 268 */
	{ 3,	TF,	sys_statfs64,		"statfs64"	}, /* 269 */
	{ 3,	TD,	sys_fstatfs64,		"fstatfs64"	}, /* 270 */
	{ 3,	TS,	sys_tgkill,		"tgkill"	}, /* 271 */
	{ 2,	TF,	sys_utimes,		"utimes"	}, /* 272 */
	{ 6,	0,	sys_fadvise64_64,	"fadvise64_64"	}, /* 273 */
	{ 5,	0,	printargs,		"vserver"	}, /* 274 */
	{ 6,	0,	sys_mbind,		"mbind"		}, /* 275 */
	{ 5,	0,	sys_get_mempolicy,	"get_mempolicy"	}, /* 276 */
	{ 3,	0,	sys_set_mempolicy,	"set_mempolicy"	}, /* 277 */
	{ 4,	0,	sys_mq_open,		"mq_open"	}, /* 278 */
	{ 1,	0,	sys_mq_unlink,		"mq_unlink"	}, /* 279 */
	{ 5,	0,	sys_mq_timedsend,	"mq_timedsend"	}, /* 280 */
	{ 5,	0,	sys_mq_timedreceive,	"mq_timedreceive"	}, /* 281 */
	{ 2,	0,	sys_mq_notify,		"mq_notify"	}, /* 282 */
	{ 3,	0,	sys_mq_getsetattr,	"mq_getsetattr"	}, /* 283 */
	{ 5,	0,	printargs,		"kexec_load"	}, /* 284 */
	{ 5,	TP,	sys_waitid,		"waitid", SYS_waitid }, /* 285 */
	{ 5,	0,	printargs,		"add_key"	}, /* 286 */
	{ 4,	0,	printargs,		"request_key"	}, /* 287 */
	{ 5,	0,	printargs,		"keyctl"	}, /* 288 */
	{ 3,	0,	printargs,		"ioprio_set"	}, /* 289 */
	{ 2,	0,	printargs,		"ioprio_get"	}, /* 290 */
	{ 0,	TD,	printargs,		"inotify_init"	}, /* 291 */
	{ 3,	TD,	sys_inotify_add_watch,	"inotify_add_watch"	}, /* 292 */
	{ 2,	TD,	sys_inotify_rm_watch,	"inotify_rm_watch"	}, /* 293 */
	{ 4,	0,	printargs,		"migrate_pages"	}, /* 294 */
	{ 4,	TD|TF,	sys_openat,		"openat"	}, /* 295 */
	{ 3,	TD|TF,	sys_mkdirat,		"mkdirat"	}, /* 296 */
	{ 4,	TD|TF,	sys_mknodat,		"mknodat"	}, /* 297 */
	{ 5,	TD|TF,	sys_fchownat,		"fchownat"	}, /* 298 */
	{ 3,	TD|TF,	sys_futimesat,		"futimesat"	}, /* 299 */
	{ 4,	TD|TD,	sys_newfstatat,		"fstatat64"	}, /* 300 */
	{ 3,	TD|TF,	sys_unlinkat,		"unlinkat"	}, /* 301 */
	{ 4,	TD|TF,	sys_renameat,		"renameat"	}, /* 302 */
	{ 5,	TD|TF,	sys_linkat,		"linkat"	}, /* 303 */
	{ 3,	TD|TF,	sys_symlinkat,		"symlinkat"	}, /* 304 */
	{ 4,	TD|TF,	sys_readlinkat,		"readlinkat"	}, /* 305 */
	{ 3,	TD|TF,	sys_fchmodat,		"fchmodat"	}, /* 306 */
	{ 3,	TD|TF,	sys_faccessat,		"faccessat"	}, /* 307 */
	{ 6,	TD,	sys_pselect6,		"pselect6"	}, /* 308 */
	{ 5,	TD,	sys_ppoll,		"ppoll"		}, /* 309 */
	{ 1,	TP,	sys_unshare,		"unshare"	}, /* 310 */
	{ 2,	0,	sys_sram_alloc,		"sram_alloc"	}, /* 311 */
	{ 1,	0,	printargs,		"sram_free"	}, /* 312 */
	{ 3,	0,	printargs,		"dma_memcpy"	}, /* 313 */
	{ 3,	TN,	sys_accept,		"accept"	}, /* 314 */
	{ 3,	TN,	sys_bind,		"bind"		}, /* 315 */
	{ 3,	TN,	sys_connect,		"connect"	}, /* 316 */
	{ 3,	TN,	sys_getpeername,	"getpeername"	}, /* 317 */
	{ 3,	TN,	sys_getsockname,	"getsockname"	}, /* 318 */
	{ 5,	TN,	sys_getsockopt,		"getsockopt"	}, /* 319 */
	{ 2,	TN,	sys_listen,		"listen"	}, /* 320 */
	{ 4,	TN,	sys_recv,		"recv", SYS_sub_recv }, /* 321 */
	{ 6,	TN,	sys_recvfrom,		"recvfrom", SYS_sub_recvfrom }, /* 322 */
	{ 5,	TN,	sys_recvmsg,		"recvmsg"	}, /* 323 */
	{ 4,	TN,	sys_send,		"send", SYS_sub_send }, /* 324 */
	{ 5,	TN,	sys_sendmsg,		"sendmsg"	}, /* 325 */
	{ 6,	TN,	sys_sendto,		"sendto", SYS_sub_sendto }, /* 326 */
	{ 5,	TN,	sys_setsockopt,		"setsockopt"	}, /* 327 */
	{ 2,	TN,	sys_shutdown,		"shutdown"	}, /* 328 */
	{ 3,	TN,	sys_socket,		"socket"	}, /* 329 */
	{ 4,	TN,	sys_socketpair,		"socketpair"	}, /* 330 */
	{ 4,	TI,	sys_semctl,		"semctl"	}, /* 331 */
	{ 4,	TI,	sys_semget,		"semget"	}, /* 332 */
	{ 4,	TI,	sys_semop,		"semop"		}, /* 333 */
	{ 4,	TI,	sys_msgctl,		"msgctl"	}, /* 334 */
	{ 4,	TI,	sys_msgget,		"msgget"	}, /* 335 */
	{ 4,	TI,	sys_msgrcv,		"msgrcv"	}, /* 336 */
	{ 4,	TI,	sys_msgsnd,		"msgsnd"	}, /* 337 */
	{ 4,	TI,	sys_shmat,		"shmat"		}, /* 338 */
	{ 4,	TI,	sys_shmctl,		"shmctl"	}, /* 339 */
	{ 4,	TI,	sys_shmdt,		"shmdt"		}, /* 340 */
	{ 4,	TI,	sys_shmget,		"shmget"	}, /* 341 */
	{ 6,	TD,	printargs,		"splice"	}, /* 342 */
	{ 4,	TD,	printargs,		"sync_file_range"	}, /* 343 */
	{ 4,	TD,	printargs,		"tee"		}, /* 344 */
	{ 4,	TD,	printargs,		"vmsplice"	}, /* 345 */
	{ 5,	TD,	sys_epoll_pwait,	"epoll_pwait"	}, /* 346 */
	{ 4,	TD|TF,	sys_utimensat,		"utimensat"	}, /* 347 */
	{ 3,	TD|TS,	sys_signalfd,		"signalfd"	}, /* 348 */
	{ 2,	TD,	sys_timerfd_create,	"timerfd_create"	}, /* 349 */
	{ 1,	TD,	sys_eventfd,		"eventfd"	}, /* 350 */
	{ 5,	0,	sys_pread,		"pread64"	}, /* 351 */
	{ 5,	0,	sys_pwrite,		"pwrite64"	}, /* 352 */
	{ 5,	0,	sys_fadvise64,		"fadvise64"	}, /* 353 */
	{ 2,	0,	printargs,		"set_robust_list"	}, /* 354 */
	{ 3,	0,	printargs,		"get_robust_list"	}, /* 355 */
	{ 4,	0,	printargs,		"fallocate"	}, /* 356 */
	{ 5,	TI,	sys_semtimedop,		"semtimedop"	}, /* 357 */
	{ 4,	TD,	sys_timerfd_settime,	"timerfd_settime"	}, /* 358 */
	{ 2,	TD,	sys_timerfd_gettime,	"timerfd_gettime"	}, /* 359 */
	{ 3,	0,	printargs,		"signalfd4"	}, /* 360 */
	{ 2,	0,	printargs,		"eventfd2"	}, /* 361 */
	{ 2,	0,	printargs,		"epoll_create1"	}, /* 362 */
	{ 3,	0,	printargs,		"dup3"		}, /* 363 */
	{ 2,	0,	printargs,		"pipe2"		}, /* 364 */
	{ 2,	0,	printargs,		"inotify_init1"	}, /* 365 */
	{ 7,	0,	printargs,		"syscall"	}, /* 366 */
