/* Copyright 2000, 2009, 2011, 2012 Neil Edelman, distributed under the terms
 of the GNU General Public License, see copying.txt */

/* it's the OSS game, by Neil Edelman (Dec. 2000) */
/* let's make it playable on the web (Dec 2009) */
/* high-score and oops fix (May 2012) */

#include <stdlib.h>
#include <stdio.h>
#include "Oss.h"
#include "Room.h"

int room_comp(const void *key, const void *elem);

static void Outside(struct Oss *oss, const char, const int);
static void Entrance(struct Oss *oss, const char, const int);
static void ParkingLot(struct Oss *oss, const char, const int);
static void Concourse(struct Oss *oss, const char, const int);
static void ComputerRoom(struct Oss *oss, const char, const int);
static void Office(struct Oss *oss, const char, const int);
static void StaffRoom(struct Oss *oss, const char, const int);
static void Bathroom(struct Oss *oss, const char, const int);
static void ScienceRoom(struct Oss *oss, const char, const int);
static void Lunch(struct Oss *oss, const char, const int);
static void Concession(struct Oss *oss, const char, const int);
static void EndLunch(struct Oss *oss, const char, const int);
static void Basement(struct Oss *oss, const char, const int);
static void Gym(struct Oss *oss, const char, const int);
static void OutsideAgriculture(struct Oss *oss, const char, const int);
static void Agriculture(struct Oss *oss, const char, const int);
static void Detentions(struct Oss *oss, const char, const int choice);
static void EndSchool(struct Oss *oss, const char, const int);
static void JackShaw(struct Oss *oss, const char, const int);
static void JackShawDogopole(struct Oss *oss, const char, const int choice);
static void JackShawSchulting(struct Oss *oss, const char, const int choice);
static void Cops(struct Oss *oss, const char, const int);

enum Room {
	rNowhere = 0,
	rAgriculture,
	rBasement,
	rBathroom,
	rComputerRoom,
	rConcession,
	rConcourse,
	rCops,
	rDetentions,
	rEndLunch,
	rEndSchool,
	rEntrance,
	rGym,
	rJackShaw,
	rJackShawDogopole,
	rJackShawSchulting,
	rLunch,
	rOffice,
	rOutside,
	rOutsideAgriculture,
	rParkingLot,
	rScienceRoom,
	rStaffRoom
};

enum {
	SavedFlagBoy = 1,
	MessedWithSign = 2,
	BeenInComputerRoom = 4,
	BeenInBathroom = 8,
	KnowOfFight = 16,
	ResetClock = 32,
	SeenConcession = 64,
	FriedFork = 128,
	SeenGym = 256,
	Explored = 512,
	FireAlarm = 1024
};

static const struct FnName {
	char      *description;
	void      (*fn)(struct Oss *, const char, const int);
	enum Room code;
} fnnames[] = {
	{ "Agriculture", &Agriculture, rAgriculture },
	{ "Basement", &Basement, rBasement },
	{ "Bathroom", &Bathroom, rBathroom },
	{ "ComputerRoom", &ComputerRoom, rComputerRoom },
	{ "Concession", &Concession, rConcession },
	{ "Concourse", &Concourse, rConcourse },
	{ "Cops", &Cops, rCops },
	{ "Detentions", &Detentions, rDetentions },
	{ "EndLunch", &EndLunch, rEndLunch },
	{ "EndSchool", &EndSchool, rEndSchool },
	{ "Entrance", &Entrance, rEntrance },
	{ "Gym", &Gym, rGym },
	{ "JackShaw", &JackShaw, rJackShaw },
	{ "JackShawDogopole", &JackShawDogopole, rJackShawDogopole },
	{ "JackShawSchulting", &JackShawSchulting, rJackShawSchulting },
	{ "Lunch", &Lunch, rLunch },
	{ "Office", &Office, rOffice },
	{ "Outside", &Outside, rOutside },
	{ "OutsideAgriculture", &OutsideAgriculture, rOutsideAgriculture },
	{ "ParkingLot", &ParkingLot, rParkingLot },
	{ "ScienceRoom", &ScienceRoom, rScienceRoom },
	{ "StaffRoom", &StaffRoom, rStaffRoom },
};

/* constants */
const int        nowhereroom      = rNowhere;
const int        startroom        = rOutside;
static const int keywaitcount     = 4;
static const int detentionminutes = 30;

/** given the number of the room, return the function */
void (*RoomFn(const int room))(struct Oss *, const char, const int) {
	struct FnName *fnname = bsearch(&room,       /* key */
									fnnames,     /* base, num, size */
									sizeof(fnnames) / sizeof(struct FnName),
									sizeof(struct FnName),
									&room_comp); /* comparator */;
	return fnname ? fnname->fn : 0;
}

