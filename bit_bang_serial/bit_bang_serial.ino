/*
Test driver for a software serial ISR (interrupt service routine).
Baudrate = 2400
Data input trough circular buffer.

Possible optimization (faster execution), remove function calls from switch case and have the code in the switch case itself.

Written by George Timmermans
ver 0.1
*/

//Constants
const uint8_t serialPin = 13;
const uint8_t ISRPin = 12;
const uint8_t MAX = 25;
static const char message[MAX] = {'G','e','o','r','g','e',' ','T','i','m','m','e','r','m','a','n','s','\0','\n','\r'};
enum states { START_BIT_STATE, DATA_BITS_STATE, STOP_BITS_STATE, IDLE_STATE };
uint8_t messageLength = 0;
const uint8_t TX_BUFF_SIZE = 30;
volatile uint8_t txBuff[TX_BUFF_SIZE];

//Variables
static uint8_t messageCount = 0;
static uint8_t bitCount = 0;
volatile enum states currentState = IDLE_STATE;
volatile uint8_t txWriteIndex, txReadIndex;
uint8_t testInt = 0;
volatile uint8_t temp = 0;

void setup() {
  //Serial.begin(9600);
  pinMode(serialPin, OUTPUT);
  pinMode(ISRPin, OUTPUT);
  digitalWrite(serialPin, HIGH);
  digitalWrite(ISRPin, LOW);
 
  // initialize timer2 
  noInterrupts();           // disable all interrupts
  TCCR2A = 0;
  TCCR2B = 0;

  TCNT2 = 151;              // preload timer = 255 - 16mHz/64/2400
  TCCR2B |= (1 << CS22);    // 64 prescaler 
  TIMSK2 |= (1 << TOIE2);   // enable timer overflow interrupt
  interrupts();             // enable all interrupts 
  
  txBuffInit();
  currentState = START_BIT_STATE;
}

ISR(TIMER2_OVF_vect)        // interrupt service routine that wraps a user defined function supplied by attachInterrupt
{
  digitalWrite( ISRPin, HIGH );
  TCNT2 = 151;            // preload timer
  
  switch(currentState)
  {
    case START_BIT_STATE: 
      startBit();
      break;
    case DATA_BITS_STATE:
      dataBit();
      break;
    case STOP_BITS_STATE:
      stopBit();
      break;
    case IDLE_STATE:
    default:
      idle();
      break;
  }
  digitalWrite( ISRPin, LOW );
}

void loop() {  
  delay(1000);
  if (txBuffEmpty())
  {
    for (int i =0; i < MAX; i++)
    {
      txBuffWrite(message[i]);
    }
  }   
}

void startBit()
{
  digitalWrite( serialPin, LOW );
  //Serial.print('0');
  bitCount++;
  currentState = DATA_BITS_STATE;
}

/*
void dataBit()  //Serialcom least significant bit LAST
{
  if ( bitCount < 9)
  {
    digitalWrite( serialPin, bitRead( message[messageCount], 8 - bitCount ) );
    Serial.print(bitRead( message[messageCount], 8 - bitCount ));
    bitCount++;
  }
  else
    currentState = STOP_BITS_STATE;
}

*/

void dataBit()  //Serialcom least significant bit FIRST
{
  if ( bitCount < 9)
  {
    digitalWrite( serialPin, bitRead( temp, bitCount - 1) );
    //Serial.print(bitRead( temp, bitCount - 1));
    //Serial.print("Data");
    bitCount++;
  }
  if ( bitCount == 9)
    currentState = STOP_BITS_STATE;
}

void stopBit()
{
  if ( bitCount < 11)
  {
    digitalWrite( serialPin, HIGH );
    //Serial.print('1');
    bitCount++; 
  }
  if ( bitCount == 11)
  {
    currentState = IDLE_STATE;  
    //Serial.println();     
    bitCount = 0; 
  }
}

void idle()
{
  if (txWriteIndex != txReadIndex)
  {
    temp = txBuff[txReadIndex];
    txReadIndex = (txReadIndex + 1) % TX_BUFF_SIZE;
    currentState = START_BIT_STATE;
  }
  else
  {
    currentState = IDLE_STATE;  
    //digitalWrite( serialPin, HIGH );
  }
}

void txBuffInit()
{
  txWriteIndex = 0;
  txReadIndex = 0;
}

bool txBuffEmpty()
{
  if (txWriteIndex == txReadIndex)
    return true;
  else
    return false;
}

void txBuffWrite(char thisChar)
{
  noInterrupts();
  txBuff[txWriteIndex] = thisChar;
  txWriteIndex = (txWriteIndex + 1) % TX_BUFF_SIZE;
  if (txBuffEmpty())
    txReadIndex = (txReadIndex + 1) % TX_BUFF_SIZE;
  interrupts();
}


