# Shell Lab

本实验需要实现一个简易版本的Unix Shell，该Shell需要具有作业控制、僵死子进程回收、内置命令等基本功能，为了完成本实验，实验者需要对于进程行为、信号处理具有一定的了解。当实验中遇到困难时，可以通过阅读CSAPP第8章（或APUE）得到相应的解决方案。

本实验的基本框架已经在 `tsh.c` 中被搭建完成，实验者仅需要对其中关键函数进行设计即可，在完成 `tsh.c` 的编写后，运行 `make` 编译链接你的Shell程序，并可以通过输入 `make test<01-15>` 和 `make rtest<01-15>` 比较自己编写的Shell程序与官方Shell程序的区别。在这里对额外添加或更改的函数进行分别讲解：

### `unix_error`

```
601  * unix_error - unix-style error routine
602  */
603 void unix_error(const char *msg,...)
604 {
605     va_list ap;
606     va_start(ap,msg);
607     char buf[MAXLINE];
608     vsnprintf(buf,MAXLINE-1,msg,ap);
609     strcat(buf,"\n");
610     fflush(stdout);
611     fputs(buf,stderr);
612     fflush(NULL);
613     va_end(ap);
614     exit(1);
615 }
```

官方的异常处理程序中不支持格式化字符串的输出，为了简化代码在这里仿照APUE格式对 `unix_error` 函数添加可变参数列表，以支持异常处理中格式化字符串的输出功能。

### `eval`

```
157 /* 
158  * eval - Evaluate the command line that the user has just typed in
159  * 
160  * If the user has requested a built-in command (quit, jobs, bg or fg)
161  * then execute it immediately. Otherwise, fork a child process and
162  * run the job in the context of the child. If the job is running in
163  * the foreground, wait for it to terminate and then return.  Note:
164  * each child process must have a unique process group ID so that our
165  * background children don't receive SIGINT (SIGTSTP) from the kernel
166  * when we type ctrl-c (ctrl-z) at the keyboard.  
167 */
168 void eval(char *cmdline)
169 {
170     char *argv[MAXARGS];
171     int bg;
172     char buf[MAXLINE];
173     sigset_t mask_chld,prev,mask_all;
174     sigemptyset(&mask_chld);
175     sigfillset(&mask_all);
176     sigaddset(&mask_chld,SIGCHLD);
177     strcpy(buf,cmdline);
178     bg = parseline(buf,argv);
179     if(argv[0] == NULL)
180         return;
181     if(!builtin_cmd(argv)){
182        sigprocmask(SIG_BLOCK,&mask_chld,&prev);
183        pid_t pid;
184        if((pid = fork()) == 0){
185            setpgid(0,0);
186            sigprocmask(SIG_SETMASK,&prev,NULL);
187            if(execve(argv[0],argv,environ) < 0){
188                cmdline[strlen(cmdline)-1] = '\0';
189                unix_error("%s: Command not found",cmdline);
190            }
191        }
192        else if(pid < 0)
193            unix_error("fork fali");
194        int status = bg ? BG : FG;
195        sigprocmask(SIG_BLOCK,&mask_all,NULL);
196        addjob(jobs,pid,status,buf);
197        sigprocmask(SIG_SETMASK,&prev,NULL);
198        if(!bg){
199            waitfg(pid);
200        }
201        else{
202            struct job_t* job = getjobpid(jobs,pid);
203            printf("[%d] (%d) %s",job->jid,job->pid,job->cmdline);
204        }
205     }
206 }
```

`eval` 函数是本Shell程序的主要处理函数，其对输入的命令字符串进行解析并做出相应的处理行为。在这里对其进行分段说明：

**170 - 178行**：初始化局部变量以及信号屏蔽集，其中 `mask_all` 代表对所有信号进行阻塞的屏蔽集、`mask_chld`代表对`SIGCHLD`信号进行阻塞的屏蔽集、`prev`用于存放程序的原有屏蔽集。然后调用 `parseline` 函数将命令字符串分割为字符串数组，并存放在 `argv` 中，并通过 `bg` 返回该命令末尾是否存在 `&` 符号，如存在则返回1代表该命令为后台命令，否则返回0代表该命令为前台命令。

**179 - 181行**：当输入命令为空命令时，直接返回。调用 `bulitin_cmd` 函数检测输入命令是否为内置命令，如为内置命令则在该函数中执行内置命令并返回1，导致 `eval` 函数直接返回。

