#include<sys/types.h>	/* basic system data types */
#include<sys/socket.h>	/* basic socket definitions */
#include<netinet/in.h>	/* sockaddr_in{} and other Internet defns */
#include<sys/time.h>	/* timeval{} for select() */
#include<time.h>		/* timespec{} for pselect() */
#include<arpa/inet.h>	/* inet(3) functions */
#include<fcntl.h>		/* for nonblocking */
#include<netdb.h>
#include<signal.h>
#include<string.h>
#include<sys/stat.h>	/* for S_xxx file mode constants */
#include<sys/uio.h>		/* for iovec{} and readv/writev */
#include<unistd.h>
#include<sys/wait.h>
#include<sys/un.h>		/* for Unix domain sockets */
#include<stdlib.h>
#include<unistd.h>
#include<iostream>
#include<cstring>
#include <string.h>
#include <stdlib.h>
#include<fstream>
#include <ctime>
#include<stdio.h>
#include<errno.h>
#define MAXBUFsock_len 1000
using namespace std;

int binary[8];
bool ackNo = 0;
int totalpackets;

struct server_config_struct
{
    string file_name;//file name
    int ps;//packet size
    int sp;//server port
    int pd;//packet drop
    int pc;//packet courpt
    server_config_struct()
    {
        sp = 1234;
        file_name = "1.txt";
        ps=1000;
        pd=-1;
        pc=-1;
    }
    void read()
    {
    	    ofstream file;
    	    file.open ("Server_Configration.txt");
    	    file.close();

            ifstream infile;
            infile.open ("Server_Configration.txt");
            string a;
            string b;
            string c;
            string d;
            int found ;
            string STRING;
            string previousLine="";
            while(getline(infile,STRING))
            {
                found= STRING.find(':');
                if (found !=0)
                {
                    string s=STRING.substr(0,found);
                    if((s=="server port"))
                    {
                        a=STRING.substr(found+1);
                        sp=atoi(a.c_str() );
                    }
                    else if((s=="file name"))
                    {
                        file_name=STRING.substr(found+1);
                    }
                    else if((s=="packet size"))
                    {
                        b=STRING.substr(found+1);
                        ps=atoi(b.c_str() );
                    }
                    else if((s=="packet number to drop"))
                    {
                        c=STRING.substr(found+1);
                        pd=atoi(c.c_str() );
                    }
                    else if((s=="packet number to corrupt"))
                    {
                        d=STRING.substr(found+1);
                        pc=atoi(d.c_str() );
                    }
                }
            }
        }
}server_config;


struct packet {
	int Pkt_No;
	uint16_t checksum;
	char data[1000];
}*mypkt;
struct ack_packet {
	int Pkt_No;
	uint16_t checksum;
} ack;

uint16_t check_sum(void * buffer, size_t buffersock_len) {
	//assert(buffer);
	uint32_t a = 0;
	size_t sock_length = buffersock_len;
	const uint16_t * b = reinterpret_cast<const uint16_t*>(buffer);
	while (sock_length > 1) {
		a += *b++;
		sock_length -= sizeof(uint16_t);
	}
	if (sock_length)
		a += *reinterpret_cast<const uint8_t *>(b);
	while (a >> 16)
		a = (a & 0xffff) + (a >> 16);

	return static_cast<uint16_t>(~a);
}

int main() {

	ofstream log;
	log.open("server_logfile.txt");
	server_config.read();
	struct sockaddr_in client_addr, server_addr;
	int conn_sock = socket(AF_INET, SOCK_DGRAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_config.sp);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	bind(conn_sock, (struct sockaddr*) &server_addr, sizeof(struct sockaddr));
	perror("bind");
	socklen_t sock_len = sizeof(client_addr);

	while (1) {
		char msg = '\0';
		recvfrom(conn_sock, (void *) &msg, sizeof(msg), 0,
				(struct sockaddr *) &client_addr, &sock_len);
		if (msg == 'c') {
			msg = 'y';
			sendto(conn_sock, (void *) &msg, sizeof(msg), 0,
					(struct sockaddr *) &client_addr, sock_len);
			cout << "OBTAINED: HandShaking\n";
			log  << "OBTAINED: HandShaking\n";

			char filename[30] = { '\0' };
			recvfrom(conn_sock, filename, 30, 0,
					(struct sockaddr *) &client_addr, &sock_len);
			ofstream filewriter;
			filewriter.open(filename, ios::binary);

			recvfrom(conn_sock, &totalpackets, sizeof(totalpackets), 0,
					(struct sockaddr *) &client_addr, &sock_len);
			cout<<"OBTAINED: server knows file length and is ready to receive data #"<< totalpackets << endl;
			log <<"OBTAINED: server knows file length and is ready to receive data #"<< totalpackets << endl;
			mypkt = new packet[totalpackets];
			clock_t tinitial = clock();

			for (int i = 0; i < totalpackets;)
			{
				ack.Pkt_No = -1; //by default is wrong
				recvfrom(conn_sock, &mypkt[i], sizeof(mypkt[i]), 0,
						(struct sockaddr *) &client_addr, &sock_len);
				cout << "RECV: Packet#" << mypkt[i].Pkt_No <<endl;
				log  << "RECV: Packet#" << mypkt[i].Pkt_No <<endl;
				if (server_config.pd == i) //To corrupt Data for debugging
				{
					sleep(1);
					cout << "timeout corrupt" << endl; ///////////////
					log << "timeout corrupt" << endl; ///////////////
				}

				if (mypkt[i].Pkt_No == i) {
					uint16_t CHKSUM = check_sum(mypkt[i].data,
							sizeof(mypkt[i].data));
					if (CHKSUM == mypkt[i].checksum) //////packet read successfull////
							{
						ack.Pkt_No = mypkt[i].Pkt_No;
						filewriter << mypkt[i].data;
						i++;
						cout << "RECV: Packet Successfully" << endl;
						log  << "RECV: Packet Successfully" << endl;
					} else {
						cout << "ERROR: Corrupted Packet \n";
						log  << "ERROR: Corrupted Packet \n";
					}
				}
				else {

					cout << "ERROR: Wrong Packet No\t Expected: " << i << endl;
					log  << "ERROR: wrong Packet No\t Expected: " << i << endl;
				}
				sendto(conn_sock, (void *) &ack, sizeof(ack), 0,
						(struct sockaddr *) &client_addr, sock_len);
			}
			/////////calculating time///////////////
			clock_t tfinal = clock();
			double totaltime = double(tfinal) - double(tinitial);
			cout << "Time taken : " << totaltime << endl;

		}
	}

	log.close();
	close(conn_sock);
	return 0;
}
