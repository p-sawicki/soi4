#include <string>
#include <iostream>
#include <thread>
#include "monitor.h"
#include <sstream>
#include <cstdlib>
#include <random>
#include <vector>
#include <fstream>
#include <chrono>
using namespace std::chrono_literals;
unsigned int maxInBuffer;
std::random_device rd;
static std::mt19937 gen(rd());
unsigned int a, b, c, d;
unsigned prodAmount, consAmount;
std::fstream bufferFile;
std::fstream *prodLogs;
std::fstream *consLogs;
inline int uniformDistribution(bool cons){
	static std::uniform_int_distribution<> disCons(a, b);
	static std::uniform_int_distribution<> disProd(c, d);
	return cons ? disCons(gen) : disProd(gen);
}
inline unsigned int sleepTime(){
	unsigned int mean = (prodAmount + consAmount) / 2;
	return mean ? mean : 1;
}
void writeBuffer(unsigned int amount){
	bufferFile.open("buffer.txt", std::ios_base::out);
	bufferFile << amount;
	bufferFile.close();
}
class Mon : public Monitor{
	public:
		Mon() : buffer(0) {}
		void produce(unsigned int id){
			std::string logFileName("producer");
			logFileName += std::to_string(id) += ".txt";
			while(true){
				enter();
				std::this_thread::sleep_for(1s);
				int produceAmount = uniformDistribution(false);
				auto timeStamp = std::chrono::system_clock::now();
				prodLogs[id].open(logFileName, std::ios_base::app);
				prodLogs[id] << "[" << std::chrono::system_clock::to_time_t(timeStamp) << "] " << "Produced " << produceAmount << " items.\n";
				prodLogs[id].close();
				while(maxInBuffer - buffer < produceAmount){
					if(buffer <= maxInBuffer / 2)
						signal(empty);
					timeStamp = std::chrono::system_clock::now();
					prodLogs[id].open(logFileName, std::ios_base::app);
					prodLogs[id] << "[" << std::chrono::system_clock::to_time_t(timeStamp) << "] " << "Can't insert due to lack of space.\n";
					prodLogs[id].close();
					wait(full);
					std::this_thread::sleep_for(1s);
				}
				buffer += produceAmount;
				writeBuffer(buffer);
				timeStamp = std::chrono::system_clock::now();
				prodLogs[id].open(logFileName, std::ios_base::app);
				prodLogs[id] << "[" << std::chrono::system_clock::to_time_t(timeStamp) << "] " << "Inserted " << produceAmount << " items.\n\n";
				prodLogs[id].close();
				if(buffer > maxInBuffer / 2)
					signal(empty);
				leave();
			}
		}
		void consume(unsigned int id){
			std::string logFileName("consumer");
			logFileName += std::to_string(id) += ".txt";
			while(true){
				enter();
				std::this_thread::sleep_for(1s);
				int consumeAmount = uniformDistribution(true);
				auto timeStamp = std::chrono::system_clock::now();
				consLogs[id].open(logFileName, std::ios_base::app);
				consLogs[id] << "[" << std::chrono::system_clock::to_time_t(timeStamp) << "] " << "Wants to take " << consumeAmount << " items.\n";
				consLogs[id].close();
				while(buffer < consumeAmount){
					if(buffer >= maxInBuffer / 2)
						signal(full);
					timeStamp = std::chrono::system_clock::now();
					consLogs[id].open(logFileName, std::ios_base::app);
					consLogs[id] << "[" << std::chrono::system_clock::to_time_t(timeStamp) << "] " << "Can't take " << consumeAmount << " due to lack of items.\n";
					consLogs[id].close();
					wait(empty);
					std::this_thread::sleep_for(1s);
				}
				buffer -= consumeAmount;
				writeBuffer(buffer);
				timeStamp = std::chrono::system_clock::now();
				consLogs[id].open(logFileName, std::ios_base::app);
				consLogs[id] << "[" << std::chrono::system_clock::to_time_t(timeStamp) << "] " << "Took " << consumeAmount << " items.\n\n";
				consLogs[id].close();
				if(buffer < maxInBuffer / 2)
					signal(full);
				leave();
			}
		}
	private:
		unsigned int buffer;
		Condition full, empty;
};
Mon m;
int main(int argc, char **argv){
	if(argc < 8){
		std::cout << "Not enough arguments.\n";
		return 1;
	}
	std::stringstream ss;
	for(int i = 1; i < 8; ++i)
		ss << argv[i] << " ";
	ss >> maxInBuffer >> consAmount >> prodAmount >> a >> b >> c >> d;
	if(prodAmount < 1){
		std::cout << "There needs to be at least 1 producer.\n";
		return 2;
	}
	std::vector<std::thread> producers;
	producers.reserve(prodAmount);
	std::vector<std::thread> consumers;
	consumers.reserve(consAmount);
	prodLogs = new std::fstream[prodAmount];
	consLogs = new std::fstream[consAmount];
	unsigned int amount = prodAmount > consAmount ? prodAmount : consAmount;
	for(unsigned int i = 0; i < amount; ++i){
		if(i < prodAmount)
			producers.emplace_back(std::thread(&Mon::produce, &m, i));
		if(i < consAmount)
			consumers.emplace_back(std::thread(&Mon::consume, &m, i));
	}
	producers.rbegin()->join();
}	
