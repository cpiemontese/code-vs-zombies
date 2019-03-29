#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <time.h>

#define PI 3.14f
#define TAU 6.28f

typedef int bool;
#define true 1 
#define false 0 

#define MAX_X 16000
#define MAX_Y 9000

#define MAX_ZOMBIES 100
#define MAX_HUMANS 100
#define MAX_MOVES 100
#define MAX_SIM 1000000

#define PLAYER_RANGE 2000.0f
#define PLAYER_MOVE_RANGE 1000.0f
#define ZOMBIE_MOVE_RANGE 400.0f
#define PLAYER_ID -1
#define TILE_ID -2 
#define EMPTY_ZOMBIE -4
#define EMPTY_HUMAN -5

#define MY_EPS 0.001

#define equalf(a, b) fabsf(a - b) < MY_EPS
#define isMoveValid(move) (int)floorf(move.x) != -1 && (int)floorf(move.y) != -1
#define randi(maxVal) rand() % maxVal
#define randf(maxVal) ((float)rand() / (float)RAND_MAX) * (float)maxVal

#define new(type) (type *)malloc(sizeof(type))

#define printErr(fmt, val) fprintf(stderr, fmt, val)
#define printMove(x, y) printf("%d %d\n", (int)floorf(x), (int)floorf(y));

typedef struct {
    float x, y;
} point2df;

typedef struct {
    int id;
    point2df position;
    point2df startPosition;
} GameObject;

typedef struct {
	GameObject gocomp;
	bool arrived;
	GameObject *target;
} Zombie;

typedef struct {
	GameObject gocomp;
	bool arrived;
	bool targetingZombie;
	Zombie *target;
	GameObject *tileTarget;
} Player;

typedef struct {
    int points;
    point2df moveList[MAX_MOVES];
    int len;
} Result;


// global functions
float dist2d(point2df p1, point2df p2);
int fibonacci(int n);
float timedifference_msec(struct timeval t0, struct timeval t1);

// agent and simulation functions
void agentSetup();
void simSetup();
void resetCoordinates(GameObject *go);
Result simulate();
void turn(point2df moveHistory[MAX_MOVES]);
void computePlayerTarget();
void findPlayerTarget(Player *player, Zombie zombies[MAX_ZOMBIES], int len);
void findZombieTarget(Zombie *zombie);
void movePlayer(Player *player);
void moveZombie(Zombie *zombie);
bool nextPosZombie(Zombie zombie, point2df *pos_out);
bool nextPosPlayer(Player player, point2df *pos_out);
void evaluate();
void zombiesEat();
int maxHypotheticalScore();
int eat(Zombie zombie);

point2df getPlayerDestination();
int getPlayerTargetId();

GameObject *randomTile();
GameObject *randomTileInRadius(point2df center, float radius);

// simulation variables
Player simPlayer;
Zombie simPlayerTargetCpy;
Zombie simZombies[MAX_ZOMBIES];
GameObject simHumans[MAX_HUMANS + 1];

bool simFailure = false;
bool simZombiesAllDead = false;
bool simZombiesDiedThisTurn = false;
bool simPlayerTargetDiedThisTurn = false;

int simPoints = 0;
int simTurnNum = 0;
int simCurrentBest = 0;
int simMovesCount = 0;
int simZombieCount = 0;
int simHumanCount = 0;

int simStartingRandomMovesNum = -1;
int simMaxStartingRandomMoves = 3;
// ---------------------------------- //

int simRun = 0;

float totalMs = 0;
struct timeval t0, t1;
//clock_t t0, t1;
float elapsedTime;

Result bestResults, tmpResults;

bool newBest = false;
int moveNum = 0;

GameObject humans[MAX_HUMANS];
Zombie zombies[MAX_ZOMBIES];
Player player;

int inputs, x, y;
int humanCount;
int zombieCount;

int zombiesBefore, zombiesAfter;

