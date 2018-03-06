# HttpClient-cpp

This is a simple HTTP Client, written to be robust and efficient.
Given the url and a file path, the Client will query the server
through ipv4 and ipv6, and based on headers will either spin a
simple thread pool to download chunks, or decide to download the file in one request.

The Client also applies a md5 hash to the downloaded file, to verify its integrity.
If the hash doesn't match, rather than crashing, the Client notifies the user and quits.

## Usage && Requirements

- Clang++-5.0
- BOOST 1.58
- make
- md5sum
- openssl

The application provides testing scripts to build and execute the application.
The `run*.sh` family scripts build the application, and execute it, hardocoded to
a specific file at a specific url, yet allows for variable amount of threads to be spun.

The `execute*.sh` family scripts only execute the application, assuming that it's been
built prior. These are intended for use in the `benchmark.sh` script, which records the
time it takes wget to download the hardcoded file, then how long it would take the application
to download it with thread pool sizes varying from [1, 16]. The file is run with three passes,
so that an average can be determined for each amounts of threads. There also exists a `graph.py`
script that takes the results from `benchmark.sh` and plots them.

For general use, do the following:
```bash
cd src/
make
./Main <url-filepath> <num-threads> <force-simple>
// <force-simple> must be a 1 or 0. This is to make the client use the 'simple' download
// method, which means only using 1 thread.
```

For building from Dockerfile:
```bash
sudo docker build -t ubuntu_clang:latest .
sudo docker run -t -i ubuntu_clang
```

## Results

![Figure](Figure_1.png)

The value at n = 0 is for the wget utility, the value at n = 1 is for the simple download
implementation, and the rest are for various thread pool sizes.

Wget is beaten by this application when using thread pool sizes of [2, 13]. Interestingly enough,
the thread pool sizes that was the quickest with the hardcoded file was n = 9, and after n = 13,
the time to download drastically shot up.

Further benchmarks can expand on the buffer-size (in this implementation it was 4096 Bytes).
