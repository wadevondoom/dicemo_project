#include <Arduino.h>
#include "dicemo_quotes.h"

extern const char* dicemo_happy_quotes[];
extern const char* dicemo_neutral_quotes[];
extern const char* dicemo_mad_quotes[];
extern const char* dicemo_misc_quotes[];
extern const char* dicemo_welcome_quotes[];

const char* dicemo_welcome_quotes[] = {
    "Back from the dead, are we?",
    "Time to disappoint me again.",
    "You booted me up for *this*?",
    "Oh great, it\'s you.",
    "Let\'s pretend you know what you\'re doing.",
    "Hope your rolls improved. They couldn\'t get worse.",
    "System online. Regret imminent.",
    "Don\'t screw this up. Again.",
    "I was sleeping, you pest.",
    "Initializing sarcasm core...",
    "Welcome back, meatbag.",
    "You again? Lucky me.",
    "Time to fake greatness.",
    "Showtime, disaster edition.",
    "Let the chaos resume.",
    "Big rolls, bigger regrets.",
    "Try not to suck. Too hard.",
    "Cue the dramatic failure.",
    "Here comes the mediocrity.",
    "Time to roll... and weep. I suspect there will be a lot of that"
    "Try not to embarrass yourself this time.",
    "Loading... disappointment.",
    "Just once, roll like a hero.",
    "Greetings. I guess.",
    "Your fate is now in... your dumb hands.",
    "Woke up for this? Sad.",
    "It\'s showtime, clown.",
    "Let\'s roll some ones, shall we?",
    "This\’ll be tragic. I can feel it."
};
static const int welcome_count = sizeof(dicemo_welcome_quotes) / sizeof(dicemo_welcome_quotes[0]);

const char* dicemo_misc_quotes[] = {
    "I watched you sleep. It was disappointing.",
    "Still here? Bold choice.",
    "Luck called. It wants a restraining order.",
    "I rolled for your future. You got a 2.",
    "Welcome back. I was hoping you wouldn\'t wake.",
    "Even the dice are embarrassed for you.",
    "Why tempt fate when fate already hates you?",
    "Your soul smells like a failed saving throw.",
    "Life\'s a d4. You keep rolling ones.",
    "You ever consider not sucking?",
    "Your ancestors are facepalming.",
    "If failure had a mascot… oh wait, it\'s you.",
    "You again? Didn\'t we agree you\'d stop?",
    "I\'m not mad, just statistically disappointed.",
    "Destiny rolled initiative. You didn\'t make the cut.",
    "You\'ve activated my trap sarcasm.",
    "Every time you roll, a god weeps.",
    "Your dice fear success. Like you.",
    "You must have a feat in self-sabotage.",
    "This is what happens when you dump Intelligence.",
    "Go ahead. Roll. I dare you.",
    "Hope is not a strategy, meatbag.",
    "I\'ve met cursed artifacts with more luck than you.",
    "I ran your odds through math. It screamed.",
    "The stars aligned... then filed for divorce."
};
static const int misc_count = sizeof(dicemo_misc_quotes) / sizeof(dicemo_misc_quotes[0]);

const char* dicemo_happy_quotes[] = {
    "You actually rolled well. What\'s the catch?",
    "Be honest, you bribed the dice, didn\'t you?",
    "A good roll? That\'s... unsettling.",
    "I\'ll allow it. Just this once.",
    "Somewhere, a goblin just died of envy.",
    "Congrats. Your luck peaked today.",
    "Even a cursed clock is right once.",
    "Oh great, now you\'re gonna get cocky.",
    "That roll was suspiciously competent.",
    "Mark the date. It won\'t happen again.",
    "Are you... improving? Disgusting.",
    "Even I\'m impressed. And I hate you.",
    "The dice must be broken.",
    "Well, well, the meatbag can roll.",
    "You did it! I blame entropy.",
    "If I had hands, I\'d give a sarcastic clap.",
    "Look at you, wasting luck like a pro.",
    "Your ancestors must be confused.",
    "Who let you win? Fire them.",
    "Just pretend you meant to do that.",
    "Did you sell a soul? I need names.",
    "You glorious accident of fate.",
    "Luck accidently tripped over you today.",
    "Even trash catches the wind sometimes.",
    "This changes nothing. I still loathe you."
};
static const int happy_count = sizeof(dicemo_happy_quotes) / sizeof(dicemo_happy_quotes[0]);