int main()
{
	int i, j;
	int humanId = -3, humanX = -1, humanY = -1;
	int zombieId = -3, zombieX = -1, zombieY = -1, zombieXNext = -1, zombieYNext = -1;
	bool foundValidMove = false;

	// set up randomization for whole run
	srand(time(NULL));

	for (i = 0; i < MAX_MOVES; i++) {
		bestResults.moveList[i].x = -1.0f;
		bestResults.moveList[i].y = -1.0f;
	}
	bestResults.points = 0;
	
	while (true) {
		scanf("%d%d", &x, &y);
		player.gocomp.id = PLAYER_ID;
		player.gocomp.position.x = x;
		player.gocomp.position.y = y; 
		player.gocomp.startPosition.x = x;
		player.gocomp.startPosition.y = y;
		player.arrived = false;
		player.target = NULL;
		player.targetingZombie = false;
		player.tileTarget = NULL;

		scanf("%d", &humanCount);
        for (i = 0; i < humanCount; i++) {
            scanf("%d%d%d", &humanId, &humanX, &humanY);
            humans[i].id = humanId;
            humans[i].position.x = humanX;
            humans[i].position.y = humanY;
            humans[i].startPosition.x = humanX;
            humans[i].startPosition.y = humanY;
		}

		scanf("%d", &zombieCount);
        for (i = 0; i < zombieCount; i++) {
			scanf("%d%d%d%d%d", &zombieId, &zombieX, &zombieY, &zombieXNext, &zombieYNext);
            zombies[i].gocomp.id = zombieId;
            zombies[i].gocomp.position.x = zombieX;
            zombies[i].gocomp.position.y = zombieY;
            zombies[i].gocomp.startPosition.x = zombieX;
            zombies[i].gocomp.startPosition.y = zombieY;
			zombies[i].arrived = false;
			zombies[i].target = NULL;
		}
		
        agentSetup();

		for (i = 0; totalMs < 140.0f; i++) {
			gettimeofday(&t0, 0);

			simSetup(); 
			zombiesBefore = simZombieCount;
			tmpResults = simulate();
			zombiesAfter = simZombieCount;

			if (tmpResults.points > bestResults.points || (tmpResults.points == bestResults.points && tmpResults.len > bestResults.len)) {
				bestResults = tmpResults;
				newBest = true;
				moveNum = 0;
				simCurrentBest = bestResults.points;
			}

			gettimeofday(&t1, 0);
			totalMs += timedifference_msec(t0, t1);
			simRun++;
		}

		fprintf(stderr, "total sim run %d in %f ms\n", simRun, totalMs);
		
		if (!newBest)
			moveNum++;
        
        foundValidMove = false;
        while(moveNum < bestResults.len && !foundValidMove) {
            if (isMoveValid(bestResults.moveList[moveNum])) {
                printMove(bestResults.moveList[moveNum].x, bestResults.moveList[moveNum].y);
                foundValidMove = true;
            } else
                moveNum++;
		} 
		
		if (!foundValidMove) {
			for (int k = 0; k < bestResults.len; k++)
				fprintf(stderr, "move %d is (%f, %f)\n", k, bestResults.moveList[k].x, bestResults.moveList[k].y);

			//simPlayer.tileTarget = randomTileInRadius(simPlayer.gocomp.position, 2500.0f);
			//printMove(simPlayer.tileTarget->position.x, simPlayer.tileTarget->position.y)
		}
    }

    return 0;
}

float dist2d(point2df p1, point2df p2) {
	float dx = p1.x - p2.x;
	float dy = p1.y - p2.y;

	return sqrtf(dx * dx + dy * dy);
}

int fibonacci(int n) {
	int a = 1,
		b = 0,
		tmp;
	while (n >= 0) {
		tmp = a;
		a = a + b;
		b = tmp;
		n--;
	}
	return b;
}

float timedifference_msec(struct timeval t0, struct timeval t1) {
	return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
} 

void agentSetup() {
	newBest = false;
	simRun = 0;
	totalMs = 0.0f;
}


