# Bitcoin CPU miner
## Why?

I like Bitcoin mining, it's a completely new thing, with it's own peculiarities and nuances, and the best way of learning something is dig down and write some code. While CPU mining is no longer profitable, it's not forbidden, one can "mine" with his own CPU and have fun with it. When creating this code, I had two things in mind:  
1. Learning
2. Creating a relatively simple codebase for future studies, after all, CPU mining is the oldest way of mining Bitcoin. Bitcoin's Archeology is very fun!  

## Why C?

Most of the old codes were write in C/C++, and you can make it with a few lines of Python code, it's so easy (and boring)!. Also, C has a unbeatable speed, because it's very close to the hardware itself. I'm working in a hardware implementation too, but this C version works great.

## How it works

In [blockproducer.c](/block.c) you find most of the magic, it creates a block, serialize it, and call miner from [miner.h](/miner.h). This codebase needs a full synced Bitcoin core, we relay on `getblocktemplate` to create a block, the only thing we do is creating a coinbase transaction. Apart from that, this code runs with no supervision (I hope).  

### Compiling
I use my own SHA-2 implementation, so no OpenSSL is required. However, I do use one lib for JSON (libjson-c) parsing and another for HTTP requesting (libcurl). On Debian, you can install then with:
```bash
sudo apt-get install libjson-c libcurl
```
Clone this repo
```bash
git clone www.github.com/Davidson-Souza/CPUMining.git

```
And use `make` to build the project
```bash
cd CPUMining
make
```
### Running
```bash
./CPUMiner
```
By default this codebase will try mining on testnet, with the minimum difficulty. If you want to change it, open [blockproducer](/blockproducer.c) and change the port and cookie dir to something else. Regtest uses a port 18443 and cookie's located at `BITCOIN_DIR/regtest/.cookie`.  
You also may change the diff, it's located inside [miner.h](/miner.h) and is called TARGET, just add a few 'f's at the end. For regtest it can be `0x7fffffff`, but will create blocks too quickly, `0x000fffff` works fine for me.