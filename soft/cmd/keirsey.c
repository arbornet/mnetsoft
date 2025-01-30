#include <stdio.h>
#include <ctype.h>

#define CLASSES 4

enum {
	EI,
	SN,
	TF,
	JP
};

/* numbers of a and b answers of each type */
int a[CLASSES], b[CLASSES], c[CLASSES];

char type[CLASSES + 1];		/* person's type string */

char ac[CLASSES + 1] = "ESTJ";
char bc[CLASSES + 1] = "INFP";

char *an[CLASSES] = {"Extravert", "Sensing", "Thinking", "Judging"};
char *bn[CLASSES] = {"Introvert", "Intuitive", "Feeling", "Perceptive"};

struct qs {
	char *q;		/* question text (NULL marks end of list) */
	char *a;		/* (a) answer text */
	char *b;		/* (b) answer text */
	int t;			/* question type:  one of EI, SN, TF, or JP */
} questions[] = {
	/* 1-7 */
	{
		"At a party do you",
		"interact with many, including strangers",
		"interact with a few, known to you",
		EI
	},
	{
		"Are you more",
		"realistic than speculative",
		"speculative than realistic",
		SN
	},
	{
		"Is it worse to",
		"have your \042head in the clouds\042",
		"be \042in a rut\042",
		SN
	},
	{
		"Are you more impressed by",
		"principles",
		"emotions",
		TF
	},
	{
		"Are you more drawn toward the",
		"convincing",
		"touching",
		TF
	},
	{
		"Do you prefer to work",
		"to deadlines",
		"just \042whenever\042",
		JP
	},
	{
		"Do you tend to choose",
		"rather carefully",
		"somewhat impulsively",
		JP
	},

	/* 8 */
	{
		"At parties do you",
		"stay late, with increasing energy",
		"leave early, with decreased energy",
		EI
	},
	/* 9 */
	{
		"Are you more attracted to",
		"sensible people",
		"imaginative people",
		SN
	},
	/* 10 */
	{
		"Are you more interested in",
		"what is actual",
		"what is possible",
		SN
	},
	/* 11 */
	{
		"In judging others are you more swayed by",
		"laws than circumstances",
		"circumstances than laws",
		TF
	},
	/* 12 */
	{
		"In approaching other is your inclination to be somewhat",
		"objective",
		"personal",
		TF
	},
	/* 13 */
	{
		"Are you more",
		"punctual",
		"leisurely",
		JP
	},
	/* 14 */
	{
		"Does it bother you more to have things",
		"incomplete",
		"completed",
		JP
	},

	/* 15-21 */
	{
		"In your social groups do you",
		"keep abreast of other's happenings",
		"get behind on the news",
		EI
	},
	{
		"In doing ordinary things are you more likely to",
		"do it the usual way",
		"do it your own way",
		SN
	},
	{
		"Writers should",
		"\042say what they mean and mean what they say\042",
		"express things more by use of analogy",
		SN
	},
	{
		"Which appeals to you more",
		"consistency of thought",
		"harmonious human relations",
		TF
	},
	{
		"Are you more comfortable in making",
		"logical judgements",
		"value judgements",
		TF
	},
	{
		"Do you want things",
		"settled and decided",
		"unsettled and undecided",
		JP
	},
	{
		"Would you say you are more",
		"serious and determined",
		"easy going",
		JP
	},

	/* 22-28 */
	{
		"In phoning do you",
		"rarely question that it will all be said",
		"rehearse what you'll say",
		EI
	},
	{
		"Facts",
		"\042speak for themselves\042",
		"illustrate principles",
		SN
	},
	{
		"Are visionaries",
		"somewhat annoying",
		"rather fascinating",
		SN
	},
	{
		"Are you more often",
		"a cool-headed person",
		"a warm-hearted person",
		TF
	},
	{
		"Is it worse to be",
		"unjust",
		"merciless",
		TF
	},
	{
		"Should one usually let events occur",
		"by careful selection",
		"randomly and by chance",
		JP
	},
	{
		"Do you feel better about",
		"having purchased",
		"having the option to buy",
		JP
	},

	/* 29-35 */
	{
		"In company do you",
		"initiate conversation",
		"wait to be approached",
		EI
	},
	{
		"Common sense is",
		"rarely questionable",
		"frequently questionable",
		SN
	},
	{
		"Children often do not",
		"make themselves useful enough",
		"exercise their fantasy enough",
		SN
	},
	{
		"In making decisions do you feel more comfortable with",
		"standards",
		"feelings",
		TF
	},
	{
		"Are you more",
		"firm than gentle",
		"gentle than firm",
		TF
	},
	{
		"Which is more admirable:",
		"the ability to organize and be methodical",
		"the ability to adapt and make do",
		JP
	},
	{
		"Do you put more value on the",
		"definite",
		"open-ended",
		JP
	},

	/* 36-42 */
	{
		"Does new and non-routine interaction with others",
		"stimulate and energize you",
		"tax your reserves",
		EI
	},
	{
		"Are you more frequently",
		"a practical sort of person",
		"a fanciful sort of person",
		SN
	},
	{
		"Are you more likely to",
		"see how other are useful",
		"see how others see",
		SN
	},
	{
		"Which is more satisfying:",
		"to discuss an issue thoroughly",
		"to arrive at agreement on an issue",
		TF
	},
	{
		"Which rules you more:",
		"your head",
		"your heart",
		TF
	},
	{
		"Are you more comfortable with work that is",
		"contracted",
		"done on a casual basis",
		JP
	},
	{
		"Do you tend to look for",
		"the orderly",
		"whatever turns up",
		JP
	},

	/* 43-49 */
	{
		"Do you prefer",
		"many friends with brief contact",
		"a few friends with more lengthy contact",
		EI
	},
	{
		"Do you go more by",
		"facts",
		"principles",
		SN
	},
	{
		"Are you more interested in",
		"production and distribution",
		"design and research",
		SN
	},
	{
		"Which is more of a compliment",
		"\042There is a very logical person\042",
		"\042There is a very sentimental person\042",
		TF
	},
	{
		"Do you value in yourself more that you are",
		"unwavering",
		"devoted",
		TF
	},
	{
		"Do you more often prefer the",
		"final and unalterable statement",
		"tentative and preliminary statement",
		JP
	},
	{
		"Are you more comfortable",
		"after a decision",
		"before a decision",
		JP
	},

	/* 50-56 */
	{
		"Do you",
		"speak easily and at length with strangers",
		"find little to say to strangers",
		EI
	},
	{
		"Are you more likely to trust your",
		"experience",
		"hunch",
		SN
	},
	{
		"Do you feel",
		"more practical than ingenious",
		"more ingenious than practical",
		SN
	},
	{
		"Which person is more to be complimented:  one of",
		"clear reason",
		"strong feeling",
		TF
	},
	{
		"Are you inclined to be",
		"fair-minded",
		"sympathetic",
		TF
	},
	{
		"Is it preferable mostly to",
		"make sure things are arranged",
		"just let things happen",
		JP
	},
	{
		"In relationships should most things be",
		"renegotiable",
		"random and circumstantial",
		JP
	},

	/* 57-63 */
	{
		"When a phone rings do you",
		"hasten to get to it first",
		"hope someone else will answer",
		EI
	},
	{
		"Do you prize more in yourself",
		"a strong sense of reality",
		"a vivid imagination",
		SN
	},
	{
		"Are you drawn more to",
		"fundamentals",
		"overtones",
		SN
	},
	{
		"Which seems the greater error:",
		"to be too passionate",
		"to be too objective",
		TF
	},
	{
		"Do you see yourself as basically",
		"hard-hearted",
		"soft-hearted",
		TF
	},
	{
		"Which situation appeals to you more:",
		"the structured and scheduled",
		"the unstructured and unscheduled",
		JP
	},
	{
		"Are you a person that is more",
		"routinized than whimsical",
		"whimsical than routinized",
		JP
	},

	/* 64-70 */
	{
		"Are you more inclined to be",
		"easy to approach",
		"somewhat reserved",
		EI
	},
	{
		"In writings do you prefer",
		"the more literal",
		"the more figurative",
		SN
	},
	{
		"Is it harder for you to",
		"identify with others",
		"utilize others",
		SN
	},
	{
		"Which do you wish more for yourself:",
		"clarity of reason",
		"strength of compassion",
		TF
	},
	{
		"Which is the greater fault:",
		"being indiscriminate",
		"being critical",
		TF
	},
	{
		"Do you prefer the",
		"planned event",
		"unplanned event",
		JP
	},
	{
		"Do you tend to be more",
		"deliberate than spontaneous",
		"spontaneous than deliberate",
		JP
	},

	{
		NULL, NULL, NULL, 0
	}
};


