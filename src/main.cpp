#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFiNINA.h>
#include <MQTT.h>
#include <Encoder.h>

#include "arduino_secrets.h"


//Screen
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 4
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Wifi
const char ssid[] = SECRET_SSID;
const char pass[] = SECRET_PASS;
WiFiClient net;
int status = WL_IDLE_STATUS;

//MQTT
MQTTClient client;
const char username[] = SECRET_MQTTU;
const char password[] = SECRET_MQTTP;
const char hostmqtt[] = SECRET_MQTTH;
////MQTT Topics
const char tball[] = "gamedata/ball";
const char tscore[] = "gamedata/score";
const char tcommand[] = "gamedata/command";

//Rotory encoder
Encoder myEnc(10,9);
int oldEncPos = 0;

//game settings
int bsize = 5;
int ppos = 0;
int pheight = 20;
int pwidth = 5;
int player = 0;
int opponent = 0;
int inypos = SCREEN_HEIGHT/2;
int indir = 0;
int myscore = 0;
int maxscore = 5;
//Hvad der skal køres efter hver loop;
#define START_GAME 0
#define CANCEL_GAME 1
#define INCOMING_GAME 2
#define PLAYBALL 3
#define GAME_LOST 4
#define MY_SCORE_ADD 5
#define GAME_WON 6
byte actionFlags = 0;


//prototyping
void connect(void);
bool PlayBall(int, bool);
void DrawBoard(int, int);
void SetPad(int);
void NewBall(void);
void SendData(String, String);
void OnMessage(String &topic, String &payload);
void RotaryButton(void);
void Reset(void);



void setup() {

	//Reset pin
	digitalWrite(PIN_A1, HIGH);
	pinMode(PIN_A1, OUTPUT);

	//Serial
	Serial.begin(115200);
	// while (!Serial)
	// {
	// 	; //Wait for serial
	// }
	Serial.println("Serial connected starting screen");

	//Screen
	// SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
	if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) 
	{ // Address for 128x64
		Serial.println(F("SSD1306 allocation failed"));
		for(;;); // Don't proceed, loop forever
	}
	display.clearDisplay();
	display.setTextSize(1); // Normal 1:1 pixel scale
	display.setTextColor(WHITE); // Draw white text
	display.setCursor(0,0); // Start at top-left corner
	display.println("Screen Ok");
	display.display();


	//MQTT and Wifi
	display.println("Connecting wifi and mqtt");
	display.display();
	connect();
	
	//Attach interrupt
	Serial.println("Attaching interrupts");
	display.println("Attaching interrupts");
	display.display();
	attachInterrupt(digitalPinToInterrupt(8), RotaryButton, LOW);


	Serial.println("Setup complete");
	display.clearDisplay();
}

void loop() {
	if (bitRead(actionFlags, START_GAME))
	{
		myscore = 0;
		player = random(1,10);
		do
		{
			opponent = random(1,10);
		} while (player == opponent);
		
		String sendgame = "";
		sendgame += "i";
		sendgame += "p:";
		sendgame += player;
		sendgame += "o:";
		sendgame += opponent;
		SendData(tcommand, sendgame);
		actionFlags = 0;
		NewBall();
	}
	else if (bitRead(actionFlags, INCOMING_GAME))
	{
		display.clearDisplay();
		display.println("Game startet waiting for ball");
		display.display();
		actionFlags = 0;
	}
	else if (bitRead(actionFlags, PLAYBALL))
	{
		bool scored = PlayBall(inypos, indir);
		bitClear(actionFlags, PLAYBALL);
	}
	else if (bitRead(actionFlags, MY_SCORE_ADD))
	{
		myscore++;
		if(myscore >= maxscore){
			String sendwin = "";
			sendwin += "w";
			sendwin += "p:";
			sendwin += player;
			SendData(tcommand, sendwin);
			actionFlags = 0;
			bitSet(actionFlags, GAME_WON);
		}
		else{
			NewBall();
			actionFlags = 0;
		}
	}
	else if (bitRead(actionFlags, GAME_LOST))
	{
		Reset();
		display.clearDisplay();
		display.setCursor(0,0);
		display.println("You Lost");
		display.display();
		delay(1000);
		display.clearDisplay();
		actionFlags = 0;
	}
	else if(bitRead(actionFlags, GAME_WON))
	{
		Reset();
		display.clearDisplay();
		display.setCursor(0,0);
		display.println("You Won");
		display.display();
		delay(1000);
		display.clearDisplay();
		actionFlags = 0;
	}
	else
	{
		display.setCursor(0,0);
		String waitstring = "Waiting";
		waitstring += " p: ";
		waitstring += player;
		waitstring += " o: ";
		waitstring += opponent;
		display.println(waitstring);
		display.display();
	}
	
	
	//Needs delay time for reading
	delay(100);
	client.loop();
}

