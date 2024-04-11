/*
    Author: Timothy Ausec
    Date: 4/10/24
    Organization: RHIT SmallSat
    This program is meant for an Arduino Uno R3.
    This program drives a three phase BLDC with variable speed and current spike mitigation.
    This program references the following datasheet: http://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf
*/

/*Pin Assignments*/
// User Interface Pins
const int speed_up_button = 2; //speeds up motor (mod NUMBER_OF_SPEEDS)
const int oh_shit_button = 3; //stops it all in its tracks
// Source/Sink pins
const int A_SOURCE = 4;
const int A_SINK = 5;
const int B_SOURCE = 6;
const int B_SINK = 7;
const int C_SOURCE = 8;
const int C_SINK = 9;
// Crossover back-emf input pins
const int A_BEMF = 11;
const int B_BEMF = 12;
const int C_BEMF = 13;

/*Constant Assignments*/
// Device Details
const int SYS_CLK_MHZ = 16; //16MHz
// Phases
const char PHASE_A = 'A';
const char PHASE_B = 'B';
const char PHASE_C = 'C';
// Delay
const double STARTUP_DELAY_MS = 5;
// Numbers of things
const int NUMBER_OF_PHASES = 3;
const int NUMBER_OF_COMBINATIONS = 6;
const int NUMBER_OF_SPEEDS = 5;

// RPM Bounds and Options
const int MIN_RPM = 1000;
const int MAX_RPM = 10000;
const int CURRENT_SPIKE_DELAY_US = 6;
const int WAIT_DELAY_US = 30;
int RPM_COUNTER_VALUES[NUMBER_OF_SPEEDS];

// Timer Bounds and Options
const int COIL_TIMER_PRESCALER = 8;
//const int RPM_TCOMPARE_MIN_VALUE = (SYS_CLK_MHZ*60*pow(10,6)) / (6*MIN_RPM*COIL_TIMER_PRESCALER);
const int RPM_TCOMPARE_MIN_VALUE =2000;
const int RPM_TCOMPARE_MAX_VALUE = 200;
const int SPIKE_TCOMPARE_VALUE = 16;
const int WAIT_TCOMPARE_VALUE = 60;

/* Global Variables */
int current_coil_combination;
bool rpm_changed;
int button_counter;

/*
   Each entry in the below array goes as such: [SOURCE,SINK,HIGHZ].
   So [PHASE_A,PHASE_B,PHASE_C] goes PHASE_A=source, PHASE_B=sink, PHASE_C=highZ
*/
char PATTERN[][NUMBER_OF_PHASES] = {{PHASE_A, PHASE_B, PHASE_C}, {PHASE_A, PHASE_C, PHASE_B}, {PHASE_B, PHASE_C, PHASE_A}, {PHASE_B, PHASE_A, PHASE_C}, {PHASE_C, PHASE_A, PHASE_B}, {PHASE_C, PHASE_B, PHASE_A}}; //CLOCKWISE

void setup()
{
  //Initialize Serial for debugging
  // Serial.begin(57600);
  
  // Disable interrupts
  noInterrupts();
  /* Initialize Timers */

  // Control regsiters: CTC mode (see page 100)
  TCCR1A = 0; //clear control A
  TCCR1B = 0; //clear control B
  TCNT1 = 0; //set timer1 counter to 0
  
  //I think the below two lines are unnecessary - T
  // TCCR1A |= (1 << COM1A1) | (1 << COM1A0); //compare output mode to set outputs; see page 108
  // TCCR1A |= (1 << COM1A0); //compare output mode to set outputs; see page 108
  
  TCCR1B |= (1 << WGM12); // enable CTC; see page 110
  TCCR1B |= (1 << CS11); // 1:8 prescaler = 2MHz timer 1 (16 bits)
  TIMSK1 |= (1 << OCIE1A); // enable compare A interrupt; see page 112
  TIMSK1 |= (1 << OCIE1B); // enable compare B interrupt; see page 112

  // Compare registers
  OCR1A = RPM_TCOMPARE_MIN_VALUE; //CCR1A for RPM control
  OCR1B = SPIKE_TCOMPARE_VALUE; //CCR1B for spike control (assign wait time and spike time)

  /* Initialize Pins  */

  //Outputs
  pinMode(A_SOURCE, OUTPUT);
  pinMode(A_SINK, OUTPUT);
  pinMode(B_SOURCE, OUTPUT);
  pinMode(B_SINK, OUTPUT);
  pinMode(C_SOURCE, OUTPUT);
  pinMode(C_SINK, OUTPUT);
  Source(PHASE_A);
  Sink(PHASE_B);
  HighZ(PHASE_C);

  //Inputs
  int interrupt_number1 = digitalPinToInterrupt(speed_up_button);
  attachInterrupt(interrupt_number1, ISR_Speed_Up, FALLING);
  int interrupt_number2 = digitalPinToInterrupt(oh_shit_button);
  attachInterrupt(interrupt_number2, ISR_Oh_Shit, FALLING);

  // Initialize variables
  for (int i = 0; i < NUMBER_OF_SPEEDS; i++)
  { // assign speeds from slowest to fastest in even increments
    RPM_COUNTER_VALUES[i] = -((RPM_TCOMPARE_MAX_VALUE - RPM_TCOMPARE_MIN_VALUE) * i / (NUMBER_OF_SPEEDS - 1)) + RPM_TCOMPARE_MIN_VALUE;
  }
  rpm_changed = false;
  current_coil_combination = 0;
  button_counter = 0;

  //Delay
  delay(0.5);

  // Enable Interrupts
  interrupts();
  
}

