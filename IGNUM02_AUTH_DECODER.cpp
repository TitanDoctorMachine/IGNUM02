#include <SHA256.h>
#include <Crypto.h>
#include <RNG.h>
#include <cstring>
#include "IGNUM02_AUTH_DECODER.h"
#include <FS.h>
#include "Base64.h"
#include <ESP8266WiFi.h>
#include "PARAMS.h"


/*

        IGNUM02 is a simplified version of IGNUM01 capable of handling
        only one secret key by device, developed for integrated devices
        with only one functionality; whatsoever, its capable of sending
        IGNUM01-AES256 requests. 

*/

SHA256 sha256, userhash;
PARAMS PARAMS;

bool ValidCommand = 0, configmode = 0;
int last_User, last_Root_user;
byte UserSessionid[32], Challenge[32], BChaos_Factor[16];
String STRKeyChallenge, Users[32], RootUsers[32], CommandList[5], User_Group, Root_Group, Chaos_Factor = "";
String UserHash, ValidToken, SYSTEM_KEY;


void IGNUM::setChaosFactor(String input){Chaos_Factor = input;}

String encrypt(String plain_data, String SymKey, String Vector){
      
      // AES CBC Encryption
      //tool from Kakopappa (from Github).
      //Modified by DocMac.

      const char* Key_Man = SymKey.c_str();
      const char* IV_Man = Vector.c_str();

      int len = plain_data.length();
      int n_blocks = len / 16 + 1;
      uint8_t n_padding = n_blocks * 16 - len;
      uint8_t data[n_blocks*16];
      memcpy(data, plain_data.c_str(), len);
      for(int i = len; i < n_blocks * 16; i++){
        data[i] = n_padding;
      }
      
      uint8_t key[16], iv[16];
      memcpy(key, Key_Man, 16);
      memcpy(iv, IV_Man, 16);

      // encryption context
      br_aes_big_cbcenc_keys encCtx;

      // reset the encryption context and encrypt the data
      br_aes_big_cbcenc_init(&encCtx, key, 16);
      br_aes_big_cbcenc_run( &encCtx, iv, data, n_blocks*16 );

      // Base64 encode
      len = n_blocks*16;
      char encoded_data[ base64_enc_len(len) ];
      base64_encode(encoded_data, (char *)data, len);
      
      return String(encoded_data);
}

String decrypt(String encoded_data_str, String SymKey, String Vector){  

  // AES CBC Decryption
  //tool from Kakopappa (from Github).
  //Modified by DocMac.

  const char* Key_Man = SymKey.c_str();
  const char* IV_Man = Vector.c_str();
  
  int input_len = encoded_data_str.length();
  char *encoded_data = const_cast<char*>(encoded_data_str.c_str());
  int len = base64_dec_len(encoded_data, input_len);
  uint8_t data[ len ];
  base64_decode((char *)data, encoded_data, input_len);
  
  uint8_t key[16], iv[16];
  memcpy(key, Key_Man, 16);
  memcpy(iv, IV_Man, 16);

  int n_blocks = len / 16;

  br_aes_big_cbcdec_keys decCtx;

  br_aes_big_cbcdec_init(&decCtx, key, 16);
  br_aes_big_cbcdec_run( &decCtx, iv, data, n_blocks*16 );

  uint8_t n_padding = data[n_blocks*16-1];
  len = n_blocks*16 - n_padding;
  char plain_data[len + 1];
  memcpy(plain_data, data, len);
  plain_data[len] = '\0';

  return String(plain_data);
}

String HashIt256(String input){
 byte hashed[32];
 const char* toHash = input.c_str();
 String BufferOut = "";
    sha256.reset();
    sha256.update(toHash, strlen(toHash));
    sha256.finalize(hashed, 32);
    
    for (int x = 0; x < 32; x++) {
      if(hashed[x] < 0x10) {
        BufferOut += '0';
      }
      BufferOut += String (hashed[x], HEX);
    }
  return BufferOut;    
}

void IGNUM::SustainLoop(){ // LOOP FUNCTION
    RNG.loop();
}