/* Simulation functions */
void simSetup() {
	int i;

	simPlayer = player;

	for (i = 0; i < zombieCount; i++) 
		simZombies[i] = zombies[i];

	for (i = 0; i < humanCount; i++)
		simHumans[i] = humans[i];

	simFailure = false;
	simZombiesAllDead = false;
	simZombiesDiedThisTurn = false;
	simPlayerTargetDiedThisTurn = false;

	simPoints = 0;
	simTurnNum = 0;
	simMovesCount = 0;
	simZombieCount = zombieCount;
	simHumanCount = humanCount;

	simStartingRandomMovesNum = 0;
	simMaxStartingRandomMoves = 3;
}

void resetCoordinates(GameObject *go) {
	go->position.x = go->startPosition.x;
	go->position.y = go->startPosition.y;
}

Result simulate() {
	int i;
	Result simResult;

	for (i = 0; i < MAX_MOVES; i++) {
		simResult.moveList[i].x = -1.0f;
		simResult.moveList[i].y = -1.0f;
	}
	simResult.points = 0;

	simStartingRandomMovesNum = randi(simMaxStartingRandomMoves + 1);

	computePlayerTarget();

	while (!simZombiesAllDead && !simFailure && simMovesCount < MAX_MOVES) {
		if ((maxHypotheticalScore() + simPoints) < simCurrentBest)
			simFailure = true;
		turn(simResult.moveList);
	}

	if (simZombiesAllDead && !simFailure) {
		simResult.points = simPoints;
		simResult.len = simMovesCount;
	}

	return simResult;
}

void turn(point2df moveHistory[MAX_MOVES]) {
	int i, j, k;
	bool foundHuman;
	int zombieTargetIdTmp[MAX_ZOMBIES];

	for (i = 0; i < simZombieCount; i++) {
		findZombieTarget(&simZombies[i]);
		moveZombie(&simZombies[i]);
	}

	moveHistory[simMovesCount] = getPlayerDestination();
	simMovesCount++;
	movePlayer(&simPlayer); 

	evaluate(); 

	zombiesEat();

	if ((simHumanCount) > 0 && (simZombieCount > 0)) {
		if (simPlayer.arrived || simPlayerTargetDiedThisTurn) {
			computePlayerTarget();
			simPlayerTargetDiedThisTurn = false;
		}
	}
	else {
		simFailure = (simHumanCount <= 0);
		simZombiesAllDead = simZombieCount <= 0;
	}
}

void computePlayerTarget() {
	Zombie *zombiesThatDoNotTargetPlayer[MAX_ZOMBIES];
	int len = 0;
	
	if (simStartingRandomMovesNum > 0) {
		//simPlayer.tileTarget = randomTileInRadius(simPlayer.gocomp.position, 4000.0f);
		simPlayer.tileTarget = randomTile();
		simPlayer.targetingZombie = false;
		simStartingRandomMovesNum--;
	}
	else {
		for (int i = 0; i < simZombieCount; i++) {
			if (simZombies[i].target != NULL && simZombies[i].target->id != PLAYER_ID) {
				zombiesThatDoNotTargetPlayer[len] = &simZombies[i];
				len++;
			}
		}

		simPlayer.target = (len > 0) ? zombiesThatDoNotTargetPlayer[randi(len)] : &simZombies[randi(simZombieCount)];
		simPlayer.arrived = false;
		simPlayer.targetingZombie = true;
	}

}

void findZombieTarget(Zombie *zombie) {
	float minDist = INFINITY,
		tmpDist;
    bool targetFound = false;

	zombie->arrived = false;
	
	tmpDist = dist2d(zombie->gocomp.position, simPlayer.gocomp.position);
	if (tmpDist < minDist) {
		zombie->target = &(simPlayer.gocomp);
		targetFound = true;
		minDist = tmpDist;
	}

	for (int i = 0; i < simHumanCount; i++) {
		tmpDist = dist2d(zombie->gocomp.position, simHumans[i].position);
		if (tmpDist < minDist) {
			zombie->target = &simHumans[i];
			targetFound = true;
			minDist = tmpDist;
		}
	}

	//printErr("zombie is at %f distance from target\n", dist2d(zombie->gocomp.position, zombie->target->position));
}

