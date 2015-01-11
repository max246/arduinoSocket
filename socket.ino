#include <SoftwareSerial.h>

//Define pins
#define DEBUG 1
#define MODEM_TX 3
#define MODEM_RX 2

#define BUFFER_SIZE 90

char incoming_char=0; 
char bufferAT[BUFFER_SIZE];

SoftwareSerial cell(MODEM_RX,MODEM_TX);  

boolean isNetworkRegistered = false;
boolean isGPRSEnable = false;
boolean isCommandSent = false;
boolean isModemReady = false; //Modem is ready to send command
boolean isInizialited = false; //Modem is inizialited and the first commands are sent

boolean isSetupPassed = false;



void setup () {

  Serial.begin(9600);
  cell.begin(9600);
  #ifdef DEBUG
    Serial.println("Starting SM5100B Communication...");
  #endif
}

void loop() {
   //If a character comes in from the cellular module...
  if(cell.available() >0)
  {
      readAT();
   // incoming_char=cell.read();    //Get the character from the cellular serial port.
   // Serial.print(incoming_char);  //Print the incoming character to the terminal.
  }
  //If a character is coming from the terminal to the Arduino...
  if(Serial.available() >0)
  {
    incoming_char=Serial.read();  //Get the character coming from the terminal
    cell.print(incoming_char);    //Send the character to the cellular module.
  }
  
}

void cleanBuffer() {
  while(cell.available() >0) cell.read();
}

void setupSocket() {
  cleanBuffer();
  isSetupPassed = true;
  if (!sendATConfirm("AT+CGDCONT=1,\"IP\",\"general.t-mobile.uk\"","OK"))  {
    isSetupPassed = false;
    return;
  }
  cleanBuffer();
  if (!sendATConfirm("AT+CGACT=1,1","OK"))  {
    isSetupPassed = false;
    return;
  }
  cleanBuffer();
  if (!sendATConfirm("AT+SDATACONF=1,\"TCP\",\"IP\",66","OK"))  {
    isSetupPassed = false;
    return;
  }
  cleanBuffer();
  if (!sendATConfirm("AT+SDATASTART=1,1","OK"))  {
    isSetupPassed = false;
    return;
  }
}

void readMessage() {
  sendAT("AT+SDATATREAD=1");
}
void sendData(String data) {
     cell.print("AT+SSTRSEND=1,\"");
     cell.print(data);
     cell.println("\"");
}
//SEnd a command to Modem
void sendAT(String command) {
    cell.println(command);
    delay(1000);
}

//Send a command to Modem and confirm the result 
boolean sendATConfirm(String command,char* confirm) {
    cell.println(command);
    delay(1000);
    
    memset(bufferAT, 0, BUFFER_SIZE); //Empty buffer
    byte index = 0;
    while(true) {
      if(cell.available() >0) {
        incoming_char=cell.read();  //Get the character coming from the terminal
        if (index >= (strlen(confirm)+2)) {
          break; //Break cycle when I arrive to the end
        }
        bufferAT[index] = incoming_char;
        index++;
      }
    } 
    if (strstr(bufferAT,confirm) != 0) return true;
    else return false;
}
//Read message from Modem and parse it
void readAT() {
  memset(bufferAT, 0, BUFFER_SIZE); //Empty buffer
  byte index = 0;
  while(cell.available() > 0) {
    incoming_char=cell.read();  //Get the character coming from the terminal
    if (incoming_char == '\n' || incoming_char == 96) break; //Break cycle when I arrive to the end
    bufferAT[index] = incoming_char;
    index++;
  } 
  
  if (index > 1) {
    #ifdef DEBUG
      Serial.print("---Debug buffer---- INDEX: ");
      Serial.print(index);
      Serial.print(" --- ");
      Serial.print(bufferAT);
      Serial.println();
    #endif
    parseAT();
  }
  if (isModemReady) {
    if (isNetworkRegistered) { //Registered
        if (isGPRSEnable) { //Gprs is not enabled
            if (!isSetupPassed) {
                #ifdef DEBUG
                  Serial.println("Send message to setup GPRS");
                #endif
                setupSocket();
                isCommandSent = true;
            } else {
		 sendData("send a text");
	         delay(2000);
            }
        } else {
            sendAT("AT+CGATT?");
        }
         
    } else { //Modem has not registered to the network
        if (!isInizialited) {
          #ifdef DEBUG
            Serial.println("Send message to setup network");
          #endif
       /*  If you modem doesnt start register to the network after a while, enable these rows
	  sendAT("AT+CFUN=1");
          sendAT("AT+CMGF=1");
          sendAT("AT+CREG=1");
          sendAT("AT+COPS=0");
          isInizialited = true;
          sendAT("AT+CGDCONT=1,\"IP\",\"general.t-mobile.uk\"");
          sendAT("AT+CGACT=1,1");*/
        }
    }
  }
}

