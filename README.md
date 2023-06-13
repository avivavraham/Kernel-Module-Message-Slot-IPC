# Kernel-Module-Message-Slot-IPC
This GitHub repository hosts a kernel module implementation
This GitHub repository hosts a kernel module implementation that introduces a new inter-process communication (IPC) mechanism called message slots.
The module enables processes to communicate through character device files, supporting multiple message channels concurrently.
Unlike pipes, message slots preserve messages until overwritten, allowing for multiple reads.
The module also includes a device driver for managing message slot files, which are created with specific major and minor numbers.
Get hands-on experience with kernel programming and explore the world of IPC design and implementation.
