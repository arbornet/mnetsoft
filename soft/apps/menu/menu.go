// Menu shell.

package main

import (
	// #include <unistd.h>
	// #include <utmpx.h>
	"C"

	"bufio"
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
	"os/signal"
	"os/user"
	"path/filepath"
	"strings"
	"syscall"
	"time"
)

const (
	menuLibDir = "/c/r/cross/sys/m-net/soft/apps/menu/menus"
	helpFile   = menuLibDir + "/help.txt"
	colWidth   = 34
	winWidth   = 80
)

var (
	in         *bufio.Reader
	myLogin    string
	myTty      string
	myHomeDir  string
	myShell    string
	myEditor   string
	myTermType string
	myHostName string
	splashOnce bool
)

type Entry struct {
	Key  string `json:"key"`
	Menu string `json:"menu"`
	Exec string `json:"exec"`
	Exit bool   `json:"exit"`
	Desc string `json:"desc"`
}

type Menu struct {
	Title    string  `json:"title"`
	ShowSys  bool    `json:"showsys"`
	Subtitle string  `json:"subtitle"`
	Footer   string  `json:"footer"`
	Entries  []Entry `json:"entries"`
}

func (m *Menu) String() string {
	var buffer bytes.Buffer

	buffer.WriteString("\n")
	if m.ShowSys {
		buffer.WriteString(sysheader())
	}
	buffer.WriteString(center(m.Title))
	if m.Subtitle != "" {
		buffer.WriteString("\n" + center(m.Subtitle))
	}
	buffer.WriteString("\n\n")
	ps := []string{"  ", ""}
	for k := 0; k < len(m.Entries); k++ {
		buffer.WriteString(ps[k%2] + " " + m.Entries[k].Key + ") ")
		if k == (len(m.Entries)-1) || (k%2) != 0 {
			buffer.WriteString(m.Entries[k].Desc + "\n")
		} else {
			s := fmt.Sprintf("%-*s", colWidth, m.Entries[k].Desc)
			buffer.WriteString(s)
		}
	}
	if m.Footer != "" {
		buffer.WriteString("\n" + m.Footer + "\n")
	}

	return buffer.String()
}

func center(s string) string {
	return fmt.Sprintf("%*s", (winWidth+len(s))/2, s)
}

func splash() {
	if !splashOnce {
		return
	}
	splashOnce = false
	fmt.Printf("")
	fmt.Println(center("M-Net Menu 4.0"))
	fmt.Println(center("Copyright 2025"))
	fmt.Println(center("M-Net Staff"))
}

func sysheader() string {
	var buffer bytes.Buffer
	date := time.Now().Format("Mon   Jan 02, 2006")
	nusers := fmt.Sprintf("Users: %d total", userCount())
	port := "Port: " + strings.TrimPrefix(myTty, "/dev/")
	login := "Login: " + myLogin
	term := "Terminal: " + myTermType
	shell := "Shell: " + filepath.Base(myShell)
	editor := "Editor: " + filepath.Base(myEditor)
	buffer.WriteString(fmt.Sprintf("%-28s %-32s %s\n", port, myHostName, login))
	buffer.WriteString(fmt.Sprintf("%-28s %-32s %s\n", editor, date, nusers))
	buffer.WriteString(fmt.Sprintf("%-28s %-32s %s\n", term, "", shell))
	return buffer.String()
}

func userCount() int {
	nusers := 0
	C.setutxent()
	for {
		ut := C.getutxent()
		if ut == nil {
			break
		}
		if ut.ut_type != C.USER_PROCESS {
			continue
		}
		if ut.ut_line[0] == 0 || ut.ut_user[0] == 0 {
			continue
		}
		nusers++
	}
	C.endutxent()
	return nusers
}

func login() string {
	user, err := user.Current()
	if err != nil {
		log.Fatal("Who are you?")
	}
	return user.Username
}

func hostname() string {
	hostname, err := os.Hostname()
	if err != nil {
		return "localhost"
	}
	return hostname
}

func tty() string {
	cptr := C.ttyname(C.int(os.Stdin.Fd()))
	if cptr == nil {
		return "(none)"
	}
	return C.GoString(cptr)
}

func env_or(env string, def string) string {
	value := os.Getenv(env)
	if value == "" {
		return def
	}
	return value
}

