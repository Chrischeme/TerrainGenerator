#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <algorithm>
#include <ctime>
#include <vector>
#include <thread>
#include <mutex>
#include <random>
#include <time.h>
#include "TextureHelper.h"

using namespace std;
int numMountains, maxAltitude, minAltitude, maxSize, minSize, beachWidth, mountainWidth, riverNum;
vector <int> borderX, borderZ;
vector <int> mountainBX, mountainBZ;
mutex m;
thread agents[10];

int intRand(const int & min, const int & max) {
	static thread_local mt19937* generator = nullptr;
	hash<thread::id> hasher;
	if (!generator) generator = new mt19937(clock() + hasher(this_thread::get_id()));
	uniform_int_distribution<int> distribution(min, max);
	return distribution(*generator);
}

void initAgents() {
	srand(time(NULL));
	string line;
	ifstream data("Data//data.txt");
	if (data.is_open()){
		int i = 0;
		while (getline(data, line)){
			if (i == 0)
				maxAltitude = stoi(line.substr(line.find(" ") + 1));
			if (i == 1)
				minAltitude = stoi(line.substr(line.find(" ") + 1));
			if (i == 2)
				numMountains = stoi(line.substr(line.find(" ") + 1));
			if (i == 3)
				maxSize = stoi(line.substr(line.find(" ") + 1));
			if (i == 4)
				minSize = stoi(line.substr(line.find(" ") + 1));
			if (i == 5)
				beachWidth = stoi(line.substr(line.find(" ") + 1));
			if (i == 6)
				mountainWidth = stoi(line.substr(line.find(" ") + 1));
			if (i == 7)
				riverNum = stoi(line.substr(line.find(" ") + 1));
			i++;
		}
		data.close();
	}
	else {
		exit(0);
	}
}

float CoastlineScore(int xCor, int zCor, int attractorX, int attractorZ, int repulserX, int repulserZ) {
	int minimum = min({ xCor, zCor, MAP_X - xCor, MAP_Z - zCor });
	float score = 
		pow(xCor - repulserX, 2.0) + pow(zCor - repulserZ, 2.0)
		-pow(xCor - attractorX, 2.0) - pow(zCor - attractorZ, 2.0)
		+ 3 * pow(minimum, 2.0);
	return score;
}

bool isBorder(int xCor, int zCor) {
	int j, k;
	for (j = xCor - 1; j <= xCor + 1; j++) {
		for (k = zCor - 1; k <= zCor + 1; k++) {
			if (j > 0 && j < MAP_X && k > 0 && k < MAP_Z && terrain[j][k][1] < 50) {
				return true;
			}
		}
	}
	return false;
}
void CoastlineHelper(int size) {
	int attractorX = intRand(0, MAP_X - 1);
	int attractorZ = intRand(0, MAP_Z - 1);
	int repulserX = intRand(0, MAP_X - 1);
	int repulserZ = intRand(0, MAP_Z - 1);
	int i, j, k;
	int randomBorder = 0;
	for (i = 0; i < size; i++) {
		m.lock();
		randomBorder = intRand(0, borderX.size() - 1);
		int pointX = borderX.at(randomBorder);
		int pointZ = borderZ.at(randomBorder);
		int newX = -1;
		int newZ = -1;
		float score = -FLT_MAX;
		for (j = pointX - 1; j <= pointX + 1; j++) {
			for (k = pointZ - 1; k <= pointZ + 1; k++) {
				if (j > 0 && j < MAP_X && k > 0 && k < MAP_Z && terrain[j][k][1] < 50) {
					if (score < CoastlineScore(j, k, attractorX, attractorZ, repulserX, repulserZ)) {
						score = CoastlineScore(j, k, attractorX, attractorZ, repulserX, repulserZ);
						newX = j;
						newZ = k;
					}
				}
			}
		}
		if (newX != -1) {
			terrain[newX][newZ][1] = 130;
		}
		if (!isBorder(pointX, pointZ)) {
			borderX.erase(borderX.begin() + randomBorder);
			borderZ.erase(borderZ.begin() + randomBorder);
		}
		if (isBorder(newX, newZ)) {
			borderX.push_back(newX);
			borderZ.push_back(newZ);
		}
		m.unlock();
	}
}
void CoastlineAgent() {
	int size = (rand() % (maxSize - minSize)) + minSize;
	int pointX = 0;
	int pointZ = 0;
	do {
		while (pointX < 10 || pointZ < 10 || pointX > MAP_X - 10 || pointZ > MAP_Z - 10) {
			pointX = rand() % MAP_X;
			pointZ = rand() % MAP_Z;
		}
	} while (!isBorder(pointX, pointZ));
	terrain[pointX][pointZ][1] = 130;
	borderX.push_back(pointX);
	borderZ.push_back(pointZ);
	int i;
	for (i = 0; i < 10; i++) {
		agents[i] = thread(CoastlineHelper, size / 10);
	}
	for (i = 0; i < 10; i++) {
		agents[i].join();
	}
}
void smoothOutBeach(vector<int> beachX, vector<int> beachZ) {
	while (beachX.size() > 0) {
		int randPlace = intRand(0, beachX.size() - 1);
		int j, k;
		int xCor = beachX.at(randPlace);
		int zCor = beachZ.at(randPlace);
		for (j = xCor - 1; j <= xCor + 1; j++) {
			for (k = zCor - 1; k <= zCor + 1; k++) {
				if (j > 0 && j < MAP_X && k > 0 && k < MAP_Z && terrain[j][k][1] > terrain[xCor][zCor][1] + 10) {
					terrain[j][k][1] = terrain[xCor][zCor][1] + intRand(1, 10);
					beachX.push_back(j);
					beachZ.push_back(k);
				}
			}
		}
		beachX.erase(beachX.begin() + randPlace);
		beachZ.erase(beachZ.begin() + randPlace);
	}
}

