#include <glib.h>
#include <terminal/process-info.h>

typedef gboolean (*filter_comparator_t)(const gchar*);
static int 
strv_inplace_filter(gchar** array, filter_comparator_t comparator)
{
  int cid = 0;
  int array_len = g_strv_length(array);
  int swap_id = -1;
  
  for (cid = 0; cid < array_len; cid++) {
    if (!comparator(array[cid])) {
      swap_id = cid;
      break;
    }
  }
  
  for (; cid < array_len; cid++) {
    if (comparator(array[cid])) {
      gchar* tmp = array[cid];
      array[cid] = array[swap_id];
      array[swap_id] = tmp;
      swap_id++;
    }
  }
  if (swap_id < 0)
    return array_len;
  else
    return swap_id;
}

gboolean process_info_read_process_info(int pid, process_info_t* pi, gboolean read_process_name)
{
#ifdef __linux__
  #define PARENT_PID_FN  3
  #define PROC_NAME_FN  1
  #define GROUP_PROCESS_FN  7

  gchar* content;
  gchar* file = g_strdup_printf("/proc/%d/status", pid);
  gchar* ptr, *ptr2;
  gchar** uid_toks;
  int tok_cnt;
  gboolean parsing_done = FALSE;
  gboolean result;
  int brackets = 0;
  int fieldno = 0;
  gchar* last_field_start;
  pi->name = NULL;
  pi->pid = pid;
  
  result = g_file_get_contents(file, &content, NULL, NULL);
  if (!result) {
    g_free(file);
    return FALSE;
  }
  
  for (ptr = content; *ptr && !parsing_done ; ptr++) {
    if (ptr == content || (*ptr == '\n' && *(ptr + 1))) {
      ptr++;
      if (g_str_has_prefix(ptr, "Uid:")) {
        ptr2 = ptr;
        while (*ptr2 && *ptr2 != '\n') ptr2++;
        *ptr2 = '\0';
        uid_toks = g_strsplit(ptr, "\t", 0);
        tok_cnt = strv_inplace_filter(uid_toks, (filter_comparator_t)strlen);
        /* String with uid should contain exactly 5 elements */
        g_assert_cmpint(tok_cnt, == , 5);
        pi->uid = strtoul(uid_toks[1], NULL, 0);
        g_strfreev(uid_toks);
        parsing_done = TRUE;
        break;
      }
    }
  }
  
  g_free(content);
  g_free(file);
  
  file = g_strdup_printf("/proc/%d/stat", pid);
  result = g_file_get_contents(file, &content, NULL, NULL);
  if (!result) {
    g_free(file);
    return FALSE;
  }
  
  /* Now parse /proc/stat. parenthesies uses for escaping spaces */
  last_field_start = content;
  
  
  for (ptr = content ; *ptr ; ptr++) {
    switch (*ptr)
    {
      case '(': brackets++; *ptr = ' '; break;
      case ')': brackets--; *ptr = ' '; break;
      default:
        if (*ptr == ' ' && brackets == 0) {
          switch (fieldno)
          {
            case PARENT_PID_FN:
              *ptr = '\0';
              pi->ppid = strtol(last_field_start, NULL, 0);
              *ptr = ' ';
              break;
            case PROC_NAME_FN:
              if (read_process_name)
                pi->name = g_strchomp(g_strchug(g_strndup(last_field_start, ptr - last_field_start)));
              break;
            case GROUP_PROCESS_FN:
              *ptr = '\0';
              pi->fgpid = strtol(last_field_start, NULL, 0);
              *ptr = ' ';
              break;
          }
          fieldno++;
          last_field_start = ptr + 1;
        }
    }
  }
  g_free(content);
  g_free(file);
  return TRUE;
#else //FIXME : add supporting for *BSD systems and MAC
  #error "No  proc info reading implementation available for current OS"
#endif
}

gchar**   process_info_read_process_args(int pid)
{
#ifdef __linux__
  gchar** result;
  gchar* content;
  gsize len;
  int i, j;
  int strcnt = 1;
  gchar* file = g_strdup_printf("/proc/%d/cmdline", pid);
  if (!g_file_get_contents(file, &content, &len, NULL)) {
    g_free(file);
    return NULL;
  }
  
  for (i = 0 ; i < (int)len; i++) {
    if (content[i] == '\0' && strlen(content + i + 1))
      strcnt++;
  }
  
  result = g_new(gchar*, strcnt + 1);
  result[strcnt] = NULL;
  result[0] = strdup(content);
  for (i = 0, j = 1; i < (int)len ; i++) {
    if (content[i] == '\0' && strlen(content + i + 1)) {
      result[j] = strdup(content + i + 1);
      j++;
    }
  }
  
  g_free(content);
  g_free(file);
  return result;
#else //FIXME : add supporting for *BSD systems and MAC
  #error "No  proc info reading implementation available for current OS"
#endif
}

gchar*   process_info_read_process_dir(int pid)
{
#ifdef __linux__
  gchar* file = g_strdup_printf("/proc/%d/cwd", pid);
  gchar* result = g_file_read_link(file, NULL);
  g_free(file);
  return result;
#else //FIXME : add supporting for *BSD systems and MAC
  #error "No  proc info reading implementation available for current OS"
#endif
}