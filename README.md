mod_go
======

Apache module for deploying web applications in Go

## Installation

To install this module compile it and install it into Apache's modules directory by running:

    $ make
    $ sudo make install

Then activate it in Apache's httpd.conf file as follows:

    LoadModule go_module modules/mod_go.so
    AddHandler golang .go

Then restart Apache via

    $ apachectl restart
