/*
*My implementation of the Morgan project exchange simulator monitoring client.
*/
#include <array>
#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <string>

using boost::asio::ip::tcp;

int main(int argc, char* argv[])
{	//read in the ip address.
	std::string ip_address;
	std::cout << "Please enter your ip address:";
	std::cin >> ip_address;
	
	boost::asio::io_service io_service;
	tcp::resolver resolver(io_service);
	tcp::resolver::query query(ip_address, "1234");
	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	tcp::socket socket(io_service);
	boost::asio::connect(socket, endpoint_iterator);
	boost::system::error_code error;

	//get the messages sent from the server and transform them to print on the console.
	while (1) {
		std::array<char, 256> input_buffer;
		std::size_t rsize = socket.read_some(boost::asio::buffer(input_buffer), error);
		std::string message_back(input_buffer.data(), input_buffer.data() + rsize);

		if (message_back == "") return 0;
		int orderid_from = message_back.find("11=");
		int orderid_to = message_back.substr(orderid_from).find(";");
		std::string order_id = message_back.substr(orderid_from + 3, orderid_to - 3);

		if (message_back.find("35=8;150=0;39=0") != -1)  std::cout << "The order(id:" << order_id << ") was accepted.\n";
		else if (message_back.find("35=9;39=8") != -1)  std::cout << "The cancel request(id:"<<order_id<<") was rejected.\n";
		else if (message_back.find("35=8;150=4;39=4") != -1) {
			int left_quantity_from = message_back.find("151=");
			std::cout << "The cancel request was accepted. The quantity left is " << message_back.substr(left_quantity_from + 4) << "\n";
		}
		else {
			int price_from = message_back.find("31=");
			int price_to = message_back.substr(price_from).find(";");
			std::stringstream ss(message_back.substr(price_from + 3, price_to - 3));
			double transaction_price;
			ss >> transaction_price;
			if (message_back.find("35=8;150=2;39=2") != -1) std::cout << "The order(id:" << order_id << ") was fully filled. The transaction price is " << transaction_price << "\n";
			else {
				int left_quantity_from = message_back.find("151=");
				std::cout << "The order(id:" << order_id << ") was partial filled. The transaction price is " << transaction_price << ". The quantity left is " << message_back.substr(left_quantity_from + 4) << "\n";
			}
		}
	}
}
