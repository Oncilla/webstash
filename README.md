# webstash
webstash is a simple command line tool to quickly share files between computers.

webstash allows to quickly upload files from the current working directory to a server, as well as downloading files from the server.

To achieve this, the server has to be running webstash-server, which can be started from anywhere.

The client calls webstash from the current working directory.

webstash [OPTION] [FILE]...

Following options are available for webstash:

-u [--upload]: upload files

-d [--download]: download files

-s [--show]: show files

-r [--remove]: remove files

-c [--config]: configure the server with parameters
	[no parameter] print current configuration
	-i set ip
	-p set port
	-k set key
	usage: webstash -c -i 127.0.0.1 
	        or
	       webstash -c -p 5002 -i 127.0.0.1

Following options are available for webstash-server:

-c [--config]: configure the server with parameters
	[no parameter] print current configuration
	-i set ip
	-p set port
	-k set key
	usage: webstash -c -i 127.0.0.1 
	        or
	       webstash -c -p 5002 -i 127.0.0.1


Attention: Currently the communication is not encrypted and there is no access control. But only files from $HOME/.webstash/stash can be read.

For easy install run 'sh install.sh'.
For easy uninstall run 'sh uninstall.sh'.

Binaries can also be manually compiled (std11 or newer) and placed as wished.
