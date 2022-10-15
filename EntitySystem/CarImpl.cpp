#include "CarInterface.h"
#include <stdio.h>

void CarImpl::onUpdate(float dt) {
	int left = 0;
	int right = 0;
	float buff[256];
	readCam(buff);
	float limit = 0.25f;
	bool found = false;
	static float lastGoodDir = 0.0f;

	// ako to funguje:
	// rozdelime si obraz na polovicu ( | ), a hladame tmavy bod (X) od stredu dolava a od stredu doprava
	// XXX------------|---XXX---------
	// --X<-----------|-->X-----------
	// nasli sme okraje lavej a pravej ciary, vypocitame teda priemer (stred medzi <, >)
	//  --|<-------X--|---->|---------
	// ako je vidno, v takomto pripade by sme mali ist dolava (bod sa nachadza vlavo od stredu obrazu)

	for (right = 64; right <= 127; right++) { //hladame ciaru smerom doprava --> (64 -> 127)
		if (buff[right] < limit) {
			found = true; break;
		}
	}
	for (left = 64; left > 0; left--) {       //hladame ciaru smerom dolava <-- (64 -> 0)
		if (buff[left] < limit) {

			found = true; break;
		}
	}

	int dir = (left + right) / 2 - 64;      //vysledok je (L + R)/2 - 64

	char msg[256];
	sprintf(msg, "Dir: %d, L: %d, R: %d", dir, left, right);
	writeLine(msg);
	
	float servo = dir / 128.0f;
	servo = servo * 2.0f;

	if (!found) {
		writeLine("$FF0000Line not found");
		servo = lastGoodDir;
	} else lastGoodDir = servo;

	float mult = 1.0f;
	if (servo < -0.25f || servo > 0.25f) mult /= 10.0F;
	setSteering(servo);
	setSpeed(0.6f * mult);
}