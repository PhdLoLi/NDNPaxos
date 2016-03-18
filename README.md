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

- Terminal 1 -- Node1 
<pre>
$ bin/servant 1 2
parameters (Node_ID Node_Num)
</pre>

- Terminal 0 -- Node0(Master) 
<pre>
$ bin/master 0 2 1 1 0
parameters (Node_ID Node_Num Value_Size Window_Size Local_orNot(0/1))
</pre>
When you are running locally, set the last parameter as 0
Now master and servant using timer to end itself, so please start them at the SAME time.

License
---
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version, with the additional exemption that compiling, linking, and/or using OpenSSL is allowed.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
