#include "shared.h"
#include "GameLogics.h"
#include "Engine.h"

void GameMain::init(IEngine *pEngine) {	// Inicializacia herneho modu, tuto funkciu vola engine pri nacitani modu a ako parameter nam posle pointer na sameho seba
	this->pEngine = (Engine*)pEngine;	// Zapamatame si pointer na engine, kedze ho budeme chciet neskor pouzivat na volanie funkcii enginu
}

void GameMain::onUpdate(double dt) {	// Tato funkcia sa vola kazdy snimok, dt = cas v sekundach od posledneho volania
	IWindow *pWnd = pEngine->getWindow();
	if (activeVehicle) {				// Ak sme vo vozidle, asi ho budeme chciet nejak ovladat
		if (!pWnd->IsKeyDown(VK_LEFT) && !pWnd->IsKeyDown(VK_RIGHT)) {	// Ked nedrzime ziadnu sipku dolava/doprava, auto by malo ist rovno
			activeVehicle->turn(0.0f);
		}
		if (!pWnd->IsKeyDown(VK_UP) && !pWnd->IsKeyDown(VK_DOWN)) {		// Ked nedrzime ziadnu sipku hore/dole, auto by uz nemalo mat akceleraciu
			activeVehicle->forward(0.0f);
		}
		//activeVehicle->getPhysics()->getBody()->activate();
	}
}

void GameMain::onKeyPressed(int key) {
	
}

void GameMain::onKeyDown(int key) {
	if (activeVehicle) {	// Ak sme v aute, chceme ho ovladat
		switch (key) {
			case VK_UP:
			activeVehicle->forward(250.0f);
			break;
			case VK_DOWN:
			activeVehicle->forward(-250.0f);
			break;
			case VK_LEFT:
			activeVehicle->turn(-30.0f);
			break;
			case VK_RIGHT:
			activeVehicle->turn(30.0f);
			break;
		}
	}
}

void GameMain::onKeyUp(int key) {
	if (key == 'F') {	// Pri F chceme nastupit do vozidla
		if (activeVehicle) {	// Ak sme vo vozidle, vystupime z neho a koncime
			activeVehicle = 0;	
			return;
		}
		RayTraceParams params;
		params.distance = 10.0f;
		params.center = true;
		auto obj = pEngine->raytraceObject(params);	// Zistime ci mierime na nejake vozidlo v rozsahu 10m od nas
		if (!obj) {
			std::cout << "No object found\n";
		} else {
			std::cout << "Object type: " << obj->getType() << ", name: " << obj->getClassName() << ", ptr: "<<obj << std::endl;
		}
		if (obj && obj->getType() == VEHICLE) {	// Ak sme nasli nejaky objekt pred nami a je to vozidlo, oznacime si ho ako terajsie vozidlo
			activeVehicleObj = obj;
			activeVehicle = dynamic_cast<IVehicle*>(obj);
		}
	} else if (key == 'R') {
		static int reverb = 0;
		char msg[100]; sprintf(msg, "Reverb: %d", reverb);
		pEngine->drawText(700.0f, 10.0f, msg, Vec3(1.0f, 1.0f, 0.0f));
		pEngine->getSoundSystem()->setReverb(reverb);
		reverb++;
	}
}

void GameMain::onMouseDown(int btn) {	// Zatial nepotrebujeme tieto implementovat
	
}

void GameMain::onMouseUp(int btn) {
	
}

void GameMain::onMousePressed(int btn) {
	
}

void GameMain::onMessage(const Message& msg) {

}

bool GameMain::isActorGhost() {
	return activeVehicle != 0;
}

bool GameMain::getCameraPos(Math::Vec3& out) {	// Tato funkcia sa vola predtym ako sa vykresli snimka, aby engine zistil, ci nahodou nechce herny mod modifikovat polohu kamery
	if (!activeVehicle) return false;	// Ak nie sme vo vozidle, chceme aby sa kamera spravala normalne ako pri chodzi
	auto target = activeVehicleObj->getPos();	// Inak chceme aby sa kamera nachadzala za vozidlom a pozerala smerom nan
	matrix mtx = Mat4(XMMatrixRotationQuaternion(activeVehicleObj->getRotation().raw())) * Mat4(XMMatrixTranslationFromVector(target.raw()));
	out = Vec4(3.0f, -5.0f, 2.5f, 1.0f) * mtx;
	return true;
}

bool GameMain::getCameraDir(Math::Vec3& out) {	// Tato funkcia sa vola predtym ako sa vykresli snimka, aby engine zistil, ci nahodou nechce herny mod modifikovat smer, ktorym sa kamera pozera
	if (!activeVehicle) return false;
	auto target = activeVehicleObj->getPos();
	matrix mtx = Mat4(XMMatrixRotationQuaternion(activeVehicleObj->getRotation().raw())) * Mat4(XMMatrixTranslationFromVector(target.raw()));
	Vec3 p = Vec4(3.0f, -5.0f, 2.5f, 1.0f) * mtx;
	out = (target - p).normalize();
	return true;
}

void GameMain::release() {	// Tato funkcia sa vola pri vypnuti hry, resp pri odnacitani nasho herneho modu
	
}

bool GameMain::overrideLoader() { return false; }

bool GameMain::overrideCamera() { return false; }

bool GameMain::overrideMouse() { return false; }

void GameMain::onScroll(int dir) { return; }

bool GameMain::overrideKeyboard() { return false; }

bool GameMain::overrideMovement() { return false; }

const char* GameMain::getName() {
	return "Default";
}