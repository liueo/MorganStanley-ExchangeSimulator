/*
*My implementation of the Morgan project exchange simulator server.
*/

#include <array>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <boost/asio.hpp>
#include <windows.h>

using boost::asio::ip::tcp;

/*
*Maps to store the trading information, which function as the order book.
*The key of the map is price(double), the value is a vector storing strings.
*The two strings in the value-vector are orderid and quantity.
*/
std::map<double, std::vector<std::string>,std::greater<double>> item1_buy;
std::map<double, std::vector<std::string>> item1_sell;
std::map<double, std::vector<std::string>,std::greater<double>> item2_buy;
std::map<double, std::vector<std::string>> item2_sell;


/*
*Function to execute an order which want to buy something. It works as follow:
*Compare the price of the buy order and the price of orders in the sell-order-map;
*Using iterator to traverse through the sell-order-map from begining to end;
*Repeat the comparison until the buy price is lower than the sell price or the buy quantity reduce to 0.
*Return a vector containing FIX message to send back to the trading client and the monitoring client.
*/
std::vector<std::string> execute_buy_trade(std::string orderid,double price, int quantity, std::map<double, std::vector<std::string>>& map1, std::map<double, std::vector<std::string>,std::greater<double>>& map2, std::vector<std::string>& value_vec) {
	std::vector<std::string> fix_message_vec;
	std::map<double, std::vector<std::string>>::iterator iter = map1.begin();
	int left_quantity = quantity;

	if (iter!=map1.end() && price > iter->first) {
		while (iter != map1.end() && price > iter->first && left_quantity > 0) {

			int quantity_exist = std::atoi(iter->second[1].c_str());
			left_quantity = quantity - quantity_exist;

			if (left_quantity < 0) {
				iter->second[1] = std::to_string(quantity_exist - quantity);
				std::string fullfill = "35=8;150=2;39=2;11=" + orderid + ";31=" + std::to_string(iter->first) + ";";				
				fix_message_vec.push_back(fullfill);
				std::string partialfill = "35=8;150=1;39=1;11=" + iter->second[0] + ";31=" + std::to_string(iter->first) + ";151=" + iter->second[1];
				fix_message_vec.push_back(partialfill);
				iter = map1.end();
			}
			else if (left_quantity > 0) {
				std::string fullfill = "35=8;150=2;39=2;11=" + iter->second[0] + ";31=" + std::to_string(iter->first) + ";";
				fix_message_vec.push_back(fullfill);
				std::string partialfill = "35=8;150=1;39=1;11=" + orderid + ";31=" + std::to_string(iter->first) + ";151=" + std::to_string(left_quantity);
				fix_message_vec.push_back(partialfill);
				map1.erase(iter);
				iter = map1.begin();
			}
			else {
				std::string fullfill = "35=8;150=2;39=2;11=" + orderid + ";31=" + std::to_string(iter->first) + ";";
				fix_message_vec.push_back(fullfill);
				std::string fullfill2 = "35=8;150=2;39=2;11=" + iter->second[0] +";31=" + std::to_string(iter->first) + ";";
				fix_message_vec.push_back(fullfill2);
				map1.erase(iter);
				iter = map1.end();
			}
		}

		if (left_quantity > 0) {
			value_vec[1] = std::to_string(left_quantity);
			map2[price] = value_vec;
		}
	}
	else map2[price] = value_vec;
	return fix_message_vec;
}


