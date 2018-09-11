#ifndef __PROCESS_INFO_H__
#define __PROCESS_INFO_H__

typedef struct {
	int pid;
	int ppid;
	int uid;
	int fgpid;
	gchar* name;
} process_info_t;

gboolean process_info_read_process_info(int pid, process_info_t* pi, gboolean read_process_name);
gchar**   process_info_read_process_args(int pid);
gchar*   process_info_read_process_dir(int pid);

#endif // __PROCESS_INFO_H__
