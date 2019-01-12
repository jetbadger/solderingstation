/*
TODO:
-meniu timeoutas
-svaresnis main temp skaitmenu piesimas
-optimizuot 24pt fonta, istrint nenaudojamus simbolius, kad sutaupyt vietos
-constraintas konvertuojant temperatura pakeitus vienetus
-grafines ikonos
-velocity control?
*/

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
//#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <SPI.h>

#define TFT_CS        10
#define TFT_RST        9 // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC         8

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

//Pin assignments
int const encoderPin1 = 2;
int const encoderPin2 = 3;
int const buttonPin   = 4;

int Astate = 0;
int Bstate = 0;
int lastAstate = 0;
int temperatureValue = 300;

int activeScreen = 0;
int tempIncrement = 10;

int const menuItemCount = 8;
char *menuItems[] = {"Idle temperature", "Idle delay", "Power off", "Increment", "Units", "Buzzer", "Brightness", "-> Exit"};

int minTempValueC = 100; //temperature constraints for Celsius
int maxTempValueC = 480;

int minTempValueF = 200; //temperature constraints for Fahrenheit
int maxTempValueF = 900;

int cfgIdleTemp  = 150;  //degrees
int cfgIdleDelay = 10;   //seconds
int cfgPowerOff  = 20;   //minutes
int cfgIncrement = 2;
int cfgUnits     = 0;
int cfgBuzzer    = 1;
int cfgBrightness = 100;

int menuItemsSettings[] =    { cfgIdleTemp, cfgIdleDelay, cfgPowerOff, cfgIncrement, cfgUnits, cfgBuzzer, cfgBrightness };
int menuItemsNewSettings[] = { cfgIdleTemp, cfgIdleDelay, cfgPowerOff, cfgIncrement, cfgUnits };

int cfgIdleConstraints[] = {90, 300, 10}; // [lowest temp, highest temp, increment] *Lowest temp value must be minus one increment. Used to signify 'off' state.
int cfgIncrementValues[] = {1, 5, 10, 20};
char *cfgUnitsValues[]   = {"C", "F"};
char *cfgBuzzerValues[]  = {"OFF", "ON"};


int menuYstart        = 5;
int menuYspacing      = 16;
int menuValueRowXpos  = 125;

int highlight = 0;
int editMode  = 0;
int editParamNum;

int long lastBtnPress = 0;
int btnDebounce       = 500;

#define ORANGE 0xFBE0
#define GRAY   0xBDF7
#define SCREENFILL   ST77XX_BLACK
#define MENULABELS   ST77XX_WHITE
#define MENUVALUES   ST77XX_WHITE
#define MAINVALUE    ST77XX_WHITE
#define MAINLABELS   ST77XX_WHITE
#define SECONDVALUES ST77XX_WHITE


void drawMainScreen() {
	activeScreen = 0;
	tft.fillScreen(SCREENFILL);

	tft.setTextColor(MAINVALUE);
	tft.setFont(&FreeSans24pt7b);
  	//tft.setTextSize(1.5);
  	//tft.setTextWrap(false);
	tft.setCursor(40, 40);
	tft.print(temperatureValue);

	tft.fillCircle(125, 14, 5, MAINVALUE);
	tft.fillCircle(125, 14, 3, SCREENFILL);

	tft.setTextColor(SECONDVALUES);
	tft.setCursor(5, 75);
	tft.setFont(&FreeSans12pt7b);
	tft.print("200");
	tft.drawCircle(48, 62, 3, SECONDVALUES);
	tft.setCursor(94, 75);
	//tft.setFont(&FreeSans12pt7b);
	tft.print("100%");
	tft.setCursor(132, 25);
	tft.print(menuItemsSettings[4] == 0 ? "C" : "F");

	tft.setFont();
	tft.setTextColor(MAINLABELS);
	//tft.setCursor(134, 10);
	tft.setCursor(132, 32);
	tft.print("SET");
	tft.setCursor(136, 78);
	tft.print("PWM");
	tft.setCursor(5, 78);
	tft.print("TEMP");

  //tft.drawRect(1, 54, 158, 35, ST77XX_RED);
  //tft.drawRect(92, 54, 68, 35, 0xBDF7);

	tft.drawLine(0, 54, 160, 54, ST77XX_RED);
	tft.drawLine(0, 89, 160, 89, ST77XX_RED);

  //tft.fillRect(0, 95, 160, 33, ST77XX_BLACK);
	tft.fillRect(9, 96, 8, 27, ST77XX_WHITE);
	tft.fillRect(28, 96, 8, 27, ST77XX_WHITE);
	tft.fillRect(10, 97, 25, 25, ST77XX_RED);
	tft.setCursor(55, 110);
	tft.setTextColor(ST77XX_WHITE);
	tft.setFont(&FreeSans12pt7b);
	tft.print("No Iron");
}

