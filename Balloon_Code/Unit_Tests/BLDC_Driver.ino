/*
 * This program is meant for an Arduino Uno R3.
 * This program drives a three phase BLDC with variable speed and back-emf feedback control.
*/
/*Pin Assignments*/
// Source/Sink pins
int A_SOURCE=2;
int A_SINK=3;
int B_SOURCE=4;
int B_SINK=5;
int C_SOURCE=6;
int C_SINK=7;
// Crossover back-emf input pins
int A_BEMF=11;
int B_BEMF=12;
int C_BEMF=13;

/*Constant Assignments*/
const char PHASE_A='A';
const char PHASE_B='B';
const char PHASE_C='C';

const int STARTUP_DELAY_MS=10;

const int NUMBER_OF_PHASES=3;
const int NUMBER_OF_COMBINATIONS=6;

/*
 * Each entry in the below array goes as such: [SOURCE,SINK,HIGHZ].
 * So [PHASE_A,PHASE_B,PHASE_C] goes PHASE_A=source, PHASE_B=sink, PHASE_C=highZ
 */
char PATTERN[][NUMBER_OF_PHASES]={{PHASE_A,PHASE_B,PHASE_C}, {PHASE_A,PHASE_C,PHASE_B}, {PHASE_B,PHASE_C,PHASE_A},{PHASE_B,PHASE_A,PHASE_C}, {PHASE_C,PHASE_A,PHASE_B}, {PHASE_C,PHASE_B,PHASE_A}}; //CLOCKWISE

void setup() 
{
  pinMode(A_SOURCE,OUTPUT);
  pinMode(A_SINK,OUTPUT);
  pinMode(B_SOURCE,OUTPUT);
  pinMode(B_SINK,OUTPUT);
  pinMode(C_SOURCE,OUTPUT);
  pinMode(C_SINK,OUTPUT);
  digitalWrite(A_SOURCE,HIGH);
  digitalWrite(A_SINK,LOW);
  digitalWrite(B_SOURCE,LOW);
  digitalWrite(B_SINK,HIGH);
  digitalWrite(C_SOURCE,LOW);
  digitalWrite(C_SINK,LOW);
  delay(0.5);
}

void loop() 
{
  static int loop_delay=STARTUP_DELAY_MS;
  AdvancePattern();
  delay(loop_delay);
}

void AdvancePattern()
{
  static int combination_number=0;
  combination_number=(combination_number+1)%NUMBER_OF_COMBINATIONS;
  Source(PATTERN[combination_number][0]);
  Sink(PATTERN[combination_number][1]);
  HighZ(PATTERN[combination_number][2]);
}

/*
 * This function turns on the low-side FET and off the high-side FET for the given phase
 */
void Sink(char phase)
{
  switch(phase)
  {
    case PHASE_A:
    digitalWrite(A_SOURCE,LOW);
    digitalWrite(A_SINK,HIGH);
    break;
    case PHASE_B:
    digitalWrite(B_SOURCE,LOW);
    digitalWrite(B_SINK,HIGH);
    break;
    case PHASE_C:
    digitalWrite(C_SOURCE,LOW);
    digitalWrite(C_SINK,HIGH);
    break;
    default:
    break;
  };
}
/*
 * This function turns on the high-side FET and off the low-side FET for the given phase
 */
void Source(char phase)
{
    switch(phase)
  {
    case PHASE_A:
    digitalWrite(A_SOURCE,HIGH);
    digitalWrite(A_SINK,LOW);
    break;
    case PHASE_B:
    digitalWrite(B_SOURCE,HIGH);
    digitalWrite(B_SINK,LOW);
    break;
    case PHASE_C:
    digitalWrite(C_SOURCE,HIGH);
    digitalWrite(C_SINK,LOW);
    break;
    default:
    break;
  };
}
/*
 * This function turns off both the high and low side FETS for the given phase
 */
void HighZ(char phase)
{
      switch(phase)
  {
    case PHASE_A:
    digitalWrite(A_SOURCE,LOW);
    digitalWrite(A_SINK,LOW);
    break;
    case PHASE_B:
    digitalWrite(B_SOURCE,LOW);
    digitalWrite(B_SINK,LOW);
    break;
    case PHASE_C:
    digitalWrite(C_SOURCE,LOW);
    digitalWrite(C_SINK,LOW);
    break;
    default:
    break;
  };
}