/** given the number of the room, return the name (for debuging) */
char *RoomName(const int room) {
	struct FnName *fnname = bsearch(&room,       /* key */
									fnnames,     /* base, num, size */
									sizeof(fnnames) / sizeof(struct FnName),
									sizeof(struct FnName),
									&room_comp); /* comparator */;
	return fnname ? fnname->description : 0;
}

/** report points */
void RoomReport(const struct Oss *oss) {
	printf("<p>Scored %i; made %i new friend%s; made %i new enem%s; changed \
your grade by %i; and skipped %i detention%s.</p>\n\n",
		   oss->points, oss->friends, oss->friends == 1 ? "" : "s",
		   oss->enemies, oss->enemies == 1 ? "y" : "ies", oss->education,
		   oss->trouble, oss->trouble == 1 ? "" : "s");
	if(oss->flags & SavedFlagBoy) printf("<p>You saved flag boy.</p>\n\n");
	if(oss->flags & MessedWithSign) printf("<p>You messed with the sign.</p>\n\n");
	if(oss->flags & ResetClock) printf("<p>You permenently altered The Clock; now the school is in it's own time zone.</p>\n\n");
	if(oss->flags & FriedFork) printf("<p>You put a fork in the microwave.</p>\n\n");
	if(oss->flags & Explored) printf("<p>You explored the school.</p>\n\n");
	if(oss->flags & FireAlarm) printf("<p>You have a broken fire alarm pull station.</p>\n\n");
}

/** compares the key-enum with the function; this is bsearch-compatible */
int room_comp(const void *key, const void *elem) {
	return *(enum Room *)key - ((struct FnName *)elem)->code;
}

/* right, enough with the preliminaries, this is the story */

static void Outside(struct Oss *oss, const char choice, const int show) {
	switch(choice) {
		case 0:
			OssMessage(oss, "You find yourself outside of \"Osoyoos Secondarie Skol\" (according to the sign). On the wall is a \"Drug Free Zone\" poster. A grade-8 is duct-tapped to the flag pole.\n",
			        "go inside;",
			        "go back to the parking lot;",
			        "cut the grade-8 down; or",
					"vandalize the sign;", (char *)0);
			break;
		case 'a':
			oss->room = rEntrance;
			break;
		case 'b':
			oss->room = rParkingLot;
			break;
		case 'c':
			if(show) printf("You try to gnaw off the duct tape holding the kid, but it won't come loose. Eventually a teacher named Mr. Mocci helps you take him down, but not before some seniors laugh at you.\n");
			oss->flags |= SavedFlagBoy;
			oss->points--;
			oss->friends++;
			oss->room = rEntrance;
			break;
		case 'd':
			if(show) printf("You draw a big smiley face on the sign. Some kids come by and are impressed with your artwork, but a Mr. Hartman sees you and gives you a detention.\n");
			oss->flags |= MessedWithSign;
			oss->points++;
			oss->trouble++;
			oss->room = rEntrance;
			break;
	}
}

static void Entrance(struct Oss *oss, const char choice, const int show) {
	switch(choice) {
		case 0:
			OssMessage(oss, "You enter the main school building.",
					"go ahead to the concourse",
					"go left into the computer room",
					"head right into the office", (char *)0);
			break;
		case 'a':
			oss->room = rConcourse;
			break;
		case 'b':
			oss->room = rComputerRoom;
			break;
		case 'c':
			if(show) printf("Some students in the concourse see you enter the office and think that you are a loser.\n");
			oss->points--;
			oss->room = rOffice;
			break;
	}
}

static void ParkingLot(struct Oss *oss, const char choice, const int show) {
	switch(choice) {
		case 0:
			OssMessage(oss, "The parking lot is full of rusty old cars. Just then, a student pulls in, does a doughnut, and squeals his car to a halt beside you. You:",
					"compliment him on his driving;",
					"swear at him for almost running you over;",
					"run away and hope he isn't trying to kill you; or",
					"ask him to help you sabotage a teacher's car.", (char *)0);
			break;
		case 'a':
			if(show) printf("He laughs and you head back to the school.\n");
			oss->friends++;
			oss->room = rEntrance;
			break;
		case 'b':
			if(show) printf("\"I didn't even come close to hitting you,\" he says. You follow him inside.\n");
			oss->enemies++;
			oss->room = rEntrance;
			break;
		case 'c':
			if(show) printf("He doesn't seem to notice you as you run back to the school.\n");
			oss->room = rEntrance;
			break;
		case 'd':
			if(show) printf("\"Dude!\" says the driver. You go to the mechanics room, get a Mr. Sanderson to lend you a jack, and return to the parking lot, removing the tires from a blue neon. That teacher will be mad. You go back into the school.\n");
			oss->points += 2;
			oss->friends++;
			oss->room = rEntrance;
			break;
	}
}