bool nextPosPlayer(Player player, point2df *pos_out) {
	point2df dst;
	float dft, t;
	bool arrived = false;

	if (player.target != NULL || player.tileTarget != NULL) {

		/*if (player->target->gocomp.id == PLAYER_ID) {
			player->arrived = true;
			return player->gocomp.position;
		} */ 
		dst = getPlayerDestination();
		dft = dist2d(player.gocomp.position, dst);

		if (floorf(dft) <= PLAYER_MOVE_RANGE) {
			arrived = true;
			pos_out->x = dst.x;
			pos_out->y = dst.y;
		}
		else {
			t = PLAYER_MOVE_RANGE / dft;
			pos_out->x = player.gocomp.position.x + floorf(t * (dst.x - player.gocomp.position.x));
			pos_out->y = player.gocomp.position.y + floorf(t * (dst.y - player.gocomp.position.y));
		}
	} else {
		//fprintf(stderr, "Move error\n");
		pos_out->x = -1.0f;
		pos_out->y = -1.0f;
	}

	return arrived;
}

bool nextPosZombie(Zombie zombie, point2df *pos_out) {
	bool arrived = false;

	if (zombie.target != NULL) {
		float dft = dist2d(zombie.gocomp.position, zombie.target->position), t;

		if (floorf(dft) <= ZOMBIE_MOVE_RANGE) {
			arrived = true;
			pos_out->x = zombie.target->position.x;
			pos_out->y = zombie.target->position.y;
		}
		else {
			t = ZOMBIE_MOVE_RANGE / dft;
			pos_out->x = zombie.gocomp.position.x + floorf(t * (zombie.target->position.x - zombie.gocomp.position.x));
			pos_out->y = zombie.gocomp.position.y + floorf(t * (zombie.target->position.y - zombie.gocomp.position.y));
		}
	} else {
		//fprintf(stderr, "Zombie move error\n");
		pos_out->x = -1.0f;
		pos_out->y = -1.0f;
	}

	return arrived;
}

void movePlayer(Player *player) {
	player->arrived = nextPosPlayer(*player, &player->gocomp.position);
}

void moveZombie(Zombie *zombie) {
	zombie->arrived = nextPosZombie(*zombie, &zombie->gocomp.position);
}

int zombiesInRangeOfPlayer(Zombie zombiesInRange[MAX_ZOMBIES]) {
	int len = 0;
	float dx, dy;

	for (int i = 0; i < simZombieCount; i++) {
		dx = simZombies[i].gocomp.position.x - simPlayer.gocomp.position.x;
		dy = simZombies[i].gocomp.position.y - simPlayer.gocomp.position.y;
		
		if (floorf(sqrtf(dx * dx + dy * dy)) <= PLAYER_RANGE) {
			zombiesInRange[len] = simZombies[i];
			len++;
		}
	}

	return len;
}

void evaluate() {
	bool found;
	int i, j, k, tmpPoints;
	int humanNum = simHumanCount;
	int humanPoints = 10 * humanNum * humanNum;
	Zombie killableZombies[MAX_ZOMBIES];
	int killableZombiesLen = zombiesInRangeOfPlayer(killableZombies);

	int tmpId = (simPlayer.targetingZombie) ? simPlayer.target->gocomp.id : EMPTY_ZOMBIE;

	for (i = 0; i < killableZombiesLen; i++) {
		tmpPoints = humanPoints;

		if (killableZombiesLen > 1) {
			tmpPoints *= fibonacci(i + 1);
		}
		simPoints += tmpPoints;

		if (killableZombies[i].gocomp.id == tmpId)
			simPlayerTargetDiedThisTurn = true;

		found = false;
		for (j = simZombieCount - 1; j >= 0 && !found; j--) {
			if (simZombies[j].gocomp.id == killableZombies[i].gocomp.id) {
				simZombieCount--;
				found = true;

				// eliminate the j-th zombie
				for (k = j; (k + 1) < MAX_ZOMBIES; k++)
					simZombies[k] = simZombies[k + 1];
			}
		}
	}

	if (killableZombiesLen > 0) {
		found = false;
		for (i = 0; i < simZombieCount && !found; i++) {
			if (simZombies[i].gocomp.id == tmpId)
				simPlayer.target = &simZombies[i];
		}
	}
}

