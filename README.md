NDNPaxos
----

NDNPaxos is an implementation combining Paxos and NDN. It also inculdes some simple testing of functionality and performance.  

For license information see LICENSE.

Structure
----

The directory structure is as follows:

* **/root**
    * **libndnpaxos/** *-- NDNPaxos library source code*
    * **waf** *-- binary waf file*
    * **wscript** *-- waf instruction file*
    * **config/** *-- config files of this project* 
    * **waf-tools/** *-- additional waf tools*
    * **test/** *-- test code*
    * **examples/** *-- some simple example codes*
    * **libzfec/** *-- zefc library using for RS code*
    * **script/** *-- some python scripts*
    * **LICENSE**
    * **README.md**

Building instructions
----
Prerequisites
==
These are prerequisites to build NDNPaxos.

**Required:**
* [clang](http://clang.llvm.org/)
* [boost](http://www.boost.org/)
* [protobuf](https://developers.google.com/protocol-buffers/)
* [yaml-cpp](http://yaml.org/)
* [zeromq](http://zeromq.org/)
* [NFD](http://named-data.net/doc/NFD/current/INSTALL.html)
* [ndn-cxx](http://named-data.net/doc/ndn-cxx/current/INSTALL.html)

Prerequisites build instructions
==

Mac OS build considerations 
-

boost
--
brew install boost

protobuf
--
brew install protobuf

zeromq
--
brew install zeromq

yaml-cpp
--
brew install libyaml

NFD
--
sudo port install nfd

ndn-cxx
--
Install from git https://github.com/named-data/ndn-cxx
Follow http://named-data.net/doc/ndn-cxx/current/INSTALL.html


Linux(Ubuntu) build considerations 
-

clang
--
sudo apt-get install clang 

boost
--
sudo apt-get install libboost-all-dev

protobuf
--
sudo apt-get install libprotobuf-dev protobuf-compiler python-protobuf 

zeromq
--
sudo apt-get install libzmq3-dev python-zmq

yaml-cpp
--
sudo apt-get install libyaml-cpp-dev 

NFD
--
Follow http://named-data.net/doc/NFD/current/INSTALL.html
using apt-get install 


ndn-cxx
--
Install from git https://github.com/named-data/ndn-cxx
Follow http://named-data.net/doc/ndn-cxx/current/INSTALL.html


Build instructions
==
<pre>
$ ./waf configure -l info
$ ./waf
</pre>


Run Test
--
Background need run 
<pre>
$ nfd-start
</pre>

If we have three nodes, we should run 3 naxos daemon processes or run them in different nodes: 

- Terminal 0  -- Node0 (Runing background / Blocking)
<pre>
$ bin/naxos 0 3 1 0 1
parameters (Node_ID Node_Num Wind_Size Local_or_Not(0/1) Log_Win_Size)
</pre>

- Terminal 1  -- Node1 (Runing background / Blocking)
<pre>
$ bin/naxos 1 3 1 0 1
parameters (Node_ID Node_Num Win_Size Local_or_Not(0/1) Log_Win_Size)
</pre>

- Terminal 2  -- Node2 (Runing background / Blocking)
<pre>
$ bin/naxos 2 3 1 0 1
parameters (Node_ID Node_Num Win_Size Local_or_Not(0/1) Log_Win_Size)
</pre>

- Terminal 3  -- Clients (Runing for 20 mins)
<pre>
$ bin/clients 1 100
parameters (Commit_Win_Size Write_Ratio)
</pre> 

License
---
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version, with the additional exemption that compiling, linking, and/or using OpenSSL is allowed.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
