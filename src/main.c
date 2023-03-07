#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <buttons.h>
#include <leds.h>
#include <display.h>
#include <usart.h>
#include <sound.h>
#include <timer.h>
#include <string.h>

#define R_CORNER ']'
#define L_CORNER '['
#define BALL 'O'
#define EMPTY_SPACE '-'
#define BOARD_LENGTH 35
#define BUTTON1 PC1
#define BUTTON2 PC2
#define BUTTON3 PC3
#define PLAYER1 'P1'
#define PLAYER2 'P2'
#define DELAY 550 // ms
#define MAX_ROUNDS 5

// global variables
const double TUNES[] = {523.250, 698.460, 987.770, 1046.500, 783.990, 587.330, 659.250, 880.00};
const int MULTIPLE = 250;
int player_scores[] = {0, 0};
int player1_rebound = 0;
int player2_rebound = 0;
int ball_speed = 50; //  50 for default
int ball_position = BOARD_LENGTH / 2;
char ping_pong_table[BOARD_LENGTH + 1]; // +1 for line terminator \0
int ball_threshold = 5;
int player_turn = 0;
int direction = 1; // left: -1 | right: 1
int round_number = 1;
int timer_count = 0;
int elapsed_time = 0;

void startingSoundEffect() {
  int period = 10000;
  for (int i = 0; i < 8; i++) { 
    playTone(TUNES[3], 140);
    soundDelay(period);
    period -= 5000;
  }
}

void player1ScoreSound() {
  for (int i = 0; i < 8; i++) { 
    playTone(TUNES[6], 100);
    soundDelay(DELAY * 10);
  }
}

void player2ScoreSound() {
  for (int i = 0; i < 8; i++) { 
    playTone(TUNES[4], 100);
    soundDelay(DELAY * 10);
  }
}

void victoryTheme() {
  for (int tune = 0; tune < (sizeof(TUNES) / sizeof(TUNES[0])); tune++) {
    playTone(TUNES[tune], 200);
  }
  for (int tune = (sizeof(TUNES) / sizeof(TUNES[0])) - 1; tune >= 0; tune--) {
    playTone(TUNES[tune], 180);
  }
}

void displayGameRules() {
  printString("\n\n=====   PING  PONG   =====\n\n");
  printString(">> WELCOME GAMER!\n");
  printString(">> Rotate the potentiometer if you want to adjust your ball speed.\n");
  printString(">> You can press Button-2 to pause the game and adjust the ball speed during game.\n");
  printString(">> Player-1 uses Button-1 (far left) and Player-2 uses Button-3 (far right)\n");
  printString(">> This game has 5 rounds, after 5 rounds the player with the most scores will win!\n");
  printString("\n>> Press Button-1 or Button-3 to start\n");
}

void initPingPongTable() {
  ping_pong_table[0] = L_CORNER;
    for (int i = 1; i < BOARD_LENGTH; i++) {
        ping_pong_table[i] = EMPTY_SPACE;
    }
  ping_pong_table[(BOARD_LENGTH / 2)] = BALL;
  ping_pong_table[BOARD_LENGTH - 1] = R_CORNER;
  ping_pong_table[BOARD_LENGTH] = '\0';
}

void printPongTable() {
  char *p = &ping_pong_table[0];
  printf("\n\n%s", p);
  // printf("\n\n%s", &ping_pong_table[0]); // other way
}

int didPlayerOnePlay() {
  if (player_turn == 1 && buttonPressed(1)) {
    if (buttonReleased(1)) {
      ++player1_rebound;
    }
    return 1;
  } else {
    return 0;
  }
}

int didPlayerTwoPlay() {
  if (player_turn == 2 && buttonPressed(3)) {
    if (buttonReleased(3)) {
      ++player2_rebound;
    }
    return 1;
  } else {
    return 0;
  }
}

// when a player scores a point this method will be called
void scoreFlash() {
  for (int i = 0; i < 5; i++) {
    lightUpAllLEDS();
    _delay_ms(150);
    lightOffAllLEDS();
    _delay_ms(150);
  }
}

void displayScores() {
  for (int i = 0; i < 3; i++) {
    for (int i = 0; i < 50; i++) {
      writeCharToSegment(0, 'P');
      _delay_ms(5);
      writeNumberToSegment(1, 1);
      _delay_ms(5);
      writeNumberToSegment(2, (player_scores[0] / 10) % 10);
      _delay_ms(5);
      writeNumberToSegment(3, player_scores[0] % 10);
      _delay_ms(5);
    }

  for (int i = 0; i < 50; i++) {
      writeCharToSegment(0, 'P');
      _delay_ms(5);
      writeNumberToSegment(1, 2);
      _delay_ms(5);
      writeNumberToSegment(2, (player_scores[1] / 10) % 10);
      _delay_ms(5);
      writeNumberToSegment(3, player_scores[1] % 10);
      _delay_ms(5);
    }
  }
}