String NewChallenge() { //GENERATE NEW CHALLENGE
  
  String internalSTRKeyChallenge = "";
  RNG.rand(Challenge, 16); // 16 now, 16 from the Chaos_Factor;

  Chaos_Factor.getBytes(BChaos_Factor, 16);
  
  for (int i = 1; i < 17; i++){
    Challenge[i+16] = BChaos_Factor[i];   
  }

  
  String buffer;

  for (int x = 0; x < 32; x++) {
      if(Challenge[x] < 0x10) {
        buffer += '0';
      }
      buffer += String (Challenge[x], HEX);
    }
  
  internalSTRKeyChallenge = HashIt256(buffer);

  STRKeyChallenge = internalSTRKeyChallenge; 

  ValidToken = HashIt256(STRKeyChallenge + UserHash);

  Serial.print("User: ");//DEBUG
  Serial.println(SYSTEM_KEY);//DEBUG
  Serial.print("UserHash: ");//DEBUG
  Serial.println(UserHash);//DEBUG
  Serial.print("UserToken: ");//DEBUG
  Serial.println(ValidToken);//DEBUG

  return internalSTRKeyChallenge;  

}

String IGNUM::begin(String reboot){
  
  if (Chaos_Factor == "" or Chaos_Factor == " "){
    Chaos_Factor = "youhavetosetsomethingherebeforetheBEGINv65h41gm11gh54151m65gh1mn5gh5nm1gh964n1";
  }
  RNG.begin(Chaos_Factor.c_str()); //key to start RNG;
  RNG.rand(Challenge, 10);
  SPIFFS.begin();
  if(reboot != "reboot"){  
  reload();
  }
  SYSTEM_KEY = PARAMS.GetUser();
  UserHash = HashIt256(SYSTEM_KEY);
  NewChallenge(); 
  return "starting";
        
}

bool IGNUM::InputPlainCode(String inputPack){ //INPUT PLAIN CODE
 
  ///Syntax = ValidUserToken+//+command+//+cond1+//+cond2+//+cond3+//+SALT 

  CommandList[0] = "";
  String PlainPackage[6];
  bool OUTPUT_Function = 0;
  int CountNumberCommand = 0;
  
  inputPack.replace("//", " ");
  inputPack = " " + inputPack; //correção de syntax_input
     
  while (inputPack.length() > 0)
   {  int index = inputPack.indexOf(' ');
      if (index == -1){
      PlainPackage[CountNumberCommand] = inputPack;
      break;
      } else {
      PlainPackage[CountNumberCommand] = inputPack.substring(0, index);
      inputPack = inputPack.substring(index+1);
      }
    CountNumberCommand++;
    }
   
  String ReceivedKeyChallenge = PlainPackage[1]; 
  String Command = PlainPackage[2]; 
  String Condition_1 = PlainPackage[3]; 
  String Condition_2 = PlainPackage[4]; 
  String Condition_3 = PlainPackage[5]; 
  
    if (ValidToken == ReceivedKeyChallenge and Command != " "){ //Bug Fixing;
      OUTPUT_Function = 1;
      CommandList[1] = "root:true";
      CommandList[2] = Command; 
      CommandList[3] = Condition_1; 
      CommandList[4] = Condition_2;
      CommandList[5] = Condition_3;
    } else {
       OUTPUT_Function = 0;
    }
    return OUTPUT_Function;

}

bool IGNUM::InputCyphercode(String CypherCode){

  if (!CypherCode.startsWith("IG01")){
    return InputPlainCode(CypherCode);
  } else {
    //thats where magic happens!
    CypherCode.remove(0,4);
  
    String DecCypherCode = decrypt(CypherCode, ValidToken, STRKeyChallenge); //InitVector = Challenge
    if (strstr(DecCypherCode.c_str(), ValidToken.c_str())){
        return InputPlainCode(DecCypherCode);
    }
  }
}

//COMMANDS TO LOAD VALUES FROM COMMANDS
void IGNUM::EndRxCommand(){ ///put in ignum reload;
  for(int y = 0; y != 6; y++){
  CommandList[y] = " ";
  }
}

void IGNUM::reload(){ NewChallenge();}
bool IGNUM::GetRxValid() {return ValidCommand;} 
String IGNUM::GetRxRootKey(){return CommandList[1];}
String IGNUM::GetRxCommand(){return CommandList[2];}
String IGNUM::GetRxCondit1(){return CommandList[3];}
String IGNUM::GetRxCondit2(){return CommandList[4];}
String IGNUM::GetRxCondit3(){return CommandList[5];}
String IGNUM::GetChallenge() {return STRKeyChallenge;}