//Parse results that are coming from Modem and then debug it
void parseAT() {
  //Parse status of a system
  /*
    0 => SIM card removed  
    1 => SIM card inserted
    8 => The network is lost
    11 => Registered to network
  */
  if (strstr(bufferAT,"+SIND") != 0) {
      byte number;
      number =bufferAT[7] - '0';
      if (((int)bufferAT[8]) != 13) { //If the number is XX 
        number = number*10; //Move the current to the left
        number += bufferAT[8]- '0';
      }
              
      switch(number) {
          case 0:
              isInizialited = false;
              isModemReady = false;
              #ifdef DEBUG
                Serial.println("Sim Card Removed");
              #endif
              break;
          case 1:
              isModemReady = true;
              #ifdef DEBUG
                Serial.println("Sim Card Inserted");
              #endif
              break;
          case 7:
              isInizialited = false;
              isNetworkRegistered = false;
              isGPRSEnable = false;
              isModemReady = false;
              #ifdef DEBUG
                Serial.println("The network service is available for an emergency call ");
              #endif
              break;
          case 8:
              isInizialited = false;
              isModemReady = false;
              #ifdef DEBUG
                Serial.println("The network is lost");
              #endif
              break;
          case 11:
              isInizialited = false;
              isModemReady = true;
              #ifdef DEBUG
                Serial.println("Registered to network");
              #endif
              break;
          default:
              isInizialited = false;
              isModemReady = false;
              #ifdef DEBUG
                Serial.print("Error code: ");Serial.print(number);Serial.println(" Plese check the manual");
              #endif
              break;
      }
  }
  //Parse status of a network
  /*
    0 => No registered network, ME does not search new network  
    1 => Register local network successfully
    2 => No registered network, ME is searching new network
    3 => Network registration is denied
    4 => unknown
    5 => Register roam network successfully
  */
  else if (strstr(bufferAT,"+CREG") != 0) {
      byte number;
      number =bufferAT[7] - '0';
      
      isNetworkRegistered = false;
      switch(number) {
          case 0:
              isInizialited = false;
              isNetworkRegistered = true;
              isModemReady = false;
              #ifdef DEBUG
                Serial.println("No registered network, ME does not search new network  ");
              #endif
              break;
          case 1:
              isNetworkRegistered = true;
              isModemReady = true;
              #ifdef DEBUG
                Serial.println("Register local network successfully");
              #endif
              break;
          case 2:
              isInizialited = false;
              isModemReady = false;
              #ifdef DEBUG
                Serial.println("No registered network, ME is searching new network");
              #endif
              break;
          case 3:
              isInizialited = false;
              isModemReady = false;
              #ifdef DEBUG
                Serial.println("Network registration is denied");
              #endif
              break;
          case 4:
              isInizialited = false;
              #ifdef DEBUG
                Serial.println("unknown");
              #endif
              break;
          case 5:
              isNetworkRegistered = true;
              isInizialited = true;
              isModemReady = true;
              #ifdef DEBUG
                Serial.println("Register roam network successfully");
              #endif
              break;
          default:
              #ifdef DEBUG
                Serial.print("Error code: ");Serial.print(number);Serial.println(" Plese check the manual");
              #endif
              break;
      }
  }
  // GPRS Status
  else if (strstr(bufferAT,"+CGATT") != 0) {
      byte number;
      number =bufferAT[8] - '0';
      switch(number) {
          case 0:
              isSetupPassed = false;
              isGPRSEnable = false;
              #ifdef DEBUG
                Serial.println("Detach GPRS service ");
              #endif
              break;
          case 1:
              isGPRSEnable = true;
              #ifdef DEBUG
                Serial.println("Attach GPRS service ");
              #endif
              break;
          default:
              isGPRSEnable = false;
              #ifdef DEBUG
                Serial.print("Error code: ");Serial.print(number);Serial.println(" Plese check the manual");
              #endif
              break;
      }
  }
  //Handle message from NodeJS
  else if (strstr(bufferAT,"+STCPD") != 0) {
      byte number;
      number =bufferAT[7] - '0';
      #ifdef DEBUG
        Serial.print("New message from NodeJS, ID: ");Serial.println(number);
      #endif
      readMessage();
  }
  //Handle message from NodeJS
  else if (strstr(bufferAT,"+SSTR") != 0) {
      //Parse your message    
  }
  
  //ERRORS
  else if (strstr(bufferAT,"+CME") != 0) {
      byte number;
      number =bufferAT[12] - '0';   
      if (((int)bufferAT[13]) != 13) { //If the number is XX 
        number = number*10; //Move the current to the left
        number += bufferAT[13]- '0';
      }
      switch(number) {
          case 3:
              #ifdef DEBUG
                Serial.println("operation not allowed");
              #endif
              break;
          case 4:
              #ifdef DEBUG
                Serial.println("operation not supported");
              #endif
              break;
          default:
              isGPRSEnable = false;
              #ifdef DEBUG
                Serial.print("Error code: ");Serial.print(number);Serial.println(" Plese check the manual");
              #endif
              break;
      }
  }//ERRORS
  else if (strstr(bufferAT,"ERROR") != 0) {
    isGPRSEnable = false;
  }
}