struct ds {
	char *t;		/* type string */
	char *d;		/* description */
} descriptions[] = {
	{
		"ISTJ",
		" Serious, quiet, earn success by concentration and thoroughness.  Practical,\n\
 orderly, matter-of-fact, logical, realistic, and dependable.  See to it that\n\
 everything is well organized.  Take responsiblity.  Make up their own minds\n\
 as to what should be accomplished and work toward it steadily, regardless\n\
 or protests or distractions.\n"
	},
	{
		"ISFJ",
		" Quiet, friendly, responsible, and conscientious.  Work devotedly to meet\n\
 their obligations.  Lend stability to any project or group.  Thorough,\n\
 painstaking, accurate.  Their interests are usually not technical.  Can be\n\
 patient with necessary details.  Loyal, considerate, perceptive, concerned\n\
 with how other people feel.\n"
	},
	{
		"INFJ",
		" Succeed by perseverance, originality, and desire to do whatever is needed\n\
 or wanted.  Put their best efforts into their work.  Quietly forceful,\n\
 conscientious, concerned for others.  Respected for their firm principles.\n\
 Likely to be honored and followed for their clear convictions as to how to\n\
 best serve the the common good.\n"
	},
	{
		"INTJ",
		" Usually have original minds and great drive for their own ideas and\n\
 purposes.  In fields that appeal to them, they have a fine power to organize\n\
 a job and carry it through with or without help.  Skeptical, critical,\n\
 independent, determined, sometimes stubborn.  Must learn to yield less\n\
 important points in order to win the most important.\n"
	},
	{
		"ISTP",
		" Cool onlookers--quiet, reserved, observing and analyzing life with detached\n\
 curiosity and unexpected flashes of original humor.  Usually interested in\n\
 cause and effect, how and why mechanical things work, and in organizing\n\
 facts using logical principles.\n"
	},
	{
		"ISFP",
		" Retiring, quietly friendly, sensitive, kind, modest about their abilities.\n\
 Shun disagreements, do not force their opinions or values on others.  Usually\n\
 do not care to lead but are often loyal followers.  Often relaxed about\n\
 getting things done, because they enjoy the present moment and do not want\n\
 to spoil it by undue haste or exertion.\n"
	},
	{
		"INFP",
		" Full of enthusiasms and loyalties, but seldom talk of these until they know\n\
 you well.  Care about learning, ideas, language, and independent projects\n\
 of their own.  Tend to undertake too much, then somehow get it done.\n\
 Friendly, but often too absorbed in what they are doing to be sociable.\n\
 Little concerned with possessions or physical surroundings.\n"
	},
	{
		"INTP",
		" Quiet and reserved.  Especially enjoy theoretical or scientific pursuits.\n\
 Like solving problems with logic and analysis.  Usually interested mainly in\n\
 ideas, with little liking for parties or small talk.  Tend to have sharply\n\
 defined interests.  Need careers where some strong interest can be used and\n\
 useful.\n"
	},
	{
		"ESTP",
		" Good at on-the-spot problem solving.  Do not worry, enjoy whatever comes\n\
 along.  Tend to like mechanical things and sports, with friends on the side.\n\
 Adaptable, tolerant, generally conservative in values.  Dislike long\n\
 explanations.  Are best with real things that can be worked, handled, take\n\
 apart, or put together.\n"
	},
	{
		"ESFP",
		" Outgoing, easygoing, accepting, friendly, enjoy everything and make things\n\
 more fun for others by their enjoyment.  Like sports and making things\n\
 happen.  Know what's going on and join in eagerly.  Find remembering facts\n\
 easier than mastering theories.  Are best in situations that need sound\n\
 common sense and practical ability with people as well as with things.\n"
	},
	{
		"ENFP",
		" Warmly enthusiastic, high-spirited, ingenious, imaginative.  Able to do\n\
 almost anything that interests them.  Quick with a solution for any\n\
 difficulty and ready to help anyone with a problem.  Often rely on their\n\
 ability to improvise instead of preparing in advance.  Can usually find\n\
 compelling reasons for whatever they want.\n"
	},
	{
		"ENTP",
		" Quick, ingenious, good at many things.  Stimulating company, alert and\n\
 outspoken.  May argue for fun on either side of a question.  Resourceful in\n\
 solving new and challenging problems, but may neglect routine assignments.\n\
 Apt to turn to one new interest after another.  Skillful in finding logical\n\
 reasons for what they want.\n"
	},
	{
		"ESTJ",
		" Practical, realistic, matter-of-fact, with a natural head for business or\n\
 mechanics.  Not interested in subjects they see no use for, but can apply\n\
 themselves when necessary.  Like to organize and run activities.  May make\n\
 good administrators, especially if they remember to consider others'\n\
 feelings and points of view.\n"
	},
	{
		"ESFJ",
		" Warm-hearted, talkative, popular, conscientious, born cooperators, active\n\
 committee members.  Need harmony and may be good at creating it.  Always\n\
 doing something nice for someone.  Work best with encouragement and praise.\n\
 Main interest is in things that directly and visibly affect people's lives.\n"
	},
	{
		"ENFJ",
		" Responsive and responsible. Generally feel real concern for what others\n\
 think or want, and try to handle things with due regard for the other\n\
 person's feelings.  Can present a proposal or lead a group discussion with\n\
 ease and tact.  Sociable, popular, sympathetic.  Responsive to praise and\n\
 criticism.\n"
	},
	{
		"ENTJ",
		" Hearty, frank, decisive, leaders in activities.  Usually good in anything\n\
 that requires reasoning and intelligent talk, such as public speaking.  Are\n\
 usually well informed and enjoy adding to their fund of knowledge.  May\n\
 sometimes appear more positive and confident than their experience in an\n\
 area warrants.\n"
	},
	{
		NULL, NULL
	}
};

