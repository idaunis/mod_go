MOD_GO
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

## Example

The following example takes the content of the URL variable "name" and writes the heading "Hello [name]!" to your
browser.

    package main

    import (
    	"io"
    	"os"
    	"fmt"
    	"bytes"
    	"net/http"
    	"net/url"
    	"io/ioutil" )

    type voidCloser struct {
    	io.Reader
    }
    func (voidCloser) Close() error { return nil }

    func ModGoRequest() (r http.Request) {
    	post, _ := ioutil.ReadAll(os.Stdin)
    	r.URL, _ = url.ParseRequestURI( "http://"+os.Getenv("HTTP_HOST")+"?"+os.Getenv("QUERY_STRING") )
    	r.Method = os.Getenv("REQUEST_METHOD");
    	r.Header = map[string][]string{
    		"Accept-Encoding": {os.Getenv("HTTP_ACCEPT_ENCODING")},
    		"Accept-Language": {os.Getenv("HTTP_ACCEPT_LANGUAGE")},
    		"Connection": {os.Getenv("HTTP_CONNECTION")},
    		"Content-Type": {os.Getenv("CONTENT_TYPE")},
    		"Content-Length": {os.Getenv("CONTENT_LENGTH")} }
    	r.Body = voidCloser{bytes.NewBuffer( post ) }
    	return r
    }

    func main() {
    	var req http.Request = ModGoRequest()

    	fmt.Print("Content-Type: text/html; charset=utf-8\r\n");
    	fmt.Print("\r\n");
    	fmt.Print("<h1>Hello "+req.FormValue("name")+"!</h1>");
    }

Note: To let Go process your GET and POST values. You'll simply need to initialize an http.Request in this way:

    type voidCloser struct { io.Reader }
    func (voidCloser) Close() error { return nil }
    func ModGoRequest() (r http.Request) {
    	post, _ := ioutil.ReadAll(os.Stdin)
    	r.URL, _ = url.ParseRequestURI( "http://"+os.Getenv("HTTP_HOST")+"?"+os.Getenv("QUERY_STRING") )
    	r.Method = os.Getenv("REQUEST_METHOD");
    	r.Header = map[string][]string{
    		"Accept-Encoding": {os.Getenv("HTTP_ACCEPT_ENCODING")},
    		"Accept-Language": {os.Getenv("HTTP_ACCEPT_LANGUAGE")},
    		"Connection": {os.Getenv("HTTP_CONNECTION")},
    		"Content-Type": {os.Getenv("CONTENT_TYPE")},
    		"Content-Length": {os.Getenv("CONTENT_LENGTH")} }
    	r.Body = voidCloser{bytes.NewBuffer( post ) }
    	return r
    }

    request := ModGoRequest();

## License

MOD_GO is available open-source under the terms of the Apache 2 License.