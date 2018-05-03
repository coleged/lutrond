
Rewrite of lutrond using threads and C++ (or rather C with a bit of C++ [compiles as C++ anyway])

STATUS:

Stable code.

I've now rolled the production instance to using this version.

the project lutrond should be considered deprecated by this (tlutrond)

May 2018

TODO: retire and delete the lutrond project and write a README for this.

Lutrond – April 2018 – working but still in development.          colege@gmail.com

Lutrond is a simple daemon to control and monitor a Lutron Homeworks QS lighting
installation. While this Lutron product is essentially targeted at commercial
applications, mine is a residential one, which is reflected in the sparse feature
functionality of this project to date.

It should be noted that the code base has evolved, rather than been developed,
which should be evident in the coding (or lack of) style.

It has been put together to schedule lighting scene setting in conjunction with
cron as a replacement to the Lutron supplied Lutron Connect App. I also use the
sunrise utility to command Lutron Scene setting schedules around sunset and sunrise
times. See my fork of the SunriseSunsetControl project

Written in C/C++, it requires libconfig.

Developed on Ubuntu, it also runs of MacOS, but little to no attempt at portability has been made.

The software in this repository is working code. Simply download to your chosen directory
and

Edit build.h and lutron.h to fit your environment and Lutron System parameters, some of which can remain at default

$ make

Edit lutrond.conf and lutrond.conf.inc

$ make install

This will install         

executables in /usr/local/bin
configuration files in /usr/local/etc

And start the daemon

$ sudo lutrond -d

A logfile is maintained in /tmp, as is a pid file and a database dump file (see below).

Use lutctrl to start turning lights on and off! See the Lutron API for description of
commands for this.

The service directory contains an init script for involking the daemon this when the
server boots. Put this in /etc/init.d and initiate the service with

$ sudo update-rc.d lutrond defaults

Operational Overview

lutrond runs under root priv. it forks a telnet session with the Lutron system and opens
a listening port for commands from a connectionless client. It reads a config file to
override certain default parameters which can be further over ridden with command line
options when invoking the daemon. All Lutron Integration API commands received on the
listening socket are sent to the Lutron and also logged into the logfile, with annotations
discovered from the conf file. Responses from the Lutron are parsed, logged and a simple state
database maintained in memory.

It is possible to envoke shell scripts in response to lutron events such as button presses etc. These
are configured via the lutron.conf.inc file attached to components of Lutron devices.

lutctrl is an example connectionless client provided. This takes commands from a file,
standard input or the command line and sends them to lutrond for processing as described
above. See the Lutron API document for description of command syntax.


lutrond traps three signals.

SIGCHLD – if the telnet session dies, this signal is caught by lutrond (the parent process)
which then re-establishes the session.

SIGHUP – sending a HUP to lutrond causes it to kill the child (telnet) which in turn raises
a SIGCHLD and the session is therefore re-established per the above mechanism. Sending a HUP
to lutrond (which can be done using lutrond -k) also causes it to dump the in memory database
containing it’s view of the state of the Lutron attached devices. The file created (in /tmp) is timestamped
so successive dumps do not overwrite, and is formatted in the same format as that used in the
lutrond.conf.inc file. The thinking behind this is that this process can assist in the
compilation of a comprehensive config.

SIGTERM - this is trapped to ensure the telnet is killed before lutrond exits to prevent zombies.




