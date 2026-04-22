// server.cpp → Islamabad Central Server
// Compile: g++ server.cpp -o server -pthread
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
using namespace std;
const int TCP_PORT = 54000;
const int UDP_PORT = 54001;
// 4 campuses
map<string, string> passwords =
{
{"Lahore",  "NU-LHR-123"},
{"Karachi", "NU-KHI-123"},
{"CFD",     "NU-CFD-123"},
{"Multan",  "NU-MUL-123"}
};
struct ClientInfo {
int tcp_fd;
string ip;
int udp_port = 0;
};
map<string, ClientInfo> clients;
vector<string> central_inbox;
mutex clients_lock, inbox_lock;
bool running = true;
string trim(const string& s) {
size_t start = s.find_first_not_of(" \t\r\n");
size_t end = s.find_last_not_of(" \t\r\n");
return (start == string::npos) ? "" : s.substr(start, end - start + 1);
}
void broadcast_udp(const string& msg) {
string payload = "BROADCAST:" + msg;
lock_guard<mutex> lock(clients_lock);
for (auto& c : clients) {
if (c.second.udp_port == 0) continue;
sockaddr_in dest{};
dest.sin_family = AF_INET;
dest.sin_port = htons(c.second.udp_port);
inet_pton(AF_INET, c.second.ip.c_str(), &dest.sin_addr);
int s = socket(AF_INET, SOCK_DGRAM, 0);
sendto(s, payload.c_str(), payload.size(), 0, (sockaddr*)&dest, sizeof(dest));
close(s);
}
cout << "Broadcast sent to " << clients.size() << " campuses!\n";
}
void handle_client(int fd, string client_ip) {
char buf[4096];
int n = recv(fd, buf, sizeof(buf)-1, 0);
if (n <= 0) { close(fd); return; }
buf[n] = '\0';
string auth = trim(buf);
string campus, pass;
size_t cp = auth.find("Campus:");
size_t pp = auth.find(";Pass:");
if (cp != string::npos && pp != string::npos) {
campus = trim(auth.substr(cp+7, pp-cp-7));
pass = trim(auth.substr(pp+6));
}
if (passwords.count(campus) && passwords[campus] == pass) {
send(fd, "AUTH_OK\n", 8, 0);
cout << "Connected: " << campus << " (" << client_ip << ")\n";
{ lock_guard<mutex> lock(clients_lock); clients[campus] = {fd, client_ip, 0}; }

while (running) {
n = recv(fd, buf, sizeof(buf)-1, 0);
if (n <= 0) break;
buf[n] = '\0';
string msg = trim(buf);
size_t to_pos = msg.find("TO:");
size_t dept_pos = msg.find("|Dept:");
size_t msg_pos = msg.find("|MSG:");
if (to_pos == 0 && dept_pos != string::npos && msg_pos != string::npos) {
string target = trim(msg.substr(3, dept_pos-3));
string dept = trim(msg.substr(dept_pos+6, msg_pos-dept_pos-6));
string text = trim(msg.substr(msg_pos+5));
if (target == "Islamabad" || target == "Central") {
string entry = "From: " + campus + " | Dept: " + dept + " | " + text;
{ lock_guard<mutex> lock(inbox_lock); central_inbox.push_back(entry); }
cout << "\nMESSAGE TO CENTRAL:\n" << entry << "\n\n";
send(fd, "SENT_TO_CENTRAL\n", 16, 0);
}
else if (clients.count(target)) {
string fwd = "FROM:" + campus + "|Dept:" + dept + "|MSG:" + text + "\n";
send(clients[target].tcp_fd, fwd.c_str(), fwd.size(), 0);
send(fd, "SENT\n", 5, 0);
}
else {
send(fd, "ERR:Target offline\n", 19, 0);
}
}
}
} else {
send(fd, "AUTH_FAIL\n", 10, 0);
cout << "AUTH FAILED: " << campus << " from " << client_ip << "\n";
}
close(fd);
lock_guard<mutex> lock(clients_lock);
for (auto it = clients.begin(); it != clients.end(); ++it) {
if (it->second.tcp_fd == fd) {
cout << it->first << " disconnected.\n";
clients.erase(it); break;
}
}
}

int main() {
int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
int opt = 1;
setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
sockaddr_in addr{};
addr.sin_family = AF_INET;
addr.sin_addr.s_addr = INADDR_ANY;
addr.sin_port = htons(TCP_PORT);
bind(tcp_sock, (sockaddr*)&addr, sizeof(addr));
listen(tcp_sock, 10);

addr.sin_port = htons(UDP_PORT);
bind(udp_sock, (sockaddr*)&addr, sizeof(addr));
cout << "\nCentral Server (Islamabad) - Online\nTCP:54000 | UDP:54001\n\n";
// HEARTBEAT LISTENER
thread([udp_sock]() {
char buf[4096];
while (running) {
sockaddr_in src{}; socklen_t len = sizeof(src);
int n = recvfrom(udp_sock, buf, sizeof(buf)-1, 0, (sockaddr*)&src, &len);
if (n > 0 && strstr(buf, "HEARTBEAT:") == buf) {
string campus = trim(buf + 10);
char ip[64]; inet_ntop(AF_INET, &src.sin_addr, ip, sizeof(ip));
int port = ntohs(src.sin_port);
lock_guard<mutex> lock(clients_lock);
if (clients.count(campus)) {
clients[campus].ip = ip;
clients[campus].udp_port = port;
cout << "Heartbeat from " << campus << " (" << ip << ":" << port << ")\n";
}
}
}
}).detach();
// TCP Acceptor
thread([tcp_sock]() {
while (running) {
sockaddr_in caddr{}; socklen_t clen = sizeof(caddr);
int cfd = accept(tcp_sock, (sockaddr*)&caddr, &clen);
if (cfd >= 0) {
char ip[64]; inet_ntop(AF_INET, &caddr.sin_addr, ip, sizeof(ip));
thread(handle_client, cfd, string(ip)).detach();
}
}
}).detach();
// Admin Menu
string choice;
while (running) {
cout << string(55, '=') << "\n";
cout << "       CENTRAL OFFICE - ISLAMABAD\n";
cout << string(55, '=') << "\n";
cout << "1. Send Broadcast Message\n";
cout << "2. View Connected Campuses\n";
cout << "3. View Central Office Inbox (" << central_inbox.size() << ")\n";
cout << "4. Exit Server\n";
cout << "> ";
if (!getline(cin, choice)) break;
choice = trim(choice);
if (choice == "1") {
cout << "Broadcast message: "; string m; getline(cin, m);
if (!m.empty()) broadcast_udp(m);
}
else if (choice == "2") {
lock_guard<mutex> lock(clients_lock);
cout << "\nConnected (" << clients.size() << "):\n";
for (auto& c : clients)
cout << " • " << c.first << " (" << c.second.ip << ":" << c.second.udp_port << ")\n";
}
else if (choice == "3") {
lock_guard<mutex> lock(inbox_lock);
if (central_inbox.empty()) cout << "\nInbox empty.\n\n";
else {
cout << "\n--- CENTRAL OFFICE INBOX ---\n";
for (size_t i = 0; i < central_inbox.size(); i++)
cout << (i+1) << ". " << central_inbox[i] << "\n\n";
}
}
else if (choice == "4") running = false;
}
return 0;
}