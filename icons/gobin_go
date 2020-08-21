package main

import (
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"strings"
)

type Format int

const (
	hexx  Format = iota // 0x00, 0x01, 0x6b
	hex                 // 0x0, 0x1, 0x6b
	dec                 // 0, 1, 107
	shexx               // "\x00\x01\x6b",
	shex                // "\x00\x01k"
)

func (f Format) String() string {
	return map[Format]string{
		hexx:  "hexx",
		hex:   "hex",
		dec:   "dec",
		shexx: "shexx",
		shex:  "shex",
	}[f]
}
func (f *Format) Set(name string) error {
	val, ok := map[string]Format{
		"hexx":  hexx,
		"hex":   hex,
		"dec":   dec,
		"shexx": shexx,
		"shex":  shex,
	}[name]
	if !ok {
		return fmt.Errorf("may be: hexx, hex, dec, shexx, or shex")
	}
	*f = val
	return nil
}

var (
	packageName = "main"
	varName = "_"
	format = hexx
	step = 16
	fileName = ""
)

func init() {
	flag.StringVar(&packageName, "package", packageName, "package `name`")
	flag.StringVar(&varName, "var", varName, "variable `name`")
	flag.IntVar(&step, "step", step, "`number` of bytes per line")
	flag.Var(&format, "format", "type of byte representation")
	flag.Parse()

	flag.Usage = func() {
		fmt.Fprintf(os.Stderr, "Usage:\n")
		fmt.Fprintf(os.Stderr, "  %s [ flags ] FILENAME\n\n", os.Args[0])
		fmt.Fprintf(os.Stderr, "Flags:\n")
		flag.PrintDefaults()
	}

	fileName = flag.Arg(0)

	if fileName == "" {
		flag.Usage()
		return
	}
}

func main() {
	var buffer = read(fileName) // TODO use reader instead of reading whole file at once

	parents := getParents(format)
	getLine := getGetLine(format)

	// TODO use output file instead of stdin

	// print out result
	fmt.Printf("package %s\n", packageName)
	fmt.Printf("\n")
	fmt.Printf("var %s = []byte%c\n", varName, parents[0])
	for i := 0; i < len(buffer); i += step {
		from, to := i, min(i+step, len(buffer))
		fmt.Println(getLine(buffer[from:to]))
	}
	fmt.Printf("%c\n", parents[1])
}

func read(fileName string) []byte {
	filecontent, err := ioutil.ReadFile(fileName)
	if err != nil {
		log.Fatal(err)
		return []byte{}
	}
	return filecontent
}

func getParents(format Format) string {
	switch format {
	case shexx, shex:
		return "()"
	case hex, hexx, dec:
		return "{}"
	}
	return ""
}

func getGetLine(format Format) func(buffer []byte) string {
	switch format {
	case hexx:
		return func(buffer []byte) string {
			items := make([]string, len(buffer))
			for i, _ := range buffer {
				items[i] = fmt.Sprintf("%#x", buffer[i:i+1])
			}
			return fmt.Sprintf("\t%s,", strings.Join(items, ", "))
		}
	case hex:
		return func(buffer []byte) string {
			items := make([]string, len(buffer))
			for i, b := range buffer {
				items[i] = fmt.Sprintf("%#x", b)
			}
			return fmt.Sprintf("\t%s,", strings.Join(items, ", "))
		}
	case dec:
		return func(buffer []byte) string {
			items := make([]string, len(buffer))
			for i, b := range buffer {
				items[i] = fmt.Sprintf("%d", b)
			}
			return fmt.Sprintf("\t%s,", strings.Join(items, ", "))
		}
	case shexx:
		return func(buffer []byte) string {
			delim := "+"
			if len(buffer) == cap(buffer) {
				delim = ","
			}
			items := make([]string, len(buffer))
			for i, _ := range buffer {
				items[i] = fmt.Sprintf("\\x%x", buffer[i:i+1])
			}
			return fmt.Sprintf("\t\"%s\"%s", strings.Join(items, ""), delim)
		}
	case shex:
		return func(buffer []byte) string {
			delim := "+"
			if len(buffer) == cap(buffer) {
				delim = ","
			}
			return fmt.Sprintf("\t%+q%s", buffer, delim)
		}
	}
	return func(buffer []byte) string { return "" }
}

func min(a, b int) int {
	if a < b {
		return a
	} else {
		return b
	}
}