void drawParam(int paramNum, int mode = 0, int save = 0) { // mode [0=draw value and signs, 1=edit mode signs, 2=redraw unit signs, 3=draw only the value]
	int yOffset = menuYstart + (menuYspacing * paramNum);
	int xSpacing = 6;
	int strLenght = menuItemsNewSettings[paramNum];

  if (mode == 1) {    //editing mode
    //erasing signs
  	if (paramNum == 0)      tft.fillRect(menuValueRowXpos + 19, yOffset - 1, 8, 8, SCREENFILL);
  	else if (paramNum == 1) tft.fillRect(menuValueRowXpos + 19, yOffset - 1, 8, 8, SCREENFILL);
  	else if (paramNum == 2) tft.fillRect(menuValueRowXpos + 19, yOffset - 1, 8, 8, SCREENFILL);
  	else if (paramNum == 3) tft.fillRect(menuValueRowXpos + 16, yOffset - 1, 8, 8, SCREENFILL);
  	else if (paramNum == 6) tft.fillRect(menuValueRowXpos + 19, yOffset - 1, 8, 8, SCREENFILL);

    //draw edit mode triangles
  	tft.fillTriangle(menuValueRowXpos - 3, yOffset, menuValueRowXpos - 3, yOffset + 6, menuValueRowXpos - 9, yOffset + 3, ORANGE);
  	tft.fillTriangle(menuValueRowXpos + 20, yOffset, menuValueRowXpos + 20, yOffset + 6, menuValueRowXpos + 26, yOffset + 3, ORANGE);
  }

  if (mode == 0 || mode == 2) {
    //erase edit mode triangles
  	tft.fillTriangle(menuValueRowXpos - 3, yOffset, menuValueRowXpos - 3, yOffset + 6, menuValueRowXpos - 9, yOffset + 3, SCREENFILL);
  	tft.fillTriangle(menuValueRowXpos + 20, yOffset, menuValueRowXpos + 20, yOffset + 6, menuValueRowXpos + 26, yOffset + 3, SCREENFILL);
  	tft.setTextColor(MENUVALUES);
  	tft.setFont();

    //draw unit signs
  	if (paramNum == 0 && menuItemsSettings[0] >= cfgIdleConstraints[0] + cfgIdleConstraints[2]) tft.drawCircle(menuValueRowXpos + 21, yOffset + 1, 2, MENUVALUES);
  	else if (paramNum == 1) {
  		tft.setCursor (menuValueRowXpos + 20, yOffset);
  		tft.print("s");
  	} else if (paramNum == 2) {
  		tft.setCursor (menuValueRowXpos + 20, yOffset);
  		tft.print("m");
  	}
  	else if (paramNum == 3)   tft.drawCircle(menuValueRowXpos + 18, yOffset + 1, 2, MENUVALUES);
  	else if (paramNum == 4)   tft.drawCircle(menuValueRowXpos + 12, yOffset + 1, 2, MENUVALUES);
  	else if (paramNum == 6) {
  		tft.setCursor (menuValueRowXpos + 20, yOffset);
  		tft.print("%");
  	}
  }

  if (mode == 0 || mode == 3) {

  	if (mode == 3) {
  	//erase value
  	if (paramNum == 0 || paramNum == 1 || paramNum == 2 || paramNum == 5 || paramNum == 6)      tft.fillRect(menuValueRowXpos, yOffset - 1, 20, 8, SCREENFILL);
  	else if (paramNum == 3) tft.fillRect(menuValueRowXpos, yOffset - 1, 15, 8, SCREENFILL);
	else if (paramNum == 4) tft.fillRect(menuValueRowXpos, yOffset - 1, 9, 8, SCREENFILL);
  	}

  	//set spacing
  	if (paramNum >= 0 && paramNum <= 3 || paramNum == 6) {
  		if (paramNum == 0 && menuItemsNewSettings[0] < cfgIdleConstraints[0] + cfgIdleConstraints[2]) strLenght = 100;
  		if (paramNum == 3) strLenght = cfgIncrementValues[menuItemsNewSettings[paramNum]];
  		if (paramNum == 6) strLenght = menuItemsSettings[6];

  		if      (strLenght > 99) xSpacing = 0;
  		else if (strLenght >  9) xSpacing = 3;
  		else if (strLenght < 10) xSpacing = 6;
  	} else if (paramNum == 4) xSpacing = 3;
  	else if (paramNum == 5) {
  		if (menuItemsSettings[paramNum] == 0) xSpacing = 0; else xSpacing = 3;
  	}
  	tft.setTextColor(MENUVALUES);
  	tft.setCursor(menuValueRowXpos + xSpacing, menuYstart + (menuYspacing * paramNum));

    //process and draw the value
  	if (paramNum == 3)      tft.print(cfgIncrementValues[menuItemsNewSettings[paramNum]]);
  	else if (paramNum == 4) tft.print(menuItemsNewSettings[paramNum] == 0 ? "C" : "F");
  	else if (paramNum == 5) tft.print(menuItemsSettings[paramNum] == 0 ? "OFF" : "ON");
  	else if (paramNum == 0) menuItemsNewSettings[paramNum] < cfgIdleConstraints[0]+cfgIdleConstraints[2] ? tft.print("OFF") : tft.print(menuItemsNewSettings[paramNum]);
  	else if (paramNum == 6) tft.print(menuItemsSettings[paramNum]);
  	else                    tft.print(menuItemsNewSettings[paramNum]);

  }



}

