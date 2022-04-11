# Bitcoin CPU miner
## Why?

I like Bitcoin mining, it's a completely new thing, with it's own peculiarities and nuances, and the best way of learning something is dig down and write some code. While CPU mining is no longer profitable, it's not forbidden, one can "mine" with his own CPU and have fun with it. When creating this code, I had two things in mind:  
1. Learning
2. Creating a relatively simple codebase for future studies, after all, CPU mining is the oldest way of mining Bitcoin. I'm not sure if this was supplied, it started as ~200 lines of C99, but bloated to a multi-file and with thousands of code lines. But it's still understandable (TM).  

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
./CPUMiner -datadir <dir> -spk <spk>
```

By default this codebase will try mining on testnet, with the minimum difficulty. You can mine with the testnet's actual difficulty you can use `-actualdiff`. You need a fully synchronized `bitcoind` with `server=1`. Prune is fine. You can use username/password with `-rpcuser=<username>` and `-rpcpassword=<password>` or cookie with `-datadir=<dir>` or `-rpccookie=<cookie>`. You should specify a payout address, right now you should pass a spk rather than an address (laziness). You can find a `spk` with `bitcoin-cli getaddressinfo <address>`, and specify with `-spk=<spk>`. A coinbase value may also be specified with `-coinbasevalue=<data>`.  

## Docker

### Build the image

```bash
docker build -t <YOUR-HANDLE>/cpu-miner .
```

### Running the container

```bash
docker run --rm -it --name miner jaonoctus/cpu-miner ./CPUMiner -spk 512024a8446601b3caf2776c540f1758b98ecac292bb3a288183ac7b7956f66175e5 -network testnet -rpcuser user -rpcpassword pass -rpchost host.docker.internal -coinbasevalue "@jaonoctus"
```
