////////////////////////////////////////////////////////////////////////////////
//  
//  RFM69HW Test Suite
//
//  S. Parker May 2014
//
//  This file includes some test routines for exercising the HopeRF RFM69HW
//  module.  
//
////////////////////////////////////////////////////////////////////////////////
#include <RFM69.h>
#include <RFM69registers.h>
#include <SPI.h>

////////////////////////////////////////////////////////////////////////////////
//  Program Control
////////////////////////////////////////////////////////////////////////////////
#define LED 13
                        
#define ST_INIT           0
#define ST_MAINMENU       1
#define ST_WAITCHAR       2
#define ST_LEDON          3
#define ST_LEDOFF         4
#define ST_INIT_RFM69HW   5

#define ST_SET_FREQ       6
#define ST_SET_POWERLEVEL 7
#define ST_SET_POWERBOOST 8
#define ST_RX_LOOP        9
#define ST_TX_LOOP        10

int state, next_state = ST_INIT;

////////////////////////////////////////////////////////////////////////////////
//  RFM68HW Control / Setup
////////////////////////////////////////////////////////////////////////////////

#define GATEWAY_ID    1                                                                     
#define NETWORKID     100                                                                  

int frequency =        915000000;               
int fstep     =        61;                                               

int power_level     =        0x12;
int boost_amp       =        2;

#define ENCRYPT_KEY          "my-EncryptionKey"                                                          
#define ACK_TIME             50                                          
#define SERIAL_BAUD          115200
#define TESTSUITE_VERSION    "1.0"
#define MSG_INTERVAL         2000

#define MSGBUFSIZE 16                                                                                  
                                                                                                                                 
byte msgBuf[MSGBUFSIZE];

RFM69 radio;

bool radioInitialized = false;
bool promiscuousMode = false;                                                       
bool requestACK=false;

union itag {
  uint8_t b[2];
  uint16_t i;
}it;
union ltag {
  byte b[4];
  long l;
}lt;                                                                                        

void setup() {

  pinMode(LED, OUTPUT);                         
  Serial.begin(115200);                           

}