int qn = 0;
char *progname;

char qflag = 0;
char iflag = 0;
char desflag = 0;

main(argc, argv)
	int argc;
	char **argv;
{
	int i, j;
	struct qs *qp;

	progname = argv[0];

	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-')
			for (j = 1; argv[i][j]; j++)
				switch (argv[i][j]) {
				case 'q':
					qflag = 1;
					break;
				case 'i':
					iflag = 1;
					break;
				default:
					usage();
				}
		else if (check_string(argv[i]) && !desflag) {
			desflag = 1;
			strcpy(type, argv[i]);
		} else
			usage();
	}

	/* Just list the questions */
	if (qflag) {
		for (qp = questions, i = 1; qp->q; qp++, i++) {
			printq(qp, i);
			putchar('\n');
		}
		exit(0);
	}
	if (!desflag) {
		printf("The Keirsey Temperament Sorter\n\n");
		printf("There are 70 questions.  Give the answer (a or b) that best describes\n");
		printf("your normal behavior.  The results of this test will not be recorded.\n\n");
		askall();
		printf("\nScoring:\n");
		if (class_string()) {
			printf("\nYou have no temperament whatsoever.\n");
			exit(0);
		}
	}
	print_class();

	exit(0);
}

usage()
{
	fprintf(stderr, "usage: %s [-iq] [(E|I|?)(S|N|?)(T|F|?)(J|P|?)]\n",
		progname);
	exit(1);
}