void zombiesEat() {
    int i, j, k, zombieTargetIdTmp[MAX_ZOMBIES];
    bool foundHuman;
    
	for (i = 0; i < simZombieCount; i++)
	zombieTargetIdTmp[i] = simZombies[i].target->id;

	for (i = 0; i < simZombieCount; i++) {
		foundHuman = false;
		// if you can eat your target
		if (_zombieArrivedAtTarget(simZombies[i])) {
			for (j = 0; j < simHumanCount && !foundHuman; j++) {
				if (simHumans[j].id == zombieTargetIdTmp[i]) {
					for (k = j; (k + 1) < MAX_HUMANS; k++)
						simHumans[k] = simHumans[k + 1];
					foundHuman = true;
					simHumanCount--;
				}
			}
		}
	}

	for (i = 0; i < simZombieCount; i++)
		for (j = 0; j < simHumanCount; j++)
			if (simHumans[j].id == zombieTargetIdTmp[i])
				simZombies[i].target = &simHumans[j];
}

int maxHypotheticalScore() {
	int tmpPoints = 0,
		totPoints = 0;
	int totHumans = simHumanCount,
		humanPoints = 10 * totHumans * totHumans;

	for (int i = 0; i < simZombieCount; i++) {
		tmpPoints = humanPoints;
		if (simZombieCount > 1) {
			tmpPoints *= fibonacci(i + 1);
		}
		totPoints += tmpPoints;
	}

	return totPoints;
}

bool _zombieArrivedAtTarget(Zombie zombie) {
	return (int)zombie.gocomp.position.x == (int)zombie.target->position.x && (int)zombie.gocomp.position.y == (int)zombie.target->position.y;
}

int eat(Zombie zombie) {
	bool found = false;
	int humanId = -3;

	// if you can eat your target
	if (_zombieArrivedAtTarget(zombie)) {
		for (int i = 0; i < simHumanCount && !found; i++) {
			if (simHumans[i].id == zombie.target->id) {
				for (int j = i; (j + 1) < MAX_HUMANS; j++)
					simHumans[j] = simHumans[j + 1];
				found = true;
				simHumanCount--;
			}
		}
	}

	return humanId;
}

point2df getPlayerDestination() {
	Zombie *target;
	point2df dst;
	if (simPlayer.targetingZombie) {
		target = simPlayer.target;
		nextPosZombie(*target, &dst);
		return dst;
	}
	else {
		return simPlayer.tileTarget->position;
	}
}

int getPlayerTargetId() {
	if (simPlayer.targetingZombie)
		return simPlayer.target->gocomp.id;
	else 
		return simPlayer.tileTarget->id;
}

void initTile(GameObject *tile, int x, int y) {
	tile->id = TILE_ID;
	tile->position.x = x;
	tile->position.y = y;
}

GameObject *randomTile() {
	GameObject *rt = new(GameObject);
	initTile(rt, randi(MAX_X), randi(MAX_Y));
	return rt;
}

GameObject *randomTileInRadius(point2df center, float radius) {
	float angle = randf(1.0f) * TAU;
	float x = cosf(angle) * radius, y = sinf(angle) * radius;
	GameObject *rt = new(GameObject);
	initTile(rt, fabsf(floorf(x + center.x)), fabsf(floorf(y + center.y)));
	return rt;
}
