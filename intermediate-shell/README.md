## C-Shell (Phase-2)  

This is a basic shell made in C. It can now(phase 2) handle piping and
redirection. Also, processes can be set as background or foreground.  

### Built-in commands  

* `echo`  
* `pwd`  
* `cd`  
* `pinfo`  
* `exit`  
* `jobs`  
* `kjob`  
* `fg`  
* `overkill`  
* `quit`  

### Brief description of some built-in commands  

* `jobs`: prints a list of all currently running jobs along with their pid,
        particularly background jobs, in order of their creation.  

* `kjob <pid> <signalNumber>`: takes the process id of a running job and sends a signal value to that process.  

* `fg <pid>`: brings background process with given pid to foreground.  

* `overkill`: kills all background processes at once.  

### How to run the shell  

* In the directory that contains this README, run `make`.  
* Run the shell by typing, `./faysh`.  
* Processes can be sent straight to the background by appending a `&` at the end. For example `vi &`. They can be brought to foreground by using the `fg` command mentioned above.  
* Exit the shell by typing `quit` in the shell.  
