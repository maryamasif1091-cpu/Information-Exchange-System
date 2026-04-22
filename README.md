# NU-Information Exchange System (FAST Multi-Campus Network)

##  Project Overview
This project simulates communication between multiple FAST-NUCES campuses using a hybrid TCP/UDP client-server architecture.



---

##  Objective
To implement a multi-campus communication system where different campuses can:
- Send direct messages (TCP)
- Broadcast announcements (UDP)
- Maintain online status using heartbeat signals

---

##  Technologies Used
- C++ (Socket Programming)
- TCP & UDP Protocols
- Multi-threading


---

##  System Architecture

###  Central Server (Islamabad)
- Handles multiple clients
- Authenticates campuses
- Routes messages
- Broadcasts announcements

###  Campus Clients
- Connect to server via TCP
- Send heartbeat via UDP
- Provide console interface for users

---

##  Network Configuration

| Campus  | Password      | UDP Port |
|---------|--------------|----------|
| Lahore  | NU-LHR-123   | 6001     |
| Karachi | NU-KHI-123   | 6002     |
| CFD     | NU-CFD-123   | 6003     |
| Multan  | NU-MUL-123   | 6004     |

---

##  Communication Protocol

###  TCP (Reliable)
- Authentication
- Direct messaging
- Command handling

###  UDP (Fast)
- Heartbeat messages
- Broadcast messages

---

##  How to Run

### 1. Compile Server
```bash
g++ server.cpp -o server -pthread
./server
```

### 2. Compile Client
```bash
g++ client.cpp -o client -pthread
./client
```

### 3. Run Clients
- Enter Campus Name
- Enter Port (e.g., 6001)
- Enter Password

---

##  Features
- Multi-client handling using threads
- Real-time messaging
- Broadcast system
- Inbox system for each campus
- Central monitoring system

---

## Learning Outcomes
- Socket programming (TCP & UDP)
- Multi-threading in C++
- Network simulation concepts

---


