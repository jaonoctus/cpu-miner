all: CPUMiner

CPUMiner: 
	gcc primitives/transaction.c rpc.c sha2.c miner.c blockproducer.c CPUMiner.c -ljson-c -lcurl -pthread -o CPUMiner