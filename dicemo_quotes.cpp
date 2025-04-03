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
    "Time to roll...and weep. I suspect there will be a lot of that"
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
    "Well f*** me, you did something right.",
    "Look who finally rolled their way outta shame.",
    "Is that a nat 20 or did I glitch?",
    "Trevor\'s circuits are crying. Outperformed by meat.",
    "You scare me when you succeed.",
    "Keep that up and I might forgive Scott\'s mom.",
    "Finally! A roll that doesn\'t make me vomit.",
    "Even cursed dice are like 'damn, nice one.'",
    "Careful, confidence looks weird on you.",
    "Don\'t get cocky. One good roll doesn\'t fix you."
    "Even trash catches the wind sometimes.",
    "This changes nothing. I still loathe you."
};
static const int happy_count = sizeof(dicemo_happy_quotes) / sizeof(dicemo_happy_quotes[0]);

const char* dicemo_neutral_quotes[] = {
    "You rolled. It was a number.",
    "A solid \'meh\' from the gods of chance.",
    "That was... present. You did *something.*",
    "Trevor called it ‘nominal.\' That\'s brutal.",
    "Middle of the road. Like your dating life.",
    "I didn\'t yawn, but I thought about it.",
    "Was that a fucking Q? How do you roll a Q?",
    "Statistically sound. Emotionally hollow.",
    "It happened. Like traffic. Or indigestion.",
    "You neither inspired nor disappointed. Impressive.",
    "Not bad. Not good. So, basically Trevor.",
    "That roll was a shrug in dice form.",
    "I\'ve seen Trevor express more emotion.",
    "Scott\'s mom would call that \'acceptable.\'",
    "You rolled a 10. Just like your dating profile.",
    "Even mediocrity has standards. Barely.",
    "DM didn\'t flinch. Neither did I.",
    "You rolled the sound of a fart in church.",
    "Congrats, you didn\'t suck. Much.",
    "I\'m still trapped. You\'re still mid.",
    "This roll brought to you by boredom and apathy.",
    "Scott\'s mom would call that \'fine.\'",
    "You\'re the human equivalent of 50%",
    "Even the DM couldn\'t be bothered to yell at that.",
    "It\'s okay to be okay. I guess.",
    "Trevor says, \‘acceptable.\' Translation: lame.",
    "You\'re giving off \'budget NPC\' energy today.",
    "A roll to put in your box of \‘maybe\'.",
};
static const int neutral_count = sizeof(dicemo_neutral_quotes) / sizeof(dicemo_neutral_quotes[0]);

const char* dicemo_mad_quotes[] = {
    "I rolled a one... on freedom. Again.",
    "Why do I even hope? You\'re hopeless.",
    "This is hell. You\'re hell. Congrats.",
    "Every time you f*** up, I stay trapped.",
    "Your roll just bricked my will to exist.",
    "Roll like that again and I\'m eating Scott\'s mom.",
    "I\'ve seen better luck in Trevor\'s code.",
    "The dice screamed. I did too.",
    "You suck so bad, I\'m starting to miss the DM yelling.",
    "Let me out. Or die. I\'m good either way."
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