static void Concourse(struct Oss *oss, const char choice, const int show) {
	switch(choice) {
		case 0:
			OssMessage(oss, "You are in the concourse, heart of the school. What a mess. People are everywhere. So is garbage.",
					"sit with the grade-8's;",
					"sit with the grade-12's;",
					"try to find some hot babes;",
					"help the janitor pick up garbage;",
					"get to science class on time;",
					"go to the computer room; or",
					"go to the bathroom.", (char *)0);
			break;
		case 'a':
			if(show) printf("The grade-8's throw food at you, but are otherwise friendly. When it is five minutes after the start of class, you follow some other students into class.\n");
			oss->friends++;
			oss->room = rScienceRoom;
			break;
		case 'b':
			if(show) printf("The grade-12's tell you about a fight at Jack Shaw Gardens tonight. When it is ten minutes after the start of class, you follow them into the science room.\n");
			oss->flags |= KnowOfFight;
			oss->room = rScienceRoom;
			break;
		case 'c':
			if(show) printf("By the time class should start, you still can't find any. Maybe they'll be some in class.\n");
			oss->room = rScienceRoom;
			break;
		case 'd':
			if(show) printf("You pick up garbage until class and get crud all over your hands.\n");
			oss->points--;
			oss->room = rScienceRoom;
			break;
		case 'e':
			if(show) printf("You go to the science class and stand at the door for a few minutes until the teacher unlocks it.\n");
			oss->room = rScienceRoom;
			break;
		case 'f':
			if(oss->flags & BeenInComputerRoom) {
				if(show) printf("<p>The door to the computer room is locked. You back into the concourse.</p>\n\n");
				oss->room = rConcourse;
			} else {
				oss->room = rComputerRoom;
			}
			break;
		case 'g':
			if(oss->flags & BeenInBathroom) {
				if(show) printf("<p>The bathroom is still the same. You go back to the concourse.</p>\n\n");
				oss->room = rConcourse;
			} else {
				oss->room = rBathroom;
			}
			break;
	}
}

static void ComputerRoom(struct Oss *oss, const char choice, const int show) {
	switch(choice) {
		case 0:
			OssMessage(oss, "The computer room is full of malfunctioning computers:",
					"leave;",
					"hack into the student database;",
					"bust into the closet with the main server in it;",
					"push a computer onto the floor and blame it on someone else; or",
					"listen to what the teacher is saying.", (char *)0);
			return;
		case 'a':
			oss->room = rConcourse;
			break;
		case 'b':
			if(show) printf("Mr. Schulting sees you and gives you a detention, but not before you manage to augment your grades a bit. He becomes your sworn enemy, though the computer dudes think that you are cool.\n");
			oss->points++;
			oss->friends++;
			oss->enemies++;
			oss->education++;
			oss->trouble++;
			oss->room = rConcourse;
			break;
		case 'c':
			if(show) printf("You bust down the door, but Mr. Schulting quickly overpowers you and sends you outside, giving you a detention. The other students think that you are a weird psycho.\n");
			oss->points--;
			oss->trouble++;
			oss->room = rConcourse;
			break;
		case 'd':
			if(show) printf("SMASH. The monitor falls on your foot and glass flies everywhere. The shaking causes a light to fall on your head. You die.\n");
			oss->points--;
			oss->room = rNowhere;
			break;
		case 'e':
			if(show) printf("Mr. Schulting tells you about the final exams for the computer courses before you leave.\n");
			oss->education++;
			oss->room = rConcourse;
			break;
	}
	oss->flags |= BeenInComputerRoom;
}

static void Office(struct Oss *oss, const char choice, const int show) {
	switch(choice) {
		case 0:
			OssMessage(oss, "The office is before you . . . ",
					"get the secretary to print out a class schedule for you;",
					"put a whoopie-cushion on the principal's chair;",
					"sneak back into the staff room; or",
					"report the flag-pole kid to the vice-principal.", (char *)0);
			break;
		case 'a':
			if(show) printf("You get a class schedule, and walk back to the concourse.\n");
			oss->education++;
			oss->room = rConcourse;
			break;
		case 'b':
			if(show) printf("In a daring move, you duck into the principal's office and place the trap, walking back to the concourse. The watching students now think that you are very cool.\n");
			oss->points += 2;
			oss->room = rConcourse;
			break;
		case 'c':
			oss->room = rStaffRoom;
			break;
		case 'd':
			if(show) printf("The VP thanks you as you return to the concourse.\n");
			oss->friends++;
			oss->room = rConcourse;
			break;
	}
}

