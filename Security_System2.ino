// Note : To Clear all the content of EEPROM is this program, we will need an extra pin as interrupt.
//-------------------------------------------------------------------------------------------------------------------------------

#include <RCSwitch.h>
#include <EEPROM.h>

RCSwitch mySwitch = RCSwitch();
int initPin = 3;
static boolean toSetNewRemote = false;  //Whether we have to setup new remote(interrupt on initPin is handling this value)
static int EEPROMlength = 0;  //total number of values in EEPROM
static long arr[3];

void setup() {
  Serial.begin(9600);
  pinMode(initPin, INPUT_PULLUP);
//  attachInterrupt(digitalPinToInterrupt(initPin), clearAllEEPROM, RISING);
  attachInterrupt(digitalPinToInterrupt(initPin), setInitRemote, RISING);
  mySwitch.enableReceive(0);  // Receiver on interrupt 0 => that is pin #2

//  Calculating initially how many numbers are available in EEPROM.
  for (int i = 0; i < EEPROM.length(); i = i + 4) {
    if (EEPROMReadLong(i) != 0) {
      EEPROMlength++;
    } else
      break;
  }
}

void loop() {
  if (mySwitch.available()) {

    //1.  if user is entering the new values for setup of new remote
    if (toSetNewRemote == true) {
      setupNewRemote(mySwitch.getReceivedValue());
      Serial.println(mySwitch.getReceivedValue());
    }

    //2.  if user have entered less than four new values in EEPROM but then he cancelled the setup process
    if (toSetNewRemote == false && EEPROMlength % 4 != 0) {
      rollback();
    }

    //3.  when user is pressing the pre-existing values for normal operations
    if (toSetNewRemote == false && EEPROMlength % 4 == 0) {
      long value = mySwitch.getReceivedValue();
      
      int keyNo = match(value);
      Serial.println("Matching : Value = " + String(value) + " Key No. :" + String(keyNo));
      if (keyNo == 1) {
        Serial.println("Pressed Key : A");
      } else if (keyNo == 2) {
        Serial.println("Pressed Key : B");
      } else if (keyNo == 3) {
        Serial.println("Pressed Key : C");
      } else if (keyNo == 4) {
        Serial.println("Pressed Key : D");
      } else {
        Serial.println("Unknown Encoding");
      }
    }
    mySwitch.resetAvailable();
  }
}

void rollback() {
  //  for exp if EEPROMlength = 10, then two remotes(i.e. 4+4 = 8 values are set) and we have to remove 2 another values;
  //  now dividend = 10/4 = 2 and remainder = 10%2 = 2;
  // Hence first address to remove = dividend*16 + (remainder-1)*4; and second to remove  = dividend*16 + (remainder-2)*4;

  int dividend = EEPROMlength / 4;
  int remainder = EEPROMlength % 4;
  long address = dividend * 16 + (remainder - 1) * 4 ;
  Serial.println("Rolling Back : Address - "+String(address)+" and Value : "+String(EEPROMReadLong(address)));
  EEPROMWriteLong(address, 0);
  EEPROMlength--;
}

/**
   To set the condition if user want to setup a new remote. Here we are handling this value through external Interrupt on intptPin
*/
void setInitRemote() {
  if (toSetNewRemote == false) {
    toSetNewRemote = true;
  } else {
    toSetNewRemote = false;
  }

  Serial.println(String(toSetNewRemote));
}

/**
   Clear the EEPROM.
*/
void clearAllEEPROM() {
  for (int i = 0; i < EEPROM.length(); i = i + 4) {
    if (EEPROMReadLong(i) != 0) {
      EEPROMWriteLong(i, 0);
    } else {
      EEPROMlength = 0;
      Serial.println("EEPROM has been Cleared");
      break;
    }
  }
}

/**
   Check if a pin exists in EEPROM. if it not, then add the values.
*/
void setRemotePin(long pinValue) {
  for (int i = 0; i < EEPROM.length(); i = i + 4) {
    long romValue = EEPROMReadLong(i);
    if (romValue == pinValue) {
      Serial.println("This pin is already set");
      break;
    } else if (romValue == 0) {
      EEPROMWriteLong(i, pinValue);
      Serial.println("Value : " + String(pinValue) + " saved at : " + String(i) + " as : " + String(EEPROMReadLong(i)));
      EEPROMlength++;
      break;
    }
  }
}

/**
   Check the address of a value in EEPROM
   return address : address where values exists in EEPROM
   return -1 : if value in EEPROM do not exists
*/
int match(long value) {
  for (int i = 0; i < EEPROM.length(); i = i + 4) {
    if (value == EEPROMReadLong(i)) {
      if (i >= 0 && i < 16)
        return (i / 4 + 1);
      else if (i >= 16 && i < 32)
        return ((i - 16) / 4 + 1);
      else if (i >= 32 && i < 48)
        return ((i - 32) / 4 + 1);
    }
  }
  return -1;
}

/**
   Takes three values from transmitter for each pin. If all are equal, then save that value in EEPROM.
   do the same for each pin.
*/
void setupNewRemote(long pinValue) {
  for (int i = 0; i < 3; i++) {
    if (arr[i] == 0) {
      arr[i] = pinValue;
      if (i == 2 && (arr[0] == arr[1]) && (arr[1] == arr[2])) {
        Serial.println("Selected Value : " + String(arr[0]));
        setRemotePin(arr[i]);
        clearArray();
      } else if (i == 2) {
        Serial.println("Selected Value : None! Try Again...");
        clearArray();
      }
      break;
    }
  }
}

/**
   Empty a general array
*/
void clearArray() {
  for (int j = 0; j < 3; j++) {
    arr[j] = 0;
  }
}

/**
   Write value in EEPROM
*/
void EEPROMWriteLong(int address, long value) {
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);

  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}

/**
   Read value from EEPROM
*/
long EEPROMReadLong(long address) {
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);

  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

