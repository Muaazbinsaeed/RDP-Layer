#include<sys/types.h>	/* basic sysExpected_AckNo data types */
#include<sys/socket.h>	/* basic socket definitions */
#include<netinet/in.h>	/* sockaddr_in{} and other Internet defns */
#include<sys/time.h>	/* timeval{} for select() */
#include<time.h>		/* timespec{} for pselect() */
#include<arpa/inet.h>	/* inet(3) functions */
#include<errno.h>
#include<fcntl.h>		/* for nonblocking */
#include<netdb.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>	/* for S_xxx file mode constants */
#include<sys/uio.h>		/* for iovec{} and readv/writev */
#include<sys/wait.h>
#include<sys/un.h>		/* for Unix domain sockets */
#include<unistd.h>
#include<iostream>
#include<cstring>
#include<fstream>
#include<assert.h>
#include <ctime>
#include <time.h>       /* clock_t, clock, CLOCKS_PER_SEC */
#include <math.h>
#include<stdio.h>
using namespace std;

struct client_config_struct
{
    string ip;
    string file_name;
    int cp;//client port
    int ps;//packetsize
    int st;//server time
    int ad;//ack drop
    int ac;//ack_corrupt
    int sp;//ser port
    client_config_struct()
    {
        ip="127.0.0.1";
        file_name="1.txt";
        sp=1234;
        cp=2234;
        st=-1;
        ps=1000;
        ad=-1;
        ac=-1;
    }
    void read()
    {
    	ofstream file;
    	file.open ("Client_Configration.txt");
    	file.close();

        ifstream infile;
        infile.open ("Client_Configration.txt");
        string a;
        string b;
        string c;
        string d;
        string e;
        string f;
        string g;
        string h;
        int found ;
        string STRING;
        string previousLine="";
        while(getline(infile,STRING))
        {
            found= STRING.find(':');
            if (found !=0)
            {
                string s=STRING.substr(0,found);
                if((s=="Server IP"))
                {
                    ip=STRING.substr(found+1);
                }
                else if((s=="server port"))
                {
                    b=STRING.substr(found+1);
                    sp=atoi(b.c_str() );
                }
                else if((s=="client port"))
                {
                    c=STRING.substr(found+1);
                    cp=atoi(c.c_str() );
                }
                else if((s=="file name"))
                {
                    file_name=STRING.substr(found+1);
                }
                else if((s=="packet size"))
                {
                    e=STRING.substr(found+1);
                    ps=atoi(e.c_str() );
                }
                else if((s=="server timeout"))
                {
                    f=STRING.substr(found+1);
                    st=atoi(f.c_str() );
                }
                else if((s=="ack number to drop"))
                {
                    g=STRING.substr(found+1);
                    ad=atoi(g.c_str() );
                }
                else if((s=="ack number to corrupt"))
                {
                    h=STRING.substr(found+1);
                    ac=atoi(h.c_str() );
                }
            }
        }

    }
}client_config;

struct packet
{
	int Pkt_No;
	uint16_t checksum;
	char data[1000];
}*mypkt;
struct ack_packet {
	int Pkt_No;
	uint16_t checksum;
} ack;
int totalpackets = 0;
int check_corrupt_pkt() {
	int f = rand() % 10 + 1;
	return f;
}

uint16_t calculate_check_sum(void * buffer, size_t bufferlen) {
	assert(buffer);
	uint32_t a = 0;
	size_t length = bufferlen;
	const uint16_t * b = reinterpret_cast<const uint16_t*>(buffer);
	while (length > 1) {
		a += *b++;
		length -= sizeof(uint16_t);
	}
	if (length)
		a += *reinterpret_cast<const uint8_t *>(b);
	while (a >> 16)
		a = (a & 0xffff) + (a >> 16);
	return static_cast<uint16_t>(~a);
}
void makePkt(char *data, int no) {
	strcpy(mypkt[no].data, data);
	mypkt[no].checksum = calculate_check_sum(mypkt[no].data,
			sizeof(mypkt[no].data));
	mypkt[no].Pkt_No = no;
}
int gettotalpackets(char file[]) {
	int totalpackets = 0;
	ifstream filereader;
	filereader.open(file, ios::binary);
	filereader.seekg(0, fstream::end);
	int size = filereader.tellg();
	totalpackets = (size / 1000) + 1;
	cout << "No of Packets :" << totalpackets << endl;
	mypkt = new packet[totalpackets];
	filereader.seekg(0, ifstream::beg);
	for (int l = 0; !filereader.eof(); l++) {
		char data[1000] = { '\0' };
		filereader.read(data, 1000);
		makePkt(data, l);
	}
	return totalpackets;
}
void sighandler(int a) {
	alarm(3);
	cout << "\nAlarm was raised!\n";
	ofstream log;
	log.open("logfile.txt", ios::app);
	log << "Timeout occured!\n";
	log.close();
}

