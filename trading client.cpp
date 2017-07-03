/*
*My implementation of the Morgan project exchange simulator trading client.
*/
#include <array>
#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <string>
#include <fstream>
#include <sstream>

using boost::asio::ip::tcp;


int main(int argc, char* argv[])
{	//the file to store the trading order sent from the client to the server.
	std::ofstream orderfile;
	orderfile.open("order.txt");

	//read in the ip address.
	std::string ip_address;
	std::cout << "Please enter your ip address:";
	std::cin >> ip_address;

	boost::asio::io_service io_service;
	tcp::resolver resolver(io_service);
	tcp::resolver::query query(ip_address, "9876");
	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	tcp::socket socket(io_service);
	boost::asio::connect(socket, endpoint_iterator);
	boost::system::error_code error;

	//analyze the input of the client and get new_or_cancel, side, symbol, quantity, price, cancel orderid.
	std::cout << "Please enter your order. \nFollow the format: new buy(sell) item1(item2) quantity price || cancel cancel_order_id\n";
	std::string new_or_cancel;
	std::string cancel_orderid;
	std::string side;
	std::string symbol;
	int quantity;
	double price;
	std::string message_to_send;
	int orderid=-1;

	while (std::cin) {
		//transform the input message of the user to FIX message.
		while (true) {
			orderid += 1;
			std::string line;
			std::getline(std::cin, line);
			if (line == "QUIT") {
				boost::asio::write(socket, boost::asio::buffer("QUIT"), error);
				return 0;
			}

			std::stringstream ssin(line);
			ssin >> new_or_cancel;
			if (new_or_cancel == "new") {
				ssin >> side >> symbol >> quantity >> price;
				if ((side == "buy" || side == "sell") && (symbol == "item1" || symbol == "item2") && ssin) {
					orderfile << orderid << " " << new_or_cancel << " " << side << " " << symbol << " " << quantity << " " << price << "\n";
					if (side == "buy") message_to_send = "35=D;54=1;55=" + symbol + ";11=" + std::to_string(orderid) + ";44=" + std::to_string(price) + ";38=" + std::to_string(quantity);
					if (side == "sell") message_to_send = "35=D;54=2;55=" + symbol + ";11=" + std::to_string(orderid) + ";44=" + std::to_string(price) + ";38=" + std::to_string(quantity);
					boost::asio::write(socket, boost::asio::buffer(message_to_send), error);
					break;
				}
				else std::cout << "Wrong Input!\n";
			}
			else if (new_or_cancel == "cancel") {
				ssin >> cancel_orderid;
				orderfile << orderid << " " << new_or_cancel << " " << cancel_orderid << "\n";
				message_to_send = "35=F;11=" + std::to_string(orderid) + ";41=" + cancel_orderid;
				boost::asio::write(socket, boost::asio::buffer(message_to_send), error);
				break;
			}
		}

		//get the messages sent from the server and transform them to print on the console.
		while (true) {
			std::array<char, 256> input_buffer;
			std::size_t rsize = socket.read_some(
				boost::asio::buffer(input_buffer), error);
			std::string message_back(input_buffer.data(), input_buffer.data() + rsize);
			if (message_back.find("Done") != -1) break;
			else if (message_back.find("35=8;150=0;39=0") != -1)  std::cout << "The order was accepted.\n";
			else if (message_back.find("35=9;39=8") != -1)  std::cout << "The cancel request was rejected.\n";
			else if (message_back.find("35=8;150=4;39=4") != -1) {
				int left_quantity_from = message_back.find("151=");
				std::cout << "The cancel request was accepted. The quantity left is " << message_back.substr(left_quantity_from + 4) << "\n";
			}
			else {
				int orderid_from = message_back.find("11=");
				int orderid_to = message_back.substr(orderid_from).find(";");
				std::string order_id = message_back.substr(orderid_from + 3, orderid_to - 3);
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
		std::cout << "\n";
	}
		return 0;
}

	
		
