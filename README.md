# ServerClientFileSystem

This was a project for my graduate Cloud Computing class, CSEN241. In this project, I had to create a server-client model in which clients cloud add/remove files to the server, and then the server would distribute each user files in a distributed file system to different hard drive disks automatically, using an MD5 hashing system.

To use:

Do "make" to create the executables for both client and server.
Type "./server.o <partition power> <IP address of hard drives>" to start the server with hard drives for the distributed file system. The server will automatically find a port for clients to connect to.
Then type "./client.o <server IP> <serveer port>" to connect a client to the server.

The client can run 7 different commands:
• download <user/object> - download a specific user's file and display the context of <user/object> 
• list <user> - display the <user>’s objects/files in “ls –lrt” format 
• upload <user/object> - upload the user's file to the distributed file system and display which disks the <user/object> is saved 
• delete <user/object> - delete a user's file from the distributed file system and display a confirmation or error message 
• add <disk> - add a new disk to the distributed file system and display new partitions with all <user/object> within 
• remove <disk>- remove a disk from the distributed file system and display where old partitions went to 
• clean – clear all disks