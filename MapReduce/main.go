package main

import (
	"fmt"
	"strconv"
)

func main() {
	fmt.Println("test - ", 123)

	v := "23.64"
	res, err := strconv.ParseFloat(v, 32)
	if err != nil {
		panic(false);
	}
	fmt.Println("Parsed value = ", res)
}
