		     +--------------------------+
       	       	     |		CS 140		|
		     | PROJECT 2: USER PROGRAMS	|
		     | 	   DESIGN DOCUMENT     	|
		     +--------------------------+

---- GROUP ----

>> Fill in the names and email addresses of your group members.

Nicolas Wilhoit <nicolas.wilhoit@gmail.com>
Peter Gibbs <pgibbs65@gmail.com>
Ritesh Sood <Ritesh324@icloud.com>

---- PRELIMINARIES ----

>> If you have any preliminary comments on your submission, notes for the
>> TAs, or extra credit, please give them here.

>> Please cite any offline or online sources you consulted while
>> preparing your submission, other than the Pintos documentation, course
>> text, lecture notes, and course staff.

Additional help given by Ivo Giorgiev in the form of documentation, and reading materials.

			   ARGUMENT PASSING
			   ================

---- DATA STRUCTURES ----

>> A1: Copy here the declaration of each new or changed `struct' or
>> `struct' member, global or static variable, `typedef', or
>> enumeration.  Identify the purpose of each in 25 words or less.

struct_arguements: Used in order to make for easy arguement passing

struct ChildStatus: Used to determine the status of the child and feed back if it needs killed to Process_Wait

Struct thread* parent: Denotes a thread as a parent thread 

struct thread* child: Denotes a thread as a child thread 

struct thread *cur: Sets up the current thread as a pointer to the running thread 

---- ALGORITHMS ----

>> A2: Briefly describe how you implemented argument parsing.  How do
>> you arrange for the elements of argv[] to be in the right order?
>> How do you avoid overflowing the stack page?

It is set up so that the first loop checks the number of arguments so that we only push
that number of arguments to the stack. We ensure that we allocate the amount of memory over 
what we need by a certain amount by using the palloc functionality.  This ensures that 
we pull as much space on the stack we need for arguments without putting overflow as an option

---- RATIONALE ----

>> A3: Why does Pintos implement strtok_r() but not strtok()?

strtok_r() has the placeholder required by the caller and strtok() does not.  Therefore
it is implemented this way so that commands may be seperated at command line
and arguements may be put in to store for later on in use. 

>> A4: In Pintos, the kernel separates commands into a executable name
>> and arguments.  In Unix-like systems, the shell does this
>> separation.  Identify at least two advantages of the Unix approach.

1.  It shortens the time a user is inside a kernel 

2. Checks to ensure the executable is there before pushing to the kernel to avoid kernel
   panic

			     SYSTEM CALLS
			     ============

---- DATA STRUCTURES ----

>> B1: Copy here the declaration of each new or changed `struct' or
>> `struct' member, global or static variable, `typedef', or
>> enumeration.  Identify the purpose of each in 25 words or less.

struct file *afile: ensures that no bad file is imported 

struct file_node* a_node: gets a node the size of the file for the stack 

>> B2: Describe how file descriptors are associated with open files.
>> Are file descriptors unique within the entire OS or just within a
>> single process?

File descriptors are one to one mapping that is opened through syscall. 
It is unique within the entire OS status because it contains information
away from the thread struct.  the program will maintain a list inside the kernel.

---- ALGORITHMS ----

>> B3: Describe your code for reading and writing user data from the
>> kernel.

Reading:  Check the buffer and the buffer size to ensure that they are valid.
Then, acquire the file system lock.  Once current thread becomes the lock holder, 
then you will see if FD is in two special cases.  Retrieve keys, and release lock. 

Writing:  Check if given buffer is valid.  Acquire the File System lock.  
Use File_write to write the buffer to the file, and release the lock. 

>> B4: Suppose a system call causes a full page (4,096 bytes) of data
>> to be copied from user space into the kernel.  What is the least
>> and the greatest possible number of inspections of the page table
>> (e.g. calls to pagedir_get_page()) that might result?  What about
>> for a system call that only copies 2 bytes of data?  Is there room
>> for improvement in these numbers, and how much?

The least number of inspections on the page table would only be 1, as it would
only need 1 inspection in order to have the page table organized.  The most that 
would need is 4096 as it would then have to inspect every byte of data. For 2,
the greatest number would be 2, since there is only 2 bytes.  As far as it goes,
it executes in n time so there is no reason to improve this. 

>> B5: Briefly describe your implementation of the "wait" system call
>> and how it interacts with process termination.