printq(qp, qn)
	struct qs *qp;
	int qn;
{
	printf("%2d.  %s\n", qn, qp->q);
	printf("      (a) %s\n", qp->a);
	printf("      (b) %s\n", qp->b);
	if (iflag)
		printf("      (c) I dunno.\n", qp->b);
}

answera(qp, qn)
	struct qs *qp;
	int qn;
{
	int ch, ans;

	for (;;) {
		printq(qp, qn);
		printf("? ");
		while ((ch = getchar()) == ' ' || ch == '\t' || ch == '(');
		if (ch == EOF)
			exit(1);
		ans = ch;
		if (ch != '\n') {
			while ((ch = getchar()) != '\n')
				if (ch == EOF)
					exit(1);
		}
		if (ans == 'a' || ans == 'A')
			return (0);
		if (ans == 'b' || ans == 'B')
			return (1);
		if (iflag && (ans == 'c' || ans == 'C'))
			return (2);
		printf("Please answer A%s, whichever is most appropriate.\n\n",
		       iflag ? " or B" : ", B, or C");
	}
}

askall()
{
	struct qs *qp;
	int qn;
	int i;

	for (i = 0; i < CLASSES; i++)
		a[i] = b[i] = c[i] = 0;

	for (qp = questions, qn = 1; qp->q; qp++, qn++) {
		switch (answera(qp, qn)) {
		case 0:
			a[qp->t]++;
			break;
		case 1:
			b[qp->t]++;
			break;
		case 2:
			c[qp->t]++;
			break;
		}
	}
}