/*
*Function to execute an order which want to sell something.
*The Function is similar to the function above.
*/
std::vector<std::string> execute_sell_trade(std::string orderid, double price, int quantity, std::map<double, std::vector<std::string>,std::greater<double>>& map1, std::map<double, std::vector<std::string>>& map2, std::vector<std::string>& value_vec) {
	std::vector<std::string> fix_message_vec;
	std::map<double, std::vector<std::string>>::iterator iter = map1.begin();
	int left_quantity = quantity;

	if (iter!=map1.end() && price < iter->first) {
		while (iter != map1.end() && price < iter->first && left_quantity > 0) {

			int quantity_exist = std::atoi(iter->second[1].c_str());
			left_quantity -= quantity_exist;

			if (left_quantity < 0) {
				iter->second[1] = std::to_string(quantity_exist - quantity);
				std::string fullfill = "35=8;150=2;39=2;11=" + orderid + ";31=" + std::to_string(iter->first) + ";";
				fix_message_vec.push_back(fullfill);
				std::string partialfill = "35=8;150=1;39=1;11=" + iter->second[0] + ";31=" + std::to_string(iter->first) + ";151=" + iter->second[1];
				fix_message_vec.push_back(partialfill);
				iter = map1.end();
			}
			else if (left_quantity > 0) {
				std::string fullfill = "35=8;150=2;39=2;11=" + iter->second[0] + ";31=" + std::to_string(iter->first) + ";";
				fix_message_vec.push_back(fullfill);
				std::string partialfill = "35=8;150=1;39=1;11=" + orderid + ";31=" + std::to_string(iter->first) + ";151=" + std::to_string(left_quantity);
				fix_message_vec.push_back(partialfill);
				map1.erase(iter);
				iter = map1.begin();
			}
			else {
				std::string fullfill = "35=8;150=2;39=2;11=" + orderid + ";31=" + std::to_string(iter->first) + ";";
				fix_message_vec.push_back(fullfill);
				std::string fullfill2 = "35=8;150=2;39=2;11=" + iter->second[0] +";31=" + std::to_string(iter->first) + ";";
				fix_message_vec.push_back(fullfill2);
				map1.erase(iter);
				iter = map1.end();
			}
		}

		if (left_quantity > 0) {
			value_vec[1] = std::to_string(left_quantity);
			map2[price] = value_vec;
		}
	}
	else map2[price] = value_vec;
	return fix_message_vec;
}


/*
*Function to execute a cancel request. It works as follow:
*Find the id of the order to be canceled. Check through the four maps which stores trading infortion using iterator.
*If find the id in the maps, execute the cancel. Else reject the cancel request.
*/
std::string execute_cancel(std::string accepted_message) {
	int orderid_from = accepted_message.find("11=");
	int orderid_to = accepted_message.substr(orderid_from).find(";");
	std::string orderid = accepted_message.substr(orderid_from + 3, orderid_to - 3);

	int cancel_orderid_from = accepted_message.find("41=");
	std::string cancel_orderid = accepted_message.substr(cancel_orderid_from + 3);

	std::map<double, std::vector<std::string>>::iterator iter;
	iter = item1_sell.begin();
	while (iter != item1_sell.end()) {
		if (iter->second[0] == cancel_orderid) {
			std::string cancel_accept = "35=8;150=4;39=4;11=" + orderid + ";151=" + iter->second[1];
			item1_sell.erase(iter);
			return cancel_accept;
		}
		else iter++;
	}
	iter = item2_sell.begin();
	while (iter != item2_sell.end()) {
		if (iter->second[0] == cancel_orderid) {
			std::string cancel_accept = "35=8;150=4;39=4;11=" + orderid + ";151=" + iter->second[1];
			item2_sell.erase(iter);
			return cancel_accept;
		}
		else iter++;
	}
	iter = item1_buy.begin();
	while (iter != item1_buy.end()) {
		if (iter->second[0] == cancel_orderid) {
			std::string cancel_accept = "35=8;150=4;39=4;11=" + orderid + ";151=" + iter->second[1];
			item1_buy.erase(iter);
			return cancel_accept;
		}
		else iter++;
	}
	iter = item2_buy.begin();
	while (iter != item2_buy.end()) {
		if (iter->second[0] == cancel_orderid) {
			std::string cancel_accept = "35=8;150=4;39=4;11=" + orderid + ";151=" + iter->second[1];
			item2_buy.erase(iter);
			return cancel_accept;
		}
		else iter++;
	}
	std::string cancel_reject = "35=9;39=8;11=" + orderid;
		return cancel_reject;
}