void loop()
{ // do nothing
  delay(10);
}
/*
   This function advances the pattern upon interrupt and changes the speed if queued by
   ISR_Speed_Up().
*/
ISR(TIMER1_COMPA_vect)
{ // Advances pattern upon interrupt; CTC resets TCNT1 automatically
  AdvancePattern();
  OCR1B = SPIKE_TCOMPARE_VALUE;
  if (rpm_changed)
  {
    rpm_changed = false;
    OCR1A = RPM_COUNTER_VALUES[button_counter];
  }
}
/*
   This function either shorts the current coil or re-energizes it to prevent large current
   spikes from destroying performance/FETs.
*/
ISR(TIMER1_COMPB_vect)
{ // Shorts current coil pair upon interrupt
  static int last_timer_count = SPIKE_TCOMPARE_VALUE; //must have for B compare - TCNT1=SPIKE_TCOMPARE_VALUE at first call
  int current_timer_count = TCNT1; //do this so TCNT1 doesn't change before condition checks
  if ((current_timer_count - last_timer_count) == SPIKE_TCOMPARE_VALUE)
  { // short energized coil if it was being energized at time of interrupt
    Sink(PATTERN[current_coil_combination][0]); //see PATTERN declaration for more details

    OCR1B += WAIT_TCOMPARE_VALUE; //set next interrupt for WAIT_TIME operations away
  }
  else if ((current_timer_count - last_timer_count) == WAIT_TCOMPARE_VALUE)
  { // re-energize coil if it was shorted at time of interrupt
    Source(PATTERN[current_coil_combination][0]); //see PATTERN declaration for more details
    OCR1B += SPIKE_TCOMPARE_VALUE; //set next interrupt for SPIKE_TIME operations away
  }
  last_timer_count = current_timer_count;
}
/*
   This function queues a speed change as dictated by the declration of RMP_COUNTER_VALUES.
   The RPM timer ISR changes the speed when next called.
*/
void ISR_Speed_Up()
{ // Increment Speed Level
  button_counter = (button_counter + 1) % NUMBER_OF_SPEEDS;
  rpm_changed = true; //queue OCR1A change for timer1 compare A ISR
  delay(0.5); //lazy debounce
}
/*
   This function turns on or off the driver depending on its current state.
   Currently on -> off. Currently off -> on.
*/
void ISR_Oh_Shit()
{ // If stopping, everything sinks! ALL INTERRUPTS STOP
  static bool stopping = true;
  if (stopping)
  {
    noInterrupts();
    Sink(PHASE_A);
    Sink(PHASE_B);
    Sink(PHASE_C);
    stopping = false; //don't stop next time
  }
  else
  { // If starting, restart pattern
    interrupts();
    current_coil_combination = 0;
    AdvancePattern(); //restarts pattern
    stopping = true; //stop next time
  }
  delay(0.5); //lazy debounce
}

/*
   This function advances the pattern according to the declrataion of PATTERN above
*/
void AdvancePattern()
{
  current_coil_combination = (current_coil_combination + 1) % NUMBER_OF_COMBINATIONS;
  Source(PATTERN[current_coil_combination][0]);
  Sink(PATTERN[current_coil_combination][1]);
  HighZ(PATTERN[current_coil_combination][2]);
}

/*
   This function turns on the low-side FET and off the high-side FET for the given phase
*/
void Sink(char phase)
{
  switch (phase)
  {
    case PHASE_A:
      digitalWrite(A_SOURCE, HIGH);
      digitalWrite(A_SINK, HIGH);
      break;
    case PHASE_B:
      digitalWrite(B_SOURCE, HIGH);
      digitalWrite(B_SINK, HIGH);
      break;
    case PHASE_C:
      digitalWrite(C_SOURCE, HIGH);
      digitalWrite(C_SINK, HIGH);
      break;
    default:
      break;
  };
}
/*
   This function turns on the high-side FET and off the low-side FET for the given phase
*/
void Source(char phase)
{
  switch (phase)
  {
    case PHASE_A:
      digitalWrite(A_SOURCE, LOW);
      digitalWrite(A_SINK, LOW);
      break;
    case PHASE_B:
      digitalWrite(B_SOURCE, LOW);
      digitalWrite(B_SINK, LOW);
      break;
    case PHASE_C:
      digitalWrite(C_SOURCE, LOW);
      digitalWrite(C_SINK, LOW);
      break;
    default:
      break;
  };
}
/*
   This function turns off both the high and low side FETS for the given phase
*/
void HighZ(char phase)
{
  switch (phase)
  {
    case PHASE_A:
      digitalWrite(A_SOURCE, HIGH);
      digitalWrite(A_SINK, LOW);
      break;
    case PHASE_B:
      digitalWrite(B_SOURCE, HIGH);
      digitalWrite(B_SINK, LOW);
      break;
    case PHASE_C:
      digitalWrite(C_SOURCE, HIGH);
      digitalWrite(C_SINK, LOW);
      break;
    default:
      break;
  };
}
