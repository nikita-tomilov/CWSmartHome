#include <iostream>
#include <cstdlib>
#include <sstream>
#include <string>
#include <RF24/RF24.h>
#include <RF24Network/RF24Network.h>
#include <unistd.h>
#include "udp.h"

#define BUF_SIZE 32

#define UDP_MSG_TYPE_GET_RQ 0
#define UDP_MSG_TYPE_PUT_RQ 1
#define UDP_MSG_TYPE_GET_RP 2
#define UDP_MSG_TYPE_PUT_RP 3

typedef struct {
	uint8_t message_type;
	uint8_t reg_id;
	uint8_t payload[8];
} nrf_message_t;

RF24 radio(22,0);
RF24Network network(radio);

const uint16_t this_node = 00;

int main() {
	std::cout << "Hi there!" << std::endl;

	radio.begin();
	network.begin(/*channel*/ 90, /*node address*/ this_node);
	radio.printDetails();

	udp_client_server::udp_server srv("0.0.0.0", 1337);
	udp_client_server::udp_client cli("192.168.0.100", 1338);

	char buf[BUF_SIZE];
	int rq_len, res;

	while (true) {

		network.update();
		//std::cout << "lol" << std::endl;
    	while (network.available()) {  
    		RF24NetworkHeader in_header;
      		nrf_message_t in_msg;
      		network.read(in_header, &in_msg, sizeof(in_msg));
      		
      		std::cout << "NRF msg: nodeID = " << (int)in_header.from_node << std::endl;
			std::cout << " type = " << (int)in_msg.message_type << std::endl;
			std::cout << " reg = " << (int)in_msg.reg_id << std::endl;

			buf[0] = (int)in_header.from_node;
			buf[1] = (int)in_msg.message_type;
			buf[2] = (int)in_msg.reg_id;

			std::cout << " data = ";
			for (int i = 0; i < 8; i++) {
				buf[3 + i] = in_msg.payload[i];
				std::cout << (int)in_msg.payload[i] << " ";
			}
			std::cout << std::endl;

			res = cli.send(buf, 11);
			if (res < 0) {
				std::cerr << "UDP send err " << (int)errno << std::endl;
			}
    	}

		rq_len = srv.recv(buf, BUF_SIZE);
		if (rq_len > 0) {
			int nodeID = (int)buf[0];
			int msgType = (int)buf[1];
			int regID = (int)buf[2];

			std::cout << "UDP rq: nodeID = " << nodeID << std::endl;
			std::cout << " type = " << msgType << std::endl;
			std::cout << " reg = " << regID << std::endl;

			RF24NetworkHeader out_header(nodeID);
      		nrf_message_t out_msg;

      		out_msg.message_type = msgType;
      		out_msg.reg_id = regID;

      		std::cout << " data = ";
      		for (int i = 0; i < 8; i++) {
				out_msg.payload[i] = buf[3 + i];
				std::cout << (int)out_msg.payload[i] << " ";
			}
			std::cout << std::endl;

      		res = network.write(out_header, &out_msg, sizeof(out_msg));	
      		if (res < 0) {
      			std::cerr << "NRF send err " << res << std::endl;
      		}

		}
	}
}