func init() {
	signal.Ignore(syscall.SIGINT)
	signal.Ignore(syscall.SIGQUIT)
	signal.Ignore(syscall.SIGTSTP)
	myLogin = login()
	myHomeDir = homedir()
	myHostName = hostname()
	myTty = tty()
	myTermType = env_or("TERM", "dumb")
	myShell = env_or("SHELL", "sh")
	myEditor = env_or("EDITOR", "nano")
	in = bufio.NewReader(os.Stdin)
	//os.Setenv("MAIL", myHomeDir+"/Mailbox")
	splashOnce = true
}

func homedir() string {
	home := os.Getenv("HOME")
	if home != "" {
		return home
	}
	usr, err := user.Current()
	if err != nil {
		log.Fatal("Who are you? ", err)
	}
	return usr.HomeDir
}

func main() {
	menu(menuLibDir + "/main.json")
	goodbye()
}

func menu(file string) {
	m := parsemenu(file)
MENU:
	for {
		clear()
		splash()
		fmt.Println(m)
		s := readcmd()
		c := strings.ToLower(s)
		switch c {
		case "":
			continue
		case "exit":
			return
		case "help":
			execv("grexmore", helpFile)
			pause()
			continue
		}
		cmd := strings.Fields(s)
		c0 := strings.ToLower(cmd[0])
		if c0 == "!cd" || c0 == "!chdir" {
			switch len(cmd) {
			case 1:
				cd(myHomeDir)
			case 2:
				cd(cmd[1])
			default:
				fmt.Fprintln(os.Stderr, "Usage: cd [dir]")
			}
			pause()
			continue
		}
		if strings.HasPrefix(s, "!") {
			system(strings.TrimPrefix(s, "!"))
			pause()
			continue
		}
		for _, entry := range m.Entries {
			if c != strings.ToLower(entry.Key) {
				continue
			}
			if entry.Exit {
				return
			}
			if entry.Menu != "" {
				menu(entry.Menu)
				continue MENU
			}
			if entry.Exec != "" {
				system(entry.Exec)
				pause()
				continue MENU
			}
		}
		fmt.Println("\nSorry, selection '" + s + "' is invalid.")
		fmt.Println("Please try again.")
		pause()
	}
}

func readcmd() string {
	fmt.Print("Command (or 'HELP'): ")
	s, err := in.ReadString('\n')
	if err == io.EOF {
		goodbye()
	}
	if err != nil {
		log.Fatal("Input ", err)
	}
	return strings.TrimSpace(s)
}

func pause() {
	fmt.Print("\nPress <ENTER> to continue\n\n")
	in.ReadString('\n')
}

func parsemenu(file string) *Menu {
	data, err := ioutil.ReadFile(file)
	if err != nil {
		log.Fatal("Error reading menu: ", err)
	}
	var m Menu
	err = json.Unmarshal(data, &m)
	if err != nil {
		log.Fatal("Error parsing menu: ", err)
	}
	if m.Title == "" {
		log.Fatal("Bad menu in ", file, ": Missing title")
	}
	foundExit := false
	for k, _ := range m.Entries {
		Menu := m.Entries[k].Menu
		Exec := m.Entries[k].Exec
		if Menu != "" && Exec != "" {
			log.Fatal("Bad menu entry in ", file, ":", m.Entries[k])
		}
		m.Entries[k].Menu = strings.Replace(Menu, "@", menuLibDir, -1)
		m.Entries[k].Exec = strings.Replace(Exec, "@", menuLibDir, -1)
		if m.Entries[k].Exit {
			foundExit = true
		}
	}
	if !foundExit {
		log.Fatal("Menu ", file, " lacks exit")
	}
	return &m
}

func cd(dir string) {
	err := os.Chdir(dir)
	if err != nil {
		log.Print("cd failed ", err)
	}
}

func goodbye() {
	fmt.Println("\nLeaving M-Net menu system.  Good bye!")
	os.Exit(0)
}

func clear() {
	execv("clear")
}

func system(cmd string) {
	execv("/bin/sh", "-c", cmd)
}

func execv(args ...string) {
	c := exec.Command(args[0], args[1:]...)
	c.Stdin = os.Stdin
	c.Stdout = os.Stdout
	c.Stderr = os.Stderr
	c.Run()
}
