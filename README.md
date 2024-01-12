# Host status checker
Checking if the host is online or not, from 2 different solutions
## solution 1 :
A simple program to check the online or offline status of a host using C and Winsock to attempt to connect a connection with an specific port of specified host ip address , if connection was succesful then host is online(or in other word the selected port is open and host is resposible with this port) if not then host is ofline(or host is not resposible with this port).

### Usage :
1.Download or clone the code:
  ```
  git  clone https://github.com/Ali-Nikaein/Host_status_checker.git
  ```
2.Open the network_status.c file using a text editor (e.g., Visual Studio Code or Notepad++).

3.Edit the target IP address and desired port in the main.c file:
  ```
  char *address = "127.0.0.1";
  int port = 445;
  ```
4.compile and Run the program:
  ```
  gcc  network_status.c  -o  host_checker  -lws2_32
  host_checker
  ```

### Result :
If the program runs successfully, it will display the result of the host status check.
If the host is online:
  ####   Host is online.
If the host is offline or the connection fails:
  ####   Host is offline.
  ####   Or you may have entered a closed or wrong port number.



## solution 2 :
This program is designed to check the online or offline status of a host using Internet Control Message Protocol (ICMP). Unlike the previous code, which attempted to establish a connection with a specific port, this solution utilizes ICMP to determine the host's status.

### Purpose of the Code :
The primary purpose of this code is to provide an alternative method for checking the network status of a specified host. It sends ICMP Echo Requests and waits for Echo Replies to assess whether the host is online or offline.

### How to Use :
1.Download or clone the code:
  ```
  git  clone https://github.com/Ali-Nikaein/Host_status_checker.git
  ```
  
2.Open the ping_icmp.c file using a text editor (e.g., Visual Studio Code or Notepad++).
   
3.Modify the target hostname in the main.c file:
  ```
  const char *hostname = "soft98.ir";
  ```

4.Run the program:
  ```
  gcc  ping_icmp.c  -o  network_status_checker  -lws2_32
  ./network_status_checker
  ```

### Result :
![Screenshot 2024-01-12 121326](https://github.com/Ali-Nikaein/Host_status_checker/assets/108432369/2e36f9c6-4fca-4db7-a9cc-3f49c3c8fca5)