void drawMenuScreen() {
	activeScreen = 1;
	highlight = 0;
	tft.fillScreen(SCREENFILL);
	tft.setTextColor(MENULABELS);
  //tft.setFont(&FreeSans12pt7b);
	tft.setFont();
	tft.setTextSize(1.5);

	int i = menuYstart;
	int a = 0;
	while (a < menuItemCount - 1) {
    //tft.setTextColor(MENULABELS);
		tft.setCursor(5, i);
		tft.print(menuItems[a]);

    //tft.setTextColor(0xBDF7);
    //tft.setCursor(130, i);
    // tft.print(menuItemValues[a]);
		drawParam(a);
		i += menuYspacing;
		a++;
	}

	//Exit
	tft.setTextColor(ORANGE);
	tft.setCursor(10, i);
	tft.print(menuItems[a]);
}

void updateTemp(int temperature) {
	temperature = constrain(temperature, (menuItemsSettings[4] == 0 ? minTempValueC : minTempValueF), (menuItemsSettings[4] == 0 ? maxTempValueC : maxTempValueF));
	if (temperature != temperatureValue) {
		unsigned x = (temperatureValue / 100U) % 10;
		unsigned y = (temperature      / 100U) % 10;
		if (x != y) tft.fillRect(40,  0, 77, 50, SCREENFILL);
		else        tft.fillRect(65, 0, 52, 50, SCREENFILL);
		temperatureValue = temperature;
		tft.setTextColor(MAINVALUE);
		tft.setFont(&FreeSans24pt7b);
		tft.setCursor(40, 40);
		tft.print(temperatureValue);
	}
}

void drawHighlight(int itemNumber) {
	for (int i = 0; i < menuItemCount; i++) {
    //tft.drawRect(1, (i == menuItemCount-1 ? 105 : menuYspacing*i), 159, 16, (i == itemNumber) ? (ORANGE) : (ST77XX_BLACK));
		tft.drawRect(1, menuYspacing * i, 159, 16, (i == itemNumber) ? ORANGE : SCREENFILL);
	}
}


void editParam(int paramNum, char *state = "UP") {
	int oldValue = paramNum < 5 ? menuItemsNewSettings[paramNum] : menuItemsSettings[paramNum];
	int newValue;
	
	if (paramNum == 0) {
		newValue = menuItemsNewSettings[paramNum] + (state == "UP" ? cfgIdleConstraints[2] : -cfgIdleConstraints[2]);
		newValue = constrain(newValue, cfgIdleConstraints[0], cfgIdleConstraints[1]);
	} else if (paramNum == 1) {
		newValue = menuItemsNewSettings[paramNum] + (state == "UP" ? 1 : -1);
		newValue = constrain(newValue, 0, 60);
	} else if (paramNum == 2) {
		newValue = menuItemsNewSettings[paramNum] + (state == "UP" ? 1 : -1);
		newValue = constrain(newValue, 0, 30);
	} else if (paramNum == 3) {
		newValue = menuItemsNewSettings[paramNum] + (state == "UP" ? 1 : -1);
		newValue = constrain(newValue, 0, 3);
	} else if (paramNum == 4) {
		newValue = menuItemsNewSettings[paramNum] + (state == "UP" ? 1 : -1);
		newValue = constrain(newValue, 0, 1);
	//parameters 5 and 6 are live and dont use temporary values
	} else if (paramNum == 5) {
		newValue = menuItemsSettings[paramNum] + (state == "UP" ? 1 : -1);
		newValue = constrain(newValue, 0, 1);
	} else if (paramNum == 6) {
		newValue = menuItemsSettings[paramNum] + (state == "UP" ? 5 : -5);
		newValue = constrain(newValue, 0, 100);
	}
	
	if (newValue != oldValue) {
		if (paramNum < 5) menuItemsNewSettings[paramNum] = newValue; //on-exit update for first 5 params
		else 			  menuItemsSettings[paramNum] = newValue;    //live update for the rest
		drawParam(paramNum, 3);
	}
}

