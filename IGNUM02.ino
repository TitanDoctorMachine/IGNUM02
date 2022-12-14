#include "IGNUM02_AUTH_DECODER.h"
#include <cstring>
#include <string>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
//#include "PARAMS.h" // PARAMS included in IGNUM02 header;

PARAMS PARAMS;
HTTPClient challenge, request;
IGNUM IGNUM;
ESP8266WebServer server(80);
WiFiClient client;

String server_state;

void setup() {

  Serial.begin(115200);
  
  delay(1000);

  Serial.println(" ");
  Serial.println("Starting_System...");
  IGNUM.setChaosFactor("maybetimeornoisepin");
  server_state = IGNUM.begin("normal");
  StartNetwork();
  
}

void loop() {
  
  server.handleClient(); //Server handleing
  MDNS.update();
  IGNUM.SustainLoop(); //to RNG function
 
}

void StartNetwork(){

 //configuring server
  WiFi.mode(WIFI_STA);
  ESP8266WiFiMulti wifiMulti;
  PARAMS.GetUser();
  wifiMulti.addAP(PARAMS.GetMASTERWIFISSID(), PARAMS.GetMASTERWIFIPSWD());
  
  Serial.println("Tentando Conexão...");
  if (wifiMulti.run() == WL_CONNECTED){
      
  //after network connected
  Serial.println("");

  //returns wifi ssid
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());
  
  //retornus wifi IP
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS started");
  }

  server.on("/Challenge", [] {
      server.send(200, "text/plain", IGNUM.GetChallenge());
      });

  server.on("/", [] {
      server.send(200, "text/plain", "Spartan_IGNUM02_TEST \n" + server_state + "\n");
      });

  server.onNotFound([]{
      String received = server.uri();
        received.remove(0,1);
        server.send(200, "text/plain", inputCommand(IGNUM.InputCyphercode(received)) + "\n");
        IGNUM.setChaosFactor("11:11/NOISE?"); // Chaos factor can be setted to current_time?
      IGNUM.reload();
  });

      server.begin();
      Serial.println("HTTP server started");
  } else { //ERROR_SECTION
  StartNetwork();
  }
} 

String send_ignum_command(String IP, String Command){

  String challgbuff, cyphebuff;

  for(int counter = 0; counter != 5; counter++){

    String challgbuff, cyphebuff;
    challenge.begin(client, "http://" + IP + ":80/Challenge");
    challenge.setTimeout(3000);

      int httpCode = challenge.GET();
      Serial.println("CHllG: " + challgbuff);    
    
      if( httpCode == HTTP_CODE_OK) {
        challgbuff = challenge.getString();
        Serial.println(challgbuff);
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", challenge.errorToString(httpCode).c_str());
      }
    
    request.begin(client, "http://" + IP + ":80/" + IGNUM.GenIgnumCommand(challgbuff, Command));
    request.setTimeout(3000);
    
    httpCode = request.GET();

    if( httpCode == HTTP_CODE_OK) {
      cyphebuff = request.getString();
      Serial.println(cyphebuff);
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", request.errorToString(httpCode).c_str());
    }

    challenge.end();
    request.end();
    if(strstr(cyphebuff.c_str(), "OK")){
        return cyphebuff;
        break;
    }
  }
  return "Fatal_Error!";
}

String Commands[16] = {"HELP", "ROOT", "EXTHELP"}; //Native commands;

int Last_Commands_NUM() {
  int result = 0;
  for (int i = 0; i != 17; i++){
    if (Commands[i] != ""){
      result++;
      } else {
      break;
      }
        
    }
  return result;
}


String HelpWhatCommands(){
  String suboutput = "";
  for (int k = 0; k != Last_Commands_NUM(); k++){
    
    suboutput += Commands[k] + "\n";
    
  }
return suboutput;
}

String inputCommand(bool allowed){

  //Syntax = return(inputCommand(IGNUM.InputPlainCode(plain_requisition_package)));
 
  String RootKey, Command, Cond1, Cond2, Cond3, command_response;

  RootKey = IGNUM.GetRxRootKey();
  Command = IGNUM.GetRxCommand();
  Cond1 = IGNUM.GetRxCondit1();
  Cond2 = IGNUM.GetRxCondit2();
  Cond3 = IGNUM.GetRxCondit3();
  IGNUM.EndRxCommand();     

    if(!allowed) {
    return "Access_Denied!";
    } else if (allowed = 1){
           String Refined_Result = "Access_Granted >> ";
                 
           for(int X = 0; X != Last_Commands_NUM(); X++){
               if (strstr(Command.c_str(),Commands[X].c_str())){ /// used that for an non identified error, need to remake this part later;
                  switch(X+1){
                    case 1:
                      //HELP
                      Refined_Result += "\n" + HelpWhatCommands();
                      return Refined_Result;
                    break;
                  
                    case 2:
                      //ROOT
                      Refined_Result += RootKey;
                      return Refined_Result;
                    break;

                    case 3:
                      //EXTHELP
                      Refined_Result += send_ignum_command(PARAMS.GetMASTERSERVERIP(), "HELP");
                      return Refined_Result;
                    break;

               }
            }
          } 
          
    Refined_Result += "Syntax_Error!";
    return Refined_Result;
    } 

}

