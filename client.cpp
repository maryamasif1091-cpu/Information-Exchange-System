// client.cpp → Campus Client (Lahore, Karachi, CFD, Multan)
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
using namespace std;
string CAMPUS, PASSWORD;
int MY_PORT;
const string SERVER_IP = "127.0.0.1";
const int TCP_PORT = 54000;
const int UDP_PORT = 54001;
int tcp_fd = -1, udp_fd = -1;
bool alive = true;
vector<string> inbox;
mutex inbox_mutex;
string trim(const string& s) {
size_t start = s.find_first_not_of(" \t\r\n");
size_t end = s.find_last_not_of(" \t\r\n");
return (start == string::npos) ? "" : s.substr(start, end - start + 1);
}
void send_heartbeat() {
sockaddr_in serv{};
serv.sin_family = AF_INET;
serv.sin_port = htons(UDP_PORT);
inet_pton(AF_INET, SERVER_IP.c_str(), &serv.sin_addr);
string payload = "HEARTBEAT:" + CAMPUS;
while (alive) {
sendto(udp_fd, payload.c_str(), payload.size(), 0, (sockaddr*)&serv, sizeof(serv));
this_thread::sleep_for(chrono::seconds(10));
}
}
void listen_udp() {
char buf[4096];
while (alive) {
sockaddr_in src{};
socklen_t len = sizeof(src);
int n = recvfrom(udp_fd, buf, sizeof(buf)-1, 0, (sockaddr*)&src, &len);
if (n > 0) {
buf[n] = '\0';
string msg = trim(buf);
if (msg.rfind("BROADCAST:", 0) == 0) {
cout << "\n[BROADCAST FROM CENTRAL] " << msg.substr(10) << "\n> ";
cout.flush();
}
}
}
}
void listen_tcp() {
char buf[4096];
while (alive) {
int n = recv(tcp_fd, buf, sizeof(buf)-1, 0);
if (n <= 0) { alive = false; break; }
buf[n] = '\0';
string msg = trim(buf);
if (msg == "AUTH_OK") cout << "Successfully connected to Central Server!\n\n";
else if (msg == "SENT" || msg == "SENT_TO_CENTRAL") cout << "Message sent successfully!\n> ";
else if (msg.rfind("ERR:", 0) == 0) cout << msg << "\n> ";
else {
lock_guard<mutex> lock(inbox_mutex);
inbox.push_back(msg);
cout << "\nNew message received!\n> ";
}
cout.flush();
}
}
void show_menu() {
cout << "\n" << string(50, '=') << "\n";
cout << "           CAMPUS: " << CAMPUS << "\n";
cout << string(50, '=') << "\n";
cout << "1. Send Message\n";
cout << "2. View Inbox (" << inbox.size() << ")\n";
cout << "3. Exit\n";
cout << "> ";
}
void view_inbox() {
lock_guard<mutex> lock(inbox_mutex);
if (inbox.empty()) { cout << "\nInbox is empty.\n\n"; return; }
cout << "\n" << string(45, '-') << "\n";
cout << "                INBOX\n";
cout << string(45, '-') << "\n";
for (size_t i = 0; i < inbox.size(); ++i)
cout << (i+1) << ". " << inbox[i] << "\n";
cout << string(45, '-') << "\n\n";
}
int main() {
cout << "Enter Campus Name (Lahore/Karachi/CFD/Multan): ";
getline(cin, CAMPUS); CAMPUS = trim(CAMPUS);
cout << "Enter UDP Port (e.g. 6001): ";
string p; getline(cin, p); MY_PORT = stoi(trim(p));
cout << "Enter Password: ";
getline(cin, PASSWORD); PASSWORD = trim(PASSWORD);
udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
sockaddr_in me{};
me.sin_family = AF_INET;
me.sin_port = htons(MY_PORT);
me.sin_addr.s_addr = INADDR_ANY;
bind(udp_fd, (sockaddr*)&me, sizeof(me));
tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
sockaddr_in srv{};
srv.sin_family = AF_INET;
srv.sin_port = htons(TCP_PORT);
inet_pton(AF_INET, SERVER_IP.c_str(), &srv.sin_addr);
connect(tcp_fd, (sockaddr*)&srv, sizeof(srv));
string auth = "Campus:" + CAMPUS + ";Pass:" + PASSWORD + "\n";
send(tcp_fd, auth.c_str(), auth.size(), 0);
thread(send_heartbeat).detach();
thread(listen_udp).detach();
thread(listen_tcp).detach();
this_thread::sleep_for(chrono::seconds(2));

while (alive) {
show_menu();
string ch; getline(cin, ch); ch = trim(ch);
if (ch == "1") {
cout << "To (Campus or Islamabad): "; string to; getline(cin, to); to = trim(to);
cout << "Department: "; string dept; getline(cin, dept); dept = trim(dept);
cout << "Message: "; string msg; getline(cin, msg); msg = trim(msg);
string packet = "TO:" + to + "|Dept:" + dept + "|MSG:" + msg + "\n";
send(tcp_fd, packet.c_str(), packet.size(), 0);
}
else if (ch == "2") view_inbox();
else if (ch == "3") alive = false;
}
close(tcp_fd); close(udp_fd);
return 0;
}