int
class_string()
{
	int i, z = 0;

	type[CLASSES] = 0;

	for (i = 0; i < CLASSES; i++) {
		if (a[i] > b[i])
			type[i] = ac[i];
		else if (a[i] < b[i])
			type[i] = bc[i];
		else {
			type[i] = '?';
			if (a[i] == 0)
				z++;
		}

		printf("  %c  %3d%% %-10s   %3d%% %-10s\n",
		       type[i],
		       100 * a[i] / (a[i] + b[i] + c[i]), an[i],
		       100 * b[i] / (a[i] + b[i] + c[i]), bn[i]);
	}
	return (z == 4);
}

int
type_match(t)
	char *t;
{
	int i;
	for (i = 0; i < CLASSES; i++)
		if (type[i] != '?' && type[i] != t[i])
			return (0);
	return (1);
}

print_class()
{
	struct ds *dp;
	int flag = 0;

	if (!desflag) {
		if (strchr(type, '?'))
			printf("\nYou fall on the borderline between several classes:\n");
		else
			printf("\nYour type is:\n");
	}
	for (dp = descriptions; dp->t; dp++)
		if (type_match(dp->t)) {
			if (!desflag && ((flag++) % 2))
				hrtc();
			printf("\n%s\n\n%s", dp->t, dp->d);
		}
}

check_string(t)
	char *t;
{
	int i;
	for (i = 0; i < CLASSES; i++)
		if (islower(t[i]))
			t[i] = toupper(t[i]);
	if (t[i] != '?' && t[i] != ac[i] && t[i] != bc[i])
		return (0);

	return (t[CLASSES] == '\0');
}

hrtc()
{
	int ch;
	printf("\nHit <return> to continue ");
	while ((ch = getchar()) != '\n' && ch != EOF);
}