void loop() {
  char mainmenu_input;
  int i;  

                                
  switch(state)
  {
    case ST_INIT : 
      next_state = ST_MAINMENU;
      break;
      
    case ST_MAINMENU :
      mainmenu();                                                                      
      next_state = ST_WAITCHAR;
      break;
      
    case ST_WAITCHAR :
      if(Serial.available())
      {
        mainmenu_input = Serial.read();
        Serial.println(mainmenu_input);
        if(mainmenu_input == 'L')
          next_state = ST_LEDON;
        else if(mainmenu_input == 'l')
          next_state = ST_LEDOFF;
        else if(mainmenu_input == 'i')
          next_state = ST_INIT_RFM69HW;
        else if(mainmenu_input == 'r')
          {
            Serial.println("Entering Receiver Loop.  Press any key to exit.");
            next_state = ST_RX_LOOP;
          }
        else if(mainmenu_input == 't')
          {
            Serial.println("Entering Transmitter Loop.  Press any key to exit.");
            next_state = ST_TX_LOOP;
          }
        else if(mainmenu_input == 'F')
          {
            Serial.println("Increase Frequency");
            frequency = frequency + 1000000;
            radio.setFrequency(frequency/fstep);
            next_state = ST_MAINMENU;
          }
        else if(mainmenu_input == 'f')
          {
            Serial.println("Decrease Frequency");
            frequency = frequency - 1000000;
            radio.setFrequency(frequency/fstep);
            next_state = ST_MAINMENU;
          }
        else if(mainmenu_input == 'P')
          {
            Serial.println("Increase Power Level");
            power_level = (radio.readReg(REG_PALEVEL) & 0x1F) + 1;
            radio.writeReg(REG_PALEVEL, (radio.readReg(REG_PALEVEL) & 0xE0) + power_level);
            next_state = ST_MAINMENU;
          }
        else if(mainmenu_input == 'p')
          {
            Serial.println("Increase Power Level");
            power_level = (radio.readReg(REG_PALEVEL) & 0x1F) - 1;
            radio.writeReg(REG_PALEVEL, (radio.readReg(REG_PALEVEL) & 0xE0) + power_level);
            next_state = ST_MAINMENU;
          }
        else if(mainmenu_input == 'B')
        {
            Serial.println("Set boost PA1 + PA2");
            power_level = (radio.readReg(REG_PALEVEL) & 0x1F);
            radio.writeReg(REG_PALEVEL, (3 << 5) + power_level);
            next_state = ST_MAINMENU;
          }
        else if(mainmenu_input == 'b')
          {
            Serial.println("Set boost PA1");
            power_level = (radio.readReg(REG_PALEVEL) & 0x1F);
            radio.writeReg(REG_PALEVEL, (2 << 5) + power_level);
            next_state = ST_MAINMENU;
          }
        else if(mainmenu_input == 'H')
         {
           Serial.println("Set High Power Mode");
           radio.writeReg(REG_TESTPA1, 0x5D);
           radio.writeReg(REG_TESTPA2, 0x7C);
           next_state = ST_MAINMENU;
          }
        else if(mainmenu_input == 'h')
          {
           Serial.println("Set Normal Power Mode");
           radio.writeReg(REG_TESTPA1, 0x55);
           radio.writeReg(REG_TESTPA2, 0x70);
           next_state = ST_MAINMENU;
          }
        else if(mainmenu_input == 'f')
          {
            Serial.println("Decrease Frequency");
            frequency = frequency - 1000000;
            radio.setFrequency(frequency/fstep);
            next_state = ST_MAINMENU;
          }


        else if(mainmenu_input == 'd')
          {
            Serial.println("Register Dump");
            for(i=0;i<0x72;i=i+8)
            {
              print_reg(i);
              print_reg(i+1);
              print_reg(i+2);
              print_reg(i+3);
              print_reg(i+4);
              print_reg(i+5);
              print_reg(i+6);
              print_reg(i+7);
              Serial.println("");              
            }

            next_state = ST_MAINMENU;
          }
      }         
      break;
    case ST_LEDON :
      Serial.println("LED ON");
      digitalWrite(LED, HIGH);
      next_state = ST_MAINMENU;
      break;  

    case ST_LEDOFF :
      Serial.println("LED OFF");
      digitalWrite(LED, LOW);
      next_state = ST_MAINMENU;
      break;  

    case ST_INIT_RFM69HW :
      Serial.println("Initializing RFM69HW");
      
      memset(msgBuf,0,sizeof(msgBuf));
      Serial.print("TGateway ");
      Serial.print(TESTSUITE_VERSION);
      Serial.print(" startup at ");
      Serial.print(frequency / 1000000);
      Serial.print("Mhz, Network ID: ");
      Serial.println(NETWORKID);
      delay(50);
           
      radio.initialize(frequency / fstep, GATEWAY_ID, NETWORKID);
      Serial.println("Success!");
      radioInitialized = true;
      radio.writeReg(REG_PALEVEL, ((boost_amp<< 5) + power_level));

      radio.writeReg(REG_TESTPA1, 0x55);
      radio.writeReg(REG_TESTPA2, 0x70);

      next_state = ST_MAINMENU;
      break;  

  case ST_RX_LOOP :
                        
    if(Serial.available())
    {
                         
      while(Serial.available()) { Serial.read(); }
      next_state = ST_INIT;
      break;
    } else
    {
      rx_loop();
      break;
    }

  case ST_TX_LOOP :
                        
    if(Serial.available())
    {
                         
      while(Serial.available()) { Serial.read(); }
      next_state = ST_INIT;
      break;
    } else
    {
      tx_loop();
      break;
    }
  }                

  state = next_state;
}

