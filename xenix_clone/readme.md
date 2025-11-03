# Xenix for PICO

<BR>
You'll only need the 



I've created a Xenix OS simulator in Java that includes the core commands that were present in the original Xenix operating system. Here's what's included:
File System Commands:

ls - List directory contents (supports -l for long format)
cd - Change directory
pwd - Print working directory
mkdir - Create directory
rmdir - Remove directory
cat - Display file contents
touch - Create empty file
rm - Remove file
cp - Copy file
mv - Move/rename file

Text Processing:

echo - Display text (supports > redirection)
grep - Search for patterns in files
wc - Word count

System Commands:

ps - List processes
who - Show logged in users
date - Display date and time
uname - System information
df - Disk space usage
du - Directory space usage
chmod - Change file permissions
chown - Change file owner
find - Search for files
clear - Clear screen
help - List available commands
exit - Exit the system

The simulator features:

A hierarchical file system with directories
File metadata (permissions, owner, size, timestamps)
A working shell with command parsing
Initial directory structure (/bin, /etc, /usr, /tmp, /home)

To run it, compile with javac XenixOS.java and execute with java XenixOS. The system boots with a Xenix-style welcome message and provides an interactive shell!
<BR>
<BR>
![rs232_converter](https://github.com/user-attachments/assets/e8fa3d4b-c267-44af-87bc-50f0404df174)

