
#include <Windows.h>
#include <iostream>
#include <cmath>
#include "MemMan.h"

#define KeyDOWN -32768
#define KeyUP 0

MemMan MemClass;

//Definir alguns offsets

struct offsets
{
	DWORD localPlayer = 0xC648AC;
	DWORD forceLMB = 0x3082DEC;
	DWORD entityList = 0x4C41704;
	DWORD crosshair = 0xB2DC;
	DWORD team = 0xF0;
	DWORD health = 0xFC;
	DWORD vectorOrigin = 0x134;
	DWORD itemDefIndex = 0x2F9A;
	DWORD activeWeapon = 0x2EE8;
	DWORD isScoped = 0x38AA;
} offset;

//Variáveis estruturadas

struct variables
{
	DWORD localPlayer;
	DWORD gameModule;
	int myTeam;
	int tbDelay;
	int myWeaponID;
} val;

//Mapeamento do Trigger

struct vector
{
	float x, y, z;
};

//Checker Básico de Scope

bool checkIfScoped()
{
	return MemClass.readMem<bool>(val.localPlayer + offset.isScoped);
}

//Delay 

void setTBDelay(float distance)
{
	float delay;
	switch (val.myWeaponID)
	{
	case 262204: delay = 3; break;
	case 7: delay = 3.3; break;
	case 40: delay = 0.15; break;
	case 9: delay = 0.15; break;
	default: delay = 0;
	}
	val.tbDelay = delay * distance;
}

//Dar get na weapon main

void getMyWeapon()
{
	int weapon = MemClass.readMem<int>(val.localPlayer + offset.activeWeapon);
	int weaponEntity = MemClass.readMem<int>(val.gameModule + offset.entityList + ((weapon & 0xFFF) - 1) * 0x10);
	if (weaponEntity != NULL)
		val.myWeaponID = MemClass.readMem<int>(weaponEntity + offset.itemDefIndex);
}

//Detecção de distancia + readMemory para apresentar o command

float getDistance(DWORD entity)
{
	vector myLocation = MemClass.readMem<vector>(val.localPlayer + offset.vectorOrigin);
	vector enemyLocation = MemClass.readMem<vector>(entity + offset.vectorOrigin);

	return sqrt(pow(myLocation.x - enemyLocation.x, 2) + pow(myLocation.y - enemyLocation.y, 2) + pow(myLocation.z - enemyLocation.z, 2)) * 0.0254;
}

//Function para atirar

void shoot()
{
	Sleep(val.tbDelay);
	MemClass.writeMem<int>(val.gameModule + offset.forceLMB, 5);
	Sleep(20);
	MemClass.writeMem<int>(val.gameModule + offset.forceLMB, 4);
}

//CheckBot e team

bool checkTBot()
{
	int crosshair = MemClass.readMem<int>(val.localPlayer + offset.crosshair);
	if (crosshair != 0 && crosshair < 64)
	{
		DWORD entity = MemClass.readMem<DWORD>(val.gameModule + offset.entityList + ((crosshair - 1) * 0x10));
		int eTeam = MemClass.readMem<int>(entity + offset.team);
		int eHealth = MemClass.readMem<int>(entity + offset.health);
		if (eTeam != val.myTeam && eHealth > 0)
		{
			float distance = getDistance(entity);
			getMyWeapon();
			setTBDelay(distance);
			if (val.myWeaponID == 40 || val.myWeaponID == 9)
				return checkIfScoped();
			else
				return true;
		}
		else
			return false;
	}
	else
		return false;
}

void handleTBot()
{
	if (checkTBot())
		shoot();
}

//Function principal

int main()
{
	bool canTBot = false, keyHeld = false;
	int proc = MemClass.getProcess("csgo.exe");

	val.gameModule = MemClass.getModule(proc, "client.dll");
	val.localPlayer = MemClass.readMem<DWORD>(val.gameModule + offset.localPlayer);

	if (val.localPlayer == NULL)
		while (val.localPlayer == NULL)
			val.localPlayer = MemClass.readMem<DWORD>(val.gameModule + offset.localPlayer);

	while (true)
	{
		if (GetAsyncKeyState(VK_F2) & 1)
		{
			val.myTeam = MemClass.readMem<int>(val.localPlayer + offset.team);
			canTBot = !canTBot;
		}

		//Definir a bind

		if (GetAsyncKeyState(VK_XBUTTON1) == KeyDOWN && !keyHeld)
		{
			keyHeld = true;
			canTBot = true;
		}
		if (GetAsyncKeyState(VK_XBUTTON1) == KeyUP && keyHeld)
		{
			keyHeld = false;
			canTBot = false;
		}

		if (canTBot)
			handleTBot();

		Sleep(1);
	}
	return 0;
}