void BeachHelper(int i) {
	vector<int> beachX, beachZ;
	int randomBorder = intRand(0, borderX.size());
	int pointX = borderX.at(randomBorder);
	int pointZ = borderZ.at(randomBorder);
	int randomWidth = rand() % (beachWidth / 2) + beachWidth / 2;
	int j, k;
	for (j = pointX - randomWidth / 2; j <= pointX + randomWidth / 2; j++) {
		for (k = pointZ - randomWidth / 2; k <= pointZ + randomWidth / 2; k++) {
			if (j > 0 && j < MAP_X && k > 0 && k < MAP_Z && terrain[j][k][1] > 50) {
				terrain[j][k][1] = 55;
				beachX.push_back(j);
				beachZ.push_back(k);
			}
		}
	}
	smoothOutBeach(beachX, beachZ);
}
void BeachAgent() {
	int i;
	for (i = 0; i < 10; i++) {
		agents[i] = thread(BeachHelper, i);
	}
	for (i = 0; i < 10; i++) {
		agents[i].join();
	}
}

bool isNotTrapped(int xCor, int zCor) {
	int j, k;
	for (j = xCor - 1; j <= xCor + 1; j++) {
		for (k = zCor - 1; k <= zCor + 1; k++) {
			if (j > 0 && j < MAP_X && k > 0 && k < MAP_Z && terrain[j][k][1] >= 130) {
				return true;
			}
		}
	}
	return false;
}
void smoothMountain(int xCor, int zCor) {
	int j, k;
	int total = 0;
	for (j = xCor - 1; j <= xCor + 1; j++) {
		for (k = zCor - 1; k <= zCor + 1; k++) {
			if (j > 0 && j < MAP_X && k > 0 && k < MAP_Z) {
				total += terrain[j][k][1];
			}
		}
	}
	terrain[xCor][zCor][1] = total / 8;
}
void MountainAgent() {
	int i;
	for (i = 0; i < numMountains; i++) {
		vector<int> mountainElevations;
		int pointX, pointZ;
		do {
			pointX = rand() % MAP_X;
			pointZ = rand() % MAP_Z;
		} while (terrain[pointX][pointZ][1] != 130);
		int direction = rand() % 8;
		int highestPoint = rand() % (maxAltitude - minAltitude) + minAltitude;
		bool correctDirection = true;
		int originalDirection = direction;
		int curHeight = terrain[pointX][pointZ][1];
		while (terrain[pointX][pointZ][1] < highestPoint) {
			correctDirection = true;
			switch (direction) {
			case 0:
				if (
					terrain[pointX + 1][pointZ][1] < 130 || terrain[pointX + 1][pointZ - 1][1] < 130 || terrain[pointX + 1][pointZ + 1][1] < 130) {
					correctDirection = false;
				}
				if (correctDirection) {
					int randSlope = rand() % 3;
					int slope;
					if (randSlope == 0) {
						slope = rand() % 100;
					}
					else {
						slope = 0;
					}
					mountainElevations.push_back(slope);
					terrain[pointX + 1][pointZ][1] += curHeight + slope;
					terrain[pointX + 1][pointZ - 1][1] += curHeight + slope * 2;
					terrain[pointX + 1][pointZ - 2][1] += curHeight + slope * 3;
					terrain[pointX + 1][pointZ - 3][1] += curHeight + slope * 2;
					terrain[pointX + 1][pointZ + 1][1] += curHeight + slope * 2;
					terrain[pointX + 1][pointZ + 2][1] += curHeight + slope * 3;
					terrain[pointX + 1][pointZ + 3][1] += curHeight + slope * 2;
					pointX++;
				}
				break;
			case 1:
				if (terrain[pointX + 1][pointZ][1] < 130 || terrain[pointX + 1][pointZ  + 1][1] < 130 || terrain[pointX][pointZ + 1][1] < 130) {
					correctDirection = false;
				}
				if (correctDirection) {
					int randSlope = rand() % 3;
					int slope;
					if (randSlope == 0) {
						slope = rand() % 100;
					}
					else {
						slope = 0;
					}
					mountainElevations.push_back(slope);
					terrain[pointX + 1][pointZ][1] += curHeight + slope * 2;
					terrain[pointX][pointZ + 1][1] += curHeight + slope * 2;
					terrain[pointX + 1][pointZ + 1][1] += curHeight + slope;
					pointX++;
					pointZ++;
				}
				break;
			case 2:
				if (terrain[pointX + 1][pointZ + 1][1] < 130 || terrain[pointX][pointZ + 1][1] < 130 || terrain[pointX - 1][pointZ + 1][1] < 130) {
					correctDirection = false;
				}
				if (correctDirection) {
					int randSlope = rand() % 3;
					int slope;
					if (randSlope == 0) {
						slope = rand() % 100;
					}
					else {
						slope = 0;
					}
					mountainElevations.push_back(slope);
					terrain[pointX + 1][pointZ + 1][1] += curHeight + slope * 2;
					terrain[pointX + 2][pointZ + 1][1] += curHeight + slope * 3;
					terrain[pointX + 3][pointZ + 1][1] += curHeight + slope * 2;
					terrain[pointX][pointZ + 1][1] += curHeight + slope;
					terrain[pointX - 1][pointZ + 1][1] += curHeight + slope * 2;
					terrain[pointX - 2][pointZ + 1][1] += curHeight + slope * 3;
					terrain[pointX - 3][pointZ + 1][1] += curHeight + slope * 2;
					pointZ++;
				}
				break;
			case 3:
				if (terrain[pointX - 1][pointZ][1] < 130 || terrain[pointX][pointZ + 1][1] < 130 || terrain[pointX - 1][pointZ + 1][1] < 130) {
					correctDirection = false;
				}
				if (correctDirection) {
					int randSlope = rand() % 3;
					int slope;
					if (randSlope == 0) {
						slope = rand() % 100;
					}
					else {
						slope = 0;
					}
					mountainElevations.push_back(slope);
					terrain[pointX - 1][pointZ][1] += curHeight + slope;
					terrain[pointX][pointZ + 1][1] += curHeight + slope;
					terrain[pointX - 1][pointZ + 1][1] += curHeight + slope;
					pointX--;
					pointZ++;
				}
				break;
			case 4:
				if (terrain[pointX - 1][pointZ][1] < 130 || terrain[pointX - 1][pointZ + 1][1] < 130 || terrain[pointX - 1][pointZ - 1][1] < 130) {
					correctDirection = false;
				}
				if (correctDirection) {
					int randSlope = rand() % 3;
					int slope;
					if (randSlope == 0) {
						slope = rand() % 100;
					}
					else {
						slope = 0;
					}
					mountainElevations.push_back(slope);
					terrain[pointX - 1][pointZ][1] += curHeight + slope;
					terrain[pointX - 1][pointZ + 1][1] += curHeight + slope * 2;
					terrain[pointX - 1][pointZ + 2][1] += curHeight + slope * 3;
					terrain[pointX - 1][pointZ + 3][1] += curHeight + slope * 2;
					terrain[pointX - 1][pointZ - 1][1] += curHeight + slope * 2;
					terrain[pointX - 1][pointZ - 2][1] += curHeight + slope * 3;
					terrain[pointX - 1][pointZ - 3][1] += curHeight + slope * 2;
					pointX--;
				}
				break;
			case 5:
				if (terrain[pointX - 1][pointZ][1] < 130 || terrain[pointX][pointZ - 1][1] < 130 || terrain[pointX - 1][pointZ - 1][1] < 130) {
					correctDirection = false;
				}
				if (correctDirection) {
					int randSlope = rand() % 3;
					int slope;
					if (randSlope == 0) {
						slope = rand() % 100;
					}
					else {
						slope = 0;
					}
					mountainElevations.push_back(slope);
					terrain[pointX - 1][pointZ][1] += curHeight + slope;
					terrain[pointX][pointZ - 1][1] += curHeight + slope;
					terrain[pointX - 1][pointZ - 1][1] += curHeight + slope;
					pointX--;
					pointZ--;
				}
				break;
			case 6:
				if (terrain[pointX + 1][pointZ - 1][1] < 130 || terrain[pointX - 1][pointZ  - 1][1] < 130 || terrain[pointX][pointZ - 1][1] < 130) {
					correctDirection = false;
				}
				if (correctDirection) {
					int randSlope = rand() % 3;
					int slope;
					if (randSlope == 0) {
						slope = rand() % 100;
					}
					else {
						slope = 0;
					}
					mountainElevations.push_back(slope);
					terrain[pointX + 1][pointZ - 1][1] += curHeight + slope * 2;
					terrain[pointX + 2][pointZ - 1][1] += curHeight + slope * 3;
					terrain[pointX + 3][pointZ - 1][1] += curHeight + slope * 2;
					terrain[pointX][pointZ - 1][1] += curHeight + slope;
					terrain[pointX - 1][pointZ - 1][1] += curHeight + slope * 2;
					terrain[pointX - 2][pointZ - 1][1] += curHeight + slope * 3;
					terrain[pointX - 3][pointZ - 1][1] += curHeight + slope * 2;
					pointZ--;
				}
				break;
			default:
				if (terrain[pointX + 1][pointZ][1] < 130 || terrain[pointX + 1][pointZ - 1][1] < 130 || terrain[pointX][pointZ - 1][1] < 130) {
					correctDirection = false;
				}
				if (correctDirection) {
					int randSlope = rand() % 3;
					int slope;
					if (randSlope == 0) {
						slope = rand() % 100;
					}
					else {
						slope = 0;
					}
					mountainElevations.push_back(slope);
					terrain[pointX + 1][pointZ][1] += curHeight + slope;
					terrain[pointX][pointZ - 1][1] += curHeight + slope;
					terrain[pointX + 1][pointZ - 1][1] += curHeight + slope;
					pointX++;
					pointZ--;
				}
				break;
			}
			if (!correctDirection) {
				direction += 1;
				if (direction == 8) {
					direction = 0;
				}
				originalDirection = direction;
			}
			else {
				int directionVariance = rand() % 3 - 1;
				direction = originalDirection + directionVariance;
				if (direction == -1) {
					direction = 7;
				}
				if (direction == 8) {
					direction = 0;
				}
			}
		}
		int o;
		for (o = 0; o < 2; o++) {
			int j, k;
			for (j = 0; j < MAP_X; j++) {
				for (k = 0; k < MAP_Z; k++) {
					if (terrain[j][k][1] > 150 && terrain[j][k][1] < 160) {
						if (terrain[j + 1][k + 1][1] > terrain[j][k][1] || terrain[j + 1][k - 1][1] > terrain[j][k][1] || terrain[j + 1][k][1] > terrain[j][k][1] || terrain[j][k - 1][1] > terrain[j][k][1] || terrain[j][k + 1][1] > terrain[j][k][1] || terrain[j - 1][k + 1][1] > terrain[j][k][1] || terrain[j - 1][k - 1][1] > terrain[j][k][1] || terrain[j - 1][k][1] > terrain[j][k][1]) {
							mountainBX.push_back(j);
							mountainBZ.push_back(k);
						}
					}
					if (terrain[j][k][1] > 150) {
						smoothMountain(j, k);
					}
				}
			}
		}
	}
}