void score(int player) {
  player_scores[0] += player == 1 ? 1 : 0;
  player_scores[1] += player == 2 ? 1 : 0;
  ball_position = BOARD_LENGTH / 2;
  ++round_number;
  player_turn = 0;
  direction = player == 1 ? 1 : -1;

  int playerScore = player == 1 ? player_scores[0] : player_scores[1];
  printf("\n\nROUND %d/%d", (round_number - 1), MAX_ROUNDS);
  printf("\n\nPLAYER %d WON THE ROUND WITH 1 POINT!\nPLAYER %d's Score: %d\n\n", player, player, playerScore);
  player == 1 ? player1ScoreSound() : player2ScoreSound();
  scoreFlash();
  displayScores();
  _delay_ms(DELAY);
}

void moveTheBall() {
  int prevPosition = ball_position;
  ping_pong_table[prevPosition] = EMPTY_SPACE;
  ball_position += direction;


  if (ball_position == 0) {
    score(2);
  } else if (ball_position == BOARD_LENGTH - 1) { 
    score(1);
  } else {
    ping_pong_table[ball_position] = BALL;
  }
}

void setTurnState() {
  if (ball_position <= ball_threshold) {
    player_turn = 1;
  } else if (ball_position >= (BOARD_LENGTH - ball_threshold - 1)) {
    player_turn = 2;
  } else {
    player_turn = 0;
  }
} 

void applySpeed(int value) {
  int speed = 255 - value; // potentiometer's max value is 255
  while (0 < speed) {
    _delay_ms(1);
    --speed;
  }
}

void initPotentiometer() {
  ADMUX |= (1 << REFS0); // reference voltage, currently using 5V
  ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); /// division factor: 128
  ADCSRA |= (1 << ADEN); // enable ADC (analog to digital converter)
}

void initDependencies() {
  enableSound();
  soundOff();
  initPotentiometer();
  initUSART();
  enableTimer();
  initDisplay();
  enableAllLEDS();
  enableAllButtons();
}

void LEDCountDown() {
  lightUpAllLEDS();
  _delay_ms(DELAY);
  for (int i = 3; i >= 0; i--) {
    lightOff(i);
    printf("\nGame Starts in %d\n", i);
    _delay_ms(DELAY);
  }
}

void adjustBallSpeed(int *ballSpeedPtr) {
  while (!bit_is_clear(PINC, BUTTON1) && !bit_is_clear(PINC, BUTTON3)) {
      ADCSRA |= ( 1 << ADSC ); // start digital convertion
      loop_until_bit_is_clear(ADCSRA, ADSC); // waiting until conversion is complete
      uint8_t potentiometerValue = ADC;
      *ballSpeedPtr = (int) potentiometerValue;
      writeNumberAndWait(ball_speed, DELAY);
      printf("\nBall Speed: %d\n", ball_speed);
    }
    printf("\nBALL SPEED: %d\n\n", ball_speed);
    startingSoundEffect();
}

ISR(PCINT1_vect) {
  if (buttonPressed(2)) {
    printString("\n\nGAME PAUSED\nAdjust your ball speed\nPress Button-1 or Button-3 to resume your game\n\n");
    adjustBallSpeed(&ball_speed);
    LEDCountDown();
  }
}

// happens every 4ms ~ 4ms * 250 == 1sec
ISR(TIMER2_COMPA_vect) {
  timer_count++;
  if (timer_count % MULTIPLE == 0 && timer_count != 0) {
    ++elapsed_time;
  }
}

void activatePauseInterrupt() {
  PCICR |= _BV(PCIE1);
  PCMSK1 |= _BV(BUTTON2); 
  sei();
}

void printFloat(float number) {
    printf("%d.",(int)number);
    int sec = (number - (int)number) * 10;
    printf("%d",abs(sec));
}

void printElapsedTime() {
  if (elapsed_time / 60 < 1) {
    printf("\n** This game took %d seconds **\n", elapsed_time);
  } else{
    printf("\n** This game took ");
    printFloat((float)elapsed_time / 60);
    printf(" minutes **\n");
  }
}


void gameOver() {
  int winner = player_scores[0] > player_scores[1] ? 1 : 2;
  for (int i = 0; i < 70; i++) {
    writeCharToSegment(0, 'P');
    _delay_ms(5);
    writeNumberToSegment(1, winner);
    _delay_ms(5);
    writeNumberToSegment(2, (player_scores[winner - 1] / 10) % 10);
    _delay_ms(5);
    writeNumberToSegment(3, player_scores[winner - 1] % 10);
    _delay_ms(5);
  }

  printString("\n\n=== GAME SUMMARY ===\n\n");
  printf("PLAYER 1 --> Score: %d - Rebounds: %d\nPLAYER 2 --> Score: %d - Rebounds: %d\n",
  player_scores[0], player1_rebound, player_scores[1], player2_rebound);
  printf("\n\n==== THE WINNER IS --> PLAYER %d ====\n\n", winner);
  printElapsedTime();
}

int main() {
  initDependencies();
  displayGameRules();
  _delay_ms(DELAY);

  initPingPongTable();
  adjustBallSpeed(&ball_speed);
  LEDCountDown();
  
  activatePauseInterrupt();
  startTimer();
  while (round_number <= MAX_ROUNDS) {
    setTurnState();    
    if (didPlayerOnePlay()) {
      direction = 1;
    } else if (didPlayerTwoPlay()) {
      direction = -1;
    }
    moveTheBall();    
    printPongTable();
    applySpeed(ball_speed);
  }
  stopTimer();
  victoryTheme();
  gameOver();
  return 0;
}
