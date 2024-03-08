package main

import (
	"fmt"
	"log"
	"net"
)

const response = `HTTP/1.1 200 OK
Content-Type: text/html; charset=utf-8
Content-Length: 13

Hello, World!
`

func worker(conn net.Conn) {
	defer conn.Close()
	b := make([]byte, 128)
	for {
		_, err := conn.Read(b)
		if err != nil {
			break
		}
		_, err = conn.Write([]byte(response))
		if err != nil {
			break
		}
	}
}

func main() {
	listner, err := net.Listen("tcp", "192.168.15.33:7878")
	if err != nil {
		log.Fatal(err)
	}
	fmt.Printf("Listering on %v\n", listner.Addr())
	for {
		conn, err := listner.Accept()
		if err != nil {
			break
		}
		go worker(conn)
	}
}