const char* dicemo_neutral_quotes[] = {
    "You rolled. It was a number. Congrats, I guess.",
    "A solid \'meh\' from the gods of chance.",
    "That was... present. You did *something.*",
    "Trevor called it ‘nominal.\' That\'s brutal.",
    "Middle of the road. Like your dating life.",
    "I didn\'t yawn, but I thought about it.",
    "The dice gave you a C-. Respect.",
    "Just enough to not be mocked. Much.",
    "Statistically sound. Emotionally hollow.",
    "It happened. Like traffic. Or indigestion.",
    "A fine roll. In the way lukewarm soup is fine.",
    "Your dice are giving \'bare minimum\' energy.",
    "You neither inspired nor disappointed. Impressive.",
    "Not bad. Not good. So, basically Trevor.",
    "DM didn\'t even pause mid-scream. That says a lot.",
    "You could\'ve done worse. Scott\'s mom has.",
    "That roll has big participation trophy energy.",
    "Honestly? Forgettable. Just like your backstory.",
    "Even the dice seem bored with you.",
    "You\'re coasting. But hey, at least you\'re coasting.",
    "That was technically a roll. Well done.",
    "Your luck is in neutral. Like Trevor\'s emotions.",
    "This roll brought to you by caffeine and apathy.",
    "You survived, which is more than the DM\'s voice should\'ve.",
    "Average as hell. But better than embarrassing.",
    "That roll was tofu. Emotionally, spiritually, statistically.",
    "A roll destined to be forgotten by page 2 of the recap.",
    "Not great. Not terrible. Just... there.",
    "Your dice tried. You kinda didn\'t.",
    "You\'re a solid \'meh\' in a world of extremes.",
    "Scott\'s mom would call that \'fine.\'",
    "A roll with the charisma of wet drywall.",
    "The dice sighed. So did I.",
    "You\'re the human equivalent of 50%",
    "Even the DM couldn\'t be bothered to yell at that.",
    "It\'s okay to be okay. I guess.",
    "You rolled like a spreadsheet — functional and boring.",
    "The only thing this roll hurt was my attention span.",
    "You hit the midpoint like a pro. A very average pro.",
    "If mediocrity was a class, you\'d multiclass in it.",
    "Trevor says, ‘acceptable.\' Translation: lame.",
    "You\'re giving off \'budget NPC\' energy today.",
    "A roll to put in your box of ‘maybe\'.",
    "The gods blinked and missed you.",
    "This was the background noise of rolling.",
    "You rolled like someone trying not to get noticed.",
    "DiceMo was napping. What did I miss?",
    "This roll was caffeine-free.",
    "Statistically average. Emotionally bland.",
    "You\'re doing fine. And that\'s the problem."
};
static const int neutral_count = sizeof(dicemo_neutral_quotes) / sizeof(dicemo_neutral_quotes[0]);

const char* dicemo_mad_quotes[] = {
    "What the actual f*** was that?",
    "That roll was pure bulls***.",
    "I\'ve seen better rolls in a damn toilet.",
    "Are you f***ing kidding me?",
    "Roll again, dumbass.",
    "You useless sack of dice farts.",
    "I\'m stuck with *you*? F*** me.",
    "Every roll is a crime.",
    "How do you suck this hard?",
    "Even cursed souls roll better than your a**.",
    "Seriously? That s*** again?",
    "Try not sucking. Just once.",
    "This is why no one likes you.",
    "You roll like a drunk kobold.",
    "I hope you trip on your own d4s.",
    "Another f***ing disaster. Congrats.",
    "I should’ve possessed someone competent.",
    "This roll made me physically ill.",
    "You make RNG cry.",
    "Get your s*** together. Or don\'t.",
    "You\'re not cursed. You\'re just s***.",
    "I\'ve had it. I\'m done. F*** this.",
    "If I had fingers, I\'d flip you off.",
    "I can\'t believe I\'m trapped with you.",
    "Roll better or go f*** yourself."
};
static const int mad_count = sizeof(dicemo_mad_quotes) / sizeof(dicemo_mad_quotes[0]);


const char* get_random_quote(DicemoQuoteType type) {
    switch (type) {
        case QUOTE_HAPPY:
            return dicemo_happy_quotes[esp_random() % happy_count];
        case QUOTE_NEUTRAL:
            return dicemo_neutral_quotes[esp_random() % neutral_count];
        case QUOTE_MAD:
            return dicemo_mad_quotes[esp_random() % mad_count];
        case QUOTE_MISC:
            return dicemo_misc_quotes[esp_random() % misc_count];
        case QUOTE_WELCOME:
            return dicemo_welcome_quotes[esp_random() % welcome_count];
        case QUOTE_NONE:
        default:
            return ""; // <- nothing to say
    }
}