bool timer = true;

void time_handle(int sig) {
	cout << "Packet Time out\n";
	timer = false;
}

int main() {
	ofstream log;
	log.open("client_logfile.txt");

	timeval timeout;
	timeout.tv_sec = 2;
	timeout.tv_usec = 1;
	srand(time(0));
	signal(SIGALRM, sighandler);

	struct sockaddr_in server_addr;
	socklen_t sock_len = sizeof(server_addr);
	int conn_sock = socket(AF_INET, SOCK_DGRAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(client_config.sp);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	setsockopt(conn_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	char msg = 'c';

	clock_t timee;
	timee = clock();
	sendto(conn_sock, (void *) &msg, sizeof(msg), 0,
			(struct sockaddr *) &server_addr, sock_len);
	while (1) {
		recvfrom(conn_sock, (void *) &msg, sizeof(msg), 0,
				(struct sockaddr *) &server_addr, &sock_len);
		if (msg == 'y') {
			timee = clock() - timee;
			cout << "OBTAINED: RTT " << ((float) timee) / CLOCKS_PER_SEC
					<< endl;
			log << "OBTAINED: RTT " << ((float) timee) / CLOCKS_PER_SEC << endl;

			cout << "OBTAINED: HandShaking\n";
			log << "OBTAINED: HandShaking\n";

			char filename[100] = { "\0" };
			cout << "Enter filename:";
			cin >> filename;
			sendto(conn_sock, filename, strlen(filename), 0,
					(struct sockaddr *) &server_addr, sock_len);
			totalpackets = gettotalpackets(filename);
			sendto(conn_sock, (void *) &totalpackets, sizeof(totalpackets), 0,
					(struct sockaddr *) &server_addr, sock_len);

			//alarm(5);
			int windowsize = 6;
			for (int index = 0; index < totalpackets;) {
				for (int i = 0; i < windowsize; i++) {
					if ((index + i) >= totalpackets)
						break;

					mypkt[index + i].checksum = calculate_check_sum(
							mypkt[index + i].data,
							sizeof(mypkt[index + i].data));

					if (client_config.ad == i) //To corrupt Data for debugging
					{
						mypkt[index + i].checksum = 0;
						cout << "Testing: sending corrupt" << endl;
						log << "Testing: sending corrupt" << endl;
					}
					cout << "SENT #: sent packet number #"
							<< mypkt[index + i].Pkt_No << endl;
					log << "SENT #: sent packet number #"
							<< mypkt[index + i].Pkt_No << endl;
					sendto(conn_sock, (void *) &mypkt[index + i],
							sizeof(mypkt[index + i]), 0,
							(struct sockaddr *) &server_addr, sock_len);
				}
				signal(SIGALRM, time_handle);
				alarm(1);
				bool start_timer = true;

				int Expected_AckNo = index;
				bool ackrecv[6] = { false, false, false, false, false, false };
				for (int i = 0; i < windowsize;) {
					start_timer = timer;
					if (start_timer == false) {

						log << "ERROR: Packet Timeout occured!" << endl;
						cout << "ERROR: Packet Timeout occured!" << endl;
						timer = true;
						break;
					}
					if ((index + i) >= totalpackets)
						break;

					recvfrom(conn_sock, &ack, sizeof(ack), 0,
							(struct sockaddr *) &server_addr, &sock_len);
					cout << "ACK #: client received ACK #" << ack.Pkt_No
							<< endl;
					log << "ACK #: client received ACK #" << ack.Pkt_No << endl;

					if (ack.Pkt_No == Expected_AckNo) {
						cout << "ACK #: Successfully Recv\n";
						log << "ACK #: Successfully Recv\n";
						if (ackrecv[Expected_AckNo - index] == false) {
							ackrecv[Expected_AckNo - index] = true;
							i++;
							Expected_AckNo++;
						}
					} else if (ack.Pkt_No > Expected_AckNo
							&& ack.Pkt_No < index + windowsize) {
						cout << "ACK #: Irregular Order\n";
						log << "ACK #: Irregular Order\n";
						if (ackrecv[ack.Pkt_No - index] == false) {
							ackrecv[ack.Pkt_No - index] = true;
							i++;
						}
					} else {
						cout << "ERROR:Corrupt Data ACk\t Expected: "
								<< Expected_AckNo << endl;
						log << "ERROR:Corrupt Data ACk\t Expected: "
								<< Expected_AckNo << endl;
					}
				}
				index = Expected_AckNo;
			}
			break;
		}
	}
	log.close();
	return 0;
}
