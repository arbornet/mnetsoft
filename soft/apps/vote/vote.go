// Grex vote program.

package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"math/rand"
	"os"
	"os/user"
	"strings"
	"time"
)

const (
	voteDir      = "/cyberspace/grex/var/vote"
	electionFile = voteDir + "/election.json"
)

type Candidate struct {
	Name          string `json:"name"`
	Login         string `json:"login"`
	StatementFile string `json:"statement"`
}

type Election struct {
	Title       string      `json:"title"`
	Start       time.Time   `json:"start"`
	End         time.Time   `json:"end"`
	Choices     int         `json:"choices"`
	Candidates  []Candidate `json:"candidates"`
	ElectionDir string      `json:"dir"`
}

var (
	in       *bufio.Reader
	election Election
	username string
)

func init() {
	user, err := user.Current()
	if err != nil {
		log.Fatal("who are you? error: ", err)
	}
	data, err := ioutil.ReadFile(electionFile)
	if err != nil {
		log.Fatal("cannot read election file: ", err)
	}
	err = json.Unmarshal(data, &election)
	if err != nil {
		log.Fatal("cannot parse election file: ", err)
	}
	rand.Seed(time.Now().UnixNano())
	shuffle := rand.Perm(len(election.Candidates))
	candidates := make([]Candidate, len(election.Candidates))
	for i, _ := range election.Candidates {
		candidates[i] = election.Candidates[shuffle[i]]
	}
	election.Candidates = candidates
	username = user.Username
	in = bufio.NewReader(os.Stdin)
}

func main() {
	fmt.Println("election =", election)
	menu()
}

func menu() {
	choices := []struct {
		key   string
		label string
		thunk func()
	}{
		{"1", "Election Information", func() { longinfo(election) }},
		{"2", "List of candidates",
			func() { list(election.Candidates, election.Choices) }},
		{"3", "View statements by the candidates",
			func() { statements(election.Candidates) }},
		{"4", "Vote", func() { vote(election) }},
		{"5", "Exit the vote program", func() { leave(election) }},
	}
	for {
		fmt.Println("Choose an option by number:")
		fmt.Println()
		for _, c := range choices {
			fmt.Printf(" %s. %s\n", c.key, c.label)
		}
		fmt.Println()
		prompt := fmt.Sprintf("Your choice (%s-%s)? ",
			choices[0].key, choices[len(choices)-1].key)
		choice := readcmd(prompt)
		for _, c := range choices {
			if choice == c.key {
				c.thunk()
				break
			}
		}
	}
}

func longinfo(election Election) {
}

func list(candidates []Candidate, nchoices int) {
	fmt.Printf("There are %d candidates for %d open positions.\n",
		len(candidates), nchoices)
	fmt.Println("The candidates are listed in random order.")
	fmt.Println("")
	for _, c := range candidates {
		fmt.Printf(" %s (%s)\n", c.Name, c.Login)
	}
	fmt.Println("")
}

func statements(candidates []Candidate) {
}

func vote(election Election) {
}

func leave(election Election) {
	fmt.Println()
	fmt.Println("Remember that the election ends on", election.End)
	fmt.Println("You can come back and change your votes until then.")
	fmt.Println("In the mean time....")
	goodbye()
}

func goodbye() {
	fmt.Println("\nThank you for visting the Grex Voting Booth.\n")
	os.Exit(0)
}

func readcmd(prompt string) string {
	fmt.Print(prompt)
	s, err := in.ReadString('\n')
	if err == io.EOF {
		goodbye()
	}
	if err != nil {
		log.Fatal("Input ", err)
	}
	return strings.TrimSpace(s)
}

func pollsAreOpen(election Election) bool {
	now := time.Now()
	return (now.After(election.Start) && now.Before(election.End)) ||
		now.Equal(election.Start) || now.Equal(election.End)
}