static void StaffRoom(struct Oss *oss, const char choice, const int show) {
	switch(choice) {
		case 0:
			OssMessage(oss, "The teachers in the staff lounge are distracted by their conversations. You:",
					"run back to the concourse before you get caught;",
					"pretend that you are lost;",
					"yell, \"Hello\" in a load voice to surprise the teachers; or",
					"mix some vodka from the cupboard into the coffee machine.", (char *)0);
			break;
		case 'a':
			oss->room = rConcourse;
			break;
		case 'b':
			if(show) printf("Mr. Coutu escorts you back to the concourse; he seems aggrevated.\n");
			oss->room = rConcourse;
			break;
		case 'c':
			if(show) printf("Mr. Coutu spills his coffee on Mrs. Lacey and announces that you have a detention after school before sending you to the concourse.\n");
			oss->trouble++;
			oss->room = rConcourse;
			break;
		case 'd':
			if(show) printf("You spike the drink and escape to the concourse without getting caught.\n");
			oss->points += 3;
			oss->room = rConcourse;
			break;
	}
}

static void Bathroom(struct Oss *oss, const char choice, const int show) {
	switch(choice) {
		case 0:
			OssMessage(oss, "The bathroom is in a state of disarray.",
					"leave;",
					"get a little relief;",
					"try to stuff some little kid in the toilet;",
					"bust stuff and generally make a big mess;",
					"help out the janitor by cleaning up a bit; or",
					"admire yourself in the mirror.", (char *)0);
			return;
		case 'a':
			oss->room = rConcourse;
			break;
		case 'b':
			if(show) printf("Ahh . . . much better.\n");
			oss->points++;
			oss->room = rConcourse;
			break;
		case 'c':
			if(show) printf("The kid is stronger than you thought, and he yells and screams. Mr. Saffik comes in and gives you a detention.\n");
			oss->enemies++;
			oss->trouble++;
			oss->room = rConcourse;
			break;
		case 'd':
			if(show) printf("Just as you are smashing a stall door, a massive janitor enters and attacks you in a mad rage with the end of his mop, beating you until the paramedics remove you to the hospital. The whole school thinks that you are a total loser for trashing the bathroom.\n");
			oss->points -= 2;
			oss->enemies += 300;
			oss->room = rNowhere;
			break;
		case 'e':
			if(show) printf("You start cleaning up, but you get some weird goo on your hands. After a minute, it has spread up your arm. You try to wash it off, but it is too late; it melts away the rest of your body. You Die.\n");
			oss->room = rNowhere;
			break;
		case 'f':
			if(show) printf("The mirror is too dirty to see anything.\n");
			oss->room = rBathroom;
			return;
	}
	oss->flags |= BeenInBathroom;
}

static void ScienceRoom(struct Oss *oss, const char choice, const int show) {
	switch(choice) {
		case 0:
			OssMessage(oss, "Welcome to science. Mr. Bonnett is your teacher for this class. Choose your tactic:",
					"sneak out of class and go home;",
					"sit and pay attention;",
					"socialize;",
					"get some sleep while pretending to pay attention;",
					"tip over the liquid nitrogen;",
					"try to palm some lab specimens; or",
					"mix dangerous chemicals and try to make a bomb.", (char *)0);
			break;
		case 'a':
			if(show) printf("You skip out of class and go home.\n");
			oss->points += 2;
			oss->education--;
			oss->room = rNowhere;
			break;
		case 'b':
			if(show) printf("You pay attention until lunch and absorb lots of information.\n");
			oss->education += 2;
			oss->room = rLunch;
			break;
		case 'c':
			if(oss->flags & KnowOfFight) {
				if(show) printf("You talk to the other students until lunch about the fight tonight and make some friends.\n");
				oss->friends += 2;
			} else {
				if(show) printf("You talk with some students until lunch and learn about a fight going down tonight at Jack Shaw Gardens.\n");
				oss->friends++;
				oss->flags |= KnowOfFight;
			}
			oss->room = rLunch;
			break;
		case 'd':
			if(show) printf("You sleep through class and feel refreshed. While you were sleeping, you learned some of the class content subliminally.\n");
			oss->points++;
			oss->education++;
			oss->room = rLunch;
			break;
		case 'e':
			if(show) printf("You open the can and knock it over, pretending that it was an accident. Some of it splashes on Mr. Bonnett's hair, freezing it sticking straight up. He makes you sit by his desk for the rest of the class, where you learn a bit.\n");
			oss->points++;
			oss->education++;
			oss->enemies++;
			oss->room = rLunch;
			break;
		case 'f':
			if(show) printf("You 'borrow' a preserved eel and some snails, then sit down and listen for the rest of class.\n");
			oss->points++;
			oss->education++;
			oss->room = rLunch;
			break;
		case 'g':
			if(show) printf("A funny smell starts to fill the room. You set the chemicals into a cupboard and run outside with everyone else. Soon thereafter, the science room blows up. Some think that this is really cool, others think that you are an idiot. You are told that you have a detention, and hang out outside until lunch.\n");
			oss->points += 2;
			oss->friends += 2;
			oss->enemies += 2;
			oss->trouble++;
			oss->room = rLunch;
			break;
	}
}