void connect()
{
	//Wifi
	Serial.print("checking wifi...\n");
	while (status != WL_CONNECTED) {
    	Serial.print("Attempting to connect to SSID: ");
    	Serial.println(ssid);
    	// Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    	status = WiFi.begin(ssid, pass);
    	// wait 10 seconds for connection:
    	delay(5000);
	}
	
	//MQTT
	Serial.print("\nconnecting");
	client.begin(hostmqtt, net);
	client.onMessage(OnMessage);
	String namebuilder = "unit";
	namebuilder += random(1,1000);
	while (!client.connect(namebuilder.c_str(), username, password, false)) 
	{
    	Serial.print(".");
    	delay(1000);
	}
	Serial.println("\nconnected!");
	client.subscribe("gamedata/command");
	client.subscribe("gamedata/ball");
	client.subscribe("gamedata/score");
}

bool PlayBall(int ypos,bool dir){
	int bypos = ypos;
	//true == ned
	bool bdir = dir;
	int rbxpos = SCREEN_WIDTH;
	for (int bxpos = 0; bxpos < SCREEN_WIDTH ; bxpos++)
	{
		if (bdir)
		{
			bypos++;
		}
		else
		{
			bypos--;
		}
		if (bypos > SCREEN_HEIGHT - bsize || bypos < 0 + bsize)
		{
			bdir = !bdir;
		}
		if(bxpos > SCREEN_WIDTH - pwidth - bsize)
		{
			if(bypos > ppos && bypos < ppos + pheight){
				rbxpos = bxpos;
				break;
			}
		}
		DrawBoard(bxpos, bypos);
	}
	//check if scored
	//Todo better pad detection
	if (bypos < ppos || bypos > ppos + pheight)
	{
		display.setCursor(0, SCREEN_HEIGHT - 7);
		display.println("Other player scored");
		display.display();
		String sendscore = "";
		sendscore += "o:";
		sendscore += opponent;
		sendscore += "s:1";
		SendData(tscore, sendscore);
		return true;
	}
	for (int bxpos = rbxpos; bxpos > 0; bxpos--)
	{
		if (bdir)
		{
			bypos++;
		}
		else
		{
			bypos--;
		}
		if (bypos > SCREEN_HEIGHT - bsize || bypos < 0 + bsize)
		{
			bdir = !bdir;
		}
		DrawBoard(bxpos, bypos);
	}
	String sendpos = "";
	sendpos += "p:";
	sendpos += player;
	sendpos += "y:";
	sendpos += bypos;
	sendpos += "d:";
	sendpos += dir;
	SendData(tball, sendpos);
	//needs delay to function
	return false;
}

void DrawBoard(int bxpos, int bypos)
{
	SetPad(myEnc.read());
	display.clearDisplay();
	//pad
	display.drawRect(SCREEN_WIDTH - pwidth, ppos, pwidth, pheight, WHITE);
	//ball
	display.drawCircle(bxpos, bypos, bsize, WHITE);
	//score
	display.setCursor(SCREEN_WIDTH/2,SCREEN_HEIGHT/2);
	display.println(myscore);
	display.display();
}

void SetPad(int encoderpos)
{
	if(encoderpos != oldEncPos)
	{
		int change = encoderpos - oldEncPos;
		int newpos = ppos;
		if (change > 1)
		{
			newpos = ppos + 4;
		}
		else if(change < -1)
		{
			newpos = ppos - 4;
		}
		
		if(newpos > SCREEN_HEIGHT - pheight)
		{
			ppos = SCREEN_HEIGHT - pheight;
		}
		else if(newpos < 0)
		{
			ppos = 0;
		}
		else
		{
			ppos = newpos;
		}
		oldEncPos = encoderpos;
	}
}

void NewBall(){
	PlayBall(random(bsize, SCREEN_HEIGHT - bsize),random(0, 2));
}

void SendData(String topic, String payload){
	if (!client.connected()) {
    	connect();
	}
	client.publish(topic, payload, false, 2);
}

void OnMessage(String &topic, String &payload){
	//Serial.println(payload);
	if(topic == tball)
	{
		if (payload.substring(2,payload.indexOf("y")).toInt() == opponent)
		{
			inypos = payload.substring(payload.indexOf("y")+2,payload.indexOf("d")).toInt();
			indir = (int)payload[payload.length()-1];
			bitSet(actionFlags, PLAYBALL);
		}
	}
	else if(topic == tscore)
	{
		if(payload.substring(2, payload.indexOf("s")).toInt() == player)
		{
			bitSet(actionFlags, MY_SCORE_ADD);
		}
	}
	else if(topic == tcommand)
	{
		if(payload[0] == 'i')
		{
			int incommingplayer = payload.substring(payload.indexOf("p") + 2, payload.indexOf("o")).toInt();
			if(incommingplayer != player)
			{
				int incommingopponent = payload.substring(payload.indexOf("o") + 2, payload.length()).toInt();
				opponent = incommingplayer;
				player = incommingopponent;
				bitSet(actionFlags, INCOMING_GAME);
			}
		}
		else if(payload[0] == 'w')
		{
			int incommingplayer = payload.substring(payload.indexOf("p") + 2, payload.length()).toInt();
			if(incommingplayer == opponent)
			{
				bitSet(actionFlags, GAME_LOST);
			}
		}
		else if(payload[0] == 'r')
		{
			digitalWrite(PIN_A1, LOW);
		}
	}
}

void RotaryButton()
{
	bitSet(actionFlags, START_GAME);
}

void Reset()
{
	myscore = 0;
	ppos = 0;
	player = 0;
	opponent = 0;
}