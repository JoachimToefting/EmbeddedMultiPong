# Multipong Embedded

## Kort beskrivelse
Projektet går ud på at kunne spille pong over to enheder mkr1010 på oled et oled panel, hvor hver enhed for en side af banen, og status af spillet vil kører over MQTT, så som score og boldens indgangvinkel. 
Opt:
 - [ ] Navn kan skrives på en liste sammen med scoren der kan ligges op gennem mqtt.

## Spillets regler
Vent til der står "Waiting p: 0 o: 0" på skærmen. En spiller trykker på knappen på rotary encoderen, den spiller vil få serveret en bold, hvor modsatte spiller skal vente på at bolden kommer eller den går bag padden (i mål). Padden kan flyttes imens bolden er på banen med rotary encoderen. Man vinder når man har fået 5 mål. (Hvis man ønsker kan der ændres i indstillingerne øverst i koden)

## Hardware
Spillet kører på to MKR wifi 1010 hvor der er tilsluttet en skærm og en rotary encoder. Software og hardware opsætning er ens på begge enheder.

### Opsætning
 ![opsætning](images/20220504_082813.jpg)

 | Display | Board |
 |-------- | ------|
 | GND     | GND   |
 | VCC     | VCC   |
 | SCL     | PIN12 |
 | SDA     | PIN11 |

 | Rotary encoder | Board |
 | -------------- | ----- |
 | GND            | GND   |
 | +              | VCC   |
 | SW             | PIN8  |
 | DT             | PIN9  |
 | CLK            | PIN10 |

## Tour de Code
 Koden er delt op i fire fokus områder:
#### Setup:
Start op af alle hardware delene, og andre protokoller såsom Wifi og MQTT bliver sat op i starten af spillet. Wifi og MQTT kan også blive tjekket og forbundet igen i løbet af programmet.

#### Loop:
Loopen vil blive ved med at kører indtil der bliver sat et flag (actionFlags) som kan fortælle hvad den hvad den skal fortsætte med, så som START_GAME som vil sætte en opponent og en player nummer og vil sende den information ud gennem MQTT.

#### PlayBall:
Denne funktion er den der kører spillet. Den regner Kollisionsdetektion mellem bolden og pladen, tegner bolden og padden på banen, og sender information om den er scoret i mål eller om den skal sende bolden til den anden bane. (Der er ikke sat nogen framelimit på, så den kører så hurtig den kan)

#### OnMessage:
OnMessage er callback functionen der skal sætte flagene så loopen kan fange dem og kører spillet videre. Den laver nogle tjeks hvem MQTT messages er til og kan sætte flag som f.eks. GAME_WON og INCOMMING_GAME. Ikke alle flag kommer fra MQTT strømmen, man kan se hvad der sendes under.

## MQTT struktur
 - Gamedata 
   - Indgående spil kommando. (gamedata/command)
   - Sende bold position og vinkel. (gamedata/ball)
   - Sende "scoret". (gamedata/score)
   - Win kommando. (gamedata/command)


## Mangler
Jeg vil gerne have en mere robust multiplayer oplevelse hvor man kan starte flere forskellige spil på samme MQTT broker (igangværende spil tjekker player nummeret og burde kunne kører). Knappen på rotary encoder kan nogle gange aktiveres af sig selv. Trykker man på knappen imens et spil kører bliver spillet på modstanderens spil ikke stoppet. Vil gerne have haft delt koden lidt mere op i stedet for en main.