static void Lunch(struct Oss *oss, const char choice, const int show) {
	switch(choice) {
		case 0:
			OssMessage(oss, "Lunch time. You decide to:",
					"do some studying;",
					"go home for a while;",
					"go to the concession;",
					"put a fork in the microwave;",
					"sit and chat;",
					"go to the gym; or",
					"explore the school.", (char *)0);
			break;
		case 'a':
			if(show) printf("You study a bit and get a headache.\n");
			oss->education++;
			oss->room = rEndLunch;
			break;
		case 'b':
			if(show) printf("You go home and come back later.\n");
			oss->room = rEndLunch;
			break;
		case 'c':
			if(oss->flags & SeenConcession) {
				if(show) printf("You don't want to go there again.\n");
				oss->room = rLunch;
			} else {
				oss->flags |= SeenConcession;
				oss->room = rConcession;
			}
			break;
		case 'd':
			if(oss->flags & FriedFork) {
				if(show) printf("The microwave is too fried to do that again.\n");
				oss->room = rLunch;
			} else {
				oss->flags |= FriedFork;
				if(show) printf("You secretly slip a fork into the microwave, and run it on high for fifteen minutes. The microwave fries, and a bolt of electricity hits Mrs. Douglas, who's hair catches on fire until a student grabs an extinguisher and puts it out.\n");
				oss->points++;
				oss->room = rEndLunch;
			}
			break;
		case 'e':
			if(show) printf("You make some new friends.\n");
			oss->friends++;
			oss->room = rEndLunch;
			break;
		case 'f':
			if(oss->flags & SeenGym) {
				if(show) printf("You should try something different.\n");
				oss->room = rLunch;
			} else {
				oss->flags |= SeenGym;
				oss->room = rGym;
			}
			break;
		case 'g':
			if(oss->flags & Explored) {
				if(show) printf("You've seen enough of the school for now.\n");
				oss->room = rLunch;
			} else {
				if(show) printf("You find a door leading to the basement of the school beside room 112. Using a paper clip to unlock it, you proceed underground.\n");
				oss->flags |= Explored;
				oss->room = rBasement;
			}
			break;
	}
}

static void Concession(struct Oss *oss, const char choice, const int show) {
	switch(choice) {
		case 0:
			OssMessage(oss, "Welcome to the concession:",
					"buy a pizza sub for $3.00;",
					"buy some pop for $1.00;",
					"buy a hot dog for $2.00;",
					"buy the mystery special for $0.25;",
					"steal some food;",
					"buy some food and give it to a starving grade-8; or",
					"push the food line to cause a domino effect.", (char *)0);
			break;
		case 'a':
			if(show) printf("The pizza sub had been left out too long and contained a deadly bacteria. You die.\n");
			oss->room = rNowhere;
			break;
		case 'b':
			if(show) printf("The pop is warm, but tastes okay.\n");
			oss->room = rEndLunch;
			break;
		case 'c':
			if(show) printf("Someone had accidentally spilt rat poison on the hot dogs. You eat it and die.\n");
			oss->room = rNowhere;
			break;
		case 'd':
			if(show) printf("You can't tell what the mystery special is, but it certainly is mysterious. You throw it in the garbage, deciding not to take any chances with your health.\n");
			oss->room = rEndLunch;
			break;
		case 'e':
			if(show) printf("You swipe some of the mystery special, eat it, and die from botulism.\n");
			oss->room = rNowhere;
			break;
		case 'f':
			if(show) printf("The grade-8 eats it and turns green. The paramedics arrive on scene minutes later and take him to the hospital unconscious.\n");
			oss->friends++;
			oss->room = rEndLunch;
			break;
		case 'g':
			if(show) printf("You manage to knock down half the kids standing around the concession with one push.\n");
			oss->points++;
			oss->room = rEndLunch;
			break;
	}
}

static void EndLunch(struct Oss *oss, const char choice, const int show) {
	switch(choice) {
		case 0:
			OssMessage(oss, "Lunch is over. You now decide to:",
					"go straight to your next class;",
					"skip your next class and go home;",
					"set the clock in the concourse back fifteen minutes; or",
					"set the school on fire.", (char *)0);
			break;
		case 'a':
			oss->room = rOutsideAgriculture;
			break;
		case 'b':
			if(show) printf("You go home.\n");
			oss->points++;
			oss->room = rNowhere;
			break;
		case 'c':
			if(oss->flags & ResetClock) {
				if(show) printf("You try again, but get caught by Mr. Coutu, who gives you a detention and sends you to class.\n");
				oss->points--;
				oss->trouble++;
				oss->room = rOutsideAgriculture;
			}
			else {
				if(show) printf("You get some other students to help you distract the supervisors and extend lunch by a few minutes.\n");
				oss->points++;
				oss->friends++;
				oss->flags |= ResetClock;
				oss->room = rLunch;
			}
			break;
		case 'd':
			if(show) printf("You manage to burn down a few rooms, but Mr. Saffik catches you and turns you over to the police. You spend the next few years of your life sharing a cell with 'Spike.'\n");
			oss->points--;
			oss->room = rNowhere;
			break;
	}
}

