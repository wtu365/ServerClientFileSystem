/*
* Name: Wesley Tu
* Date: 4/14/24 
* Title: Programming Assignment 1
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <vector>
#include <string>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <iostream>
#include <iomanip>
#include <map>
#include <cmath>
#include <unordered_set>
#include <limits.h>
#include <openssl/md5.h>
#include <sstream>
#include <fstream>

//Declare socket file descriptor.
int sock_fd;

//Declare server address to which to bind for receiving messages and client address to fill in sending address
struct sockaddr_in serveraddr;
struct sockaddr_in clientaddr;

std::string md5(const std::string &str){
  unsigned char hash[MD5_DIGEST_LENGTH];

  MD5_CTX md5;
  MD5_Init(&md5);
  MD5_Update(&md5, str.c_str(), str.size());
  MD5_Final(hash, &md5);

  std::stringstream ss;

  for(int i = 0; i < MD5_DIGEST_LENGTH; i++){
    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>( hash[i] );
  }
  return ss.str();
}

const char* hex_char_to_bin(char c)
{
    switch(toupper(c))
    {
        case '0': return "0000";
        case '1': return "0001";
        case '2': return "0010";
        case '3': return "0011";
        case '4': return "0100";
        case '5': return "0101";
        case '6': return "0110";
        case '7': return "0111";
        case '8': return "1000";
        case '9': return "1001";
        case 'A': return "1010";
        case 'B': return "1011";
        case 'C': return "1100";
        case 'D': return "1101";
        case 'E': return "1110";
        case 'F': return "1111";
    }
}

std::string hex_str_to_bin_str(const std::string& hex)
{
    std::string bin;
    for(unsigned i = 0; i != hex.length(); ++i)
       bin += hex_char_to_bin(hex[i]);
    return bin;
}

void downloadCommand(std::string userObject, int sock_fd, int partition_power, std::vector<std::string> &vDisks, 
                   char loginname[], std::map<std::string, std::vector<std::string> > &diskNameToFiles, 
                   std::vector<int> &DiskPartitions, std::vector<std::string> &FilesPerPartition, std::unordered_set<int> &FileHashSet) {
    //printf("In download function\n");
    size_t slashPos = userObject.find("/");
    if (slashPos == std::string::npos) {
        printf("user/object not in correct format!\n");
        return;
    }
    std::string user = userObject.substr(0, slashPos);
    std::string object = userObject.substr(slashPos+1);

    std::string fileHash = md5(userObject);
    std::string binary_string = hex_str_to_bin_str(fileHash); 
    long int partition_number = std::stol(binary_string.substr(128 - partition_power), nullptr, 2);

    std::string message;
    //std::cout << FilesPerPartition[partition_number] << std::endl;
    if (FilesPerPartition[partition_number].compare(userObject) != 0) {
        message = "The file does not exist in any hard drive!";
        send(sock_fd, message.c_str(), message.length(), 0);
        return;
    }
    else {
        message = "Starting the download.";
        send(sock_fd, message.c_str(), message.length(), 0);
    }

    std::string MainDisk = vDisks[DiskPartitions[partition_number]];
    std::string BackupDisk;
    if (DiskPartitions[partition_number] != vDisks.size() - 1)
        BackupDisk = vDisks[DiskPartitions[partition_number] + 1];
    else
        BackupDisk = vDisks[0];

    if (system(("scp -o StrictHostKeyChecking=no "+ std::string(loginname) + "@" + MainDisk + ":/tmp/" + std::string(loginname) + "/" + user + "/" + object + " /tmp/tmpfiles").c_str()) != 0) {
        std::cout << "Download from main disk failed! Trying backup disk..." << std::endl;
        system(("scp -o StrictHostKeyChecking=no "+ std::string(loginname) + "@" + BackupDisk + ":/tmp/" + std::string(loginname) + "/backupfolder/" + user + "/" + object + " /tmp/tmpfiles").c_str());
    }
    
    std::string input = "Download file transfered over to server!";
    std::cout << input << std::endl;
    send(sock_fd, input.c_str(), input.length(), 0);

    char buf[100];
    std::fill(buf, buf+100, 0);
    recv(sock_fd, buf, 100, 0);
    printf(buf);
    printf("\n");

    std::cout << "\nFile Contents: " << std::endl;
    system(("cat " + object).c_str());
    std::cout << std::endl;

    system(("rm -f /tmp/tmpfiles/" + object).c_str());
    printf((user + "/" + object + " was downloaded from " + MainDisk + "\n").c_str());

    for (auto it = diskNameToFiles.cbegin(); it != diskNameToFiles.cend(); ++it)
    {
        std::cout << it->first << ": ";
        for (int j = 0; j < it->second.size(); j++) 
            std::cout << it->second[j] << " ";
        std::cout << std::endl;
    }

    std::cout << "FileHashSet: ";
    for (auto val : FileHashSet) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
}

void listCommand(std::string user, int sock_fd, int partition_power, std::vector<std::string> &vDisks, 
                   char loginname[], std::map<std::string, std::vector<std::string> > &diskNameToFiles, 
                   std::vector<int> &DiskPartitions, std::vector<std::string> &FilesPerPartition, std::unordered_set<int> &FileHashSet) {
    printf(("username: " + user + "\n").c_str());
    for (int i = 0; i < vDisks.size(); i++) {
        std::string MainDisk = vDisks[i];
        std::vector<std::string> fileList = diskNameToFiles[MainDisk];
        for (int j = 0; j < fileList.size(); j++) {
            size_t pos = fileList[j].find(user);
            //std::cout << user << "/" << object << std::endl;
            //std::cout << pos << std::endl;
            if (pos != std::string::npos)
            {
                size_t slashPos1 = fileList[j].find("/");
                std::string userObject = fileList[j].substr(slashPos1+1);
                size_t slashPos2 = userObject.find("/");
                std::string object = userObject.substr(slashPos2);

                system(("ssh -o StrictHostKeyChecking=no " + std::string(loginname) + "@" + MainDisk + " \"cd /tmp/" 
                        + std::string(loginname) + "/" + user + "; ls -lrt >> /home/" + std::string(loginname) + "/output" + MainDisk + ".txt\"").c_str());
                system(("scp -o StrictHostKeyChecking=no " + std::string(loginname) + "@" + MainDisk + ":/home/" 
                        + std::string(loginname) + "/output" + MainDisk + ".txt /tmp/tmpfiles/output" + MainDisk + ".txt").c_str());
                system(("ssh -o StrictHostKeyChecking=no " + std::string(loginname) + "@" + MainDisk + " \"rm /home/"
                        + std::string(loginname) + "/output" + MainDisk + ".txt\"").c_str());
                break;
            }
        }
    }

    std::ofstream outputFile("/tmp/tmpfiles/output.txt");
    for (int i = 0; i < vDisks.size(); i++) {
        std::string MainDisk = vDisks[i];
        std::string directoryPath = "/tmp/tmpfiles/output" + MainDisk + ".txt";
        struct stat sb;

        if (stat(directoryPath.c_str(), &sb) == 0) {
            std::ifstream inputFile(directoryPath);
            std::string line;

            std::getline(inputFile, line);
            while (std::getline(inputFile, line)) {
                outputFile << line << std::endl;
            }
            inputFile.close();
            system(("rm -f " + directoryPath).c_str());
        } 
    }
    outputFile.close();

    std::string message = ("Output for list command is completed.");
    send(sock_fd, message.c_str(), message.length(), 0);
    std::cout << message << std::endl;

    char buf[100];
    std::fill(buf, buf+100, 0);
    recv(sock_fd, buf, 100, 0);
    printf(buf);
    printf("\n");

    system("rm -f /tmp/tmpfiles/output.txt");
    printf("List command is complete.\n");
}

void uploadCommand(std::string userObject, int sock_fd, int partition_power, std::vector<std::string> &vDisks, 
                   char loginname[], std::map<std::string, std::vector<std::string> > &diskNameToFiles, 
                   std::vector<int> &DiskPartitions, std::vector<std::string> &FilesPerPartition, std::unordered_set<int> &FileHashSet) {
    //printf("In upload function\n");
    size_t slashPos = userObject.find("/");
    if (slashPos == std::string::npos) {
        printf("user/object not in correct format!\n");
        return;
    }
    std::string user = userObject.substr(0, slashPos);
    std::string object = userObject.substr(slashPos+1);

    std::string fileHash = md5(userObject);
    std::string binary_string = hex_str_to_bin_str(fileHash); 
    long int partition_number = std::stol(binary_string.substr(128 - partition_power), nullptr, 2);
    std::string MainDisk = vDisks[DiskPartitions[partition_number]];
    //printf(("Main Disk: " + MainDisk + "\nlogin name: " + std::string(loginname) + "\nUser/Object: " + user + "/" + object + "\n").c_str());
    
    std::string BackupDisk;
    if (DiskPartitions[partition_number] != vDisks.size() - 1)
        BackupDisk = vDisks[DiskPartitions[partition_number] + 1];
    else
        BackupDisk = vDisks[0];

    FileHashSet.insert(partition_number);
    FilesPerPartition[partition_number] = userObject;

    //printf(("ssh -o StrictHostKeyChecking=no " + std::string(loginname) + "@" + MainDisk + " \"mkdir -p /tmp/" + std::string(loginname) + "/" + user + "\"\n").c_str());
    //printf(("scp -o StrictHostKeyChecking=no /tmp/tmpfiles/" + object + " " + std::string(loginname) + "@" + MainDisk + ":/tmp/" + std::string(loginname) + "/" + user + "\n").c_str());

    char buf[100];
    std::fill(buf, buf+100, 0);
    recv(sock_fd, buf, 100, 0);
    printf(buf);
    printf("\n");

    system(("ssh -o StrictHostKeyChecking=no " + std::string(loginname) + "@" + MainDisk + " \"mkdir -p /tmp/" + std::string(loginname) + "/" + user + "\"").c_str());
    system(("scp -o StrictHostKeyChecking=no /tmp/tmpfiles/" + object + " " + std::string(loginname) + "@" + MainDisk + ":/tmp/" + std::string(loginname) + "/" + user).c_str());
    system(("ssh -o StrictHostKeyChecking=no " + std::string(loginname) + "@" + BackupDisk + " \"mkdir -p /tmp/" + std::string(loginname) + "/backupfolder/" + user + "\"").c_str());
    system(("scp -o StrictHostKeyChecking=no /tmp/tmpfiles/" + object + " " + std::string(loginname) + "@" + BackupDisk + ":/tmp/" + std::string(loginname) + "/backupfolder/" + user).c_str());
    system(("rm -f /tmp/tmpfiles/" + object).c_str());

    diskNameToFiles.at(MainDisk).push_back(std::string(loginname) + "/" + user + "/" + object);
    diskNameToFiles.at(BackupDisk).push_back(std::string(loginname) + "/backupfolder/" + user + "/" + object);

    std::string message = (user + "/" + object + " has been uploaded to main disk " + MainDisk + " and uploaded to backup disk " + BackupDisk);
    send(sock_fd, message.c_str(), message.length(), 0);

    printf((message + "\n").c_str());

    for (auto it = diskNameToFiles.cbegin(); it != diskNameToFiles.cend(); ++it)
    {
        std::cout << it->first << ": ";
        for (int j = 0; j < it->second.size(); j++) 
            std::cout << it->second[j] << " ";
        std::cout << std::endl;
    }

    std::cout << "FileHashSet: ";
    for (auto val : FileHashSet) {
        std::cout << val << " ";
    }
    std::cout << std::endl; 
    std::cout << FilesPerPartition[partition_number] << std::endl;
    
}

void deleteCommand(std::string userObject, int sock_fd, int partition_power, std::vector<std::string> &vDisks, 
                   char loginname[], std::map<std::string, std::vector<std::string> > &diskNameToFiles, 
                   std::vector<int> &DiskPartitions, std::vector<std::string> &FilesPerPartition, std::unordered_set<int> &FileHashSet) {
    
    //std::cout << "In delete command" << std::endl;
    size_t slashPos = userObject.find("/");
    if (slashPos == std::string::npos) {
        printf("user/object not in correct format!\n");
        return;
    }
    std::string user = userObject.substr(0, slashPos);
    std::string object = userObject.substr(slashPos+1);

    std::string fileHash = md5(userObject);
    std::string binary_string = hex_str_to_bin_str(fileHash); 
    long int partition_number = std::stol(binary_string.substr(128 - partition_power), nullptr, 2);

    std::string message;
    //std::cout << userObject << std::endl;
    //std::cout << FilesPerPartition[partition_number] << std::endl;
    if (FilesPerPartition[partition_number].compare(userObject) != 0) {
        message = "The file does not exist in any hard drive!";
        send(sock_fd, message.c_str(), message.length(), 0);
        return;
    }
    else {
        message = "Starting the deletion.";
        send(sock_fd, message.c_str(), message.length(), 0);
    }

    std::string MainDisk = vDisks[DiskPartitions[partition_number]];
    printf(("File is in main disk: " + MainDisk + "\n").c_str());
    std::string BackupDisk;
    if (DiskPartitions[partition_number] != vDisks.size() - 1)
        BackupDisk = vDisks[DiskPartitions[partition_number] + 1];
    else
        BackupDisk = vDisks[0];
    printf(("File is in backup disk: " + BackupDisk + "\n").c_str());

    system(("ssh -o StrictHostKeyChecking=no " + std::string(loginname) + "@" + MainDisk + " \"cd /tmp/" + std::string(loginname) + "/" + user + "; rm " + object + "\"").c_str());
    system(("ssh -o StrictHostKeyChecking=no " + std::string(loginname) + "@" + BackupDisk + " \"cd /tmp/" + std::string(loginname) + "/backupfolder/" + user + "; rm " + object + "\"").c_str());

    for (int i = 0; i < vDisks.size(); i++) {
        for (int j = 0; j < diskNameToFiles[vDisks[i]].size(); j++) {
            size_t pos = diskNameToFiles[vDisks[i]][j].find(user + "/" + object);
            //std::cout << user << "/" << object << std::endl;
            //std::cout << pos << std::endl;
            if (pos != std::string::npos)
                diskNameToFiles[vDisks[i]].erase(diskNameToFiles[vDisks[i]].begin() + j);
        }
    }

    FileHashSet.erase(partition_number);

    FilesPerPartition[partition_number] = "";

    message = (user + "/" + object + " has been deleted from main disk " + MainDisk + " and from backup disk " + BackupDisk);
    send(sock_fd, message.c_str(), message.length(), 0);
    printf((message + "\n").c_str());

    for (auto it = diskNameToFiles.cbegin(); it != diskNameToFiles.cend(); ++it)
    {
        std::cout << it->first << ": ";
        for (int j = 0; j < it->second.size(); j++) 
            std::cout << it->second[j] << " ";
        std::cout << std::endl;
    }

    std::cout << "FileHashSet: ";
    for (auto val : FileHashSet) {
        std::cout << val << " ";
    }
    std::cout << std::endl; 

}

void addCommand(std::string disk, int sock_fd, int partition_power, std::vector<std::string> &vDisks, 
                   char loginname[], std::map<std::string, std::vector<std::string> > &diskNameToFiles, 
                   std::vector<int> &DiskPartitions, std::vector<std::string> &FilesPerPartition, 
                   std::unordered_set<int> &FileHashSet, std::map<std::string, int> &diskNametodiskNumber) {
    vDisks.push_back(disk);
    diskNameToFiles[disk] = std::vector<std::string>();
    int numDisks = vDisks.size();
    int oldDiskSize = diskNametodiskNumber.size();
    diskNametodiskNumber[disk] = oldDiskSize;
    int numDiskNumbers = diskNametodiskNumber.size();
    //std::cout << diskNametodiskNumber[disk] << std::endl;

    for (int i = 0; i < numDiskNumbers - 1; i++) {
        int count = 0;
        int indexOfOld = 0;
        for (int j = 0; j < DiskPartitions.size(); j++) {
            if (DiskPartitions[j] == i && count < (pow(2, partition_power)/(numDisks - 1)/numDisks)) {
                DiskPartitions[j] = diskNametodiskNumber[disk];

                if (FileHashSet.find(j) != FileHashSet.end()) {
                    size_t slashPos = FilesPerPartition[j].find("/");
                    if (slashPos == std::string::npos) {
                        printf("user/object not in correct format!\n");
                        return;
                    }
                    std::string user = FilesPerPartition[j].substr(0, slashPos);
                    std::string object = FilesPerPartition[j].substr(slashPos+1);

                    std::string oldMainDisk;
                    for (int k = 0; k < numDisks - 1; k++) {
                        if (diskNametodiskNumber[vDisks[k]] == i)
                            oldMainDisk = vDisks[k];
                            indexOfOld = k;
                    }
                    std::string newMainDisk = vDisks[diskNametodiskNumber[disk]];
                    std::string oldBackupDisk;
                    if (indexOfOld != numDisks - 2)
                        oldBackupDisk = vDisks[indexOfOld + 1];
                    else
                        oldBackupDisk = vDisks[0];
                    std::string newBackupDisk = vDisks[0];

                    printf(("Moving from old disk " + oldMainDisk + " to new disk " + newMainDisk + "\n").c_str());
                    system(("scp -o StrictHostKeyChecking=no "+ std::string(loginname) + "@" + oldMainDisk + ":/tmp/" + std::string(loginname) + "/" + user + "/" + object + " /tmp/tmpfiles").c_str());
                    system(("ssh -o StrictHostKeyChecking=no " + std::string(loginname) + "@" + newMainDisk + " \"mkdir -p /tmp/" + std::string(loginname) + "/" + user + "\"").c_str());
                    system(("scp -o StrictHostKeyChecking=no /tmp/tmpfiles/" + object + " " + std::string(loginname) + "@" + newMainDisk + ":/tmp/" + std::string(loginname) + "/" + user).c_str());
                    system(("rm -f /tmp/tmpfiles/" + object).c_str());
                    system(("ssh -o StrictHostKeyChecking=no " + std::string(loginname) + "@" + oldMainDisk + " \"cd /tmp/" + std::string(loginname) + "/" + user + "; rm " + object + "\"").c_str());

                    printf(("Moving from old backup disk " + oldBackupDisk + " to new backup disk " + newBackupDisk + "\n").c_str());
                    system(("scp -o StrictHostKeyChecking=no "+ std::string(loginname) + "@" + oldBackupDisk + ":/tmp/" + std::string(loginname) + "/backupfolder/" + user + "/" + object + " /tmp/tmpfiles").c_str());
                    system(("ssh -o StrictHostKeyChecking=no " + std::string(loginname) + "@" + newBackupDisk + " \"mkdir -p /tmp/" + std::string(loginname) + "/backupfolder/" + user + "\"").c_str());
                    system(("scp -o StrictHostKeyChecking=no /tmp/tmpfiles/" + object + " " + std::string(loginname) + "@" + newBackupDisk + ":/tmp/" + std::string(loginname) + "/backupfolder/" + user).c_str());
                    system(("rm -f /tmp/tmpfiles/" + object).c_str());
                    system(("ssh -o StrictHostKeyChecking=no " + std::string(loginname) + "@" + oldBackupDisk + " \"cd /tmp/" + std::string(loginname) + "/backupfolder/" + user + "; rm " + object + "\"").c_str());

                    diskNameToFiles[newMainDisk].push_back(std::string(loginname) + "/" + user + "/" + object);
                    for (int k = 0; k < diskNameToFiles[oldMainDisk].size(); k++) {
                        size_t pos = diskNameToFiles[oldMainDisk][k].find(user + "/" + object);
                        //std::cout << user << "/" << object << std::endl;
                        //std::cout << pos << std::endl;
                        if (pos != std::string::npos)
                            diskNameToFiles[oldMainDisk].erase(diskNameToFiles[oldMainDisk].begin() + k);
                    }

                    diskNameToFiles[newBackupDisk].push_back(std::string(loginname) + "/backupfolder/" + user + "/" + object);
                    for (int k = 0; k < diskNameToFiles[oldBackupDisk].size(); k++) {
                        size_t pos = diskNameToFiles[oldBackupDisk][k].find(user + "/" + object);
                        //std::cout << user << "/" << object << std::endl;
                        //std::cout << pos << std::endl;
                        if (pos != std::string::npos)
                            diskNameToFiles[oldBackupDisk].erase(diskNameToFiles[oldBackupDisk].begin() + k);
                    }
                }
                count += 1;
            }
            else if (DiskPartitions[j] == i) {
                if (FileHashSet.find(j) != FileHashSet.end() && DiskPartitions[j] == diskNametodiskNumber[vDisks[numDisks-2]]) {
                    size_t slashPos = FilesPerPartition[j].find("/");
                    if (slashPos == std::string::npos) {
                        printf("user/object not in correct format!\n");
                        return;
                    }
                    std::string user = FilesPerPartition[j].substr(0, slashPos);
                    std::string object = FilesPerPartition[j].substr(slashPos+1);

                    diskNameToFiles[vDisks[numDisks-1]].push_back(std::string(loginname) + "/backupfolder/" + user + "/" + object);
                    for (int k = 0; k < diskNameToFiles[vDisks[0]].size(); k++) {
                        size_t pos = diskNameToFiles[vDisks[0]][k].find(user + "/" + object);
                        //std::cout << user << "/" << object << std::endl;
                        //std::cout << pos << std::endl;
                        if (pos != std::string::npos)
                            diskNameToFiles[vDisks[0]].erase(diskNameToFiles[vDisks[0]].begin() + k);
                    }
                }
            
                count += 1;
            }
        }
    }

    std::string diskNametoFilesString = "\nFiles are now on disks: \n";

    for (auto it = diskNameToFiles.cbegin(); it != diskNameToFiles.cend(); ++it)
    {
        std::cout << it->first << ": ";
        diskNametoFilesString += (it->first + ": ");
        for (int j = 0; j < it->second.size(); j++) {
            std::cout << it->second[j] << " ";
            diskNametoFilesString += (it->second[j] + " ");
        }
        std::cout << std::endl;
        diskNametoFilesString += "\n";
    }

    std::cout << "FileHashSet: ";
    for (auto val : FileHashSet) {
        std::cout << val << " ";
    }
    std::cout << std::endl; 

    std::cout << "DiskPartitions: ";
    for (int i = 0; i < DiskPartitions.size(); i++) {
        if (FileHashSet.find(i) != FileHashSet.end()) {
            std::cout << i << ": " << DiskPartitions[i] << " ";
        }
    }
    std::cout << std::endl; 

    send(sock_fd, diskNametoFilesString.c_str(), diskNametoFilesString.length(), 0);
    std::cout << diskNametoFilesString << std::endl;
}

void removeCommand(std::string disk, int sock_fd, int partition_power, std::vector<std::string> &vDisks, 
                   char loginname[], std::map<std::string, std::vector<std::string> > &diskNameToFiles, 
                   std::vector<int> &DiskPartitions, std::vector<std::string> &FilesPerPartition, 
                   std::unordered_set<int> &FileHashSet, std::map<std::string, int> &diskNametodiskNumber) {
    std::vector<int>OldDiskPartitions = DiskPartitions;

    bool usedDisk = false;
    for (int i = 0; i < vDisks.size(); i++) {
        if (vDisks[i] == disk) {
            usedDisk = true;
        }
    }
    if (!usedDisk)
    {
        printf("Disk given is not being used by server!\n");
        return;
    }
    int oldDiskNum;
    int prevDiskNum;
    int nextDiskNum;
    std::cout << oldDiskNum << std::endl;

    int numDisks = vDisks.size();
    int numDiskNumbers = diskNametodiskNumber.size();

    for (int i = 0; i < numDisks; i++) {
        if (vDisks[i] == disk) {
            if (i == 0) {
                prevDiskNum = numDisks - 1;
                oldDiskNum = i;
                nextDiskNum = i + 1;
            }
            else if (i == numDisks - 1) {
                prevDiskNum = i - 1;
                oldDiskNum = i;
                nextDiskNum = 0;
            }
            else {
                prevDiskNum = i - 1;
                oldDiskNum = i;
                nextDiskNum = i + 1;
            }
        }
    }
    for (int i = 0; i < numDisks; i++) {
        if (vDisks[i] == disk)
            continue;
        int count = 0;
        for (int j = 0; j < DiskPartitions.size(); j++) {
             if (DiskPartitions[j] == oldDiskNum && (count < pow(2, partition_power)/numDisks/(numDisks-1))) {
                DiskPartitions[j] = diskNametodiskNumber[vDisks[i]];
                count += 1;
             }
             else if (DiskPartitions[j] == oldDiskNum) {
                count += 1;
             }
        }
    }
    /*
    std::cout << "OldDiskPartitions: ";
    for (int i = 0; i < OldDiskPartitions.size(); i++) {
        if (FileHashSet.find(i) != FileHashSet.end()) {
            std::cout << i << ": " << OldDiskPartitions[i] << " ";
        }
    }
    std::cout << std::endl; 
    */
    for (int k = 0; k < DiskPartitions.size(); k++) {
        if (OldDiskPartitions[k] == prevDiskNum && FileHashSet.find(k) != FileHashSet.end()) {
            size_t slashPos = FilesPerPartition[k].find("/");
            if (slashPos == std::string::npos) {
                printf("user/object not in correct format!\n");
                return;
            }
            std::string user = FilesPerPartition[k].substr(0, slashPos);
            std::string object = FilesPerPartition[k].substr(slashPos+1);

            printf(("Moving from old backup disk " + disk + " to new backup disk " + vDisks[nextDiskNum] + "\n").c_str());
            system(("scp -o StrictHostKeyChecking=no "+ std::string(loginname) + "@" + disk + ":/tmp/" + std::string(loginname) + "/backupfolder/" + user + "/" + object + " /tmp/tmpfiles").c_str());
            system(("ssh -o StrictHostKeyChecking=no " + std::string(loginname) + "@" + vDisks[nextDiskNum] + " \"mkdir -p /tmp/" + std::string(loginname) + "/backupfolder/" + user + "\"").c_str());
            system(("scp -o StrictHostKeyChecking=no /tmp/tmpfiles/" + object + " " + std::string(loginname) + "@" + vDisks[nextDiskNum] + ":/tmp/" + std::string(loginname) + "/backupfolder/" + user).c_str());
            system(("rm -f /tmp/tmpfiles/" + object).c_str());
            system(("ssh -o StrictHostKeyChecking=no " + std::string(loginname) + "@" + disk + " \"cd /tmp/" + std::string(loginname) + "/backupfolder/" + user + "; rm " + object + "\"").c_str());

            diskNameToFiles[vDisks[nextDiskNum]].push_back(std::string(loginname) + "/backupfolder/" + user + "/" + object);
        }

        if (OldDiskPartitions[k] == oldDiskNum && FileHashSet.find(k) != FileHashSet.end()) {
            //std::cout << k << ": " << FilesPerPartition[k] << " " << std::endl;
            size_t slashPos = FilesPerPartition[k].find("/");
            if (slashPos == std::string::npos) {
                printf("user/object not in correct format!\n");
                return;
            }
            std::string user = FilesPerPartition[k].substr(0, slashPos);
            std::string object = FilesPerPartition[k].substr(slashPos+1);

            std::string oldMainDisk = disk;
            std::string newMainDisk;
            for (auto ip : diskNametodiskNumber) {
                if (ip.second == DiskPartitions[k]) 
                    newMainDisk = ip.first;
            }
            std::string oldBackupDisk;
            int oldIndex = 0;
            int newIndex = 0;
            for (int m = 0; m < numDisks; m++) {
                if (diskNametodiskNumber[vDisks[m]] == DiskPartitions[k])
                    oldIndex = m;
                if (diskNametodiskNumber[vDisks[m]] == DiskPartitions[k])
                    oldIndex = m;
            }
            if (oldIndex != numDisks - 1)
                oldBackupDisk = vDisks[nextDiskNum];
            else
                oldBackupDisk = vDisks[0];
            std::string newBackupDisk;
            if (oldIndex != prevDiskNum)
                oldBackupDisk = vDisks[oldIndex + 1 % numDisks];
            else
                oldBackupDisk = vDisks[oldIndex + 2 % numDisks];

            printf(("Moving from old disk " + oldMainDisk + " to new disk " + newMainDisk + "\n").c_str());
            system(("scp -o StrictHostKeyChecking=no "+ std::string(loginname) + "@" + oldMainDisk + ":/tmp/" + std::string(loginname) + "/" + user + "/" + object + " /tmp/tmpfiles").c_str());
            system(("ssh -o StrictHostKeyChecking=no " + std::string(loginname) + "@" + newMainDisk + " \"mkdir -p /tmp/" + std::string(loginname) + "/" + user + "\"").c_str());
            system(("scp -o StrictHostKeyChecking=no /tmp/tmpfiles/" + object + " " + std::string(loginname) + "@" + newMainDisk + ":/tmp/" + std::string(loginname) + "/" + user).c_str());
            system(("rm -f /tmp/tmpfiles/" + object).c_str());
            system(("ssh -o StrictHostKeyChecking=no " + std::string(loginname) + "@" + oldMainDisk + " \"cd /tmp/" + std::string(loginname) + "/" + user + "; rm " + object + "\"").c_str());

            printf(("Moving from old backup disk " + disk + " to new backup disk " + vDisks[nextDiskNum] + "\n").c_str());
            system(("scp -o StrictHostKeyChecking=no "+ std::string(loginname) + "@" + disk + ":/tmp/" + std::string(loginname) + "/backupfolder/" + user + "/" + object + " /tmp/tmpfiles").c_str());
            system(("ssh -o StrictHostKeyChecking=no " + std::string(loginname) + "@" + vDisks[nextDiskNum] + " \"mkdir -p /tmp/" + std::string(loginname) + "/backupfolder/" + user + "\"").c_str());
            system(("scp -o StrictHostKeyChecking=no /tmp/tmpfiles/" + object + " " + std::string(loginname) + "@" + vDisks[nextDiskNum] + ":/tmp/" + std::string(loginname) + "/backupfolder/" + user).c_str());
            system(("rm -f /tmp/tmpfiles/" + object).c_str());
            system(("ssh -o StrictHostKeyChecking=no " + std::string(loginname) + "@" + disk + " \"cd /tmp/" + std::string(loginname) + "/backupfolder/" + user + "; rm " + object + "\"").c_str());

            diskNameToFiles[vDisks[nextDiskNum]].push_back(std::string(loginname) + "/backupfolder/" + user + "/" + object);
            diskNameToFiles[newMainDisk].push_back(std::string(loginname) + "/" + user + "/" + object);
        }
    }

    for (int i = 0; i < vDisks.size(); i++) {
        if (vDisks[i] == disk) {
            vDisks.erase(vDisks.begin() + i);
        }
    }
    diskNameToFiles[disk].clear();
    diskNameToFiles.erase(disk);

    std::string diskNametoFilesString = "\nFiles are now on disks: \n";

    for (auto it = diskNameToFiles.cbegin(); it != diskNameToFiles.cend(); ++it)
    {
        std::cout << it->first << ": ";
        diskNametoFilesString += (it->first + ": ");
        for (int j = 0; j < it->second.size(); j++) {
            std::cout << it->second[j] << " ";
            diskNametoFilesString += (it->second[j] + " ");
        }
        std::cout << std::endl;
        diskNametoFilesString += "\n";
    }

    std::cout << "FileHashSet: ";
    for (auto val : FileHashSet) {
        std::cout << val << " ";
    }
    std::cout << std::endl; 

    std::cout << "DiskPartitions: ";
    for (int i = 0; i < DiskPartitions.size(); i++) {
        if (FileHashSet.find(i) != FileHashSet.end()) {
            std::cout << i << ": " << DiskPartitions[i] << " ";
        }
    }
    std::cout << std::endl; 
    
    send(sock_fd, diskNametoFilesString.c_str(), diskNametoFilesString.length(), 0);
    std::cout << diskNametoFilesString << std::endl;
}

