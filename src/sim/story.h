#pragma once

#include "boundary.h"

#include <filesystem>

namespace odysseus::sim {

// Numbers of the story engine (M2b), from assets/data/sim/story.json, one section per kind
// of interaction. Whole numbers only (Charter rule 6); percentages are 0..100.

// A failed harvest now and then gives the clan a hard winter to live through (US-110).
struct SeasonStory {
    int leanAutumnPercent = 20; // chance each autumn that the harvest fails
    int leanForagePercent = 45; // the land then gives this share of its usual autumn food
};

// How far back an event looks for its causes, and how much of it a person remembers (US-110).
struct CauseStory {
    int theftWindowDays = 20;       // thefts this recent are blamed for an empty store
    int starvationWindowDays = 60;  // an empty store this recent is blamed for a death by hunger
    int maxCauses = 4;              // an event lists at most this many causes
    int grudgeLimit = 16;           // a person holds at most this many grudges
};

// Every morning each person meets someone; people who dislike each other may quarrel (US-111).
struct QuarrelStory {
    int grudgeMeetPercent = 40;   // chance that the one they meet is someone they hold a grudge against
    int dislikeOpinion = -10;     // both must think this badly of each other or worse
    int irritableBelow = 60;      // Hunger or Energy below this makes a person short-tempered
    int irritablePercent = 40;    // chance of a quarrel when short-tempered
    int calmPercent = 6;          // chance of a quarrel otherwise
    int opinionLoss = 12;         // each thinks this much less of the other
    int feeling = -40;            // how badly each remembers it
};

// Grief looks for someone to blame (US-111).
struct BlameStory {
    int opinionLoss = 40;         // the griever thinks this much less of the one blamed
    int feeling = -70;            // a memory kept for life
};

// A feud that keeps worsening ends in a fight, or the clan drives the aggressor out (US-111).
struct RevengeStory {
    int minFeudDays = 10;         // a feud must be this old
    int revengeOpinion = -60;     // and one side must think this badly of the other or worse
    int percentPerDay = 8;        // chance per day that revenge is taken
    int cooldownDays = 60;        // days before the same feud can lead to revenge again
    int exileClanOpinion = -25;   // the clan exiles an aggressor it thinks this badly of (on average)
    int fightDeathPercent = 20;   // chance the loser of a fight dies
    int winnerHurtPercent = 30;   // chance the winner is hurt too
    int satisfaction = 30;        // the aggressor thinks better of the victim after taking revenge
    int victimOpinionLoss = 30;   // the victim thinks worse of the aggressor
    int feeling = -80;            // the victim's memory of the attack
};

// Wounds (US-111) and, later, sickness (US-112).
struct HealthStory {
    int woundDaysMin = 5;         // an injury lasts this many days at least...
    int woundDaysMax = 12;        // ...and at most this many
    int woundDeathPerMille = 10;  // chance per day, in thousandths, that a wound kills
};

// Sickness strikes the weak (US-112). Wounds are told under HealthStory.
struct SicknessStory {
    int basePerMille = 1;         // chance a day, in thousandths, that anyone falls sick
    int hungerBelow = 25;         // Hunger this low weakens a person...
    int hungerPerMille = 25;      // ...and adds this to the chance
    int coldBelow = 25;           // so does Warmth this low
    int coldPerMille = 25;
    int daysMin = 3;              // a sickness lasts this many days at least...
    int daysMax = 9;              // ...and at most this many
    int deathPerMille = 12;       // chance a day that a sickness kills
    int huntWoundPerMille = 1;    // chance per hour of hunting that a hunter is hurt
};

// A kind or close person nurses the sick and hurt (US-112).
struct NursingStory {
    int minScore = 30;            // opinion + bonuses a carer needs to come forward
    int kinBonus = 40;            // partner, parent, child, sibling, guardian
    int kindBonus = 30;           // a Kind carer
    int percent = 80;             // chance a day that the best carer comes
    int extraHealPerDay = 1;      // a nursed patient mends this many days faster each day
    int deathPercentWhenNursed = 40; // the death chance is only this share of the usual one
    int opinionGain = 20;         // the patient thinks better of the carer
    int carerOpinionGain = 5;     // and the carer of the patient
    int feeling = 60;             // the patient's memory of it: gratitude
};

// Sharing food in a famine (US-112).
struct SharingStory {
    int giverHungerMin = 40;      // only someone this well fed can spare food...
    int gapMin = 20;              // ...and only for someone at least this much hungrier
    int receiverHungerMax = 35;   // only someone this hungry is helped
    int amount = 15;              // Hunger moved from giver to receiver
    int repeatDays = 30;          // the same giver feeding the same person again within this many days is not news
    int minScore = 20;            // opinion + bonuses a giver needs
    int kinBonus = 30;
    int kindBonus = 20;
    int childBonus = 30;          // for a child who cannot work yet
    int opinionGain = 15;
    int feeling = 50;
};

// Orphans are taken in (US-112).
struct AdoptionStory {
    int minScore = 20;            // opinion + bonuses an adopter needs
    int kinBonus = 50;            // a brother or sister
    int kindBonus = 20;
    int limit = 2;                // children a person takes in at most
    int opinionGain = 30;         // both ways
    int feeling = 80;             // the child's memory of being taken in
};

// An unpaired adult courts the one they like best (US-113).
struct CourtshipStory {
    int favourOpinion = 20;        // they court someone they like at least this much
    int opinionPerDay = 2;         // gifts and time together: the loved one thinks this much better of the suitor each day
    int suitorOpinionPerDay = 1;   // and the suitor a little better of the loved one
    int rejectBelow = 0;           // a loved one who thinks worse of the suitor than this turns them down at once
    int giveUpDays = 40;           // a suitor who has not won them by now gives up
    int rejectionOpinionLoss = 15; // the turned-down suitor thinks this much less of the loved one
    int rejectionFeeling = -50;    // and remembers it
    int pauseDays = 30;            // a broken heart waits this long before courting again
};

// The suitors who lose (US-113).
struct RivalStory {
    int minCourtDays = 2;          // a suitor who has courted this long is hurt when another wins
    int opinionLoss = 25;          // they think this much worse of the winner
    int feeling = -60;             // and remember being passed over for life
    int quarrelPercent = 50;       // chance that rival and winner quarrel at once
    int pauseDays = 30;
};

// Partners fall out of love (US-113).
struct PartingStory {
    int partingOpinion = -20;      // a partner who thinks less than this of the other leaves
    int pauseDays = 30;            // both wait this long before courting again
};

// Masters teach youths (US-114).
struct TeachingStory {
    int masterMinSkill = 30;       // a master knows at least this much
    int skillGap = 15;             // and is at least this much better than the youth
    int takePercent = 20;          // chance a day that a master takes an apprentice
    int minOpinion = -10;          // master and youth think at least this well of each other
    int kinBonus = 40;             // a relative is likelier to be taught
    int youthMaxYears = 15;        // youths from the working age (actions.json) up to this age are taught
    int skillPerDay = 1;           // the apprentice's skill grows this much a day
    int opinionPerDay = 2;         // and they grow close
    int graduateGap = 5;           // graduation: within this many points of the master
};

// Hunting parties, roles, danger and rescue (US-114).
struct HuntStory {
    int minSize = 3;               // fewer hunters than this and the sighter hunts alone
    int maxSize = 5;
    int joinPercent = 70;          // chance a hunter joins a party...
    int braveJoinBonus = 20;       // ...more if Brave...
    int timidJoinPenalty = 30;     // ...less if Timid
    int successBase = 20;          // percent chance of success before the party's strength is added
    int leaderBonus = 10;
    int heroBonus = 15;
    int cowardPenalty = 15;
    int heroCourage = 55;          // the most courageous is the hero if at least this
    int cowardCourage = 30;        // the least courageous is the coward if at most this
    int dangerPercent = 25;        // chance a member (not the coward) is in danger
    int rescuePercent = 70;        // chance the best rescuer saves them...
    int braveRescueBonus = 15;     // ...more if Brave
    int rescuerHurtPercent = 30;
    int dangerDeathPercent = 60;   // an unsaved member dies with this chance, else is hurt
    int heroFeeling = 60;          // what the others feel about the hero
    int heroOpinion = 10;
    int cowardFeeling = -50;
    int cowardOpinionLoss = 15;
    int rescueFeeling = 90;        // the rescued one's gratitude
    int rescueOpinion = 40;
};

struct StoryConfig {
    SeasonStory season;
    CauseStory causes;
    QuarrelStory quarrel;
    BlameStory blame;
    RevengeStory revenge;
    HealthStory health;
    SicknessStory sickness;
    NursingStory nursing;
    SharingStory sharing;
    AdoptionStory adoption;
    CourtshipStory courtship;
    RivalStory rivals;
    PartingStory parting;
    TeachingStory teaching;
    HuntStory hunt;
};

StoryConfig loadStoryConfig(const std::filesystem::path& file);

} // namespace odysseus::sim