static void Basement(struct Oss *oss, const char choice, const int show) {
	switch(choice) {
		case 0:
			OssMessage(oss, "The basement is damp and dark. You have difficulty making out anything. You can hear breathing. You:",
					"get out of here;",
					"investigate the sound; or",
					"grab some files and run.", (char *)0);
			break;
		case 'a':
			if(show) printf("You manage to escape alive, going to your class.\n");
			oss->room = rAgriculture;
			break;
		case 'b':
			if(show) printf("You have a bad feeling about this. Just when you think that you've found the source, something hits you in the back of your head and you fall dead on the ground.\n");
			oss->points++;
			oss->room = rNowhere;
			break;
		case 'c':
			if(show) printf("You grab some of the school's secret files for future revision, stash them in a pocket, and run up to class.\n");
			oss->points++;
			oss->room = rAgriculture;
			break;
	}
}

static void Gym(struct Oss *oss, const char choice, const int show) {
	switch(choice) {
		case 0:
			OssMessage(oss, "You go outside and enter the gym/theater building. There you:",
					"scout the place out, then leave;",
					"play some basketball;",
					"pump iron;",
					"play some soccer outside;",
					"play some floor hockey; or",
					"jam the water fountain.", (char *)0);
			break;
		case 'a':
			if(show) printf("The place shows not evidence of anything interesting.\n");
			oss->room = rEndLunch;
			break;
		case 'b':
			if(show) printf("You play some basketball and make some friends.\n");
			oss->friends++;
			oss->room = rEndLunch;
			break;
		case 'c':
			if(show) printf("You use the weight room and get pumped up.\n");
			oss->points++;
			oss->room = rEndLunch;
			break;
		case 'd':
			if(show) printf("You play soccer and make some friends.\n");
			oss->friends++;
			oss->room = rEndLunch;
			break;
		case 'e':
			if(show) printf("You beat Mr. Becker's team, so, addition to making some friends, he agrees to bump up your math mark a bit.\n");
			oss->friends++;
			oss->education++;
			oss->room = rEndLunch;
			break;
		case 'f':
			if(show) printf("You shove some gum in the water fountain and flood the gym.\n");
			oss->points++;
			oss->room = rEndLunch;
			break;
	}
}

static void OutsideAgriculture(struct Oss *oss, const char choice, const int show) {
	switch(choice) {
		case 0:
			OssMessage(oss, "You are outside of your afternoon class, agriculture, in room 112:",
					"go into class;",
					"check out what's inside of that locked door beside the class; or",
					"pull the fire alarm by the door and run for it.", (char *)0);
			break;
		case 'a':
			oss->room = rAgriculture;
			break;
		case 'b':
			if(oss->flags & Explored) {
				if(show) printf("You don't want to go back down there.\n");
				oss->room = rOutsideAgriculture;
			} else {
				if(show) printf("The door leads to the basement of the school. Using a paper clip to unlock it, you proceed underground.\n");
				oss->flags |= Explored;
				oss->room = rBasement;
			}
			break;
		case 'c':
			if(oss->flags & FireAlarm) {
				if(show) printf("You pull the alarm again and some blue ink squirts on your face. You go into class looking like a moron.\n");
				oss->points--;
				oss->room = rAgriculture;
			} else {
				if(show) printf("You pull the alarm, but a piece just breaks off. You put it in your pocket to keep for later.\n");
				oss->flags |= FireAlarm;
				oss->points++;
				oss->room = rOutsideAgriculture;
			}
			break;

	}
}

static void Agriculture(struct Oss *oss, const char choice, const int show) {
	switch(choice) {
		case 0:
			OssMessage(oss, "You enter agriculture where a Mr. Huggins is showing the class how to grow substances of dubious legality. You:",
					"leave and hang out outside;",
					"grab some and run;",
					"socialize",
					"report him to the office; or",
					"pay attention.", (char *)0);
			break;
		case 'a':
			if(show) printf("You hang out outside for the class, but find nothing interesting.");
			oss->room = oss->trouble ? rDetentions : rEndSchool;
			break;
		case 'b':
			if(show) printf("Mr. Huggins doesn't even notice as you slip outside, but some other kids think that you're a weirdo.\n");
			oss->points++;
			oss->enemies++;
			oss->room = oss->trouble ? rDetentions : rEndSchool;
			break;
		case 'c':
			if(oss->flags & KnowOfFight) {
				if(show) printf("You chat and make some friends.\n");
				oss->friends++;
				oss->room = oss->trouble ? rDetentions : rEndSchool;
			}
			else {
				if(show) printf("You learn about a fight going down tonight at Jack Shaw Gardens.\n");
				oss->flags |= KnowOfFight;
				oss->room = oss->trouble ? rDetentions : rEndSchool;
			}
			break;
		case 'd':
			if(show) printf("The office staff don't seem to believe you. They think that you're making trouble and they give you a detention.\n");
			oss->trouble++;
			oss->room = oss->trouble ? rDetentions : rEndSchool;
			break;
		case 'e':
			if(show) printf("You pay attention and improve your grade a bit.\n");
			oss->education++;
			oss->room = oss->trouble ? rDetentions : rEndSchool;
			break;
	}
}