The implementation will check to see if the child thread is alive.  If it is, then
the parent thread will absorb the child thread data, and the parent thread will 
give data to the child.  They will keep in constant communication until either 
the child dies or the parent dies.  Ideally, the children will die before the 
parents, and the process will exit normally.  In the event a parent dies, the 
child will run to completion, and then dump data as the parent thread no longer 
needs it.   

>> B6: Any access to user program memory at a user-specified address
>> can fail due to a bad pointer value.  Such accesses must cause the
>> process to be terminated.  System calls are fraught with such
>> accesses, e.g. a "write" system call requires reading the system
>> call number from the user stack, then each of the call's three
>> arguments, then an arbitrary amount of user memory, and any of
>> these can fail at any point.  This poses a design and
>> error-handling problem: how do you best avoid obscuring the primary
>> function of code in a morass of error-handling?  Furthermore, when
>> an error is detected, how do you ensure that all temporarily
>> allocated resources (locks, buffers, etc.) are freed?  In a few
>> paragraphs, describe the strategy or strategies you adopted for
>> managing these issues.  Give an example.

You check to make sure the user isn't trying to do a bad memory access first, 
and if they are you deny it.  If a valid user address is given, then the user 
may do what they need, and then exit that space.  If an error occurs,
it is taken care of in the page_fault section to ensure that the user can not 
end up accidentally destroying data. If a user is putting in bad address calls, 
the data is terminated before it can cause harm. 

---- SYNCHRONIZATION ----

>> B7: The "exec" system call returns -1 if loading the new executable
>> fails, so it cannot return before the new executable has completed
>> loading.  How does your code ensure this?  How is the load
>> success/failure status passed back to the thread that calls "exec"?

The child will feed its status actively to the parent.  That way, if the child dies, 
the parent will know when it does and stop feeding in data to the child. This way,
the success/failure status can be found promptly, and if there is a failure, then 
the parent thread can stop sending work to child threads that haven't loaded 
properly. 

>> B8: Consider parent process P with child process C.  How do you
>> ensure proper synchronization and avoid race conditions when P
>> calls wait(C) before C exits?  After C exits?  How do you ensure
>> that all resources are freed in each case?  How about when P
>> terminates without waiting, before C exits?  After C exits?  Are
>> there any special cases?

We ensure that race conditions are not met by locking out the thread that is 
being worked on.  That way, no matter what, the thread can complete if it does
load successfully.  The Child status helps us determine if a race condition is 
occurring or not. 

---- RATIONALE ----

>> B9: Why did you choose to implement access to user memory from the
>> kernel in the way that you did?

We did it that way because:

1. It was suggested we use palloc and malloc to get a user stack reference to the 
   kernel stack. 
   
2. It helped create efficient communication between the user stack and the kernel stack 

>> B10: What advantages or disadvantages can you see to your design
>> for file descriptors?

An advantage is that file discriptors will be easy to read, and easy to implement 

A disadvantage is that it may take a little more time than max efficiency. 

>> B11: The default tid_t to pid_t mapping is the identity mapping.
>> If you changed it, what advantages are there to your approach?

If it was changed, we would have to change how we implemented the 
current thread and child thread functions work since both rely on the tid_t
in order to tell where exactly they are at in process.  An advantage to changing it 
however, would be that we could go ahead and keep track of children while also 
keeping track of the part of the page we are in. 

			   SURVEY QUESTIONS
			   ================

Answering these questions is optional, but it will help us improve the
course in future quarters.  Feel free to tell us anything you
want--these questions are just to spur your thoughts.  You may also
choose to respond anonymously in the course evaluations at the end of
the quarter.

>> In your opinion, was this assignment, or any one of the three problems
>> in it, too easy or too hard?  Did it take too long or too little time?

This assignment was too hard in our honest opinion.  It also took too little 
time given the limited resources we were given for help. 

>> Did you find that working on a particular part of the assignment gave
>> you greater insight into some aspect of OS design?

It did give greater insight into how page faults and memory faults end up working.


>> Is there some particular fact or hint we should give students in
>> future quarters to help them solve the problems?  Conversely, did you
>> find any of our guidance to be misleading?

The hint given on March 25th by Ivo should have been given earlier in the assignment
as it was an immense help. 

>> Do you have any suggestions for the TAs to more effectively assist
>> students, either for future quarters or the remaining projects?

I would recommend that the TAs be: 

1. more TAs. 

2. More knowledgeable about the Pintos 2 assignment.

>> Any other comments?

Overall, this is a far reaching scope if any student has limited resources. 