int main(int argc, char *argv[]){
    //Get from the command line, server IP, src and dst files.
    if (argc < 3){
        printf ("Usage: %s <priority power> <IP addresses of hard drives>\n",argv[0]);
        exit(0);
    } 

    //Open a TCP socket, if successful, returns a descriptor
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Error: Cannot create socket.\n");
        return 0;
    }

    srand(time(NULL));
    int port_number = rand() % 8900 + 1024;

    //Setup the server address to bind using socket addressing structure
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port_number);
    int partition_power = atoi(argv[1]);
    std::vector<std::string> vDisks;
    
    for (int i = 2; i < argc; i++) {
        vDisks.push_back(argv[i]);
    }

    std::map<std::string, std::vector<std::string> >diskNameToFiles;
    std::map<std::string, int> diskNametodiskNumber;
    for (int i = 0; i < vDisks.size(); i++) {
        diskNameToFiles[vDisks[i]] = std::vector<std::string>();
        diskNametodiskNumber[vDisks[i]] = i;
    }

    std::vector<int> DiskPartitions(pow(2, partition_power), 0);
    std::vector<std::string> FilesPerPartition(pow(2, partition_power), "");

    for (int i = 0; i < DiskPartitions.size(); i++) {
        for (int j = 0; j < vDisks.size(); j++) {
            if (i >= pow(2, partition_power) * j/vDisks.size() and i < pow(2, partition_power) * (j+1)/vDisks.size())
                DiskPartitions[i] = j;
        }
    }
    
    std::unordered_set<int> FileHashSet;

    //bind IP address and port for server endpoint socket 
    while (bind(sock_fd, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr)) < 0) {
        port_number = rand() % 8900 + 1024;
        serveraddr.sin_port = port_number;
    }

    const char* directoryPath = "/tmp/tmpfiles";
    struct stat sb;

    if (stat(directoryPath, &sb) != 0) {
        std::cout << "Directory for temporary files does not exist!" << std::endl;
        system("mkdir -p /tmp/tmpfiles");
    } 
    else {
        std::cout << "Directory for temporary files does exist!" << std::endl;
    }

    printf("Server listening/waiting for client at port %d\n", port_number);
    if (listen(sock_fd, 10) < 0)
    {
        printf("Could not find client.");
        exit(0);
    }
    while (1) {
        char buf[100];
        std::fill(buf, buf+100, 0);
        socklen_t size = (socklen_t)sizeof(struct sockaddr_in);
        //Server accepts the connection and call the connection handler
        int new_sock_fd = accept(sock_fd, (struct sockaddr *)&clientaddr, &size);
        printf("Accept client.\n");
        recv(new_sock_fd, buf, 100, 0);
        
        std::string input = "";
        for (char ch: buf) {
            if (ch != '\0')
                input += ch;
        }
        std::string token;
        size_t spacePos;
        std::vector<std::string> tokens;
        
        while(input.size() != 0)
        {
            spacePos = input.find(" ");
            if(spacePos != std::string::npos)
            {
                token = input.substr(0, spacePos );
                input = input.substr(spacePos  + 1);
            }
            else
            {
                token = input;
                input = "";
            }

            // do token stuff
            tokens.push_back(token);
        }

        std::string cmd = tokens[0];
        char loginname[LOGIN_NAME_MAX];
        getlogin_r(loginname, LOGIN_NAME_MAX);

        try {
            if (cmd == "download") {
                downloadCommand(tokens[1], new_sock_fd, partition_power, vDisks, loginname, diskNameToFiles, DiskPartitions, FilesPerPartition, FileHashSet);
            }
            else if (cmd == "list") {
                listCommand(tokens[1], new_sock_fd, partition_power, vDisks, loginname, diskNameToFiles, DiskPartitions, FilesPerPartition, FileHashSet);
            }
            else if (cmd == "upload") {
                uploadCommand(tokens[1], new_sock_fd, partition_power, vDisks, loginname, diskNameToFiles, DiskPartitions, FilesPerPartition, FileHashSet);
            }
            else if (cmd == "delete") {
                deleteCommand(tokens[1], new_sock_fd, partition_power, vDisks, loginname, diskNameToFiles, DiskPartitions, FilesPerPartition, FileHashSet);
            }
            else if (cmd == "add") {
                addCommand(tokens[1], new_sock_fd, partition_power, vDisks, loginname, diskNameToFiles, DiskPartitions, FilesPerPartition, FileHashSet, diskNametodiskNumber);
            }
            else if (cmd == "remove") {
                removeCommand(tokens[1], new_sock_fd, partition_power, vDisks, loginname, diskNameToFiles, DiskPartitions, FilesPerPartition, FileHashSet, diskNametodiskNumber);
            }
        }
        catch (...) {
            std::cout << "Error! Please try again." << std::endl;
            break;
        }
    }
    //close socket descriptor
    close(sock_fd);

    //if (stat("/tmp/tmpfiles", &sb) == 0) {
        //system("rm -rf /tmp/tmpfiles");
    //}

    return 0;
}