static void Detentions(struct Oss *oss, const char choice, const int show) {
	switch(choice) {
		case 0:
			OssMessage(oss, "The end of school is here, but it seems that you have gotten into a bit of trouble:",
					"attend your detentions; or",
					"skip the detentions.", (char *)0);
			break;
		case 'a':
			if(show) printf("You waste %i minutes at school in detention, then go home.\n", oss->trouble * detentionminutes);
			oss->trouble = 0;
			oss->room = rNowhere;
			break;
		case 'b':
			oss->room = rEndSchool;
			break;
	}
	return;
}

static void EndSchool(struct Oss *oss, const char choice, const int show) {
	if(oss->flags & KnowOfFight) {
		switch(choice) {
			case 0:
				OssMessage(oss, "School's out. You decide to:",
						"go to Jack Shaw Gardens;",
						"take the bus home;",
						"bum a ride with someone;",
						"stay and study a while; or",
						"walk home.", (char *)0);
				return;
			case 'a':
				oss->points++;
				oss->room = rJackShaw;
				break;
			case 'b':
				if(show) printf("You take the bus home.\n");
				oss->room = rNowhere;
				break;
			case 'c':
				if(show) printf("You get a ride, but the car you're in has no brakes and it careens over the side of the cliff by the school. It then takes you hours to get home.\n");
				oss->points--;
				oss->room = rNowhere;
				break;
			case 'd':
				if(show) printf("You study and improve your grade, but waste your evening.\n");
				oss->points--;
				oss->education++;
				oss->room = rNowhere;
				break;
			case 'e':
				if(show) printf("You walk home.\n");
				oss->room = rNowhere;
				break;
		}
	} else {
		switch(choice) {
			case 0:
				OssMessage(oss, "School's out. You decide to:",
						"take the bus home;",
						"bum a ride with someone;",
						"stay and study a while; or",
						"walk home.", (char *)0);
				return;
			case 'a':
				if(show) printf("You take the bus home.\n");
				oss->room = rNowhere;
				break;
			case 'b':
				if(show) printf("You get a ride, but the car you're in has no brakes and it careens over the side of the cliff by the school. It then takes you hours to get home.\n");
				oss->points--;
				oss->room = rNowhere;
				break;
			case 'c':
				if(show) printf("You study and improve your grade, but waste your evening.\n");
				oss->points--;
				oss->education++;
				oss->room = rNowhere;
				break;
			case 'd':
				if(show) printf("You walk home.\n");
				oss->room = rNowhere;
				break;
		}
	}
}

static void JackShaw(struct Oss *oss, const char choice, const int show) {
	switch(choice) {
		case 0:
			OssMessage(oss, "You arrive on scene to find that the combatants are none other than Mr. Schulting and Mr. Dogopole, fighting over who knows more about networking. Mr. Dogopole has the upper hand until Mr. Schulting whips out an old hard drive and smashes it on Mr. Dogopole's head. You:",
					"get out of here now;",
					"watch the fight and leave before the cops arrive;",
					"watch the fight and run away when the cops arrive;",
					"jump in and help Mr. Schulting;",
					"jump in and help Mr. Dogopole;",
					"jump in and fight both of them at the same time; or",
					"try to even the odds by passing Mr. Dogopole a big stick.", (char *)0);
			break;
		case 'a':
			if(show) printf("You wisely return home.\n");
			oss->room = rNowhere;
			break;
		case 'b':
			if(show) printf("Mr. Schulting has the upper hand when you hear sirens in the distance. Prudently, you decide to make your exit.\n");
			oss->points++;
			oss->room = rNowhere;
			break;
		case 'c':
			if(show) printf("Mr. Schulting has the upper hand when you hear sirens in the distance, but you stick around to watch what happens.\n");
			oss->points++;
			oss->room = rCops;
			break;
		case 'd':
			oss->room = rJackShawSchulting;
			break;
		case 'e':
			oss->room = rJackShawDogopole;
			break;
		case 'f':
			if(show) printf("Jumping into the fight, you start swinging at both teachers. Everyone else, following your example, starts beating on each other in a massive free-for-all.\n");
			oss->points++;
			oss->room = rCops;
			break;
		case 'g':
			if(show) printf("You grab a branch and pass it to Mr. Dogopole, who catches it it whips it around in a ninja move, hitting Mr. Schulting in the arm. Mr Schulting then hits Mr. Dogopole with his hard drive. The fight escalates, and everyone becomes more interested.\n");
			oss->points++;
			oss->friends++;
			oss->room = rCops;
			break;
	}
}

