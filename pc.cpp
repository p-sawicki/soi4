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
				int produceAmount = uniformDistribution(false);
				prodLogs[id].open(logFileName, std::ios_base::app);
				prodLogs[id] << "Produced " << produceAmount << " items.\n";
				while(maxInBuffer - buffer < produceAmount){
					prodLogs[id] << "Can't insert due to lack of space.\n";
					wait(full);
				}
				buffer += produceAmount;
				writeBuffer(buffer);
				prodLogs[id] << "Inserted " << produceAmount << " items.\n\n";
				prodLogs[id].close();
				signal(empty);
				leave();
				std::this_thread::sleep_for(2s);
			}
		}
		void consume(unsigned int id){
			std::string logFileName("consumer");
			logFileName += std::to_string(id) += ".txt";
			while(true){
				enter();
				int consumeAmount = uniformDistribution(true);
				consLogs[id].open(logFileName, std::ios_base::app);
				consLogs[id] << "Wants to take " << consumeAmount << " items.\n";
				while(buffer < consumeAmount){
					consLogs[id] << "Can't take " << consumeAmount << " due to lack of items.\n";
					wait(empty);
				}
				buffer -= consumeAmount;
				writeBuffer(buffer);
				consLogs[id] << "Took " << consumeAmount << " items.\n\n";
				consLogs[id].close();
				signal(full);
				leave();
				std::this_thread::sleep_for(2s);
			}
		}
	private:
		unsigned int buffer;
		Condition full, empty;
};
Mon m;
int main(int argc, char **argv){
	if(argc < 8)
		return 1;
	std::stringstream ss;
	for(int i = 1; i < 8; ++i)
		ss << argv[i] << " ";
	ss >> maxInBuffer >> consAmount >> prodAmount >> a >> b >> c >> d;
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
	if(!consumers.empty())
		consumers.rbegin()->join();
	if(!producers.empty())
		producers.rbegin()->join();
}	