void saveParam() {
	float a;

	//check if increment has changed and round up the temperature value
	if (menuItemsNewSettings[3] != menuItemsSettings[3]) {
		a = temperatureValue/cfgIncrementValues[menuItemsNewSettings[3]];
		temperatureValue = round(a)*cfgIncrementValues[menuItemsNewSettings[3]];
		Serial.println("new temperature adjusted after increment change:");
		Serial.println(temperatureValue);
	}

	//check if units have changed and perform conversion and round up to the increment
	if (menuItemsNewSettings[4] != menuItemsSettings[4]) { 
		//menuItemsSettings[4] = newUnits;
		//conversion C to F or vice versa
		a = (menuItemsNewSettings[4] == 1 ? (temperatureValue*1.8+32) : (temperatureValue-32)/1.8)/cfgIncrementValues[menuItemsNewSettings[3]];
		temperatureValue = round(a)*cfgIncrementValues[menuItemsNewSettings[3]];
		Serial.println("new temperature adjusted after unit change:");
		Serial.println(temperatureValue);
	}

	int i = 0;
	while (i < 5) {		//updates first 5 parameters, the others will be changing live
		if (menuItemsNewSettings[i] != menuItemsSettings[i]) menuItemsSettings[i] = menuItemsNewSettings[i];
		i++;
	}
}

void setup() {
	Serial.begin(9600);
	pinMode(encoderPin1, INPUT_PULLUP);
	pinMode(encoderPin2, INPUT_PULLUP);
	pinMode(buttonPin,   INPUT_PULLUP);

 	tft.initR(INITR_BLACKTAB); // Init ST7735S chip, black tab
 	tft.setRotation(1);

 	drawMainScreen();
}



void loop() {

  //button processing
	if (digitalRead(buttonPin) == 0 && millis() - lastBtnPress >= btnDebounce) {
		if (activeScreen == 0) {
			drawMenuScreen();
			drawHighlight(0);
		} else if (activeScreen == 1 && highlight == menuItemCount - 1) {
			saveParam();
			drawMainScreen();
		} else if (activeScreen == 1 && editMode == 0) {
			for (int i = 0; i < menuItemCount; i++) {
				if (highlight == i) {
					editMode = 1;
					editParamNum = i;
					drawParam(i, 1);
					break;
				}
			}
		} else if (editMode == 1) {
			editMode = 0;
			drawParam(editParamNum);
		}
		lastBtnPress = millis();
	}


  //encoder processing
	Astate = digitalRead(encoderPin1);
	Bstate = digitalRead(encoderPin2);
	if (lastAstate != Astate) {
		if (Astate == 0 && Bstate == 0) {
			if (activeScreen == 0) {
				updateTemp(temperatureValue + cfgIncrementValues[menuItemsSettings[3]]);
			} else if (activeScreen == 1 && highlight < menuItemCount - 1 && editMode == 0) {
				highlight++;
				drawHighlight(highlight);
			} else if (editMode == 1) {
				editParam(highlight, "UP");
			}
		} else if (Astate == 1 && Bstate == 0) {
			if (activeScreen == 0) {
				updateTemp(temperatureValue - cfgIncrementValues[menuItemsSettings[3]]);
			} else if (activeScreen == 1 && highlight > 0 && editMode == 0) {
				highlight--;
				drawHighlight(highlight);
			} else if (editMode == 1) {
				editParam(highlight, "DOWN");
			}

		}
		lastAstate = Astate;
	}



  /*tft.fillRect(0, 95, 160, 33, ST77XX_BLACK);
    tft.setCursor(42, 115);
    tft.setTextColor(ST77XX_RED);
    tft.setFont(&FreeSans12pt7b);
    tft.print("No Iron");

    delay(2000);

    tft.fillRect(0, 90, 160, 38, ST77XX_BLACK);
    tft.setCursor(30, 115);
    tft.setTextColor(ST77XX_WHITE);
    tft.setFont(&FreeSans12pt7b);
    tft.print("Heating...");

    delay(2000);

    tft.fillRect(0, 90, 160, 38, ST77XX_BLACK);
    tft.setCursor(45, 115);
    tft.setTextColor(ST77XX_GREEN);
    tft.setFont(&FreeSans12pt7b);
    tft.print("Ready");

    delay(2000);

    tft.fillRect(0, 90, 160, 38, ST77XX_BLACK);
    tft.setCursor(60, 115);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setFont(&FreeSans12pt7b);
    tft.print("Idle");

    delay(2000);*/

}
