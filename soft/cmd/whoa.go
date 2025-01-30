package main

import (
	"bufio"
	"bytes"
	"encoding/binary"
	"flag"
	"fmt"
	"io"
	"log"
	"os"
	"os/user"
	"sort"
	"strings"
	"unicode"
	"unicode/utf8"
)

const (
	utmpfile = "/var/run/utmp"

	resetColor   = "\033[0m"
	headingColor = "\033[1;32m"
	letterColor  = "\033[32m"
	countColor   = "\033[1;34m"
	knownColor   = "\033[1;36m"
)

var nocolor bool

type Utmp struct {
	Name string
	Line string
	Host string
	Time int64
}

func binString(b []byte) string {
	n := bytes.IndexByte(b, 0)
	if n < 0 {
		n = len(b)
	}
	return string(b[0:n])
}

func nextUtmp(r io.Reader, ut *Utmp) bool {
	var rec struct {
		Line [8]byte
		Name [32]byte
		Host [256]byte
		Time int64
	}
	for {
		err := binary.Read(r, binary.LittleEndian, &rec)
		if err == io.EOF {
			return false
		}
		if err != nil {
			log.Fatal("read")
		}
		if rec.Name[0] == 0 || rec.Line[0] == 0 {
			continue
		}
		break
	}
	ut.Name = binString(rec.Name[:])
	ut.Line = binString(rec.Line[:])
	ut.Host = binString(rec.Host[:])
	ut.Time = rec.Time
	return true
}

func insert(a []string, s string, i int) []string {
	return append(a[:i], append([]string{s}, a[i:]...)...)
}

func readUserLists(file io.Reader) map[rune][]string {
	var ut Utmp
	users := make(map[rune][]string)
	for nextUtmp(file, &ut) {
		r, _ := utf8.DecodeRuneInString(ut.Name)
		r = unicode.ToUpper(r)
		i := sort.SearchStrings(users[r], ut.Name)
		if i < len(users[r]) && users[r][i] == ut.Name {
			continue
		}
		users[r] = insert(users[r], ut.Name, i)
	}
	return users
}

func sortedKeys(users map[rune][]string) []rune {
	var ks []rune
	for r, _ := range users {
		ks = append(ks, r)
	}
	sort.Slice(ks, func(i, j int) bool { return ks[i] < ks[j] })
	return ks
}

func whoIKnow() map[string]bool {
	whoiknow := make(map[string]bool)
	home := os.Getenv("HOME")
	if home == "" {
		usr, err := user.Current()
		if err != nil {
			log.Fatal("Who are you? ", err)
		}
		home = usr.HomeDir
	}
	filename := home + "/.whoiknow"
	file, err := os.Open(filename)
	if err != nil {
		return whoiknow
	}
	defer file.Close()
	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		whoiknow[scanner.Text()] = true
	}
	return whoiknow
}

func colorize(s string, c string) string {
	if nocolor {
		return s
	}
	return c + s + resetColor
}

func printUsers(users map[rune][]string) {

	letterf := colorize("%c", letterColor)
	countf := colorize("%03d", countColor)
	numberf := colorize("%d users total", headingColor)
	titlef := colorize("Current Grex Users", headingColor)
	f := "     [" + letterf + "] (" + countf + ") %s\n"

	iknow := whoIKnow()
	fmt.Printf("\n     " + titlef + "\n\n")
	nusers := 0
	for _, r := range sortedKeys(users) {
		nusers += len(users[r])
		for i, s := range users[r] {
			if iknow[s] {
				users[r][i] = colorize(s, knownColor)
			}
		}
		fmt.Printf(f, r, len(users[r]), strings.Join(users[r], ", "))
	}
	fmt.Printf("\n     "+numberf+"\n\n", nusers)
}

func init() {
	var helpme bool
	flag.BoolVar(&nocolor, "c", false, "Do not use colors")
	flag.BoolVar(&helpme, "h", false, "Show this message")
	flag.BoolVar(&helpme, "?", false, "alias for -h")
	flag.Usage = usage
	flag.Parse()
	if helpme {
		usage()
	}
}

func main() {
	flag.Usage = usage
	flag.Parse()

	file, err := os.Open(utmpfile)
	if err != nil {
		log.Fatal(err)
	}
	defer file.Close()

	users := readUserLists(file)
	printUsers(users)
}

func usage() {
	fmt.Fprintln(os.Stderr, "Usage: whoa [-ch]")
	fmt.Fprintln(os.Stderr, "  -c\tdo not use colors")
	fmt.Fprintln(os.Stderr, "  -h\tshow this message")
	fmt.Fprintln(os.Stderr, "  Create ~/.whoiknow with one login name per line to highlight known users in output.")
	os.Exit(1)
}