**182 - 193行**：在执行 `fork` 之前需要屏蔽 `SIGCHLD` 信号，避免在主函数执行 `addjob` 前子进程结束导致信号处理程序提前调用 `deletejob` ，并留下僵死任务项及僵死进程。在子进程中，调用 `setpgid` 使得子进程与主进程处于不同的进程组，并在取消对 `SIGCHLD` 的阻塞后调用 `exerve` 执行目标程序。当 `fork` 或 `execve` 出错时执行异常处理函数。

**194 - 197行**：在主进程中，调用 `addjob` 在全局变量 `jobs` 中添加本次任务项，在调用该函数前需对所有信号进行阻塞以保护该全局变量。

**198 - 204行**：当本次输入命令为前台命令时，执行 `waitfg` 等待该命令完成，当本次输入命令为后台命令时，输出该命令信息。

### `builtin_cmd`

```
265 /* 
266  * builtin_cmd - If the user has typed a built-in command then execute
267  *    it immediately.  
268 */
269 int builtin_cmd(char **argv)
270 {
271     if(!strcmp(argv[0],"quit"))
272         exit(0);
273     if(!strcmp(argv[0],"jobs")){
274         do_jobs();
275         return 1;
276     }
277     if(!strcmp(argv[0],"bg") || !strcmp(argv[0],"fg")){
278         do_bgfg(argv);
279         return 1;
280     }
281     return 0;     /* not a builtin command */
282 }
283 /*
284  * do_jobs - Execute the builtin jobs commands
285  */
286 void do_jobs(){
287     int i;
288     for(i = 0; i < MAXJOBS; i++){
289         if(jobs[i].state == BG)
290             printf("[%d] (%d) Running %s",jobs[i].jid,jobs[i].pid,jobs[i].cmdline);
291         else if(jobs[i].state == ST)
292             printf("[%d] (%d) Stopped %s",jobs[i].jid,jobs[i].pid,jobs[i].cmdline);
293     }
294 }
```

`builtin_cmd` 通过检查 `argv` 的第一个元素（即命令名）检查该命令是否为内置命令，如为内置命令则调用相应的内置命令；`do_jobs` 遍历 `jobs` 并打印非前台任务的任务信息。

### `do_bgfg`

```
295 /* 
296  * do_bgfg - Execute the builtin bg and fg commands
297  */
298 void do_bgfg(char **argv)
299 {
300     if(argv[1] == NULL){
301         printf("%s command need a PID or %%jobid argument\n",argv[0]);
302         return;
303     }
304     struct job_t* job;
305     pid_t pid;
306     if(*argv[1] == '%'){
307         int jid = atoi(argv[1]+1);
308         job = getjobjid(jobs,jid);
309         if(job == NULL){
310             printf("%%%d No such job\n",jid);
311             return;
312         }
313         pid = job->pid;
314     }
315     else{
316         pid = atoi(argv[1]);
317         job = getjobpid(jobs,pid);
318         if(job == NULL){
319             printf("(%d) No such process\n",pid);
320             return;
321         }
322     }
323     kill(-pid,SIGCONT);
324     if(!strcmp(argv[0],"bg")){
325         job->state = BG;
326         printf("[%d] (%d) %s",job->jid,job->pid,job->cmdline);
327     }
328     else{
329         jobs->state = FG;
330         waitfg(pid);
331     }
332     return;
333 }
```

**300 - 322行**：首先，检验该命令的格式是否正确，当 `argv` 的第二个参数为空时，直接返回（需要注意的，由于该函数为Shell主进程进行调用的，不能调用异常处理函数杀死该进程）。通过检查第二个参数的首字符判断输入的为进程号还是任务号，并检查相应的进程或任务是否存在，当输入的为任务号时需将任务号转化为进程号。

**323 - 332行**：对相应的进程组发送 `SIGCONT`信号使该进程组继续工作，当对应进程组为后台进程组时，打印其任务信息；当对应进程组为前台进程组时，调用 `waitfg` 等待该任务完成。

### `waitfg`