void mainmenu() {
  Serial.println("");
  Serial.println("RFM69HW Test Suite - Main Menu");  
  Serial.println("");
  Serial.println("-----  RFM69HW STATUS -----");

  if(!radioInitialized)
  {
    Serial.println("RFM69HW Not Initialized");
  } else
  {
    Serial.print("Frequency: ");
    Serial.print(frequency);  
    Serial.println(" Hz");
  
    Serial.print("Boost: ");
    switch ((radio.readReg(REG_PALEVEL) & 0xE0) >> 5)
    {
      case 4:
        Serial.println("PA0 not supported on RFM69HW");
        break;
      case 3:
        Serial.println("PA1 + PA2");
        break;
      case 2:
        Serial.println("PA1 only");
        break;
      default:
        Serial.print  ("Invalid Setting 0x");
        Serial.println((REG_PALEVEL & 0xE0) >> 5);
        break;      
    }   
    
    Serial.print("High Power PA1: ");
    switch (radio.readReg(REG_TESTPA1) )
    {
      case 0x5D:
        Serial.println("High Power (0x5D)");
        break;
      case 0x55:
        Serial.println("Normal Power / Receive Mode (0x55)");
        break;
      default:
        Serial.println("Invalid Setting");
        break;      
    }   

    Serial.print("High Power PA2: ");
    switch (radio.readReg(REG_TESTPA2) )
    {
      case 0x7C:
        Serial.println("High Power (0x7C)");
        break;
      case 0x70:
        Serial.println("Normal Power / Receive Mode (0x70)");
        break;
      default:
        Serial.println("Invalid Setting");
        break;      
    }   

  Serial.print("Output Power setting: 0x");
  Serial.println(radio.readReg(REG_PALEVEL) & 0x1F, HEX);  
  
  Serial.print("Output Power(dBm): ");

  if((radio.readReg(REG_PALEVEL)&0xE0)>>5==2)                                                   // PA1 Enabled only                                                         
  {
   Serial.println((radio.readReg(REG_PALEVEL) & 0x1F) - 18);
  } else if ((radio.readReg(REG_PALEVEL)&0xE0)>>5==3)                                           // PA1 + PA2 Enabled                                                 
  {
    if ( (radio.readReg(REG_TESTPA1)==0x5D) && (radio.readReg(REG_TESTPA2)==0x7C))                                   
    {
     Serial.println((radio.readReg(REG_PALEVEL) & 0xF) + 5);                                
    } else if ( (radio.readReg(REG_TESTPA1)==0x55) && (radio.readReg(REG_TESTPA2)==0x70))                          
    {
      Serial.println((radio.readReg(REG_PALEVEL) & 0xF) + 2);      
    } else
     Serial.println("Invalid Setting 2");   
  } else
     Serial.println("Invalid Setting 1");   
  }
  
  Serial.println("-----  Available Commands -----");
  Serial.println("\"L\"  LED ON,                \"l\"  LED OFF");  
  Serial.println("\"i\"  Initialize RFM69HW");  
  Serial.println("\"F\"  Increase frequency,    \"f\"  Decrease frequency");
  Serial.println("\"P\"  Increase power setting,\"p\"  Decrease power setting");
  Serial.println("\"B\"  Boost: PA1 + PA2,      \"b\"  Boost:  PA1");
  Serial.println("\"H\"  High power Settings,   \"h\"  Receive mode settings");    
  Serial.println("\"r\"  Enter RX loop state");  
  Serial.println("\"t\"  Enter TX loop state"); 
  Serial.println("\"d\"  Register dump"); 
  Serial.println("");
  Serial.print("> ");  
}

void rx_loop() {
  if (radio.receiveDone()) {
    for(int i=0; i<radio.DATALEN; i++) {
      if(i==sizeof(msgBuf))
        break;                                                                                                                  
        msgBuf[i] = radio.DATA[i];
      }
                                                    
    Serial.print("Received from TNODE: ");
    Serial.print((byte)msgBuf[0],DEC);
                           
    lt.b[3] = msgBuf[1];
    lt.b[2] = msgBuf[2];
    lt.b[1] = msgBuf[3];
    lt.b[0] = msgBuf[4];
    Serial.print(", t=");
    Serial.print(lt.l,DEC);
    Serial.print(", tempC=");
    Serial.print(msgBuf[5],DEC);
    Serial.print(", REGLNA=");
    Serial.print(radio.readReg(REG_LNA), HEX);
    Serial.print(", rssi=");
    Serial.println(radio.RSSI);    
    it.b[1] = msgBuf[6];
    it.b[0] = msgBuf[7];
  }
}


void tx_loop() {

  byte tempC =  radio.readTemperature(-1);                                                    
  int n=1; 
  lt.l = millis();              
  msgBuf[1] = lt.b[3];
  msgBuf[2] = lt.b[2];
  msgBuf[3] = lt.b[1];
  msgBuf[4] = lt.b[0];
  msgBuf[5] = tempC;              
  it.i      = 0;                      
  msgBuf[6] = it.b[1];
  msgBuf[7] = it.b[0];
    
  radio.send((byte)GATEWAY_ID, (const void *)&msgBuf[0], (byte)sizeof(msgBuf), requestACK);
  delay(MSG_INTERVAL);
    
}

void print_reg(int i)
{
  Serial.print("    0x");
  if((i)<0x10) Serial.print("0"); Serial.print(i, HEX);
  Serial.print(" 0x");
  if(radio.readReg(i)<0x10) Serial.print("0"); Serial.print(radio.readReg(i), HEX); 
}


