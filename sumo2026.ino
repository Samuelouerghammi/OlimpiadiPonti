#include <Servo.h>

/*
  ouerghammi samuel versione stati finiti -- 06/02/2026




  rielaborazioni codici 
    concetto a stati:andrea
    idea dei millis:marco
    logica di funzionamento:del gaiso/samuelito
    supporto morale: richard
  
              VERSIONE ULTRA DIFENSIVA 
  
  - ATTESA:
    robot fermo per 5 secondi dopo lo start (regolamento)
  - CENTRO:
     oscillazione lenta sul posto (stallo di ricerca)
     micro-scatto   periodico per evitare penalità di inattività a quanto pare il robot non deve stare fermo sennò ci ciulano punti
     passaggio ad ATTACCA solo se il nemico è in range frontale
  -ATTACCA:
      avanzamento alla massima velocità in linea retta
       ritorno a CENTRO se il nemico viene perso
  - SALVA:
     stato di emergenza attivato quando viene rilevato il bordo del ring
     manovre di torno in dre e rotazione per rientrare nell’area di gioco

*/

Servo servoSinistro;
Servo servoDestro;

const int pinServoSinistro = 9;
const int pinServoDestro   = 8;

const int pinLineaSX = A2;
const int pinLineaCX = A1;
const int pinLineaDX = A0;

// Soglie sensori linea
const int LINEA_BIANCO = 300;   // sicuramente sotto il bianco
const int LINEA_NERO   = 400;   // sicuramente  sopra per il nero fino a 900

bool bordoSX = false;
bool bordoCX = false;
bool bordoDX = false;

const int trigFronte = 4;
const int echoFronte = 23;
const int DISTANZA_NEMICO = 25;

const unsigned long START_DELAY = 5000;
const unsigned long SCAPPA_TEMPO = 180;
const unsigned long SCATTO_INTERVALLO = 600;

enum Stato {
  ATTESA,
  CENTRO,
  ATTACCA,
  SALVA
};

Stato stato = ATTESA;
unsigned long timerStato = 0;
unsigned long ultimoScatto = 0;

void ferma() {
  servoSinistro.write(90);
  servoDestro.write(90);
}

void avanti() {
  servoSinistro.write(180);
  servoDestro.write(0);
}

void indietro() {
  servoSinistro.write(0);
  servoDestro.write(150);
}

void giraSinistra() {
  servoSinistro.write(0);
  servoDestro.write(0);
}

void giraDestra() {
  servoSinistro.write(180);
  servoDestro.write(180);
}

void oscillaCentro() {
  servoSinistro.write(100);
  servoDestro.write(80);
}
long distanzaFronte() {
  digitalWrite(trigFronte, LOW);
  delayMicroseconds(2);
  digitalWrite(trigFronte, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigFronte, LOW);

  long durata = pulseIn(echoFronte, HIGH, 20000);
  if (durata == 0) return -1;
  return durata * 0.034 / 2;
}

void setup() {
  Serial.begin(9600);
  Serial.println("=== MINI SUMO DEBUG AVVIATO ===");

  servoSinistro.attach(pinServoSinistro);
  servoDestro.attach(pinServoDestro);

  pinMode(trigFronte, OUTPUT);
  pinMode(echoFronte, INPUT);

  ferma();
  timerStato = millis();
}

void loop() {
  unsigned long now = millis();

  // ---------- LETTURA LINEA ----------
  int linea_colore_sinistro = analogRead(pinLineaSX);
  int linea_colore_centrale = analogRead(pinLineaCX);
  int linea_colore_destro = analogRead(pinLineaDX);

  if (linea_colore_sinistro < LINEA_BIANCO) bordoSX = true;
  else if (linea_colore_sinistro > LINEA_NERO) bordoSX = false;

  if (linea_colore_centrale < LINEA_BIANCO) bordoCX = true;
  else if (linea_colore_centrale > LINEA_NERO) bordoCX = false;

  if (linea_colore_destro < LINEA_BIANCO) bordoDX = true;
  else if (linea_colore_destro > LINEA_NERO) bordoDX = false;

  long dF = distanzaFronte();

  Serial.print("LINEA SX:" +linea_colore_sinistro);
  Serial.print(bordoSX ? " bordo sinistro rilevato " : " [OK BORDO SX. IN AREA SICURA] ");

  Serial.println("LINEA CX:"+linea_colore_centrale);
  Serial.print(bordoCX ? " bordo centrale rilevato " : " [OK BORDO CX. IN AREA SICURA] ");

  Serial.print("LINEA DX:"+ linea_colore_destro);
  Serial.print(bordoDX ? "bordo destro rilevato " : " [OK BORDO DX. IN AREA SICURA] ");

  Serial.print(" | DISTANZA FRONTALE:");
  Serial.print(dF);

  Serial.print(" | STATO:");
  switch (stato) {
    case ATTESA:  Serial.println("ATTESA"); break;
    case CENTRO:  Serial.println("CENTRO"); break;
    case ATTACCA: Serial.println("ATTACCA"); break;
    case SALVA:   Serial.println("SALVA"); break;
  }

  delay(150); // SOLO PER DEBUG OPZIONALE DA TOGLIERE IN GARA

  // ---------- evita bordo--------
  if (bordoSX || bordoCX || bordoDX) {
    stato = SALVA;
    timerStato = now;
  }

  // ----------STATI----------
  switch (stato) {

    case ATTESA:
      ferma();
      if (now - timerStato >= START_DELAY) {
        stato = CENTRO;
      }
      break;

    case CENTRO:
      oscillaCentro();

      if (now - ultimoScatto > SCATTO_INTERVALLO) {
        avanti();
        delay(80);
        ultimoScatto = now;
      }

      if (dF > 0 && dF < DISTANZA_NEMICO) {
        stato = ATTACCA;
      }
      break;

    case ATTACCA:
      avanti();
      if (dF == -1 || dF > DISTANZA_NEMICO) {
        stato = CENTRO;
      }
      break;

    case SALVA:
      if (bordoCX) {
        indietro();
        delay(150);
        giraDestra();
        delay(120);
      }
      else if (bordoSX) {
        giraDestra();
      }
      else if (bordoDX) {
        giraSinistra();
      }
      else {
        indietro();
      }

      if (now - timerStato > SCAPPA_TEMPO) {
        stato = CENTRO;
      }
      break;
  }
}