```
335 /* 
336  * waitfg - Block until process pid is no longer the foreground process
337  */
338 void waitfg(pid_t pid)
339 {
340     sigset_t mask_all,prev,mask_empty;
341     sigfillset(&mask_all);
342     sigemptyset(&mask_empty);
343     sigprocmask(SIG_BLOCK,&mask_all,&prev);
344     pid_t FG_pid = fgpid(jobs);
345     while(FG_pid == pid){
346         sigsuspend(&mask_empty);
347         FG_pid = fgpid(jobs);
348     }
349     sigprocmask(SIG_SETMASK,&prev,NULL);
350     if(verbose)
351         printf("wait pid:%d return\n",pid);
352     return;
353 }
```

`waitfg`的实现是本实验的难点之一，本文的实现方式是通过调用 `fgpid` 获得前台进程ID，并判断其是否与所要等待完成的进程ID相等，当相等时说明该进程仍未前台进程，调用 `sigsuspend` 取消对信号的阻塞并睡眠，当Shell主进程收到信号时，再次检查前台进程是否改变，如尚未改变则继续执行循环。需要注意的是，`fgpid`需要访问全局变量 `jobs` ，当对其调用时必须保证全部信号被阻塞以保护全局变量。**350 - 351行**中的语句用于调试程序，当Shell命令行参数为 `-v` 时输出额外信息以方便调试。

### `sigchld_handler`

```
359 /* 
360  * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
361  *     a child job terminates (becomes a zombie), or stops because it
362  *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
363  *     available zombie children, but doesn't wait for any other
364  *     currently running children to terminate.  
365  */
366 void sigchld_handler(int sig)
367 {
368     sigset_t mask_all,prev;
369     pid_t pid;
370     sigfillset(&mask_all);
371     while((pid = waitpid(-1,NULL,WUNTRACED|WNOHANG)) > 0){
372         sigprocmask(SIG_BLOCK,&mask_all,&prev);
373         struct job_t* job = getjobpid(jobs,pid);
374         if(job->state != ST)
375             deletejob(jobs,pid);
376         sigprocmask(SIG_SETMASK,&prev,NULL);
377     }
378     return;
379 }
```

`SIGCHLD` 的信号处理程序：当Shell收到 `SIGCHID` 信号时，其子进程终止或收到 `SIGTSTP` 信号而停止，当子进程终止时，调用 `deletejob` 从全局变量 `jobs` 中移除该进程所属任务的信息。需要注意两点：

* 应当以 `while` 的形式调用 `waitpid` 防止因多个进程同时终止导致信号丢失而产生僵死进程、任务，并且在`waitpid` 的 `option` 中输入 `WUNTRACED|WNOHANG` ，使得 `waitpid` 在其子进程没有停止或终止时立即返回。
* 当调用 `deletejob` 时，需阻塞全部信号以保护全局变量。

### `sigint_handler`

```
381 /* 
382  * sigint_handler - The kernel sends a SIGINT to the shell whenver the
383  *    user types ctrl-c at the keyboard.  Catch it and send it along
384  *    to the foreground job.  
385  */
386 void sigint_handler(int sig)
387 {
388     pid_t pid;
389     pid = fgpid(jobs);
390     if(pid != 0){
391         struct job_t* job = getjobpid(jobs,pid);
392         printf("Job [%d] (%d) terminated by signal 2\n",job->jid,job->pid);
393         kill(-pid,SIGINT);
394     }
395     return;
396 }
```

`SIGINT` 的信号处理程序，当Shell收到 `SIGINT` 信号时，将该信号转发至前台进程组。

### `sigtstp_handler`

```
398 /*
399  * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
400  *     the user types ctrl-z at the keyboard. Catch it and suspend the
401  *     foreground job by sending it a SIGTSTP.  
402  */
403 void sigtstp_handler(int sig)
404 {
405     pid_t pid;
406     pid = fgpid(jobs);
407     if(pid != 0){
408         struct job_t* job = getjobpid(jobs,pid);
409         job->state = ST;
410         printf("Job [%d] (%d) stopped by signal 20\n",job->jid,job->pid);
411         kill(-pid,SIGTSTP);
412     }
413     return;
414 }
```

`SIGTSTP` 的信号处理程序，当Shell收到 `SIGTSTP` 信号时，将该信号转发至前台进程组，并将前台任务的状态设置为 `ST` 表示该任务处于停止状态。

通过运行 `make test01` 至 `make test15` ，得到本实验中编写的程序得到的输出结果与官方Shell的输出结果相同，实验结束。