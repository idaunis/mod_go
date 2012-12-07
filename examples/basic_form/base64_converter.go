/*
**  Copyright (C) 2012 Use Labs, LLC and/or its subsidiary(-ies).
**
**  Licensed under the Apache License, Version 2.0 (the "License");
**  you may not use this file except in compliance with the License.
**  You may obtain a copy of the License at
**
**  http://www.apache.org/licenses/LICENSE-2.0
**
**  Unless required by applicable law or agreed to in writing, software
**  distributed under the License is distributed on an "AS IS" BASIS,
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**  See the License for the specific language governing permissions and
**  limitations under the License.
*/

package main

import "os"
import "io"
import "fmt"
import "bytes"
import "net/http"
import "net/url"
import "io/ioutil"

import "encoding/base64"

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

func Base64Encode(body string) string {
	raw := make([]byte, base64.StdEncoding.EncodedLen(len(body)))
	base64.StdEncoding.Encode(raw, []byte(body))
	return string(raw)
}

func Base64Decode(body string) string {
	res,_ := base64.StdEncoding.DecodeString(body)
	return string(res)
}

func main() {
	var out, enc, dec string;
	var req http.Request = ModGoRequest()

	if( req.FormValue("op") == "Encode" ) {
			enc = Base64Encode(req.FormValue("decoded"))
			dec = req.FormValue("decoded")
	}
	if( req.FormValue("op") == "Decode" ) {
			dec = Base64Decode(req.FormValue("encoded"))
			enc = req.FormValue("encoded")
	}

	out =
`<!DOCTYPE html>
<html>
<head>
	<title>Base64 Encoder/Decoder - Apache MOD_GO Example</title>
</head>
<body>
    <form method="post" accept-charset="UTF-8" enctype="multipart/form-data">
        <div style="float:left; margin-right:15px">
        	<label for="decoded" style="display:block">Decoded string:</label>
        	<textarea name="decoded" id="decoded" rows="10" cols="40">`+dec+`</textarea>
        </div>
		<div style="float:left">
        	<label for="encoded" style="display:block">Encoded string:</label>
        	<textarea name="encoded" id="encoded" rows="10" cols="40">`+enc+`</textarea>
        </div>
        <br style="clear:both;" />
		<input type="submit" name="op" value="Encode" />
		<input type="submit" name="op" value="Decode" />
    </form>
</body>
</html>`

    fmt.Print("Content-Type: text/html; charset=utf-8\r\n");
    fmt.Print("\r\n");
    fmt.Print( out );
}