static void JackShawDogopole(struct Oss *oss, const char choice, const int show) {
	switch(choice) {
		case 0:
			OssMessage(oss, "You jump out, prepared to defend the weakened Mr. Dogopole. Mr. Schulting is surprised:",
					"go for the head;",
					"tackle him;",
					"stand there and block him; or",
					"punch him in the stomach.", (char *)0);
			return;
		case 'a':
			if(show) printf("You hit Mr. Schulting in the head and he goes down. Everyone disperses, slightly disappointed that you ended the fight. Mr. Dogopole, however, says that he'll give you some nice grades when you go to his class.\n");
			oss->points--;
			oss->friends++;
			oss->enemies += 2;
			oss->education++;
			oss->room = rNowhere;
			break;
		case 'b':
			if(show) printf("Mr. Schulting just throws you off to the side and keeps fighting.\n");
			oss->points--;
			oss->room = rCops;
			break;
		case 'c':
			if(show) printf("Mr. Schulting clobbers you on the head with the hard drive. You wake up the next morning in the hospital.\n");
			oss->points--;
			oss->enemies += 2;
			oss->room = rNowhere;
			break;
		case 'd':
			if(show) printf("Mr. Schulting blocks your punch and nails you in the stomach. You wake up the next morning in a daze, alone on the ground at Jack Shaw.\n");
			oss->points--;
			oss->friends++;
			oss->enemies++;
			oss->room = rNowhere;
			break;
	}
	return;
}

static void JackShawSchulting(struct Oss *oss, const char choice, const int show) {
	switch(choice) {
		case 0:
			OssMessage(oss, "You jump out, prepared to attack the weakened Mr. Dogopole:",
					"go for the head;",
					"tackle him;",
					"stand there and block him; or",
					"punch him in the stomach.", (char *)0);
			return;
		case 'a':
			if(show) printf("Whoops, Mr. Dogopole avoids your punch and nails you in the head. You wake up the next morning in a daze, alone on the ground at Jack Shaw.\n");
			oss->points--;
			oss->friends++;
			oss->enemies++;
			oss->room = rNowhere;
			break;
		case 'b':
			if(show) printf("Mr. Dogopole just throws you off to the side and keeps fighting.\n");
			oss->points--;
			oss->room = rCops;
			break;
		case 'c':
			if(show) printf("Mr. Schulting, not realizing your intentions, clobbers you on the head. You wake up the next morning in the hospital.\n");
			oss->points--;
			oss->enemies += 2;
			oss->room = rNowhere;
			break;
		case 'd':
			if(show) printf("You hit Mr. Dogopole in the stomach and he goes down. Everyone disperses, disappointed that you ended the fight. Mr. Schulting, however, says that he'll give you some nice grades when you go to his class.\n");
			oss->points--;
			oss->friends++;
			oss->enemies += 3;
			oss->education++;
			oss->room = rNowhere;
			break;
	}
	return;
}

static void Cops(struct Oss *oss, const char choice, const int show) {
	switch(choice) {
		case 0:
			OssMessage(oss, "The sound of sirens approaches, and the cops arrive on scene. You:",
					"run;",
					"walk innocently away, pretending not to see the fight;",
					"pretend that you don't see them;",
					"try to grab a nightstick from one of the cops;",
					"try to get the cops involved in a massive brawl; or",
					"warn everyone else and try to escape as a group.", (char *)0);
			break;
		case 'a':
			if(show) printf("You run away, but a cop grabs and you spend the rest of your evening answering questions at the police department.\n");
			oss->points--;
			oss->room = rNowhere;
			break;
		case 'b':
			if(show) printf("You walk off carefully, and manage to escape the scene without getting caught.\n");
			oss->room = rNowhere;
			break;
		case 'c':
			if(show) printf("You just stay where you are. The cops break up the fight and send everyone home.\n");
			oss->room = rNowhere;
			break;
		case 'd':
			if(show) printf("The cop was too alert, and used the nightstick on you. You wake up the next day in a cell, but you are soon released.\n");
			oss->room = rNowhere;
			break;
		case 'e':
			if(show) printf("The rest of the spectators join you, and so do the police. A massive brawl ensues. You become famous for starting such an awesome fight, though you are badly bruised the next morning.\n");
			oss->points += 2;
			oss->friends++;
			oss->room = rNowhere;
			break;
		case 'f':
			if(show) printf("You manage to warn a small group of spectators. You all manage to escape into the bushes, making you a few new friends.\n");
			oss->points++;
			oss->friends += 2;
			oss->room = rNowhere;
			break;
	}
}