/*
*Main function.
*/
int main()
{
	boost::asio::io_service io_service_trade;
	tcp::acceptor acc_trade(io_service_trade, tcp::endpoint(tcp::v6(), 9876));
	boost::asio::io_service io_service_monitor;
	tcp::acceptor acc_monitor(io_service_monitor, tcp::endpoint(tcp::v6(), 1234));

	while (1) {
		boost::system::error_code ignored;

		//the socket for the trading client.
		tcp::socket socket_trade(io_service_trade);
		acc_trade.accept(socket_trade);

		//the socket for the monitoring client.
		tcp::socket socket_monitor(io_service_monitor);
		acc_monitor.accept(socket_monitor);

		//analyze the messages sent from the trading client and execute them.
		while (1) {
			std::array<char, 256> input_buffer;
			std::size_t input_size = socket_trade.read_some(boost::asio::buffer(input_buffer),ignored);
			std::string accepted_message(input_buffer.data(),input_buffer.data() + input_size);

			if (accepted_message.find("QUIT") != -1) return 0;

			//"35=D" stands for new order.
			if (accepted_message.find("35=D") != -1) {

				int orderid_from = accepted_message.find("11=");
				int orderid_to = accepted_message.substr(orderid_from).find(";");
				std::string orderid = accepted_message.substr(orderid_from + 3, orderid_to - 3);

				std::string new_Ack = "35=8;150=0;39=0;11=" + orderid;
				boost::asio::write(socket_trade, boost::asio::buffer(new_Ack), ignored);
				boost::asio::write(socket_monitor, boost::asio::buffer(new_Ack), ignored);

				//analyze the information and get the orderid, price, quantity(stored as string to put into the vector)			
				int price_from = accepted_message.find("44=");
				int price_to = accepted_message.substr(price_from).find(";");
				std::stringstream ss(accepted_message.substr(price_from + 3, price_to - 3));
				double price;
				ss >> price;
				int quantity_from = accepted_message.find("38=");
				int quantity_to = accepted_message.substr(quantity_from).find(";");
				std::string quantitystr = accepted_message.substr(quantity_from + 3, quantity_to - 3);
				std::vector<std::string> value_vec;
				int quantity = std::atoi(quantitystr.c_str());
				value_vec.push_back(orderid);
				value_vec.push_back(quantitystr);

				//the case of buying item1.
				if ((accepted_message.find("55=item1") != -1) && (accepted_message.find("54=1") != -1)) {
					std::vector<std::string> message_back_vec = execute_buy_trade(orderid, price, quantity, item1_sell, item1_buy, value_vec);
					
					for (int i = 0; i < message_back_vec.size(); i++) {
						boost::asio::write(socket_trade, boost::asio::buffer(message_back_vec[i]), ignored);
						boost::asio::write(socket_monitor, boost::asio::buffer(message_back_vec[i]), ignored);
						Sleep(100);
					}
					boost::asio::write(socket_trade, boost::asio::buffer("Done"), ignored);
				}
				//the case of selling item1.
				else if ((accepted_message.find("55=item1") != -1) && (accepted_message.find("54=2") != -1)) {
					std::vector<std::string> message_back_vec = execute_sell_trade(orderid, price, quantity, item1_buy, item1_sell, value_vec);
					for (int i = 0; i < message_back_vec.size(); i++) {
						boost::asio::write(socket_trade, boost::asio::buffer(message_back_vec[i]), ignored);
						boost::asio::write(socket_monitor, boost::asio::buffer(message_back_vec[i]), ignored);
						Sleep(100);
					}
					boost::asio::write(socket_trade, boost::asio::buffer("Done"), ignored);
				}
				//the case of buying item2.
				else if ((accepted_message.find("55=item2") != -1) && (accepted_message.find("54=1") != -1)) {
					std::vector<std::string> message_back_vec = execute_buy_trade(orderid, price, quantity, item2_sell, item2_buy, value_vec);
					for (int i = 0; i < message_back_vec.size(); i++) {
						boost::asio::write(socket_trade, boost::asio::buffer(message_back_vec[i]), ignored);
						boost::asio::write(socket_monitor, boost::asio::buffer(message_back_vec[i]), ignored);
						Sleep(100);
					}
					boost::asio::write(socket_trade, boost::asio::buffer("Done"), ignored);
				}
				//the case of selling item2.
				else if ((accepted_message.find("55=item2") != -1) && (accepted_message.find("54=2") != -1)) {
					std::vector<std::string> message_back_vec = execute_sell_trade(orderid, price, quantity, item2_buy, item2_sell, value_vec);
					for (int i = 0; i < message_back_vec.size(); i++) {
						boost::asio::write(socket_trade, boost::asio::buffer(message_back_vec[i]), ignored);
						boost::asio::write(socket_monitor, boost::asio::buffer(message_back_vec[i]), ignored);
						Sleep(100);
					}
					boost::asio::write(socket_trade, boost::asio::buffer("Done"), ignored);
				}
			}
			//the case of cancelling
			else {
				std::string message_back = execute_cancel(accepted_message);
				boost::asio::write(socket_trade, boost::asio::buffer(message_back), ignored);
				boost::asio::write(socket_monitor, boost::asio::buffer(message_back), ignored);
				Sleep(100);
				boost::asio::write(socket_trade, boost::asio::buffer("Done"), ignored);
			}
		}
	}
	return 0;
}
	
