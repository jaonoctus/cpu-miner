all: CPUMiner

CPUMiner: 
	gcc sha2.c miner.c blockproducer.c -ljson-c -lcurl -pthread -o CPUMiner