void RiverHelper() {
	int randomBorder;
	int randomBase;
		randomBorder = rand() % borderX.size();
		randomBase = rand() % mountainBX.size();
	int curX = borderX.at(randomBorder);
	int curZ = borderZ.at(randomBorder);
	int finalX = mountainBX.at(randomBase);
	int finalZ = mountainBZ.at(randomBase);
	terrain[curX][curZ][1] = 0;
	while ((pow(finalX - curX, 2)+pow(finalZ - curZ, 2)) > 25) {
		if (curX != finalX) {
			if (curX > finalX) {
				terrain[curX - 1][curZ][1] = 0;
				terrain[curX - 1][curZ - 1][1] -= 100;
				terrain[curX - 1][curZ - 2][1] -= 50;
				terrain[curX - 1][curZ - 3][1] -= 25;
				terrain[curX - 1][curZ + 1][1] -= 100;
				terrain[curX - 1][curZ + 2][1] -= 50;
				terrain[curX - 1][curZ + 3][1] -= 25;
				smoothMountain(curX - 1, curZ - 3);
				smoothMountain(curX - 1, curZ + 3);
				curX--;
			}
			else {
				terrain[curX + 1][curZ][1] = 0;
				terrain[curX + 1][curZ - 1][1] -= 100;
				terrain[curX + 1][curZ - 2][1] -= 50;
				terrain[curX + 1][curZ - 3][1] -= 25;
				terrain[curX + 1][curZ + 1][1] -= 100;
				terrain[curX + 1][curZ + 2][1] -= 50;
				terrain[curX + 1][curZ + 3][1] -= 25;
				smoothMountain(curX + 1, curZ - 3);
				smoothMountain(curX + 1, curZ + 3);
				curX++;
			}
		}
		else {
			if (curZ > finalZ) {
				terrain[curX][curZ - 1][1] = 0;
				terrain[curX - 1][curZ - 1][1] -= 100;
				terrain[curX - 2][curZ - 1][1] -= 50;
				terrain[curX - 3][curZ - 1][1] -= 25;
				terrain[curX + 1][curZ - 1][1] -= 100;
				terrain[curX + 2][curZ - 1][1] -= 50;
				terrain[curX + 3][curZ - 1][1] -= 25;
				smoothMountain(curX - 3, curZ - 1);
				smoothMountain(curX + 3, curZ - 1);
				curZ--;
			}
			else {
				terrain[curX][curZ + 1][1] = 0;
				terrain[curX - 1][curZ + 1][1] -= 100;
				terrain[curX - 2][curZ + 1][1] -= 50;
				terrain[curX - 3][curZ + 1][1] -= 25;
				terrain[curX + 1][curZ + 1][1] -= 100;
				terrain[curX + 2][curZ + 1][1] -= 50;
				terrain[curX + 3][curZ + 1][1] -= 25;
				smoothMountain(curX - 3, curZ + 1);
				smoothMountain(curX + 3, curZ + 1);
				curZ++;
			}
		}
	}
}
void RiverAgent() {
	int i;
	for (i = 0; i < riverNum; i++) {
		agents[i] = thread(RiverHelper);
	}
	for (i = 0; i < riverNum; i++) {
		agents[i].join();
	}
}